#pragma once

#include "pch.h"

#include "zirconium_hook.h"
#include "zirconium_render.h"
#include "zirconium_overlay.h"

namespace zirconium {

    // D3D/ImGui backend state. Defined in zirconium_imgui_overlay.cpp.
    struct RenderCtx {
        ID3D11RenderTargetView* renderTargetView = nullptr;
        bool                    initialized      = false;
        ImGuiIO*                io               = nullptr;
        ImGuiContext*           ctx              = nullptr;
        HWND                    hwnd             = nullptr;
        RECT                    rect             = {};
        WNDPROC                 oWndProc         = nullptr;
    };

    extern RenderCtx g_renderCtx;

    // Hook/trampoline state. Defined in zirconium_hook_dx11.cpp.
    struct HookCtx {
        BYTE*               resumeAddr                   = nullptr;
        BYTE*               oPresent                     = nullptr;
        BYTE                originalBytes[TRAMPOLINE_SZ] = {};
        volatile bool       ejectRequested               = false;
        volatile bool       cleanupDone                  = false;
        HMODULE             hSelf                        = nullptr;
        ID3D11Device*       device                       = nullptr;
        ID3D11DeviceContext* deviceContext               = nullptr;
    };

    extern HookCtx g_hookCtx;

    // UI/menu state, defined in zirconium_imgui_overlay.cpp.
    struct OverlayCtx {
        bool                showMenu            = true;
        ImGuiTabBarFlags    tabBarFlags         = ImGuiTabBarFlags_Reorderable
                                                | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton
                                                | ImGuiTabBarFlags_FittingPolicyScroll;

        // memory table (debug inspector)
        std::vector<memoryTableEntry> memoryTable = {};
        unsigned int        memoryTableIdx      = 0;
        unsigned int        structViewIdx       = 0;

        // aimbot settings
        bool                aimbotEnabled       = false;
        int                 aimbotTargetMode    = 0;   // 0=nearest, 1=fastest, 2=slowest, 3=lowest HP, 4=highest HP
        bool                selectAimKey        = false;
        bool                lockUntilEliminated = false;
        int                 aimPreset           = 0;   // 0=silent, 1=legit, 2=rage
        int                 aimFov              = 0;
        float               lockSpeed           = 0.0f;

        // ESP settings
        bool                espEnabled          = false;
        int                 espRenderType       = ESP_RENDER_TYPE_NONE;

    };

    extern OverlayCtx g_overlayCtx;

} // namespace zirconium
