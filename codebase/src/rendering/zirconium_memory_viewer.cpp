#include "pch.h"
#include "zirconium_memory_viewer.h"
#include "zirconium_globals.h"
#include "zirconium_game.h"
#include "zirconium_log.h"

#include <cstdio>
#include <algorithm>

namespace zirconium {

    // ── Memory safety (cached per frame) ────────────────────────────────────────

    // Cache VirtualQuery results per 4KB page to avoid syscall-per-read
    static std::unordered_map<uintptr_t, bool> s_pageCache;
    static int s_pageCacheFrame = -1;

    static void resetPageCacheIfNewFrame() {
        // Use a simple frame counter from ImGui
        int frame = ImGui::GetFrameCount();
        if (frame != s_pageCacheFrame) {
            s_pageCache.clear();
            s_pageCacheFrame = frame;
        }
    }

    bool isReadableMemory(uintptr_t addr, size_t len) {
        if (addr == 0) return false;
        resetPageCacheIfNewFrame();

        // Check each page the range touches
        uintptr_t pageSize = 4096;
        uintptr_t startPage = addr & ~(pageSize - 1);
        uintptr_t endPage = (addr + len - 1) & ~(pageSize - 1);

        for (uintptr_t page = startPage; page <= endPage; page += pageSize) {
            auto it = s_pageCache.find(page);
            if (it != s_pageCache.end()) {
                if (!it->second) return false;
                continue;
            }

            // Query this page
            MEMORY_BASIC_INFORMATION mbi;
            bool ok = false;
            if (VirtualQuery(reinterpret_cast<const void*>(page), &mbi, sizeof(mbi)) != 0) {
                if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
                    ok = true;
                }
            }
            s_pageCache[page] = ok;
            if (!ok) return false;
        }
        return true;
    }

    // ── Predefined globals ──────────────────────────────────────────────────────

    static const PredefinedGlobal PREDEFINED_GLOBALS[] = {
        { "Zombie Entity [0]",     ADDR_ZM_ENTITY_LIST_BASE, "gentity_t",  true  },
        { "View Matrix",           ADDR_VIEW_MATRIX,          "float[16]",  true  },
    };
    static constexpr int NUM_PREDEFINED = sizeof(PREDEFINED_GLOBALS) / sizeof(PREDEFINED_GLOBALS[0]);

    // ── Node child computation ──────────────────────────────────────────────────

    void MemoryViewerNode::computeChildren(const TypeRegistry& reg) {
        if (childrenComputed) return;
        childrenComputed = true;
        children.clear();

        const TypeInfo* ti = reg.resolveType(typeName);
        if (!ti) return;

        switch (ti->kind) {
            case TypeKind::Struct:
            case TypeKind::Union: {
                if (!ti->parentType.empty()) {
                    MemoryViewerNode baseNode;
                    baseNode.label = "[base: " + ti->parentType + "]";
                    baseNode.typeName = ti->parentType;
                    baseNode.address = address;
                    baseNode.size = reg.getSizeOf(ti->parentType);
                    children.push_back(std::move(baseNode));
                }
                for (const auto& field : ti->fields) {
                    MemoryViewerNode child;
                    child.label = field.name;
                    child.typeName = field.typeName;
                    child.address = address + field.offset;
                    child.size = field.size;
                    children.push_back(std::move(child));
                }
                break;
            }
            case TypeKind::Pointer: {
                uint32_t ptrVal = 0;
                if (safeRead<uint32_t>(address, ptrVal) && ptrVal != 0) {
                    MemoryViewerNode deref;
                    deref.label = "*" + label;
                    deref.typeName = ti->pointeeType;
                    deref.address = static_cast<uintptr_t>(ptrVal);
                    deref.size = reg.getSizeOf(ti->pointeeType);
                    children.push_back(std::move(deref));
                }
                break;
            }
            case TypeKind::Array: {
                uint32_t elemSize = reg.getSizeOf(ti->elementType);
                uint32_t count = ti->arrayCount;
                if (count > 256) count = 256;
                for (uint32_t i = 0; i < count; ++i) {
                    MemoryViewerNode child;
                    child.label = "[" + std::to_string(i) + "]";
                    child.typeName = ti->elementType;
                    child.address = address + i * elemSize;
                    child.size = elemSize;
                    children.push_back(std::move(child));
                }
                break;
            }
            case TypeKind::Typedef: {
                typeName = ti->aliasOf;
                childrenComputed = false;
                computeChildren(reg);
                break;
            }
            default:
                break;
        }
    }

    void MemoryViewerNode::invalidateChildren() {
        childrenComputed = false;
        children.clear();
    }

    // ── Value helpers ───────────────────────────────────────────────────────────

    static bool isExpandable(const TypeInfo* ti) {
        if (!ti) return false;
        switch (ti->kind) {
            case TypeKind::Struct:
            case TypeKind::Union:
            case TypeKind::Array:
                return true;
            case TypeKind::Pointer: return true; // always show as expandable, check null inside
            default: return false;
        }
    }

    static std::string getValuePreview(uintptr_t addr, const TypeInfo* ti, const TypeRegistry& reg) {
        if (!ti) return "???";
        char buf[128];

        switch (ti->kind) {
            case TypeKind::Primitive: {
                switch (ti->primitiveKind) {
                    case PrimitiveKind::Bool: {
                        uint8_t v; if (!safeRead(addr, v)) return "??";
                        return v ? "true" : "false";
                    }
                    case PrimitiveKind::Char: {
                        char v; if (!safeRead(addr, v)) return "??";
                        if (v >= 32 && v < 127) snprintf(buf, sizeof(buf), "'%c' (%d)", v, (int)(uint8_t)v);
                        else snprintf(buf, sizeof(buf), "%d (0x%02X)", (int)(uint8_t)v, (uint8_t)v);
                        return buf;
                    }
                    case PrimitiveKind::UChar:
                    case PrimitiveKind::UInt8: {
                        uint8_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%u (0x%02X)", v, v); return buf;
                    }
                    case PrimitiveKind::Int8: {
                        int8_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%d", v); return buf;
                    }
                    case PrimitiveKind::Int16: {
                        int16_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%d", v); return buf;
                    }
                    case PrimitiveKind::UInt16: {
                        uint16_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%u (0x%04X)", v, v); return buf;
                    }
                    case PrimitiveKind::Int32: {
                        int32_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%d (0x%08X)", v, (uint32_t)v); return buf;
                    }
                    case PrimitiveKind::UInt32: {
                        uint32_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%u (0x%08X)", v, v); return buf;
                    }
                    case PrimitiveKind::Int64: {
                        int64_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%lld", v); return buf;
                    }
                    case PrimitiveKind::UInt64: {
                        uint64_t v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%llu", v); return buf;
                    }
                    case PrimitiveKind::Float: {
                        float v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%.4f", v); return buf;
                    }
                    case PrimitiveKind::Double: {
                        double v; if (!safeRead(addr, v)) return "??";
                        snprintf(buf, sizeof(buf), "%.6f", v); return buf;
                    }
                    default: return "";
                }
            }
            case TypeKind::Pointer: {
                uint32_t v; if (!safeRead(addr, v)) return "??";
                if (v == 0) return "(null)";
                snprintf(buf, sizeof(buf), "0x%08X", v); return buf;
            }
            case TypeKind::Enum: {
                int32_t v; if (!safeRead(addr, v)) return "??";
                const char* name = reg.enumValueToName(ti, (int64_t)v);
                if (name) snprintf(buf, sizeof(buf), "%d", v);
                else snprintf(buf, sizeof(buf), "%d (0x%X)", v, (uint32_t)v);
                return buf;
            }
            case TypeKind::Array: {
                const TypeInfo* elemTi = reg.resolveType(ti->elementType);
                if (elemTi && (elemTi->primitiveKind == PrimitiveKind::Char || elemTi->primitiveKind == PrimitiveKind::UChar)) {
                    char strBuf[65] = {};
                    uint32_t readLen = (ti->arrayCount < 64) ? ti->arrayCount : 64;
                    for (uint32_t i = 0; i < readLen; ++i) {
                        uint8_t ch;
                        if (!safeRead(addr + i, ch)) break;
                        strBuf[i] = (char)ch;
                        if (ch == 0) break;
                    }
                    strBuf[64] = 0;
                    return "\"" + std::string(strBuf) + "\"";
                }
                snprintf(buf, sizeof(buf), "[%u]", ti->arrayCount);
                return buf;
            }
            case TypeKind::Struct:
            case TypeKind::Union:
                snprintf(buf, sizeof(buf), "{...}");
                return buf;
            case TypeKind::Typedef:
                return getValuePreview(addr, reg.resolveType(ti->aliasOf), reg);
            default: return "";
        }
    }

    static std::string getEnumName(uintptr_t addr, const TypeInfo* ti, const TypeRegistry& reg) {
        if (!ti || ti->kind != TypeKind::Enum) return "";
        int32_t v;
        if (!safeRead(addr, v)) return "";
        const char* name = reg.enumValueToName(ti, (int64_t)v);
        return name ? name : "";
    }

    // ── Edit control (rendered in the Value column) ─────────────────────────────

    static void renderValueEdit(uintptr_t addr, const TypeInfo* ti, int uniqueId) {
        if (!ti) return;

        char editId[32];
        snprintf(editId, sizeof(editId), "##ve_%08X_%d", (uint32_t)addr, uniqueId);

        ImGui::PushItemWidth(-1); // fill column

        switch (ti->kind) {
            case TypeKind::Primitive: {
                switch (ti->primitiveKind) {
                    case PrimitiveKind::Int32: {
                        int32_t v = 0;
                        if (safeRead(addr, v)) {
                            if (ImGui::InputScalar(editId, ImGuiDataType_S32, &v, nullptr, nullptr, "%d", ImGuiInputTextFlags_EnterReturnsTrue))
                                safeWrite(addr, v);
                        }
                        break;
                    }
                    case PrimitiveKind::UInt32: {
                        uint32_t v = 0;
                        if (safeRead(addr, v)) {
                            if (ImGui::InputScalar(editId, ImGuiDataType_U32, &v, nullptr, nullptr, "%u", ImGuiInputTextFlags_EnterReturnsTrue))
                                safeWrite(addr, v);
                        }
                        break;
                    }
                    case PrimitiveKind::Float: {
                        float v = 0;
                        if (safeRead(addr, v)) {
                            if (ImGui::InputScalar(editId, ImGuiDataType_Float, &v, nullptr, nullptr, "%.4f", ImGuiInputTextFlags_EnterReturnsTrue))
                                safeWrite(addr, v);
                        }
                        break;
                    }
                    case PrimitiveKind::Double: {
                        double v = 0;
                        if (safeRead(addr, v)) {
                            if (ImGui::InputScalar(editId, ImGuiDataType_Double, &v, nullptr, nullptr, "%.6f", ImGuiInputTextFlags_EnterReturnsTrue))
                                safeWrite(addr, v);
                        }
                        break;
                    }
                    case PrimitiveKind::Bool: {
                        uint8_t v = 0;
                        if (safeRead(addr, v)) {
                            bool bv = v != 0;
                            if (ImGui::Checkbox(editId, &bv))
                                safeWrite<uint8_t>(addr, bv ? 1 : 0);
                        }
                        break;
                    }
                    case PrimitiveKind::UInt16: {
                        uint16_t v = 0;
                        if (safeRead(addr, v)) {
                            int iv = v;
                            if (ImGui::InputInt(editId, &iv, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue))
                                safeWrite(addr, (uint16_t)(iv & 0xFFFF));
                        }
                        break;
                    }
                    case PrimitiveKind::Int16: {
                        int16_t v = 0;
                        if (safeRead(addr, v)) {
                            int iv = v;
                            if (ImGui::InputInt(editId, &iv, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue))
                                safeWrite(addr, (int16_t)iv);
                        }
                        break;
                    }
                    case PrimitiveKind::UInt8:
                    case PrimitiveKind::UChar: {
                        uint8_t v = 0;
                        if (safeRead(addr, v)) {
                            int iv = v;
                            if (ImGui::InputInt(editId, &iv, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue))
                                safeWrite(addr, (uint8_t)(iv & 0xFF));
                        }
                        break;
                    }
                    default: {
                        std::string val = getValuePreview(addr, ti, TypeRegistry());
                        ImGui::TextUnformatted(val.c_str());
                        break;
                    }
                }
                break;
            }

            case TypeKind::Enum: {
                int32_t currentVal = 0;
                if (!safeRead(addr, currentVal)) { ImGui::Text("??"); break; }

                const char* currentName = nullptr;
                for (const auto& m : ti->enumMembers)
                    if (m.value == currentVal) { currentName = m.name.c_str(); break; }

                const char* preview = currentName ? currentName : "???";
                if (ImGui::BeginCombo(editId, preview, ImGuiComboFlags_HeightLarge)) {
                    for (const auto& m : ti->enumMembers) {
                        bool selected = (m.value == currentVal);
                        if (ImGui::Selectable(m.name.c_str(), selected))
                            safeWrite(addr, (int32_t)m.value);
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                break;
            }

            default:
                break;
        }

        ImGui::PopItemWidth();
    }

    // ── Table-based tree node renderer ──────────────────────────────────────────

    static int s_nodeCounter = 0;

    // Set to >= 0 to remove a root after rendering the table
    static int s_rootRemoveIdx = -1;

    static void renderNodeRow(MemoryViewerNode& node, const TypeRegistry& reg, MemoryViewerCtx& ctx, int depth = 0, int rootIndex = -1) {
        const TypeInfo* ti = reg.resolveType(node.typeName);
        bool expandable = isExpandable(ti);

        // For pointers, check if null (not expandable if null)
        if (ti && ti->kind == TypeKind::Pointer) {
            uint32_t ptrVal = 0;
            if (!safeRead(node.address, ptrVal) || ptrVal == 0)
                expandable = false;
        }

        int nodeId = s_nodeCounter++;
        ImGui::TableNextRow();
        ImGui::PushID(nodeId);

        // Column 0: Name (with tree expansion)
        ImGui::TableSetColumnIndex(0);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
        if (!expandable)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (node.isRoot)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        bool opened = ImGui::TreeNodeEx(node.label.c_str(), flags);

        // Remove button for root nodes
        if (node.isRoot && rootIndex >= 0) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 0.7f));
            char rmId[16];
            snprintf(rmId, sizeof(rmId), "X##rm%d", rootIndex);
            if (ImGui::SmallButton(rmId))
                s_rootRemoveIdx = rootIndex;
            ImGui::PopStyleColor();
        }

        // Column 1: Type
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(node.typeName.c_str());

        // Column 2: Address
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("0x%08X", (uint32_t)node.address);

        // Hex dump tooltip on address hover
        if (ImGui::IsItemHovered() && node.size > 0) {
            ImGui::BeginTooltip();
            ImGui::Text("Size: %u bytes", node.size);
            uint32_t dumpLen = (node.size < 64) ? node.size : 64;
            if (dumpLen > 0 && isReadableMemory(node.address, dumpLen)) {
                ImGui::Separator();
                char hexLine[256];
                for (uint32_t row = 0; row < dumpLen; row += 16) {
                    int pos = snprintf(hexLine, sizeof(hexLine), "%04X: ", row);
                    for (uint32_t col = 0; col < 16 && (row + col) < dumpLen; ++col) {
                        uint8_t b;
                        if (safeRead(node.address + row + col, b))
                            pos += snprintf(hexLine + pos, sizeof(hexLine) - pos, "%02X ", b);
                        else
                            pos += snprintf(hexLine + pos, sizeof(hexLine) - pos, "?? ");
                    }
                    ImGui::TextUnformatted(hexLine);
                }
            }
            ImGui::EndTooltip();
        }

        // Column 3: Value (editable for primitives/enums, preview for others)
        ImGui::TableSetColumnIndex(3);
        if (ti && (ti->kind == TypeKind::Primitive || ti->kind == TypeKind::Enum)) {
            renderValueEdit(node.address, ti, nodeId);
        } else {
            std::string preview = getValuePreview(node.address, ti, reg);
            ImGui::TextUnformatted(preview.c_str());

            // "Go" button for non-null pointers
            if (ti && ti->kind == TypeKind::Pointer) {
                uint32_t ptrVal = 0;
                if (safeRead(node.address, ptrVal) && ptrVal != 0) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Go")) {
                        NavEntry entry;
                        entry.label = ti->pointeeType;
                        entry.typeName = ti->pointeeType;
                        entry.address = (uintptr_t)ptrVal;

                        if (ctx.navCurrent >= 0 && ctx.navCurrent < (int)ctx.navHistory.size() - 1)
                            ctx.navHistory.resize(ctx.navCurrent + 1);
                        ctx.navHistory.push_back(entry);
                        ctx.navCurrent = (int)ctx.navHistory.size() - 1;

                        MemoryViewerNode newRoot;
                        char lbl[64];
                        snprintf(lbl, sizeof(lbl), "%s @ 0x%08X", ti->pointeeType.c_str(), ptrVal);
                        newRoot.label = lbl;
                        newRoot.typeName = ti->pointeeType;
                        newRoot.address = (uintptr_t)ptrVal;
                        newRoot.size = reg.getSizeOf(ti->pointeeType);
                        newRoot.isRoot = true;
                        ctx.roots.push_back(std::move(newRoot));
                    }
                }
            }
        }

        // Column 4: Enum name (if applicable)
        ImGui::TableSetColumnIndex(4);
        if (ti && ti->kind == TypeKind::Enum) {
            std::string enumName = getEnumName(node.address, ti, reg);
            if (!enumName.empty())
                ImGui::TextUnformatted(enumName.c_str());
        }

        // Expand children
        if (opened && expandable) {
            node.computeChildren(reg);
            for (auto& child : node.children)
                renderNodeRow(child, reg, ctx, depth + 1);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    // ── Find header file relative to DLL ────────────────────────────────────────

    static std::string findHeaderFile() {
        char dllPath[MAX_PATH] = {};
        GetModuleFileNameA(g_hookCtx.hSelf, dllPath, MAX_PATH);

        std::string dir(dllPath);
        size_t lastSlash = dir.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            dir = dir.substr(0, lastSlash + 1);

        // Try same directory, parent, and grandparent
        std::string candidate = dir + "t6zm.exe.h";
        if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) return candidate;

        std::string parent = dir.substr(0, dir.size() - 1);
        lastSlash = parent.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            parent = parent.substr(0, lastSlash + 1);
            candidate = parent + "t6zm.exe.h";
            if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) return candidate;

            std::string grandparent = parent.substr(0, parent.size() - 1);
            lastSlash = grandparent.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                grandparent = grandparent.substr(0, lastSlash + 1);
                candidate = grandparent + "t6zm.exe.h";
                if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) return candidate;
            }
        }
        return "";
    }

    // ── Main render function ────────────────────────────────────────────────────

    void __stdcall overlay_showMemoryViewer(MemoryViewerCtx& ctx) {
        // Lazy-load the header
        if (!ctx.headerLoaded && !ctx.headerLoadFailed) {
            if (ctx.headerPath.empty())
                ctx.headerPath = findHeaderFile();

            if (ctx.headerPath.empty()) {
                ctx.headerLoadFailed = true;
                LOG_ERROR("[MemoryViewer] Could not find t6zm.exe.h");
            } else {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Parsing types from t6zm.exe.h...");
                if (ctx.registry.getState() == TypeRegistry::State::NotLoaded) {
                    bool ok = ctx.parser.parseFile(ctx.headerPath, ctx.registry);
                    if (ok) {
                        ctx.headerLoaded = true;
                        LOG_INFO("[MemoryViewer] Header parsed: %zu types", ctx.registry.typeCount());
                    } else {
                        ctx.headerLoadFailed = true;
                        LOG_ERROR("[MemoryViewer] Parse failed: %s", ctx.registry.getError().c_str());
                    }
                }
                return;
            }
        }

        if (ctx.headerLoadFailed) {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to load type information.");
            ImGui::TextWrapped("Place t6zm.exe.h next to the DLL or in the project root.");
            static char pathBuf[MAX_PATH] = {};
            ImGui::InputText("Header path", pathBuf, sizeof(pathBuf));
            if (ImGui::Button("Load") && pathBuf[0] != '\0') {
                ctx.headerPath = pathBuf;
                ctx.headerLoadFailed = false;
                ctx.headerLoaded = false;
                ctx.registry = TypeRegistry();
            }
            return;
        }

        // ── Navigation bar ──────────────────────────────────────────────────
        if (!ctx.navHistory.empty()) {
            bool canBack = ctx.navCurrent > 0;
            bool canFwd = ctx.navCurrent < (int)ctx.navHistory.size() - 1;

            if (!canBack) ImGui::BeginDisabled();
            if (ImGui::SmallButton("< Back")) --ctx.navCurrent;
            if (!canBack) ImGui::EndDisabled();

            ImGui::SameLine();
            if (!canFwd) ImGui::BeginDisabled();
            if (ImGui::SmallButton("Forward >")) ++ctx.navCurrent;
            if (!canFwd) ImGui::EndDisabled();

            if (ctx.navCurrent >= 0 && ctx.navCurrent < (int)ctx.navHistory.size()) {
                ImGui::SameLine();
                auto& cur = ctx.navHistory[ctx.navCurrent];
                ImGui::Text("  %s @ 0x%08X", cur.typeName.c_str(), (uint32_t)cur.address);
            }
            ImGui::Separator();
        }

        // ── Add Root section ────────────────────────────────────────────────
        if (ImGui::CollapsingHeader("Add Root Entry")) {
            ImGui::Text("Predefined:");
            ImGui::Indent();
            for (int i = 0; i < NUM_PREDEFINED; ++i) {
                const auto& pg = PREDEFINED_GLOBALS[i];
                char btnLabel[128];
                snprintf(btnLabel, sizeof(btnLabel), "%s (0x%08X : %s)##p%d",
                         pg.name, (uint32_t)pg.address, pg.typeName, i);
                if (ImGui::Button(btnLabel)) {
                    MemoryViewerNode root;
                    root.label = pg.name;
                    root.typeName = pg.typeName;
                    root.address = pg.address;
                    root.size = ctx.registry.getSizeOf(pg.typeName);
                    root.isRoot = true;
                    ctx.roots.push_back(std::move(root));
                }
            }
            ImGui::Unindent();

            ImGui::Spacing();
            ImGui::Text("Manual:");
            ImGui::Indent();
            ImGui::PushItemWidth(200);
            ImGui::InputText("Address##m", ctx.manualAddrBuf, sizeof(ctx.manualAddrBuf));

            bool filterChanged = ImGui::InputText("Type##f", ctx.manualTypeFilter, sizeof(ctx.manualTypeFilter));
            ImGui::PopItemWidth();

            if (filterChanged) {
                ctx.filteredTypes = ctx.registry.searchTypes(ctx.manualTypeFilter, 30);
                ctx.selectedTypeIdx = -1;
            }

            if (!ctx.filteredTypes.empty()) {
                int listH = (std::min)(8, (int)ctx.filteredTypes.size());
                if (ImGui::BeginListBox("##tl", ImVec2(300, ImGui::GetTextLineHeightWithSpacing() * listH))) {
                    for (int i = 0; i < (int)ctx.filteredTypes.size(); ++i) {
                        if (ImGui::Selectable(ctx.filteredTypes[i].c_str(), ctx.selectedTypeIdx == i)) {
                            ctx.selectedTypeIdx = i;
                            strncpy(ctx.manualTypeFilter, ctx.filteredTypes[i].c_str(), sizeof(ctx.manualTypeFilter) - 1);
                        }
                    }
                    ImGui::EndListBox();
                }
            }

            if (ImGui::Button("Add##m")) {
                uintptr_t addr = (uintptr_t)std::strtoul(ctx.manualAddrBuf, nullptr, 16);
                std::string typeName = ctx.manualTypeFilter;
                if (addr != 0 && !typeName.empty() && ctx.registry.findType(typeName)) {
                    MemoryViewerNode root;
                    char lbl[128];
                    snprintf(lbl, sizeof(lbl), "%s @ 0x%08X", typeName.c_str(), (uint32_t)addr);
                    root.label = lbl;
                    root.typeName = typeName;
                    root.address = addr;
                    root.size = ctx.registry.getSizeOf(typeName);
                    root.isRoot = true;
                    ctx.roots.push_back(std::move(root));
                }
            }
            ImGui::Unindent();
        }

        ImGui::Separator();

        // ── Toolbar ─────────────────────────────────────────────────────────
        if (!ctx.roots.empty()) {
            if (ImGui::Button("Refresh All")) {
                for (auto& r : ctx.roots) r.invalidateChildren();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear All")) {
                ctx.roots.clear();
                ctx.navHistory.clear();
                ctx.navCurrent = -1;
            }
            ImGui::Separator();
        }

        // ── Main table ──────────────────────────────────────────────────────
        if (ctx.roots.empty()) {
            ImGui::TextDisabled("No entries. Use 'Add Root Entry' above.");
            return;
        }

        static ImGuiTableFlags tblFlags =
            ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoBordersInBody;

        float availH = ImGui::GetContentRegionAvail().y;
        if (ImGui::BeginTable("##TypeViewerTable", 5, tblFlags, ImVec2(0, availH))) {
            // Setup columns
            ImGui::TableSetupColumn("Name",    ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch, 0.30f);
            ImGui::TableSetupColumn("Type",    ImGuiTableColumnFlags_WidthStretch, 0.20f);
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Value",   ImGuiTableColumnFlags_WidthStretch, 0.30f);
            ImGui::TableSetupColumn("Enum",    ImGuiTableColumnFlags_WidthStretch, 0.20f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            s_nodeCounter = 0;

            s_rootRemoveIdx = -1;
            for (int i = 0; i < (int)ctx.roots.size(); ++i) {
                ImGui::PushID(i + 10000);
                renderNodeRow(ctx.roots[i], ctx.registry, ctx, 0, i);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        // Deferred root removal (after table rendering is done)
        if (s_rootRemoveIdx >= 0 && s_rootRemoveIdx < (int)ctx.roots.size()) {
            ctx.roots.erase(ctx.roots.begin() + s_rootRemoveIdx);
            s_rootRemoveIdx = -1;
        }
    }

} // namespace zirconium
