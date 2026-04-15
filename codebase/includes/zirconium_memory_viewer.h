#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "zirconium_type_registry.h"
#include "zirconium_header_parser.h"

namespace zirconium {

    // Simple address validity check via VirtualQuery
    bool isReadableMemory(uintptr_t addr, size_t len);

    // Direct memory read with page check
    template<typename T>
    bool safeRead(uintptr_t addr, T& out) {
        if (!isReadableMemory(addr, sizeof(T))) return false;
        out = *reinterpret_cast<const T*>(addr);
        return true;
    }

    template<typename T>
    bool safeWrite(uintptr_t addr, const T& value) {
        if (!isReadableMemory(addr, sizeof(T))) return false;
        *reinterpret_cast<T*>(addr) = value;
        return true;
    }

    struct MemoryViewerNode {
        std::string     label;
        std::string     typeName;
        uintptr_t       address   = 0;
        uint32_t        size      = 0;
        bool            isRoot    = false;

        bool                         childrenComputed = false;
        std::vector<MemoryViewerNode> children;

        void computeChildren(const TypeRegistry& reg);
        void invalidateChildren();
    };

    struct NavEntry {
        std::string label;
        std::string typeName;
        uintptr_t   address;
    };

    struct PredefinedGlobal {
        const char* name;
        uintptr_t   address;
        const char* typeName;
        bool        isAbsolute;
    };

    struct MemoryViewerCtx {
        TypeRegistry    registry;
        HeaderParser    parser;

        std::vector<MemoryViewerNode> roots;

        // Navigation history
        std::vector<NavEntry>   navHistory;
        int                     navCurrent = -1;

        // Manual entry
        char manualAddrBuf[20]      = {};
        char manualTypeFilter[128]  = {};
        std::vector<std::string> filteredTypes;
        int  selectedTypeIdx        = -1;

        // State
        bool headerLoaded = false;
        bool headerLoadFailed = false;
        std::string headerPath;
    };

    void __stdcall overlay_showMemoryViewer(MemoryViewerCtx& ctx);

} // namespace zirconium
