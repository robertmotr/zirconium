#include "pch.h"

#include "zirconium_render.h"
#include "zirconium_overlay.h"
#include "zirconium_globals.h"
#include "zirconium_imgui_style.h"
#include "zirconium_game.h"
#include "zirconium_memory_viewer.h"

zirconium::RenderCtx  zirconium::g_renderCtx{};
zirconium::OverlayCtx zirconium::g_overlayCtx{};

/*
* Hooked window procedure. Forwards messages to ImGui first.
* If ImGui wants keyboard/mouse input, we consume the message instead of passing it to the game.
*/
LRESULT CALLBACK hookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    ImGuiIO *io = zirconium::g_renderCtx.io;

    // if ImGui wants the keyboard, eat keyboard messages
    if (io->WantCaptureKeyboard && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP))
        return true;

    // if ImGui wants the mouse, eat mouse messages
    if (io->WantCaptureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST))
        return true;

    return CallWindowProc(zirconium::g_renderCtx.oWndProc, hWnd, msg, wParam, lParam);
}

/*
* Styles the ImGui overlay.
* credits to https://github.com/GraphicsProgramming/dear-imgui-styles
*/
void __stdcall zirconium::overlay_setImGuiStyle() {
    using namespace ImGui;

    ImGuiStyle* style = &ImGui::GetStyle();
    style->GrabRounding = 4.0f;

    ImVec4* colors = style->Colors;
    colors[ImGuiCol_Text] = ColorConvertU32ToFloat4(Spectrum::GRAY800); // text on hovered controls is gray900
    colors[ImGuiCol_TextDisabled] = ColorConvertU32ToFloat4(Spectrum::GRAY500);
    colors[ImGuiCol_WindowBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ColorConvertU32ToFloat4(Spectrum::GRAY50); // not sure about this. Note: applies to tooltips too.
    colors[ImGuiCol_Border] = ColorConvertU32ToFloat4(Spectrum::GRAY300);
    colors[ImGuiCol_BorderShadow] = ColorConvertU32ToFloat4(Spectrum::Static::NONE); // We don't want shadows. Ever.
    colors[ImGuiCol_FrameBg] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // this isnt right, spectrum does not do this, but it's a good fallback
    colors[ImGuiCol_FrameBgHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
    colors[ImGuiCol_FrameBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
    colors[ImGuiCol_TitleBg] = ColorConvertU32ToFloat4(Spectrum::GRAY300); // those titlebar values are totally made up, spectrum does not have this.
    colors[ImGuiCol_TitleBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
    colors[ImGuiCol_TitleBgCollapsed] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    colors[ImGuiCol_MenuBarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
    colors[ImGuiCol_ScrollbarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100); // same as regular background
    colors[ImGuiCol_ScrollbarGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    colors[ImGuiCol_ScrollbarGrabHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
    colors[ImGuiCol_ScrollbarGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    colors[ImGuiCol_CheckMark] = ColorConvertU32ToFloat4(Spectrum::BLUE500);
    colors[ImGuiCol_SliderGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    colors[ImGuiCol_SliderGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY800);
    colors[ImGuiCol_Button] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // match default button to Spectrum's 'Action Button'.
    colors[ImGuiCol_ButtonHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
    colors[ImGuiCol_ButtonActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
    colors[ImGuiCol_Header] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
    colors[ImGuiCol_HeaderHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE500);
    colors[ImGuiCol_HeaderActive] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
    colors[ImGuiCol_Separator] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    colors[ImGuiCol_SeparatorHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
    colors[ImGuiCol_SeparatorActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    colors[ImGuiCol_ResizeGrip] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
    colors[ImGuiCol_ResizeGripHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
    colors[ImGuiCol_ResizeGripActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
    colors[ImGuiCol_PlotLines] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
    colors[ImGuiCol_PlotLinesHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
    colors[ImGuiCol_PlotHistogram] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
    colors[ImGuiCol_PlotHistogramHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
    colors[ImGuiCol_TextSelectedBg] = ColorConvertU32ToFloat4((Spectrum::BLUE400 & 0x00FFFFFF) | 0x33000000);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ColorConvertU32ToFloat4((Spectrum::GRAY900 & 0x00FFFFFF) | 0x0A000000);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

bool __stdcall zirconium::overlay_memTbl_init() {
    auto& tbl = g_overlayCtx.memoryTable;
    tbl.clear();

    if (!g_addrs.resolve()) {
        LOG_ERROR("Failed to resolve game addresses.");
        return false;
    }

    // player
    tbl.push_back({ "Player struct",                   (uintptr_t)0x02188290, "N/A",  "Player struct base address", "Unknown" });
    tbl.push_back({ "Health",                          (uintptr_t)g_addrs.playerHealth,    0u,   "Realtime player health", "Unknown" });
    tbl.push_back({ "Health maximum",                  (uintptr_t)g_addrs.playerHealthMax, 0u,   "Maximum player health, 100 w/o Juggernog, 150 otherwise", "Unknown" });
    tbl.push_back({ "X-coordinate",                    (uintptr_t)g_addrs.playerPosX,      0.0f, "Player x-coordinate", "Player" });
    tbl.push_back({ "Y-coordinate",                    (uintptr_t)g_addrs.playerPosY,      0.0f, "Player y-coordinate", "Player" });
    tbl.push_back({ "Z-coordinate",                    (uintptr_t)g_addrs.playerPosZ,      0.0f, "Player z-coordinate", "Player" });
    tbl.push_back({ "Gravity",                         (uintptr_t)g_addrs.playerGravity,   0u,   "Local player gravity multiplier", "Player" });
    tbl.push_back({ "Static speed",                    (uintptr_t)g_addrs.playerSpeed,     0u,   "Local player speed", "Player" });
    tbl.push_back({ "Local name",                      (uintptr_t)g_addrs.playerName,      "name", "Local player name", "Player" });
    tbl.push_back({ "View Angle X",                    (uintptr_t)g_addrs.viewAngleX,      0.0f, "Local player view angle X", "Player" });
    tbl.push_back({ "View Angle Y",                    (uintptr_t)g_addrs.viewAngleY,      0.0f, "Local player view angle Y", "Player" });

    // weapons/inventory
    tbl.push_back({ "Primary weapon reserve ammo",     (uintptr_t)g_addrs.primaryReserveAmmo,   0u, "Primary weapon reserve ammo", "WeaponStats" });
    tbl.push_back({ "Secondary weapon reserve ammo",   (uintptr_t)g_addrs.secondaryReserveAmmo, 0u, "Secondary weapon reserve ammo", "WeaponStats" });
    tbl.push_back({ "Grenades",                        (uintptr_t)g_addrs.grenades,    0u, "Number of grenades", "WeaponStats" });
    tbl.push_back({ "Claymores",                       (uintptr_t)g_addrs.claymores,   0u, "Number of claymores", "WeaponStats" });
    tbl.push_back({ "Monkey bombs",                    (uintptr_t)g_addrs.monkeyBombs, 0u, "Number of monkey bombs", "WeaponStats" });
    tbl.push_back({ "Money",                           (uintptr_t)g_addrs.money,       0u, "Player money", "PlayerStats" });

    // game state
    tbl.push_back({ "Zombie round display",            (uintptr_t)g_addrs.zombieRound,     0u, "Current zombie round display", "GameStats" });
    tbl.push_back({ "Zombies spawned in",              (uintptr_t)g_addrs.zombiesSpawned,  0u, "Number of zombies spawned in", "GameStats" });

    tbl.push_back({ "Player struct with offset", (uintptr_t)OFF_PLAYER_STRUCT_ALT, "N/A", "Player struct with offset", "Player" });

    // view matrix
    for (int i = 0; i < 16; ++i) {
        uintptr_t addr = (uintptr_t)g_addrs.viewMatrix + (i * 0x4);
        std::string name = "viewMatrix[" + std::to_string(i) + "]";
        std::string description = "View matrix element [" + std::to_string(i) + "]";
        tbl.push_back({ name, addr, 0.0f, description, "Camera" });
    }

    return true;
}

void __stdcall zirconium::overlay_memTbl_sort() {
    ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
    if (sortSpecs && sortSpecs->SpecsDirty) {
        std::sort(g_overlayCtx.memoryTable.begin(), g_overlayCtx.memoryTable.end(),
            [sortSpecs](const memoryTableEntry& a, const memoryTableEntry& b) {
                for (int n = 0; n < sortSpecs->SpecsCount; n++) {
                    const ImGuiTableColumnSortSpecs* spec = &sortSpecs->Specs[n];
                    int delta = 0;
                    switch (spec->ColumnUserID) {
                    case 0: // Name
                        delta = a.name.compare(b.name);
                        break;
                    case 1: // Hex Address
                        delta = (a.addr < b.addr) ? -1 : (a.addr > b.addr) ? 1 : 0;
                        break;
                    case 2: // Type
                        delta = a.value.index() - b.value.index(); // Compare based on type index
                        break;
                    case 3: // Value
                        delta = zirconium::overlay_memTbl_compareValues(a.value, b.value);
                        break;
                    case 4: // Description
                        delta = a.description.compare(b.description);
                        break;
                    case 5: // Struct
                        delta = a.structName.compare(b.structName);
                        break;
                    default:
                        delta = 0;
                        break;
                    }
                    if (delta != 0) {
                        if (spec->SortDirection == ImGuiSortDirection_Descending)
                            delta = -delta;
                        return delta < 0;
                    }
                }
                return false;
            });
        sortSpecs->SpecsDirty = false;
    }
}

int __stdcall zirconium::overlay_memTbl_compareValues(const MEM_TYPES& a, const MEM_TYPES& b) {
    if (a.index() != b.index())
        return a.index() - b.index();

    if (std::holds_alternative<unsigned int>(a)) {
        unsigned int va = std::get<unsigned int>(a);
        unsigned int vb = std::get<unsigned int>(b);
        return (va < vb) ? -1 : (va > vb) ? 1 : 0;
    }
    else if (std::holds_alternative<float>(a)) {
        float va = std::get<float>(a);
        float vb = std::get<float>(b);
        return (va < vb) ? -1 : (va > vb) ? 1 : 0;
    }
    else if (std::holds_alternative<std::string>(a)) {
        const std::string& va = std::get<std::string>(a);
        const std::string& vb = std::get<std::string>(b);
        return va.compare(vb);
    }
    return 0;
}

void __stdcall zirconium::overlay_showMemTblTab() {
    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Sortable |
        ImGuiTableFlags_Resizable;

    if (ImGui::BeginTable("MemTable", 6, tableFlags)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort, -1.0f, 0);
        ImGui::TableSetupColumn("Hex Address", ImGuiTableColumnFlags_DefaultSort, -1.0f, 1);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_DefaultSort, -1.0f, 2);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_None, -1.0f, 3);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_DefaultSort, -1.0f, 4);
        ImGui::TableSetupColumn("Struct", ImGuiTableColumnFlags_DefaultSort, -1.0f, 5);
        ImGui::TableHeadersRow();

        zirconium::overlay_memTbl_sort();

        int rowIndex = 0;

        for (auto& entry : g_overlayCtx.memoryTable) {
            ImGui::TableNextRow();

            // Column 0: Name
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(entry.name.c_str());

            // Column 1: Hex Address
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%08X", entry.addr);

            // hex editor (readonly) tooltip
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Memory at 0x%08X:", entry.addr);

                static unsigned char buffer[128] = {};

                memcpy(buffer, reinterpret_cast<void*>(entry.addr), sizeof(buffer));

                ImGui::Text("Hex Dump:");
                ImGui::BeginTable("HexDump", 8);
                for (int i = 0; i < 128; ++i) {
                    if (i % 8 == 0)
                        ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(i % 8);
                    ImGui::Text("%02X", buffer[i]);
                }

                ImGui::EndTable();
                ImGui::EndTooltip();
            }

            // Column 2: Type
            ImGui::TableSetColumnIndex(2);
            std::string typeStr;
            if (std::holds_alternative<unsigned int>(entry.value)) {
                typeStr = "unsigned int";
            }
            else if (std::holds_alternative<float>(entry.value)) {
                typeStr = "float";
            }
            else if (std::holds_alternative<std::string>(entry.value)) {
                typeStr = "string";
            }
            else {
                typeStr = "unknown";
            }
            ImGui::TextUnformatted(typeStr.c_str());

            // Column 3: Value (Editable)
            ImGui::TableSetColumnIndex(3);
            char idBuffer[32];
            sprintf_s(idBuffer, "##value_%d", rowIndex);

            if (std::holds_alternative<unsigned int>(entry.value)) {
                unsigned int& value = std::get<unsigned int>(entry.value);
                value = *reinterpret_cast<unsigned int*>(entry.addr);

                if (ImGui::InputScalar(idBuffer, ImGuiDataType_U32, &value)) {
                    *reinterpret_cast<unsigned int*>(entry.addr) = value;
                }
            }
            else if (std::holds_alternative<float>(entry.value)) {
                float& value = std::get<float>(entry.value);
                value = *reinterpret_cast<float*>(entry.addr);

                if (ImGui::InputFloat(idBuffer, &value)) {
                    *reinterpret_cast<float*>(entry.addr) = value;
                }
            }
            else if (std::holds_alternative<std::string>(entry.value)) {
                char buffer[256];
                strncpy_s(buffer, reinterpret_cast<const char*>(entry.addr), sizeof(buffer));

                if (ImGui::InputText(idBuffer, buffer, sizeof(buffer))) {
                    strcpy_s(reinterpret_cast<char*>(entry.addr), sizeof(buffer), buffer);
                }
            }
            else {
                ImGui::Text("Unknown type");
            }

            // Column 4: Description
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(entry.description.c_str());

            // Column 5: Struct
            ImGui::TableSetColumnIndex(5);
            ImGui::TextUnformatted(entry.structName.c_str());

            rowIndex++;
        }
        ImGui::EndTable();
    }

    // ImGui::Spacing();
    // ImGui::Separator();
    // ImGui::Spacing();

    // if (!g_addrs.zmEntityList) return;

    // ImGui::Text("Zombie Entity List (%d slots)", ZM_ENTITY_LIST_SZ);
    // ImGui::Spacing();

    // for (int i = 0; i < ZM_ENTITY_LIST_SZ; i++) {
    //     const Zombie& zm = g_addrs.zmEntityList[i];
    //     uintptr_t zmBase = (uintptr_t)&g_addrs.zmEntityList[i];

    //     char nodeLabel[64];
    //     std::snprintf(nodeLabel, sizeof(nodeLabel), "zm_entity_list[%d] - HP: %u/%u %s",
    //         i, zm.currentHealth, zm.startHealth, zm.isAlive() ? "(alive)" : "(dead)");

    //     if (!ImGui::TreeNode(nodeLabel))
    //         continue;

    //     static ImGuiTableFlags zmTblFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
    //     char tblId[32];
    //     std::snprintf(tblId, sizeof(tblId), "##zmTbl_%d", i);

    //     if (ImGui::BeginTable(tblId, 4, zmTblFlags)) {
    //         ImGui::TableSetupColumn("Field");
    //         ImGui::TableSetupColumn("Address");
    //         ImGui::TableSetupColumn("Type");
    //         ImGui::TableSetupColumn("Value");
    //         ImGui::TableHeadersRow();

    //         // helper lambda to add a row
    //         auto addFloatRow = [&](const char* name, uintptr_t addr) {
    //             ImGui::TableNextRow();
    //             ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name);
    //             ImGui::TableSetColumnIndex(1); ImGui::Text("0x%08X", addr);
    //             ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted("float");
    //             ImGui::TableSetColumnIndex(3); ImGui::Text("%.4f", *reinterpret_cast<float*>(addr));
    //         };
    //         auto addUintRow = [&](const char* name, uintptr_t addr) {
    //             ImGui::TableNextRow();
    //             ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name);
    //             ImGui::TableSetColumnIndex(1); ImGui::Text("0x%08X", addr);
    //             ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted("unsigned int");
    //             ImGui::TableSetColumnIndex(3); ImGui::Text("%u", *reinterpret_cast<unsigned int*>(addr));
    //         };

    //         addFloatRow("Position X",          zmBase + OFF_ZM_POS_X);
    //         addFloatRow("Position Y",          zmBase + OFF_ZM_POS_Y);
    //         addFloatRow("Position Z",          zmBase + OFF_ZM_POS_Z);
    //         addFloatRow("Pathfinding Dir",     zmBase + OFF_ZM_PATHFINDING);
    //         addUintRow ("Current Health",      zmBase + OFF_ZM_CURRENT_HEALTH);
    //         addUintRow ("Start Health",        zmBase + OFF_ZM_START_HEALTH);

    //         // bone matrix 4x4
    //         for (int r = 0; r < 4; ++r) {
    //             for (int c = 0; c < 4; ++c) {
    //                 uintptr_t addr = zmBase + OFF_ZM_BONE_MATRIX + (r * 0x10) + (c * 0x4);
    //                 char fieldName[32];
    //                 std::snprintf(fieldName, sizeof(fieldName), "boneMatrix[%d][%d]", r, c);
    //                 addFloatRow(fieldName, addr);
    //             }
    //         }

    //         ImGui::EndTable();
    //     }

    //     ImGui::TreePop();
    // }
}

bool __stdcall zirconium::render_initOverlay(ID3D11Device *device, ID3D11DeviceContext *deviceContext) {
	if (!device || !deviceContext) {
		LOG_ERROR("Invalid device or device context.");
		return false;
	}

    LOG_INFO("Initializing ImGui...");
    LOG_DEBUG("Creating ImGui context...");
    g_renderCtx.ctx = ImGui::CreateContext();
    if (!g_renderCtx.ctx) {
        LOG_ERROR("Failed to create ImGui context.");
        return false;
    }

    ImGui::SetCurrentContext(g_renderCtx.ctx);

    LOG_DEBUG("Setting ImGui style...");
    zirconium::overlay_setImGuiStyle();

    g_renderCtx.io = &ImGui::GetIO();
	if (!g_renderCtx.io) {
		LOG_ERROR("Failed to get ImGui IO (nullptr).");
		return false;
	}
    g_renderCtx.io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // hook the game's WndProc so ImGui receives input messages
    g_renderCtx.oWndProc = (WNDPROC)SetWindowLongPtr(g_renderCtx.hwnd, GWLP_WNDPROC, (LONG_PTR)hookedWndProc);
    if (!g_renderCtx.oWndProc) {
        LOG_ERROR("Failed to hook WndProc.");
        return false;
    }
    LOG_INFO("Hooked WndProc.");

    // init backend with correct window handle
    if (!ImGui_ImplWin32_Init(g_renderCtx.hwnd)) {
        LOG_ERROR("ImGui_ImplWin32_Init failed.");
        return false;
    }
    if (!ImGui_ImplDX11_Init(device, deviceContext)) {
        LOG_ERROR("ImGui_ImplDX11_Init failed.");
        return false;
    }
    LOG_INFO("ImGui initialized successfully.");

    LOG_INFO("Initializing memory table...");
    if (!zirconium::overlay_memTbl_init()) {
		LOG_ERROR("Failed to initialize memory table.");
		return false;
    }
    LOG_INFO("Memory table initialized successfully.");
	g_renderCtx.initialized = true;
    return true;
}

void __stdcall zirconium::overlay_show() {
	using namespace ImGui;

    Begin("Zirconium", nullptr, 0);
	if (BeginTabBar("##tabs", g_overlayCtx.tabBarFlags)) {
		if (BeginTabItem("Aimbot")) {
            Checkbox("Aimbot", &g_overlayCtx.aimbotEnabled);
            if (g_overlayCtx.aimbotEnabled) {
                Text("Lock condition:");
                RadioButton("Nearest", &g_overlayCtx.aimbotTargetMode, 0);
                RadioButton("Fastest", &g_overlayCtx.aimbotTargetMode, 1);
                RadioButton("Slowest", &g_overlayCtx.aimbotTargetMode, 2);
                RadioButton("Lowest HP", &g_overlayCtx.aimbotTargetMode, 3);
                RadioButton("Highest HP", &g_overlayCtx.aimbotTargetMode, 4);

                Spacing();

                Text("Aim key:");
                Text("(TODO)");
                Spacing();

                // TODO
                SliderInt("FOV", &g_overlayCtx.aimFov, 0, 360);
                SliderFloat("Lock speed", &g_overlayCtx.lockSpeed, 0.0f, 1.0f);

                Text("Aim bone (TODO)");
                Spacing();

                Text("Lock until:"); SameLine();
                if (!g_overlayCtx.lockUntilEliminated) {
                    if (Button("Lock condition updates")) {
                        g_overlayCtx.lockUntilEliminated = !g_overlayCtx.lockUntilEliminated;
                    }
                }
                else {
                    if (Button("Eliminated")) {
                        g_overlayCtx.lockUntilEliminated = !g_overlayCtx.lockUntilEliminated;
                    }
                }

                Spacing();

                Text("Aim mode:");
                RadioButton("Silent", &g_overlayCtx.aimPreset, 0);
                RadioButton("Legit", &g_overlayCtx.aimPreset, 1);
                RadioButton("Rage", &g_overlayCtx.aimPreset, 2);


            }
			EndTabItem();
		}
		if(BeginTabItem("ESP/visuals")) {
			Checkbox("Toggle ESP", &g_overlayCtx.espEnabled);
            if (g_overlayCtx.espEnabled)
            {
                zirconium::overlay_showESPTab();
            }
			EndTabItem();
		}
		if (BeginTabItem("Memory")) {
            if (BeginTabBar("##mem_subtabs")) {
                if (BeginTabItem("Table View")) {
                    zirconium::overlay_showMemTblTab();
                    EndTabItem();
                }
                if (BeginTabItem("Type Viewer")) {
                    static zirconium::MemoryViewerCtx s_memViewerCtx;
                    zirconium::overlay_showMemoryViewer(s_memViewerCtx);
                    EndTabItem();
                }
                EndTabBar();
            }
			EndTabItem();
		}
        if (BeginTabItem("Debug")) {
            zirconium::overlay_showDebugTab();
			EndTabItem();
        }
		if (BeginTabItem("Settings")) {
			Text("Settings tab");
			EndTabItem();
		}
        if (BeginTabItem("About")) {
			Text("Zirconium v0.1");
            Text("Made by Robert Motrogeanu for educational purposes.");
			Text("Source available here: https://www.github.com/robertmotr/zirconium");
			Text("The purpose of this project is to encourage learning about reverse engineering and to provide resources to help others get started.");
            EndTabItem();
        }

        // push all three tab states so the red colour persists when hovered/selected
        PushStyleColor(ImGuiCol_Tab,        ImVec4(0.55f, 0.0f, 0.0f, 1.0f));
        PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.75f, 0.0f, 0.0f, 1.0f));
        PushStyleColor(ImGuiCol_TabActive,  ImVec4(1.0f,  0.0f, 0.0f, 1.0f));
        if (BeginTabItem("Eject Zirconium")) {
            Spacing();
            TextUnformatted("Eject Zirconium from the process.");
            TextUnformatted("This restores the original Present hook and WndProc,");
            TextUnformatted("shuts down ImGui, and unloads the DLL cleanly.");
            Spacing();
            Separator();
            Spacing();
            PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
            PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            if (Button("Eject Zirconium")) {
                g_hookCtx.ejectRequested = true;
            }
            PopStyleColor(3);
            EndTabItem();
        }
        PopStyleColor(3);

		EndTabBar();
	}
    End();
}

/*
* Called every frame to render the ImGui overlay from inside our hook.
*/
void __stdcall zirconium::render_frame(ID3D11Device* device, ID3D11DeviceContext* deviceContext) {
    if (!GetClientRect(g_renderCtx.hwnd, &g_renderCtx.rect)) {
		LOG_ERROR("Failed to get client rect.");
    }
    g_renderCtx.io->DisplaySize = ImVec2((float)(g_renderCtx.rect.right - g_renderCtx.rect.left),
        (float)(g_renderCtx.rect.bottom - g_renderCtx.rect.top));

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // buttons, text, etc.
    zirconium::overlay_show();

    if (g_overlayCtx.espEnabled && g_overlayCtx.espRenderType != ESP_RENDER_TYPE_NONE)
    {
        zirconium::game_drawESP();
    }

    ImGui::Render();
	deviceContext->OMSetRenderTargets(1, &g_renderCtx.renderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void __stdcall zirconium::overlay_showESPTab() {
    using namespace ImGui;

    Text("Render Type:");
    RadioButton("Skeleton", &g_overlayCtx.espRenderType, ESP_RENDER_TYPE_SKELETON);
    RadioButton("Box", &g_overlayCtx.espRenderType,      ESP_RENDER_TYPE_BOX);
    RadioButton("Line", &g_overlayCtx.espRenderType,     ESP_RENDER_TYPE_LINE);

    Spacing();

    if (g_overlayCtx.espRenderType == ESP_RENDER_TYPE_SKELETON) {
        Separator();
        Text("Skeleton Settings:");
        SliderInt("Zombie Entity Offset", &g_addrs.zombieEntityOffset, 0, 1024);
        if (IsItemHovered()) {
            SetTooltip("Offset added to zombie slot index to get centity index.\n"
                        "Use Debug tab to find correct mapping.");
        }
        if (g_addrs.bonesResolved) {
            Text("Bone IDs resolved successfully");
        } else {
            TextColored(ImVec4(1, 0.5f, 0, 1), "Bone IDs not yet resolved (will resolve on first draw)");
        }
    }

    Spacing();
}

// ---- debug function caller ----

namespace {
    struct DebugCallable {
        const char* name;
        const char* paramNames[5];
        const char* paramTypes[5];  // "int", "string", "float"
        int paramCount;
        const char* returnType;
    };

    static const DebugCallable DEBUG_FUNCTIONS[] = {
        {
            "SL_FindString",
            {"boneName", "", "", "", ""},
            {"string", "", "", "", ""},
            1, "int (string ID)"
        },
        {
            "Com_GetClientDObj",
            {"entityIndex", "localClientNumber", "", "", ""},
            {"int", "int", "", "", ""},
            2, "pointer (DObj*)"
        },
        {
            "CG_GetEntity",
            {"localClientNum", "entityIndex", "", "", ""},
            {"int", "int", "", "", ""},
            2, "pointer (centity*)"
        },
        {
            "DObjGetBoneIndex",
            {"DObj_ptr", "boneNameSLStringID", "modelFilter", "", ""},
            {"int", "int", "int", "", ""},
            3, "int (result), boneIdx output"
        },
        {
            "CG_DObjGetWorldBoneMatrix",
            {"pose_ptr (centity*)", "DObj_ptr", "boneIndex", "", ""},
            {"int", "int", "int", "", ""},
            3, "int (result), worldPos output"
        },
    };
    static const int DEBUG_FUNCTION_COUNT = sizeof(DEBUG_FUNCTIONS) / sizeof(DEBUG_FUNCTIONS[0]);

    uintptr_t parseIntParam(const char* str) {
        if (!str || str[0] == '\0') return 0;
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            return static_cast<uintptr_t>(std::strtoul(str, nullptr, 16));
        }
        return static_cast<uintptr_t>(std::strtol(str, nullptr, 10));
    }

}

void __stdcall zirconium::overlay_showDebugTab() {
    using namespace ImGui;

    // all debug tab state is local-static to avoid bloating OverlayCtx
    static int  debugFuncSelected = 0;
    static char debugParams[5][64] = {};
    static char debugResultBuf[256] = {};
    static std::vector<std::string> debugCallLog;

    if (!g_addrs.resolved) {
        TextColored(ImVec4(1, 0, 0, 1), "Address book not resolved yet!");
        return;
    }

    Text("Game Function Caller");
    Separator();
    Spacing();

    // function selector
    const char* currentName = DEBUG_FUNCTIONS[debugFuncSelected].name;
    if (BeginCombo("Function", currentName)) {
        for (int i = 0; i < DEBUG_FUNCTION_COUNT; i++) {
            bool selected = (debugFuncSelected == i);
            if (Selectable(DEBUG_FUNCTIONS[i].name, selected)) {
                debugFuncSelected = i;
                for (int p = 0; p < 5; p++) {
                    debugParams[p][0] = '\0';
                }
                debugResultBuf[0] = '\0';
            }
            if (selected) SetItemDefaultFocus();
        }
        EndCombo();
    }

    const DebugCallable& func = DEBUG_FUNCTIONS[debugFuncSelected];

    Spacing();
    Text("Parameters:");
    for (int p = 0; p < func.paramCount; p++) {
        char label[128];
        std::snprintf(label, sizeof(label), "%s (%s)##param%d", func.paramNames[p], func.paramTypes[p], p);
        InputText(label, debugParams[p], sizeof(debugParams[p]));
    }

    Spacing();
    Text("Returns: %s", func.returnType);
    Spacing();

    if (Button("Call Function")) {
        char resultBuf[256] = {};
        int funcIdx = debugFuncSelected;

        switch (funcIdx) {
        case 0: { // SL_FindString
            const char* param = debugParams[0];
            int result = g_addrs.SL_FindString(param);
            std::snprintf(resultBuf, sizeof(resultBuf), "SL_FindString(\"%s\") = %d (0x%X)", param, result, result);
            break;
        }
        case 1: { // Com_GetClientDObj
            int entityIdx = static_cast<int>(parseIntParam(debugParams[0]));
            int localClient = static_cast<int>(parseIntParam(debugParams[1]));
            char* result = g_addrs.Com_GetClientDObj(entityIdx, localClient);
            std::snprintf(resultBuf, sizeof(resultBuf), "Com_GetClientDObj(%d, %d) = 0x%p", entityIdx, localClient, (void*)result);
            break;
        }
        case 2: { // CG_GetEntity
            int localClient = static_cast<int>(parseIntParam(debugParams[0]));
            int entityIdx = static_cast<int>(parseIntParam(debugParams[1]));
            void* result = g_addrs.CG_GetEntity(localClient, entityIdx);
            std::snprintf(resultBuf, sizeof(resultBuf), "CG_GetEntity(%d, %d) = 0x%p", localClient, entityIdx, result);
            break;
        }
        case 3: { // DObjGetBoneIndex
            int dobjPtr = static_cast<int>(parseIntParam(debugParams[0]));
            int slStringId = static_cast<int>(parseIntParam(debugParams[1]));
            int modelFilter = static_cast<int>(parseIntParam(debugParams[2]));
            unsigned char boneIdx = 0;
            int result = g_addrs.DObjGetBoneIndex(dobjPtr, slStringId, &boneIdx, modelFilter);
            std::snprintf(resultBuf, sizeof(resultBuf), "DObjGetBoneIndex(0x%X, %d, &out, %d) = %d, boneIdx = %u",
                dobjPtr, slStringId, modelFilter, result, (unsigned)boneIdx);
            break;
        }
        case 4: { // CG_DObjGetWorldBoneMatrix
            void* posePtr = reinterpret_cast<void*>(parseIntParam(debugParams[0]));
            void* dobjPtr = reinterpret_cast<void*>(parseIntParam(debugParams[1]));
            int boneIdx = static_cast<int>(parseIntParam(debugParams[2]));
            float rotMatrix[9] = {};
            float worldPos[3] = {};
            int result = g_addrs.CG_DObjGetWorldBoneMatrix(posePtr, dobjPtr, boneIdx, rotMatrix, worldPos, false);
            std::snprintf(resultBuf, sizeof(resultBuf),
                "CG_DObjGetWorldBoneMatrix(0x%p, 0x%p, %d) = %d\nworldPos = (%.2f, %.2f, %.2f)",
                posePtr, dobjPtr, boneIdx, result, worldPos[0], worldPos[1], worldPos[2]);
            break;
        }
        }

        strncpy_s(debugResultBuf, resultBuf, sizeof(debugResultBuf));

        debugCallLog.push_back(std::string(resultBuf));
        if (debugCallLog.size() > 20) {
            debugCallLog.erase(debugCallLog.begin());
        }
    }

    Spacing();
    Separator();
    Spacing();

    // result display
    if (debugResultBuf[0] != '\0') {
        Text("Result:");
        TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", debugResultBuf);
    }

    Spacing();
    Separator();
    Spacing();

    // call history
    if (!debugCallLog.empty()) {
        Text("Call History (recent %d):", (int)debugCallLog.size());
        if (BeginChild("##calllog", ImVec2(0, 150), true)) {
            for (int i = static_cast<int>(debugCallLog.size()) - 1; i >= 0; i--) {
                TextWrapped("[%d] %s", i, debugCallLog[i].c_str());
            }
        }
        EndChild();
    }
}
