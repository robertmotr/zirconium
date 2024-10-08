#include "render.h"

namespace renderVars {
    bool                                                initialized = false; // check if ImGui is initialized
    ImGuiIO*                                            io = nullptr; // stored globally to reduce overhead inside renderOverlay
    ImGuiContext*                                       ctx = nullptr; // stored globally to reduce overhead inside renderOverlay
    LPDIRECT3DDEVICE9                                   g_pDevice = nullptr; 
    HWND                                                g_hwnd = nullptr; // background window we're running on top of
    RECT                                                rect = {}; // used for dynamically checking if wnd was resized
    D3DPRESENT_PARAMETERS*                              g_pD3DPP = nullptr; 
}

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
    return S_OK;
}

/*
* Called every frame within renderOverlay() for the code behind the ImGui components.
*/
void __stdcall renderContent() {
    ImGui::Begin("example imgui overlay after hooking");
    ImGui::Text("Now hooked using ImGui.");
    if (ImGui::Button("Close")) {
        exit(0);
    }
    ImGui::End();
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