#include "pch.h"
#include "zirconium_header_parser.h"
#include "zirconium_type_registry.h"
#include "zirconium_log.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <algorithm>

namespace zirconium {

    // ── Public entry point ──────────────────────────────────────────────────────

    bool HeaderParser::parseFile(const std::string& filePath, TypeRegistry& registry) {
        m_registry = &registry;
        m_warningCount = 0;
        m_currentPack = 8;
        while (!m_packStack.empty()) m_packStack.pop();

        // Read entire file
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            registry.setError("Failed to open file: " + filePath);
            return false;
        }

        auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        m_fileData.resize(static_cast<size_t>(fileSize));
        file.read(&m_fileData[0], fileSize);
        file.close();

        m_src = m_fileData.c_str();
        m_end = m_src + m_fileData.size();
        m_cur = m_src;
        m_line = 1;

        // Register builtin types first
        registry.registerBuiltinTypes();
        registry.setState(TypeRegistry::State::Loading);

        // Tokenize
        tokenize();

        // Parse
        m_pos = 0;
        parseTopLevel();

        // Build search index
        registry.buildSortedNames();
        registry.setState(TypeRegistry::State::Ready);

        LOG_INFO("[TypeRegistry] Loaded %zu types (%d warnings)", registry.typeCount(), m_warningCount);
        return true;
    }

    // ── Lexer ───────────────────────────────────────────────────────────────────

    void HeaderParser::skipWhitespaceAndComments() {
        while (m_cur < m_end) {
            // Whitespace
            if (std::isspace(static_cast<unsigned char>(*m_cur))) {
                if (*m_cur == '\n') ++m_line;
                ++m_cur;
                continue;
            }
            // Line comment
            if (m_cur + 1 < m_end && m_cur[0] == '/' && m_cur[1] == '/') {
                m_cur += 2;
                while (m_cur < m_end && *m_cur != '\n') ++m_cur;
                continue;
            }
            // Block comment
            if (m_cur + 1 < m_end && m_cur[0] == '/' && m_cur[1] == '*') {
                m_cur += 2;
                while (m_cur + 1 < m_end) {
                    if (m_cur[0] == '*' && m_cur[1] == '/') { m_cur += 2; break; }
                    if (*m_cur == '\n') ++m_line;
                    ++m_cur;
                }
                continue;
            }
            break;
        }
    }

    HeaderParser::Token HeaderParser::nextRawToken() {
        skipWhitespaceAndComments();
        if (m_cur >= m_end) return { TokenKind::Eof, "", m_line };

        Token tok;
        tok.line = m_line;
        char c = *m_cur;

        // Single-char tokens
        switch (c) {
            case ';': tok.kind = TokenKind::Semicolon; tok.text = ";"; ++m_cur; return tok;
            case ',': tok.kind = TokenKind::Comma; tok.text = ","; ++m_cur; return tok;
            case '{': tok.kind = TokenKind::LBrace; tok.text = "{"; ++m_cur; return tok;
            case '}': tok.kind = TokenKind::RBrace; tok.text = "}"; ++m_cur; return tok;
            case '(': tok.kind = TokenKind::LParen; tok.text = "("; ++m_cur; return tok;
            case ')': tok.kind = TokenKind::RParen; tok.text = ")"; ++m_cur; return tok;
            case '[': tok.kind = TokenKind::LBracket; tok.text = "["; ++m_cur; return tok;
            case ']': tok.kind = TokenKind::RBracket; tok.text = "]"; ++m_cur; return tok;
            case '*': tok.kind = TokenKind::Star; tok.text = "*"; ++m_cur; return tok;
            case '&': tok.kind = TokenKind::Ampersand; tok.text = "&"; ++m_cur; return tok;
            case '=': tok.kind = TokenKind::Equals; tok.text = "="; ++m_cur; return tok;
            case '#': tok.kind = TokenKind::Hash; tok.text = "#"; ++m_cur; return tok;
            case '~': tok.kind = TokenKind::Tilde; tok.text = "~"; ++m_cur; return tok;
            case '<': tok.kind = TokenKind::LAngle; tok.text = "<"; ++m_cur; return tok;
            case '>': tok.kind = TokenKind::RAngle; tok.text = ">"; ++m_cur; return tok;
        }

        // Colon or ColonColon
        if (c == ':') {
            ++m_cur;
            if (m_cur < m_end && *m_cur == ':') {
                ++m_cur;
                tok.kind = TokenKind::ColonColon;
                tok.text = "::";
            } else {
                tok.kind = TokenKind::Colon;
                tok.text = ":";
            }
            return tok;
        }

        // Dot / Ellipsis
        if (c == '.') {
            if (m_cur + 2 < m_end && m_cur[1] == '.' && m_cur[2] == '.') {
                tok.kind = TokenKind::Ellipsis;
                tok.text = "...";
                m_cur += 3;
            } else {
                tok.kind = TokenKind::Dot;
                tok.text = ".";
                ++m_cur;
            }
            return tok;
        }

        // Numbers (hex and decimal)
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '-' && m_cur + 1 < m_end && std::isdigit(static_cast<unsigned char>(m_cur[1])))) {
            const char* start = m_cur;
            bool isNeg = (c == '-');
            if (isNeg) ++m_cur;

            if (m_cur + 1 < m_end && m_cur[0] == '0' && (m_cur[1] == 'x' || m_cur[1] == 'X')) {
                m_cur += 2;
                while (m_cur < m_end && std::isxdigit(static_cast<unsigned char>(*m_cur))) ++m_cur;
                tok.kind = isNeg ? TokenKind::NegLiteral : TokenKind::HexLiteral;
            } else {
                while (m_cur < m_end && std::isdigit(static_cast<unsigned char>(*m_cur))) ++m_cur;
                tok.kind = isNeg ? TokenKind::NegLiteral : TokenKind::IntLiteral;
            }
            // Skip suffixes like ULL, LL, U, L
            while (m_cur < m_end && (*m_cur == 'u' || *m_cur == 'U' || *m_cur == 'l' || *m_cur == 'L'))
                ++m_cur;
            tok.text = std::string(start, m_cur);
            return tok;
        }

        // Identifiers and keywords (including $ for anonymous types, and __ prefixes)
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
            const char* start = m_cur;
            ++m_cur;
            while (m_cur < m_end && (std::isalnum(static_cast<unsigned char>(*m_cur)) || *m_cur == '_' || *m_cur == '$'))
                ++m_cur;
            tok.kind = TokenKind::Identifier;
            tok.text = std::string(start, m_cur);
            return tok;
        }

        // Unknown
        tok.kind = TokenKind::Unknown;
        tok.text = std::string(1, c);
        ++m_cur;
        return tok;
    }

    void HeaderParser::tokenize() {
        m_tokens.clear();
        m_tokens.reserve(500000); // pre-allocate for ~101K lines
        while (true) {
            Token t = nextRawToken();
            m_tokens.push_back(t);
            if (t.kind == TokenKind::Eof) break;
        }
    }

    // ── Parser helpers ──────────────────────────────────────────────────────────

    static const HeaderParser::Token s_eofToken = { HeaderParser::TokenKind::Eof, "", 0 };

    const HeaderParser::Token& HeaderParser::peek(int offset) const {
        size_t idx = m_pos + offset;
        if (idx < m_tokens.size()) return m_tokens[idx];
        return s_eofToken;
    }

    const HeaderParser::Token& HeaderParser::advance() {
        if (m_pos < m_tokens.size()) return m_tokens[m_pos++];
        return s_eofToken;
    }

    bool HeaderParser::check(TokenKind kind) const {
        return peek().kind == kind;
    }

    bool HeaderParser::match(TokenKind kind) {
        if (check(kind)) { advance(); return true; }
        return false;
    }

    bool HeaderParser::matchIdentifier(const std::string& text) {
        if (peek().kind == TokenKind::Identifier && peek().text == text) {
            advance();
            return true;
        }
        return false;
    }

    void HeaderParser::expect(TokenKind kind) {
        if (!match(kind)) {
            warn("Expected token kind " + std::to_string(static_cast<int>(kind)) +
                 " but got '" + peek().text + "'");
        }
    }

    void HeaderParser::skipToSemicolon() {
        int depth = 0;
        while (!check(TokenKind::Eof)) {
            if (check(TokenKind::LBrace) || check(TokenKind::LParen)) { ++depth; advance(); continue; }
            if (check(TokenKind::RBrace) || check(TokenKind::RParen)) { --depth; advance(); continue; }
            if (check(TokenKind::Semicolon) && depth <= 0) { advance(); return; }
            advance();
        }
    }

    void HeaderParser::skipBalancedBraces() {
        int depth = 0;
        while (!check(TokenKind::Eof)) {
            if (check(TokenKind::LBrace)) { ++depth; advance(); continue; }
            if (check(TokenKind::RBrace)) {
                --depth;
                advance();
                if (depth <= 0) return;
                continue;
            }
            advance();
        }
    }

    void HeaderParser::warn(const std::string& msg) {
        ++m_warningCount;
        if (m_warningCount <= 50) {
            LOG_DEBUG("[HeaderParser] line %d: %s", peek().line, msg.c_str());
        }
    }

    // ── Top-level parser ────────────────────────────────────────────────────────

    void HeaderParser::parseTopLevel() {
        while (!check(TokenKind::Eof)) {
            // Skip #define lines
            if (check(TokenKind::Hash)) {
                advance(); // #
                if (matchIdentifier("pragma")) {
                    parsePragma();
                } else {
                    // Skip #define and other preprocessor directives
                    // Just skip to next line by consuming tokens until we see something that
                    // looks like a new top-level declaration
                    skipToSemicolon(); // not ideal but #define lines don't have ; so this moves past
                }
                continue;
            }

            // enum
            if (peek().text == "enum") {
                parseEnum();
                continue;
            }

            // struct / union (with optional qualifiers)
            if (peek().text == "struct" || peek().text == "const" ||
                peek().text == "__cppobj" || peek().text == "__declspec" ||
                peek().text == "__unaligned") {
                // Check if this is "const struct" or just "const"
                if (peek().text == "const") {
                    size_t saved = m_pos;
                    advance();
                    if (peek().text == "struct") {
                        parseStructOrUnion(false);
                        continue;
                    }
                    m_pos = saved;
                    // Not a const struct, skip
                    skipToSemicolon();
                    continue;
                }
                parseStructOrUnion(false);
                continue;
            }

            if (peek().text == "union") {
                parseStructOrUnion(true);
                continue;
            }

            // typedef
            if (peek().text == "typedef") {
                parseTypedef();
                continue;
            }

            // Forward declarations: "struct NAME;" or "union NAME;"
            if (peek().kind == TokenKind::Identifier) {
                skipToSemicolon();
                continue;
            }

            // Anything else, skip
            advance();
        }
    }

    // ── #pragma pack ────────────────────────────────────────────────────────────

    void HeaderParser::parsePragma() {
        // We're after #pragma, expect "pack"
        if (!matchIdentifier("pack")) {
            skipToSemicolon();
            return;
        }
        if (!match(TokenKind::LParen)) { skipToSemicolon(); return; }

        if (matchIdentifier("push")) {
            if (match(TokenKind::Comma)) {
                // pack(push, N)
                if (peek().kind == TokenKind::IntLiteral || peek().kind == TokenKind::HexLiteral) {
                    int val = static_cast<int>(std::strtol(advance().text.c_str(), nullptr, 0));
                    m_packStack.push(m_currentPack);
                    m_currentPack = val;
                }
            } else {
                m_packStack.push(m_currentPack);
            }
        } else if (matchIdentifier("pop")) {
            if (!m_packStack.empty()) {
                m_currentPack = m_packStack.top();
                m_packStack.pop();
            } else {
                m_currentPack = 8;
            }
        }

        // consume remaining tokens until )
        int depth = 1;
        while (!check(TokenKind::Eof) && depth > 0) {
            if (check(TokenKind::LParen)) ++depth;
            if (check(TokenKind::RParen)) --depth;
            advance();
        }
    }

    // ── Enum parsing ────────────────────────────────────────────────────────────

    void HeaderParser::parseEnum() {
        advance(); // consume "enum"

        TypeInfo info;
        info.kind = TypeKind::Enum;

        // Parse name (may be Namespace::Name or $HEXNAME)
        info.name = parseFullTypeName();

        // Optional base type ": __int32"
        if (match(TokenKind::Colon)) {
            // Consume the underlying type tokens
            info.underlyingType = "";
            while (!check(TokenKind::LBrace) && !check(TokenKind::Semicolon) && !check(TokenKind::Eof)) {
                info.underlyingType += peek().text;
                advance();
            }
        }
        if (info.underlyingType.empty()) info.underlyingType = "__int32";

        // Forward declaration
        if (match(TokenKind::Semicolon)) {
            info.size = 4;
            info.alignment = 4;
            if (!info.name.empty()) m_registry->registerType(std::move(info));
            return;
        }

        if (!match(TokenKind::LBrace)) {
            skipToSemicolon();
            return;
        }

        // Parse enum members
        while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
            EnumMember member;

            if (peek().kind != TokenKind::Identifier) {
                advance(); // skip unexpected token
                continue;
            }

            member.name = advance().text;

            if (match(TokenKind::Equals)) {
                // Parse value (may be hex, int, or negative)
                bool negative = false;
                if (peek().kind == TokenKind::NegLiteral) {
                    member.value = std::strtoll(advance().text.c_str(), nullptr, 0);
                } else if (peek().kind == TokenKind::HexLiteral) {
                    member.value = static_cast<int64_t>(std::strtoull(advance().text.c_str(), nullptr, 16));
                } else if (peek().kind == TokenKind::IntLiteral) {
                    member.value = std::strtoll(advance().text.c_str(), nullptr, 10);
                } else {
                    // Could be a reference to another enum value, skip
                    while (!check(TokenKind::Comma) && !check(TokenKind::RBrace) && !check(TokenKind::Eof))
                        advance();
                }
            }

            info.enumMembers.push_back(std::move(member));
            match(TokenKind::Comma);
        }

        match(TokenKind::RBrace);
        match(TokenKind::Semicolon);

        // Determine size from underlying type
        if (info.underlyingType.find("64") != std::string::npos) {
            info.size = 8; info.alignment = 8;
        } else if (info.underlyingType.find("16") != std::string::npos) {
            info.size = 2; info.alignment = 2;
        } else if (info.underlyingType.find("8") != std::string::npos) {
            info.size = 1; info.alignment = 1;
        } else {
            info.size = 4; info.alignment = 4;
        }

        if (!info.name.empty())
            m_registry->registerType(std::move(info));
    }

    // ── Struct/Union parsing ────────────────────────────────────────────────────

    void HeaderParser::parseStructOrUnion(bool isUnion) {
        TypeInfo info;
        info.kind = isUnion ? TypeKind::Union : TypeKind::Struct;

        uint32_t explicitAlign = 0;

        // Parse qualifiers before struct/union keyword
        while (true) {
            if (matchIdentifier("const")) continue;
            if (matchIdentifier("__cppobj")) continue;
            if (matchIdentifier("__unaligned")) continue;
            if (matchIdentifier("__declspec")) {
                // __declspec(align(N))
                if (match(TokenKind::LParen)) {
                    if (matchIdentifier("align")) {
                        if (match(TokenKind::LParen)) {
                            if (peek().kind == TokenKind::IntLiteral || peek().kind == TokenKind::HexLiteral) {
                                explicitAlign = static_cast<uint32_t>(std::strtoul(advance().text.c_str(), nullptr, 0));
                            }
                            match(TokenKind::RParen);
                        }
                    }
                    match(TokenKind::RParen);
                }
                continue;
            }
            break;
        }

        // Consume "struct" or "union" keyword
        if (peek().text == "struct" || peek().text == "union") {
            isUnion = (peek().text == "union");
            info.kind = isUnion ? TypeKind::Union : TypeKind::Struct;
            advance();
        }

        // More qualifiers after keyword
        while (true) {
            if (matchIdentifier("__cppobj")) continue;
            if (matchIdentifier("__unaligned")) continue;
            if (matchIdentifier("const")) continue;
            if (matchIdentifier("__declspec")) {
                if (match(TokenKind::LParen)) {
                    if (matchIdentifier("align")) {
                        if (match(TokenKind::LParen)) {
                            if (peek().kind == TokenKind::IntLiteral || peek().kind == TokenKind::HexLiteral) {
                                explicitAlign = static_cast<uint32_t>(std::strtoul(advance().text.c_str(), nullptr, 0));
                            }
                            match(TokenKind::RParen);
                        }
                    }
                    match(TokenKind::RParen);
                }
                continue;
            }
            break;
        }

        // Parse name
        info.name = parseFullTypeName();

        // Check for inheritance ": ParentType"
        if (match(TokenKind::Colon)) {
            info.parentType = parseFullTypeName();
        }

        // Forward declaration
        if (match(TokenKind::Semicolon)) {
            if (!info.name.empty() && !m_registry->findType(info.name)) {
                info.size = 0;
                m_registry->registerType(std::move(info));
            }
            return;
        }

        if (!match(TokenKind::LBrace)) {
            skipToSemicolon();
            return;
        }

        // Parse fields
        uint32_t currentOffset = 0;
        uint32_t maxFieldEnd = 0;
        uint32_t maxAlign = 1;

        // If inheriting, account for parent size
        if (!info.parentType.empty()) {
            uint32_t parentSize = m_registry->getSizeOf(info.parentType);
            if (parentSize > 0) {
                currentOffset = parentSize;
                maxFieldEnd = parentSize;
            }
        }

        while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
            // Skip VFT comments (already stripped by lexer)
            // Handle inline anonymous struct/union
            if (peek().text == "struct" || peek().text == "union") {
                size_t saved = m_pos;
                bool inlineUnion = (peek().text == "union");
                advance(); // struct/union

                // Check if this is an anonymous inline member
                if (check(TokenKind::LBrace)) {
                    // Anonymous inline struct/union
                    advance(); // {

                    // Parse fields of anonymous struct/union
                    uint32_t anonOffset = currentOffset;
                    uint32_t anonMaxEnd = 0;

                    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
                        // Parse field
                        ParsedType fieldType = parseTypeSpec();
                        if (fieldType.name.empty()) { skipToSemicolon(); continue; }

                        std::string fieldName;
                        if (peek().kind == TokenKind::Identifier) fieldName = advance().text;

                        // Array suffix
                        uint32_t arrayCount = 0;
                        if (match(TokenKind::LBracket)) {
                            if (peek().kind == TokenKind::IntLiteral || peek().kind == TokenKind::HexLiteral)
                                arrayCount = static_cast<uint32_t>(std::strtoul(advance().text.c_str(), nullptr, 0));
                            match(TokenKind::RBracket);
                        }

                        // Bitfield
                        uint8_t bitWidth = 0;
                        if (match(TokenKind::Colon)) {
                            if (peek().kind == TokenKind::IntLiteral || peek().kind == TokenKind::HexLiteral)
                                bitWidth = static_cast<uint8_t>(std::strtoul(advance().text.c_str(), nullptr, 0));
                        }

                        match(TokenKind::Semicolon);

                        // Resolve field type name
                        std::string resolvedTypeName = fieldType.name;
                        if (fieldType.pointerDepth > 0) {
                            resolvedTypeName += " *";
                        }

                        uint32_t fieldSize;
                        uint32_t fieldAlign;
                        if (fieldType.pointerDepth > 0) {
                            fieldSize = 4;
                            fieldAlign = 4;
                        } else {
                            fieldSize = m_registry->getSizeOf(resolvedTypeName);
                            fieldAlign = m_registry->getAlignOf(resolvedTypeName);
                        }

                        if (arrayCount > 0) fieldSize *= arrayCount;

                        FieldInfo fi;
                        fi.name = fieldName;
                        fi.typeName = resolvedTypeName;
                        fi.isPointer = fieldType.pointerDepth > 0;
                        fi.isConst = fieldType.isConst;
                        fi.bitWidth = bitWidth;
                        fi.size = fieldSize;

                        if (inlineUnion) {
                            fi.offset = currentOffset;
                            if (fieldSize > anonMaxEnd) anonMaxEnd = fieldSize;
                        } else {
                            fi.offset = computeFieldOffset(anonOffset, fieldSize, (std::min)(fieldAlign, static_cast<uint32_t>(m_currentPack)));
                            anonOffset = fi.offset + fieldSize;
                            anonMaxEnd = anonOffset - currentOffset;
                        }

                        info.fields.push_back(std::move(fi));
                    }

                    match(TokenKind::RBrace);
                    match(TokenKind::Semicolon);

                    if (isUnion) {
                        if (anonMaxEnd > maxFieldEnd) maxFieldEnd = anonMaxEnd;
                    } else {
                        if (inlineUnion) {
                            currentOffset += anonMaxEnd;
                        } else {
                            currentOffset += anonMaxEnd;
                        }
                        if (currentOffset > maxFieldEnd) maxFieldEnd = currentOffset;
                    }
                    continue;
                }

                // Not anonymous, restore and fall through to normal field parsing
                m_pos = saved;
            }

            // Parse type specifier for field
            ParsedType fieldType = parseTypeSpec();
            if (fieldType.name.empty()) {
                // Could be a function pointer field or something weird
                skipToSemicolon();
                continue;
            }

            // Check for function pointer field: RETTYPE (__convention *name)(params);
            if (check(TokenKind::LParen)) {
                // This looks like a function pointer: RetType (__stdcall *Name)(params);
                advance(); // (
                // Skip calling convention
                while (matchIdentifier("__stdcall") || matchIdentifier("__cdecl") ||
                       matchIdentifier("__thiscall") || matchIdentifier("__fastcall")) {}
                match(TokenKind::Star); // *

                std::string fpName;
                if (peek().kind == TokenKind::Identifier) fpName = advance().text;

                match(TokenKind::RParen);

                // Skip parameter list
                if (match(TokenKind::LParen)) {
                    int depth = 1;
                    while (depth > 0 && !check(TokenKind::Eof)) {
                        if (check(TokenKind::LParen)) ++depth;
                        if (check(TokenKind::RParen)) --depth;
                        advance();
                    }
                }

                match(TokenKind::Semicolon);

                // Add as function pointer field
                FieldInfo fi;
                fi.name = fpName;
                fi.typeName = "void *"; // treat function pointers as opaque pointers
                fi.isPointer = true;
                fi.size = 4;

                uint32_t fieldAlign = (std::min)(static_cast<uint32_t>(4), static_cast<uint32_t>(m_currentPack));
                if (isUnion) {
                    fi.offset = 0;
                    if (fi.size > maxFieldEnd) maxFieldEnd = fi.size;
                } else {
                    fi.offset = computeFieldOffset(currentOffset, fi.size, fieldAlign);
                    currentOffset = fi.offset + fi.size;
                    if (currentOffset > maxFieldEnd) maxFieldEnd = currentOffset;
                }
                if (fi.size > 0 && (fi.size / fieldAlign) > 0 && fieldAlign > maxAlign)
                    maxAlign = fieldAlign;

                info.fields.push_back(std::move(fi));
                continue;
            }

            // Parse field name
            std::string fieldName;
            if (peek().kind == TokenKind::Identifier) {
                fieldName = advance().text;
            }

            // Array suffix [N] — possibly multi-dimensional
            uint32_t totalArrayCount = 0;
            while (check(TokenKind::LBracket)) {
                advance(); // [
                uint32_t dim = 1;
                if (peek().kind == TokenKind::IntLiteral || peek().kind == TokenKind::HexLiteral)
                    dim = static_cast<uint32_t>(std::strtoul(advance().text.c_str(), nullptr, 0));
                match(TokenKind::RBracket);
                totalArrayCount = (totalArrayCount == 0) ? dim : totalArrayCount * dim;
            }

            // Bitfield ": N"
            uint8_t bitWidth = 0;
            if (match(TokenKind::Colon)) {
                if (peek().kind == TokenKind::IntLiteral || peek().kind == TokenKind::HexLiteral)
                    bitWidth = static_cast<uint8_t>(std::strtoul(advance().text.c_str(), nullptr, 0));
            }

            match(TokenKind::Semicolon);

            // Resolve type
            std::string resolvedTypeName = fieldType.name;
            bool isPtr = fieldType.pointerDepth > 0;
            if (isPtr) {
                // Register pointer type if needed
                std::string ptrTypeName = fieldType.name + " *";
                if (!m_registry->findType(ptrTypeName)) {
                    TypeInfo ptrInfo;
                    ptrInfo.name = ptrTypeName;
                    ptrInfo.kind = TypeKind::Pointer;
                    ptrInfo.size = 4;
                    ptrInfo.alignment = 4;
                    ptrInfo.pointeeType = fieldType.name;
                    ptrInfo.pointeeConst = fieldType.isConst;
                    m_registry->registerType(std::move(ptrInfo));
                }
                resolvedTypeName = ptrTypeName;
            }

            uint32_t fieldSize;
            uint32_t fieldAlign;
            if (isPtr) {
                fieldSize = 4;
                fieldAlign = 4;
            } else {
                fieldSize = m_registry->getSizeOf(resolvedTypeName);
                fieldAlign = m_registry->getAlignOf(resolvedTypeName);
            }

            if (totalArrayCount > 0) {
                fieldSize *= totalArrayCount;
            }

            FieldInfo fi;
            fi.name = fieldName;
            fi.isPointer = isPtr;
            fi.isConst = fieldType.isConst;
            fi.bitWidth = bitWidth;
            fi.size = fieldSize;

            if (totalArrayCount > 0 && !isPtr) {
                // Store as array type
                std::string arrayTypeName = fieldType.name + "[" + std::to_string(totalArrayCount) + "]";
                if (!m_registry->findType(arrayTypeName)) {
                    TypeInfo arrInfo;
                    arrInfo.name = arrayTypeName;
                    arrInfo.kind = TypeKind::Array;
                    arrInfo.elementType = fieldType.name;
                    arrInfo.arrayCount = totalArrayCount;
                    arrInfo.size = fieldSize;
                    arrInfo.alignment = fieldAlign;
                    m_registry->registerType(std::move(arrInfo));
                }
                fi.typeName = arrayTypeName;
            } else {
                fi.typeName = resolvedTypeName;
            }

            // Compute offset
            uint32_t effectiveAlign = fieldAlign;
            if (effectiveAlign > static_cast<uint32_t>(m_currentPack))
                effectiveAlign = static_cast<uint32_t>(m_currentPack);

            if (isUnion) {
                fi.offset = 0;
                if (fieldSize > maxFieldEnd) maxFieldEnd = fieldSize;
            } else {
                if (bitWidth > 0) {
                    // Simplified bitfield handling: just place at current offset
                    fi.offset = currentOffset;
                    // Don't advance offset for bitfields within the same storage unit
                } else {
                    fi.offset = computeFieldOffset(currentOffset, fieldSize, effectiveAlign);
                    currentOffset = fi.offset + fieldSize;
                    if (currentOffset > maxFieldEnd) maxFieldEnd = currentOffset;
                }
            }

            if (effectiveAlign > maxAlign) maxAlign = effectiveAlign;

            info.fields.push_back(std::move(fi));
        }

        match(TokenKind::RBrace);
        match(TokenKind::Semicolon);

        // Compute struct size with alignment padding
        if (explicitAlign > 0) info.alignment = explicitAlign;
        else info.alignment = maxAlign;

        info.size = alignTo(maxFieldEnd, info.alignment);

        if (!info.name.empty())
            m_registry->registerType(std::move(info));
    }

    // ── Typedef parsing ─────────────────────────────────────────────────────────

    void HeaderParser::parseTypedef() {
        advance(); // "typedef"

        // Handle "typedef struct NAME *ALIAS;"
        // Handle "typedef enum NAME ALIAS;"
        // Handle "typedef EXISTING NEW;"

        // Check for struct/union/enum keyword
        if (peek().text == "struct" || peek().text == "union" || peek().text == "enum") {
            std::string keyword = advance().text;

            ParsedType pt = parseTypeSpec();
            std::string baseName = pt.name;
            if (pt.pointerDepth > 0) baseName += " *";

            // The alias name
            std::string aliasName;
            if (peek().kind == TokenKind::Identifier)
                aliasName = advance().text;

            match(TokenKind::Semicolon);

            if (!aliasName.empty() && !baseName.empty()) {
                TypeInfo td;
                td.name = aliasName;
                td.kind = TypeKind::Typedef;
                td.aliasOf = baseName;
                td.size = m_registry->getSizeOf(baseName);
                td.alignment = m_registry->getAlignOf(baseName);
                m_registry->registerType(std::move(td));
            }
            return;
        }

        // Check for function pointer typedef: typedef RETTYPE (__conv *NAME)(params);
        // Or simple typedef: typedef OLD NEW;

        ParsedType pt = parseTypeSpec();

        // Function pointer typedef: typedef void (__stdcall *Name)(params);
        if (check(TokenKind::LParen)) {
            // Skip the whole thing
            skipToSemicolon();
            return;
        }

        // Simple typedef
        std::string aliasName;
        if (peek().kind == TokenKind::Identifier)
            aliasName = advance().text;

        match(TokenKind::Semicolon);

        std::string baseName = pt.name;
        if (pt.pointerDepth > 0) baseName += " *";

        if (!aliasName.empty() && !baseName.empty()) {
            TypeInfo td;
            td.name = aliasName;
            td.kind = TypeKind::Typedef;
            td.aliasOf = baseName;
            td.size = m_registry->getSizeOf(baseName);
            td.alignment = m_registry->getAlignOf(baseName);
            m_registry->registerType(std::move(td));
        }
    }

    // ── Type specifier parsing ──────────────────────────────────────────────────

    std::string HeaderParser::parseFullTypeName() {
        // Parses: Identifier (:: Identifier)* (< ... >)?
        // Also handles $HEXNAME anonymous types and <unnamed_type_xxx> patterns
        if (peek().kind != TokenKind::Identifier && peek().kind != TokenKind::LAngle) {
            return "";
        }

        std::string name;

        // Handle <unnamed_type_xxx> or <unnamed_tag>
        if (check(TokenKind::LAngle)) {
            name = "<";
            advance();
            while (!check(TokenKind::RAngle) && !check(TokenKind::Eof)) {
                name += peek().text;
                advance();
            }
            if (match(TokenKind::RAngle)) name += ">";
            // Continue if :: follows
        } else {
            name = advance().text;
        }

        // Handle :: scoping and template args
        while (true) {
            if (check(TokenKind::ColonColon)) {
                advance();
                name += "::";
                if (check(TokenKind::LAngle)) {
                    // <unnamed_type_xxx>
                    name += "<";
                    advance();
                    while (!check(TokenKind::RAngle) && !check(TokenKind::Eof)) {
                        name += peek().text;
                        advance();
                    }
                    if (match(TokenKind::RAngle)) name += ">";
                } else if (peek().kind == TokenKind::Identifier) {
                    name += advance().text;
                } else if (peek().kind == TokenKind::Tilde) {
                    // Destructor ~Name
                    name += "~";
                    advance();
                    if (peek().kind == TokenKind::Identifier) name += advance().text;
                }
                continue;
            }

            // Template args <T1, T2, ...>
            if (check(TokenKind::LAngle)) {
                name += "<";
                advance();
                int depth = 1;
                while (depth > 0 && !check(TokenKind::Eof)) {
                    if (check(TokenKind::LAngle)) { ++depth; name += "<"; advance(); continue; }
                    if (check(TokenKind::RAngle)) {
                        --depth;
                        if (depth > 0) { name += ">"; advance(); continue; }
                        else { name += ">"; advance(); break; }
                    }
                    name += peek().text;
                    // Add space after commas for readability
                    advance();
                }
                continue;
            }

            break;
        }

        return name;
    }

    HeaderParser::ParsedType HeaderParser::parseTypeSpec() {
        ParsedType result;

        // Skip qualifiers
        while (matchIdentifier("const") || matchIdentifier("volatile") ||
               matchIdentifier("__unaligned")) {
            result.isConst = true;
        }

        // Handle "unsigned" / "signed" prefix
        bool hasUnsigned = false;
        bool hasSigned = false;
        if (matchIdentifier("unsigned")) hasUnsigned = true;
        else if (matchIdentifier("signed")) hasSigned = true;

        // Skip qualifiers again
        while (matchIdentifier("const") || matchIdentifier("volatile") ||
               matchIdentifier("__unaligned")) {
            result.isConst = true;
        }

        // Handle "struct" or "union" prefix in type specs (e.g., "struct __crt_locale_data *locinfo")
        if (peek().text == "struct" || peek().text == "union" || peek().text == "enum") {
            advance(); // skip the keyword, just use the type name
        }

        // Base type name
        if (peek().kind == TokenKind::Identifier || peek().kind == TokenKind::LAngle) {
            std::string baseName = parseFullTypeName();

            // Handle "unsigned __int32" etc.
            if (hasUnsigned) {
                result.name = "unsigned " + baseName;
            } else if (hasSigned) {
                result.name = baseName; // signed is default
            } else {
                result.name = baseName;
            }
        } else if (hasUnsigned) {
            result.name = "unsigned int"; // bare "unsigned"
        } else {
            return result; // empty, parsing failed
        }

        // Post-type qualifiers
        while (matchIdentifier("const") || matchIdentifier("volatile"))
            result.isConst = true;

        // Pointer stars
        while (check(TokenKind::Star)) {
            advance();
            result.pointerDepth++;
            while (matchIdentifier("const") || matchIdentifier("volatile"))
                result.isPointerConst = true;
        }

        return result;
    }

    // ── Offset computation ──────────────────────────────────────────────────────

    uint32_t HeaderParser::computeFieldOffset(uint32_t currentOffset, uint32_t fieldSize, uint32_t fieldAlign) const {
        if (fieldAlign == 0) fieldAlign = 1;
        return alignTo(currentOffset, fieldAlign);
    }

    uint32_t HeaderParser::alignTo(uint32_t offset, uint32_t alignment) const {
        if (alignment <= 1) return offset;
        return (offset + alignment - 1) & ~(alignment - 1);
    }

} // namespace zirconium
