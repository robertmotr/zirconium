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

    // Hook state. Defined in zirconium_main.cpp.
    struct HookCtx {
        void*               presentAddr                  = nullptr;  // resolved Present fn address
        volatile bool       ejectRequested               = false;
        volatile bool       cleanupDone                  = false;
        HMODULE             hSelf                        = nullptr;
        ID3D11Device*       device                       = nullptr;
        ID3D11DeviceContext* deviceContext               = nullptr;

#ifdef ZIRCONIUM_USE_TRAMPOLINE
        BYTE*               resumeAddr                   = nullptr;
        BYTE*               oPresent                     = nullptr;
        BYTE                originalBytes[TRAMPOLINE_SZ] = {};
#else
        int                 presentHwbpHandle            = -1;       // guac_handle_t
#endif

        int                 spreadHandle                 = 0;        // guac_handle_t
        int                 recoilHandle                 = 0;        // guac_handle_t
        int                 godmodeHandle                = 0;        // guac_handle_t
    };

    extern HookCtx g_hookCtx;

    // UI/menu state, defined in zirconium_imgui_overlay.cpp.
    struct OverlayCtx {
        ImGuiTabBarFlags    tabBarFlags         = ImGuiTabBarFlags_Reorderable
                                                | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton
                                                | ImGuiTabBarFlags_FittingPolicyScroll;

        // aim/aimbot settings
        bool                aimbotEnabled         = false;
        int                 aimbotTargetMode      = 0;
        int                 aimbotTargetBone      = 1;   // index into AIMBOT_BONE_OPTIONS[] (default: forehead)
        bool                noSpread              = false;
        bool                noRecoil              = false;

        // esp/visuals
        // ESP settings
        bool                espEnabled            = false;
        bool                espRenderSnapLines    = false;
        bool                espRenderZombieInfo   = false;

        // misc. visuals
        bool                thirdPerson           = false;
        
        // misc. settings
        bool                godMode               = false;
        float               gravity               = 800.0f;
        bool                ignoreMe              = false;
        float               jumpHeight            = 39.0F;
        int                 speed                 = 0;

        // only works when server runs in same process 
        bool                isLanGame             = false;

        // overlays other misc debugging shit for aimbot/esp
        bool                visualDebug           = false;

    };

    extern OverlayCtx g_overlayCtx;

} // namespace zirconium
