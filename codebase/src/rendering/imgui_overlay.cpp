#include "render.h"
#include "globals.h"
#include "scan_mem.h"

#ifdef _MENU_ONLY
static PlaceholderData placeholderData = {};
#endif

/*
* Styles the ImGui overlay.
* credits to https://github.com/GraphicsProgramming/dear-imgui-styles
*/
void __stdcall setImGuiStyle() {
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

void initMemTable() {
    guiVars::memoryTable.clear();

#ifdef _MENU_ONLY
    guiVars::memoryTable.push_back({ "Player struct", reinterpret_cast<uintptr_t>(&placeholderData), "N/A", "Player struct base address", "Unknown" });
    guiVars::memoryTable.push_back({ "Health", reinterpret_cast<uintptr_t>(&placeholderData.health), 0u, "Realtime player health", "Unknown" });
    guiVars::memoryTable.push_back({ "Health maximum", reinterpret_cast<uintptr_t>(&placeholderData.healthMax), 0u, "Maximum player health, 100 w/o Juggernog, 150 otherwise", "Unknown" });
    guiVars::memoryTable.push_back({ "X-coordinate", reinterpret_cast<uintptr_t>(&placeholderData.xCoordinate), 0.0f, "Player x-coordinate", "Player" });
    guiVars::memoryTable.push_back({ "Y-coordinate", reinterpret_cast<uintptr_t>(&placeholderData.yCoordinate), 0.0f, "Player y-coordinate", "Player" });
    guiVars::memoryTable.push_back({ "Z-coordinate", reinterpret_cast<uintptr_t>(&placeholderData.zCoordinate), 0.0f, "Player z-coordinate", "Player" });

    guiVars::memoryTable.push_back({ "Gravity", reinterpret_cast<uintptr_t>(&placeholderData.gravity), 0u, "Local player gravity multiplier", "Player" });
    guiVars::memoryTable.push_back({ "Static speed", reinterpret_cast<uintptr_t>(&placeholderData.staticSpeed), 0u, "Local player speed", "Player" });
    guiVars::memoryTable.push_back({ "Local name", reinterpret_cast<uintptr_t>(placeholderData.localName), "name", "Local player name", "Player" });
    guiVars::memoryTable.push_back({ "Crosshair", reinterpret_cast<uintptr_t>(&placeholderData.crosshair), 0u, "Local player crosshair", "Player" });

    guiVars::memoryTable.push_back({ "View Angle X", reinterpret_cast<uintptr_t>(&placeholderData.viewAngleX), 0.0f, "Local player view angle X", "Player" });
    guiVars::memoryTable.push_back({ "View Angle Y", reinterpret_cast<uintptr_t>(&placeholderData.viewAngleY), 0.0f, "Local player view angle Y", "Player" });

    guiVars::memoryTable.push_back({ "Primary weapon reserve ammo", reinterpret_cast<uintptr_t>(&placeholderData.primaryWeaponAmmo), 0u, "Primary weapon reserve ammo", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Secondary weapon reserve ammo", reinterpret_cast<uintptr_t>(&placeholderData.secondaryWeaponAmmo), 0u, "Secondary weapon reserve ammo", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Grenades", reinterpret_cast<uintptr_t>(&placeholderData.grenades), 0u, "Number of grenades", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Claymores", reinterpret_cast<uintptr_t>(&placeholderData.claymores), 0u, "Number of claymores", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Monkey bombs", reinterpret_cast<uintptr_t>(&placeholderData.monkeyBombs), 0u, "Number of monkey bombs", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Money", reinterpret_cast<uintptr_t>(&placeholderData.money), 0u, "Player money", "PlayerStats" });
    guiVars::memoryTable.push_back({ "Zombie round display", reinterpret_cast<uintptr_t>(&placeholderData.zombieRoundDisplay), 0u, "Current zombie round display", "GameStats" });
    guiVars::memoryTable.push_back({ "Zombies spawned in", reinterpret_cast<uintptr_t>(&placeholderData.zombiesSpawnedIn), 0u, "Number of zombies spawned in", "GameStats" });

    guiVars::memoryTable.push_back({ "zm_entity_list[0]", reinterpret_cast<uintptr_t>(placeholderData.zmEntityList0), "N/A", "Base address of first zombie struct in entity list", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] pathfinding direction", reinterpret_cast<uintptr_t>(&placeholderData.zmEntityPathfindingDirection), 0.0f, "Zombie pathfinding direction", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] coordinate X", reinterpret_cast<uintptr_t>(&placeholderData.zmEntityCoordinateX), 0.0f, "X coordinate of zombie", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] coordinate Y", reinterpret_cast<uintptr_t>(&placeholderData.zmEntityCoordinateY), 0.0f, "Y coordinate of zombie", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] coordinate Z", reinterpret_cast<uintptr_t>(&placeholderData.zmEntityCoordinateZ), 0.0f, "Z coordinate of zombie", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] start health", reinterpret_cast<uintptr_t>(&placeholderData.zmEntityStartHealth), 0u, "Zombie start health", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] current health", reinterpret_cast<uintptr_t>(&placeholderData.zmEntityCurrentHealth), 0u, "Zombie current health", "Zombie" });

    for (int i = 0; i < 4; ++i) { // Rows
        for (int j = 0; j < 4; ++j) { // Columns
            uintptr_t addr = reinterpret_cast<uintptr_t>(&placeholderData.boneMatrix[i][j]);
            std::string name = "zm_entity_list[0].boneMatrix[" + std::to_string(i) + "][" + std::to_string(j) + "]";
            guiVars::memoryTable.push_back({ name, addr, 0.0f, "4x4 bone matrix", "Zombie" });
        }
    }

    guiVars::memoryTable.push_back({ "Player struct with offset", reinterpret_cast<uintptr_t>(placeholderData.playerStructWithOffset), "N/A", "Player struct with offset", "Player" });

    for (int i = 0; i < 16; ++i) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(&placeholderData.viewMatrix[i]);
        std::string name = "viewMatrix[" + std::to_string(i) + "]";
        std::string description = "View matrix element [" + std::to_string(i) + "]";
        guiVars::memoryTable.push_back({ name, addr, 0.0f, description, "Camera" });
    }
#else
    // self-explanatory, sets up global memtable
    uintptr_t moduleBase = resolveModuleAddress(PE_MODULE_NAME, 0);
    if (moduleBase == 0) {
        LOG("Failed to get module base addr!");
        return;
    }

    uintptr_t playerBasePtr = resolveModuleAddress(PE_MODULE_NAME, 0x1D88290);
    uintptr_t playerBase = 0;
    if (playerBasePtr != 0) {
        playerBase = *reinterpret_cast<uintptr_t*>(playerBasePtr);
    }

    uintptr_t crosshairAddress = 0;
    if (playerBase != 0) {
        uintptr_t crosshairPtr = playerBase + 0x588;
        crosshairAddress = *reinterpret_cast<uintptr_t*>(crosshairPtr);
    }

    guiVars::memoryTable.push_back({ "Player struct", 0x02188290, "N/A", "Player struct base address", "Unknown" });
    guiVars::memoryTable.push_back({ "Health", resolveModuleAddress(PE_MODULE_NAME, 0x1DC1568), 0u, "Realtime player health", "Unknown" });
    guiVars::memoryTable.push_back({ "Health maximum", 0x021C1568 + 0x4, 0u, "Maximum player health, 100 w/o Juggernog, 150 otherwise", "Unknown" });
    guiVars::memoryTable.push_back({ "X-coordinate", 0x23427C8, 0.0f, "Player x-coordinate", "Player" });
    guiVars::memoryTable.push_back({ "Y-coordinate", 0x23427CC, 0.0f, "Player y-coordinate", "Player" });
    guiVars::memoryTable.push_back({ "Z-coordinate", 0x23427CC + 4, 0.0f, "Player z-coordinate", "Player" });

    guiVars::memoryTable.push_back({ "Gravity", playerBase + 0x8C, 0u, "Local player gravity multiplier", "Player" });
    guiVars::memoryTable.push_back({ "Static speed", playerBase + 0x94, 0u, "Local player speed", "Player" });
    guiVars::memoryTable.push_back({ "Local name", 0x2347BC8 + 0x10C, "name", "Local player name", "Player" });

    guiVars::memoryTable.push_back({ "Crosshair", crosshairAddress, 0u, "Local player crosshair", "Player" });

    guiVars::memoryTable.push_back({ "View Angle X", 0x23453A0, 0.0f, "Local player view angle X", "Player" });
    guiVars::memoryTable.push_back({ "View Angle Y", 0x23453A4, 0.0f, "Local player view angle Y", "Player" });

    guiVars::memoryTable.push_back({ "Primary weapon reserve ammo", resolveModuleAddress(PE_MODULE_NAME, 0x1F42B90), 0u, "Primary weapon reserve ammo", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Secondary weapon reserve ammo", 0x02342B98, 0u, "Secondary weapon reserve ammo", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Grenades", resolveModuleAddress(PE_MODULE_NAME, 0x1F42BD0), 0u, "Number of grenades", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Claymores", resolveModuleAddress(PE_MODULE_NAME, 0x1F42BD8), 0u, "Number of claymores", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Monkey bombs", resolveModuleAddress(PE_MODULE_NAME, 0x1F42BDC), 0u, "Number of monkey bombs", "WeaponStats" });
    guiVars::memoryTable.push_back({ "Money", resolveModuleAddress(PE_MODULE_NAME, 0x1F47D68), 0u, "Player money", "PlayerStats" });
    guiVars::memoryTable.push_back({ "Zombie round display", resolveModuleAddress(PE_MODULE_NAME, 0x1F3B710), 0u, "Current zombie round display", "GameStats" });
    guiVars::memoryTable.push_back({ "Zombies spawned in", resolveModuleAddress(PE_MODULE_NAME, 0x1F30388), 0u, "Number of zombies spawned in", "GameStats" });

    guiVars::memoryTable.push_back({ "zm_entity_list[0]", 0x021C5828, "N/A", "Base address of first zombie struct in entity list", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] pathfinding direction", 0x021C5828 + 0x40, 0.0f, "Zombie pathfinding direction", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] coordinate X", 0x021C5828 + 0x18, 0.0f, "X coordinate of zombie", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] coordinate Y", 0x021C5828 + 0x1C, 0.0f, "Y coordinate of zombie", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] coordinate Z", 0x021C5828 + 0x20, 0.0f, "Z coordinate of zombie", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] start health", 0x021C5924 + 0xB0, 0u, "Zombie start health", "Zombie" });
    guiVars::memoryTable.push_back({ "zm_entity_list[0] current health", 0x021C5924 + 0xAC, 0u, "Zombie current health", "Zombie" });

    uintptr_t boneMatrixBase = 0x021C5940;
    for (int i = 0; i < 4; ++i) { // Rows
        for (int j = 0; j < 4; ++j) { // Columns
            uintptr_t addr = boneMatrixBase + (i * 0x10) + (j * 0x4);
            std::string name = "zm_entity_list[0].boneMatrix[" + std::to_string(i) + "][" + std::to_string(j) + "]";
            guiVars::memoryTable.push_back({ name, addr, 0.0f, "4x4 bone matrix (118 offset from zm_entity_list[0] struct)", "Zombie" });
        }
    }

    guiVars::memoryTable.push_back({ "Player struct with offset", 0x02330324 + 0x34, "N/A", "Player struct with offset", "Player" });

    // View matrix entries
    uintptr_t viewMatrixBase = 0x0103AD40;
    for (int i = 0; i < 16; ++i) {
        uintptr_t addr = viewMatrixBase + (i * 0x4);
        std::string name = "viewMatrix[" + std::to_string(i) + "]";
        std::string description = "View matrix element [" + std::to_string(i) + "]";
        guiVars::memoryTable.push_back({ name, addr, 0.0f, description, "Camera" });
    }
#endif
}

void tableSortHelper() {
    ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
    if (sortSpecs && sortSpecs->SpecsDirty) {
        std::sort(guiVars::memoryTable.begin(), guiVars::memoryTable.end(),
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
                        delta = compareValues(a.value, b.value);
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

/*
* Helper fn for sorting within the memory table.
* @param operand a of std::variant MEM_TYPES
* @param operand b of std::variant MEM_TYPES
* @return sorting value
*/
int compareValues(const MEM_TYPES& a, const MEM_TYPES& b) {
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

void showMemoryTable() {
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

        tableSortHelper();

        int rowIndex = 0;

        for (auto& entry : guiVars::memoryTable) {
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

#ifdef _MENU_ONLY
                // In test mode, fill buffer with placeholder data
                for (int i = 0; i < 128; ++i) {
                    buffer[i] = static_cast<unsigned char>(i);
                }
#else
                memcpy(buffer, reinterpret_cast<void*>(entry.addr), sizeof(buffer));
#endif
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
}


/*
	* Initializes ImGui overlay inside our hook.
	* @param pDevice the Direct3D device in the target application to render the overlay on
	* @return S_OK if successful, E_FAIL otherwise
*/
HRESULT __stdcall initOverlay(LPDIRECT3DDEVICE9 pDevice) {
    LOG("Initializing ImGui...");
    renderVars::initialized = true;

    LOG("Creating ImGui context...");
    renderVars::ctx = ImGui::CreateContext();
    if (!renderVars::ctx) {
        LOG("Failed to create ImGui context.");
        return E_FAIL;
    }

    ImGui::SetCurrentContext(renderVars::ctx);

    LOG("Setting ImGui style...");
    setImGuiStyle();

    renderVars::io = &ImGui::GetIO();
    renderVars::io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    renderVars::io->WantCaptureKeyboard = true;
    renderVars::io->WantCaptureMouse = true;

    // init backend with correct window handle
    if (!ImGui_ImplWin32_Init(renderVars::g_hwnd)) {
        LOG("ImGui_ImplWin32_Init failed.");
        return E_FAIL;
    }
    if (!ImGui_ImplDX9_Init(pDevice)) {
        LOG("ImGui_ImplDX9_Init failed.");
        return E_FAIL;
    }
    LOG("ImGui initialized successfully.");

    LOG("Initializing memory table...");
    initMemTable();
    LOG("Memory table initialized successfully");
    return S_OK;
}

/*
* Called every frame within renderOverlay() for the code behind the ImGui components.
*/
void __stdcall renderContent() {
	using namespace ImGui;

    Begin("Zirconium", nullptr, 0);
	if (BeginTabBar("##tabs", guiVars::tab_bar_flags)) {
		if (BeginTabItem("Aimbot")) {
			Text("Aimbot tab");
			EndTabItem();
		}
		if(BeginTabItem("ESP/visuals")) {
			Text("ESP tab");
			EndTabItem();
		}
		if (BeginTabItem("Memory")) {
            showMemoryTable();
			EndTabItem();
		}
        if (BeginTabItem("Debug")) {
			Text("Debug tab");
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
		EndTabBar();
	}
    End();
}

/*
* Called every frame to render the ImGui overlay from inside our hook.
*/
void __stdcall renderOverlay(LPDIRECT3DDEVICE9 pDevice) {

    HRESULT hr;
    if (!renderVars::initialized) {
        hr = initOverlay(pDevice);
        if (FAILED(hr)) {
            LOG("Failed to initialize ImGui overlay.");
            return;
        }
        LOG("Initialized ImGui overlay.");
    }

    GetClientRect(renderVars::g_hwnd, &renderVars::rect);
    renderVars::io->DisplaySize = ImVec2((float)(renderVars::rect.right - renderVars::rect.left),
        (float)(renderVars::rect.bottom - renderVars::rect.top));

#ifndef _MENU_ONLY
    // this needs to be done every frame in order to have responsive input (e.g. for buttons to work when clicked)
    // only applicable for actual DLL inside cheat, for menu testing this breaks things
    renderVars::io->MouseDown[0] = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
    renderVars::io->MouseDown[1] = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
    renderVars::io->MouseDown[2] = GetAsyncKeyState(VK_MBUTTON) & 0x8000;
#endif

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // buttons, text, etc.
    renderContent();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}