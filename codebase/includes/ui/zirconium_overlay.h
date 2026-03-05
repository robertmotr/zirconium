#pragma once

#include "pch.h"

#include <variant>
#include <string>

namespace zirconium {

    /*
    * Style the ImGui overlay. 
    * credits to https://github.com/adobe/imgui/blob/master/imgui_spectrum.h
    */
    void __stdcall overlay_setImGuiStyle();

    /*
    * Called every frame within render_frame() for the ImGui components displayed within the menu.
    */
    void __stdcall overlay_show();

    void __stdcall overlay_showAimbotTab();

    typedef enum _espRenderType {
        ESP_RENDER_TYPE_NONE     = 0,
        ESP_RENDER_TYPE_SKELETON = 1,
        ESP_RENDER_TYPE_BOX      = 2,
        ESP_RENDER_TYPE_LINE     = 3,
        ESP_RENDER_TYPE_MAX      = (INT32_MAX)
    } espRenderType;

    void __stdcall overlay_showESPTab();

    void __stdcall overlay_showMemTblTab();
    
    void __stdcall overlay_showDebugTab();

    void __stdcall overlay_showSettingsTab();

    void __stdcall overlay_showEjectTab();

    using MEM_TYPES = std::variant<unsigned int, float, std::string>;

    struct memoryTableEntry {
        std::string name;
        uintptr_t addr;
        MEM_TYPES value;
        std::string description;
        std::string structName;
    };

    int __stdcall overlay_memTbl_compareValues(const MEM_TYPES& a, const MEM_TYPES& b);

    void __stdcall overlay_memTbl_sort();

    /*
    * Initializes the ImGui memory table with the desired values read from the game.
    */
    bool __stdcall overlay_memTbl_init();
    
}