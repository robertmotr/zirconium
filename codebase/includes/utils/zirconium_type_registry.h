#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace zirconium {

    enum class PrimitiveKind : uint8_t {
        Void,
        Bool,
        Char, UChar,
        Int8, UInt8,
        Int16, UInt16,
        Int32, UInt32,
        Int64, UInt64,
        Float, Double,
    };

    enum class TypeKind : uint8_t {
        Primitive,
        Enum,
        Struct,
        Union,
        Typedef,
        Pointer,
        Array,
        FunctionPtr,
    };

    struct EnumMember {
        std::string name;
        int64_t     value;
    };

    struct FieldInfo {
        std::string  name;
        std::string  typeName;
        uint32_t     offset   = 0;
        uint32_t     size     = 0;
        uint8_t      bitWidth = 0;    // 0 = not a bitfield
        uint8_t      bitOffset = 0;
        bool         isPointer = false;
        bool         isConst   = false;
    };

    struct TypeInfo {
        std::string              name;
        TypeKind                 kind       = TypeKind::Primitive;
        uint32_t                 size       = 0;
        uint32_t                 alignment  = 0;

        // Primitive
        PrimitiveKind            primitiveKind = PrimitiveKind::Void;

        // Enum
        std::string              underlyingType;
        std::vector<EnumMember>  enumMembers;

        // Struct/Union
        std::vector<FieldInfo>   fields;
        std::string              parentType;

        // Typedef
        std::string              aliasOf;

        // Pointer
        std::string              pointeeType;
        bool                     pointeeConst = false;

        // Array
        std::string              elementType;
        uint32_t                 arrayCount = 0;

        // FunctionPtr
        std::string              funcSignature;
    };

    class TypeRegistry {
    public:
        enum class State { NotLoaded, Loading, Ready, Error };

        State getState() const { return m_state; }
        const std::string& getError() const { return m_error; }

        void registerBuiltinTypes();
        void registerType(TypeInfo&& info);

        const TypeInfo* findType(const std::string& name) const;
        const TypeInfo* resolveType(const std::string& name) const;

        const std::vector<std::string>& getTypeNames() const { return m_sortedNames; }
        std::vector<std::string> searchTypes(const std::string& prefix, int maxResults = 20) const;

        const char* enumValueToName(const TypeInfo* enumType, int64_t value) const;

        uint32_t getSizeOf(const std::string& typeName) const;
        uint32_t getAlignOf(const std::string& typeName) const;

        void buildSortedNames();

        void setState(State s) { m_state = s; }
        void setError(const std::string& err) { m_error = err; m_state = State::Error; }

        size_t typeCount() const { return m_types.size(); }

    private:
        State                                       m_state = State::NotLoaded;
        std::string                                 m_error;
        std::unordered_map<std::string, TypeInfo>    m_types;
        std::vector<std::string>                     m_sortedNames;
    };

} // namespace zirconium
