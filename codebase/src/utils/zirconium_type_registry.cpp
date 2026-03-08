#include "pch.h"
#include "zirconium_type_registry.h"
#include "zirconium_log.h"

#include <algorithm>

namespace zirconium {

    void TypeRegistry::registerBuiltinTypes() {
        auto addPrimitive = [this](const std::string& name, PrimitiveKind pk, uint32_t size) {
            TypeInfo ti;
            ti.name = name;
            ti.kind = TypeKind::Primitive;
            ti.primitiveKind = pk;
            ti.size = size;
            ti.alignment = size;
            m_types[name] = std::move(ti);
        };

        addPrimitive("void",               PrimitiveKind::Void,   0);
        addPrimitive("bool",               PrimitiveKind::Bool,   1);
        addPrimitive("_BOOL1",             PrimitiveKind::Bool,   1);
        addPrimitive("_BOOL4",             PrimitiveKind::UInt32, 4);
        addPrimitive("BOOL",               PrimitiveKind::Int32,  4);
        addPrimitive("char",               PrimitiveKind::Char,   1);
        addPrimitive("signed char",        PrimitiveKind::Char,   1);
        addPrimitive("unsigned char",      PrimitiveKind::UChar,  1);
        addPrimitive("wchar_t",            PrimitiveKind::UInt16, 2);
        addPrimitive("WCHAR",              PrimitiveKind::UInt16, 2);
        addPrimitive("__int8",             PrimitiveKind::Int8,   1);
        addPrimitive("unsigned __int8",    PrimitiveKind::UInt8,  1);
        addPrimitive("__int16",            PrimitiveKind::Int16,  2);
        addPrimitive("short",              PrimitiveKind::Int16,  2);
        addPrimitive("unsigned short",     PrimitiveKind::UInt16, 2);
        addPrimitive("unsigned __int16",   PrimitiveKind::UInt16, 2);
        addPrimitive("__int32",            PrimitiveKind::Int32,  4);
        addPrimitive("int",                PrimitiveKind::Int32,  4);
        addPrimitive("unsigned int",       PrimitiveKind::UInt32, 4);
        addPrimitive("unsigned __int32",   PrimitiveKind::UInt32, 4);
        addPrimitive("long",               PrimitiveKind::Int32,  4);
        addPrimitive("unsigned long",      PrimitiveKind::UInt32, 4);
        addPrimitive("__int64",            PrimitiveKind::Int64,  8);
        addPrimitive("long long",          PrimitiveKind::Int64,  8);
        addPrimitive("unsigned __int64",   PrimitiveKind::UInt64, 8);
        addPrimitive("unsigned long long", PrimitiveKind::UInt64, 8);
        addPrimitive("float",              PrimitiveKind::Float,  4);
        addPrimitive("double",             PrimitiveKind::Double, 8);

        // Windows types
        addPrimitive("BYTE",               PrimitiveKind::UInt8,  1);
        addPrimitive("WORD",               PrimitiveKind::UInt16, 2);
        addPrimitive("DWORD",              PrimitiveKind::UInt32, 4);
        addPrimitive("LONG",               PrimitiveKind::Int32,  4);
        addPrimitive("ULONG",              PrimitiveKind::UInt32, 4);
        addPrimitive("UINT",               PrimitiveKind::UInt32, 4);
        addPrimitive("USHORT",             PrimitiveKind::UInt16, 2);
        addPrimitive("CHAR",               PrimitiveKind::Char,   1);
        addPrimitive("UCHAR",              PrimitiveKind::UChar,  1);
        addPrimitive("HRESULT",            PrimitiveKind::Int32,  4);
        addPrimitive("INTERNET_PORT",      PrimitiveKind::UInt16, 2);
        addPrimitive("INTERNET_SCHEME",    PrimitiveKind::Int32,  4);

        // Common pointer aliases
        addPrimitive("LPSTR",              PrimitiveKind::UInt32, 4);  // char* on 32-bit
        addPrimitive("LPCSTR",             PrimitiveKind::UInt32, 4);
        addPrimitive("LPWSTR",             PrimitiveKind::UInt32, 4);
        addPrimitive("LPCWSTR",            PrimitiveKind::UInt32, 4);
        addPrimitive("LPCWCH",             PrimitiveKind::UInt32, 4);
        addPrimitive("HANDLE",             PrimitiveKind::UInt32, 4);
        addPrimitive("HWND",               PrimitiveKind::UInt32, 4);
        addPrimitive("HMODULE",            PrimitiveKind::UInt32, 4);
        addPrimitive("HINSTANCE",          PrimitiveKind::UInt32, 4);
        addPrimitive("HICON",              PrimitiveKind::UInt32, 4);
        addPrimitive("HCURSOR",            PrimitiveKind::UInt32, 4);
        addPrimitive("HBRUSH",             PrimitiveKind::UInt32, 4);
        addPrimitive("HMENU",              PrimitiveKind::UInt32, 4);
        addPrimitive("HDC",                PrimitiveKind::UInt32, 4);
        addPrimitive("HGDIOBJ",            PrimitiveKind::UInt32, 4);
        addPrimitive("LPVOID",             PrimitiveKind::UInt32, 4);
        addPrimitive("LPCVOID",            PrimitiveKind::UInt32, 4);
        addPrimitive("SIZE_T",             PrimitiveKind::UInt32, 4);

        // _GUID
        TypeInfo guid;
        guid.name = "_GUID";
        guid.kind = TypeKind::Struct;
        guid.size = 16;
        guid.alignment = 4;
        guid.fields.push_back({"Data1", "unsigned long",     0,  4, 0, 0, false, false});
        guid.fields.push_back({"Data2", "unsigned short",    4,  2, 0, 0, false, false});
        guid.fields.push_back({"Data3", "unsigned short",    6,  2, 0, 0, false, false});
        guid.fields.push_back({"Data4", "unsigned char",     8,  8, 0, 0, false, false}); // char[8]
        m_types["_GUID"] = std::move(guid);
        m_types["GUID"] = m_types["_GUID"];
        m_types["GUID"].name = "GUID";

        // LARGE_INTEGER
        TypeInfo largeInt;
        largeInt.name = "LARGE_INTEGER";
        largeInt.kind = TypeKind::Union;
        largeInt.size = 8;
        largeInt.alignment = 8;
        largeInt.fields.push_back({"QuadPart", "__int64", 0, 8, 0, 0, false, false});
        m_types["LARGE_INTEGER"] = std::move(largeInt);
    }

    void TypeRegistry::registerType(TypeInfo&& info) {
        m_types[info.name] = std::move(info);
    }

    const TypeInfo* TypeRegistry::findType(const std::string& name) const {
        auto it = m_types.find(name);
        if (it != m_types.end())
            return &it->second;
        return nullptr;
    }

    const TypeInfo* TypeRegistry::resolveType(const std::string& name) const {
        const TypeInfo* ti = findType(name);
        int depth = 0;
        while (ti && ti->kind == TypeKind::Typedef && depth < 32) {
            ti = findType(ti->aliasOf);
            ++depth;
        }
        return ti;
    }

    std::vector<std::string> TypeRegistry::searchTypes(const std::string& prefix, int maxResults) const {
        std::vector<std::string> results;
        if (prefix.empty()) return results;

        // Case-insensitive prefix search
        std::string lowerPrefix = prefix;
        std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);

        for (const auto& name : m_sortedNames) {
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(lowerPrefix) != std::string::npos) {
                results.push_back(name);
                if (static_cast<int>(results.size()) >= maxResults)
                    break;
            }
        }
        return results;
    }

    const char* TypeRegistry::enumValueToName(const TypeInfo* enumType, int64_t value) const {
        if (!enumType || enumType->kind != TypeKind::Enum) return nullptr;
        for (const auto& m : enumType->enumMembers) {
            if (m.value == value)
                return m.name.c_str();
        }
        return nullptr;
    }

    uint32_t TypeRegistry::getSizeOf(const std::string& typeName) const {
        const TypeInfo* ti = resolveType(typeName);
        if (!ti) return 0;

        if (ti->kind == TypeKind::Pointer) return 4; // 32-bit
        if (ti->kind == TypeKind::Array) {
            uint32_t elemSize = getSizeOf(ti->elementType);
            return elemSize * ti->arrayCount;
        }
        return ti->size;
    }

    uint32_t TypeRegistry::getAlignOf(const std::string& typeName) const {
        const TypeInfo* ti = resolveType(typeName);
        if (!ti) return 1;

        if (ti->kind == TypeKind::Pointer) return 4;
        if (ti->alignment > 0) return ti->alignment;
        return ti->size > 0 ? ti->size : 1;
    }

    void TypeRegistry::buildSortedNames() {
        m_sortedNames.clear();
        m_sortedNames.reserve(m_types.size());
        for (const auto& [name, _] : m_types) {
            // Skip anonymous types and pointer/array types from search
            if (name.empty() || name[0] == '$') continue;
            if (name.find(" *") != std::string::npos) continue;
            m_sortedNames.push_back(name);
        }
        std::sort(m_sortedNames.begin(), m_sortedNames.end());
    }

} // namespace zirconium
