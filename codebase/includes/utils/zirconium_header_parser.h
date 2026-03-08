#pragma once

#include <string>
#include <vector>
#include <stack>

namespace zirconium {

    class TypeRegistry;

    class HeaderParser {
    public:
        bool parseFile(const std::string& filePath, TypeRegistry& registry);

        // Lexer types (public for static token usage)
        enum class TokenKind {
            Eof, Identifier, IntLiteral, HexLiteral, NegLiteral,
            Semicolon, Comma, Colon, ColonColon,
            LBrace, RBrace, LParen, RParen, LBracket, RBracket,
            Star, Ampersand, Equals, Hash, Tilde, Dot, Ellipsis,
            LAngle, RAngle,
            Unknown,
        };

        struct Token {
            TokenKind   kind = TokenKind::Eof;
            std::string text;
            int         line = 0;
        };

    private:
        // Lexer state
        const char* m_src    = nullptr;
        const char* m_end    = nullptr;
        const char* m_cur    = nullptr;
        int         m_line   = 1;
        std::string m_fileData;

        // Parser state
        std::vector<Token>  m_tokens;
        size_t              m_pos = 0;
        TypeRegistry*       m_registry = nullptr;
        std::stack<int>     m_packStack;
        int                 m_currentPack = 8; // default MSVC packing
        int                 m_warningCount = 0;

        // Lexer methods
        void tokenize();
        void skipWhitespaceAndComments();
        Token nextRawToken();

        // Parser helpers
        const Token& peek(int offset = 0) const;
        const Token& advance();
        bool check(TokenKind kind) const;
        bool match(TokenKind kind);
        bool matchIdentifier(const std::string& text);
        void expect(TokenKind kind);
        void skipToSemicolon();
        void skipBalancedBraces();

        // Top-level parsers
        void parseTopLevel();
        void parsePragma();
        void parseEnum();
        void parseStructOrUnion(bool isUnion);
        void parseTypedef();
        void parseForwardDecl(const std::string& keyword);

        // Struct field parsing
        struct ParsedType {
            std::string name;
            bool        isConst = false;
            int         pointerDepth = 0;
            bool        isPointerConst = false;
        };

        ParsedType parseTypeSpec();
        std::string parseFullTypeName();
        uint32_t computeFieldOffset(uint32_t currentOffset, uint32_t fieldSize, uint32_t fieldAlign) const;
        uint32_t alignTo(uint32_t offset, uint32_t alignment) const;

        void warn(const std::string& msg);
    };

} // namespace zirconium
