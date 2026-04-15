#pragma once

#include "imgui.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <dxgi.h>

// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_win32.cpp see line 571
// need to forward declare this for some reason
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace zirconium {

    /*
    * Called every frame to render the ImGui overlay from inside our hook.
    */
    void __stdcall render_frame(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

    /*
        * Initializes ImGui overlay inside our hook.
        * @param device the Direct3D device in the target application to render the overlay on
        * @param deviceContext Direct3D device ctx
        * @return true on success, false otherwise
    */
    bool __stdcall render_initOverlay(ID3D11Device *device, ID3D11DeviceContext *deviceContext);

} // namespace zirconium