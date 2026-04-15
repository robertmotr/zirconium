#include "pch.h"

#include "zirconium_render.h"
#include "zirconium_overlay.h"
#include "zirconium_globals.h"
#include "zirconium_imgui_style.h"
#include "zirconium_game.h"

#include "guac.h"

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

	g_renderCtx.initialized = true;
    return true;
}

/*
* Called every frame to render the ImGui overlay from inside our hook.
*/
void __stdcall zirconium::render_frame(ID3D11Device* device, ID3D11DeviceContext* deviceContext) 
{
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

    if (!g_addrBook.resolved)
        g_addrBook.resolveStatic();
    g_addrBook.refreshVolatile();

    if (g_overlayCtx.espEnabled)
    {
        zirconium::game_drawESP();
    }

    if (g_overlayCtx.aimbotEnabled)
    {
        zirconium::game_aimbot();
    }

    ImGui::Render();
	deviceContext->OMSetRenderTargets(1, &g_renderCtx.renderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void __stdcall zirconium::overlay_show() {
	using namespace ImGui;

    Begin("Zirconium", nullptr, 0);
	if (BeginTabBar("##tabs", g_overlayCtx.tabBarFlags)) 
    {
		if (BeginTabItem("Aimbot")) 
        {
            Checkbox("Aimbot", &g_overlayCtx.aimbotEnabled);
            if (g_overlayCtx.aimbotEnabled) 
            {
                Text("Lock condition:");
                RadioButton("Nearest (distance)", &g_overlayCtx.aimbotTargetMode, 0);
                RadioButton("Nearest (crosshair)", &g_overlayCtx.aimbotTargetMode, 1);
                if (g_overlayCtx.isLanGame)
                {
                    RadioButton("Lowest HP", &g_overlayCtx.aimbotTargetMode, 2);
                    RadioButton("Highest HP", &g_overlayCtx.aimbotTargetMode, 3);
                }
                else
                {
                    BeginDisabled();
                    RadioButton("Lowest HP (LAN only)", &g_overlayCtx.aimbotTargetMode, 2);
                    RadioButton("Highest HP (LAN only)", &g_overlayCtx.aimbotTargetMode, 3);
                    EndDisabled();
                }

                Spacing();

                Text("Aim bone:");
                if (BeginCombo("##aimbone", zirconium::AIMBOT_BONE_OPTIONS[g_overlayCtx.aimbotTargetBone].name))
                {
                    for (int i = 0; i < zirconium::AIMBOT_BONE_OPTION_COUNT; i++)
                    {
                        bool selected = (g_overlayCtx.aimbotTargetBone == i);
                        if (Selectable(zirconium::AIMBOT_BONE_OPTIONS[i].name, selected))
                            g_overlayCtx.aimbotTargetBone = i;
                        if (selected)
                            SetItemDefaultFocus();
                    }
                    EndCombo();
                }
                Spacing();
            } /* g_overlayCtx.aimbotEnabled */

            if (Checkbox("No spread", &g_overlayCtx.noSpread)) {
                if (g_overlayCtx.noSpread)
                    guac_hook(&g_hookCtx.spreadHandle, (void*)ADDR_FN_NOSPREAD, (void*)&zirconium::noSpreadHook, NULL);
                else
                    guac_unhook(&g_hookCtx.spreadHandle);
            }

            if (Checkbox("No recoil", &g_overlayCtx.noRecoil)) {
                if (g_overlayCtx.noRecoil)
                    guac_hook(&g_hookCtx.recoilHandle, (void*)ADDR_FN_NORECOIL, (void*)&zirconium::noRecoilHook, NULL);
                else
                    guac_unhook(&g_hookCtx.recoilHandle);
            }
			EndTabItem();
		} 

		if(BeginTabItem("ESP/visuals")) 
        {
			Checkbox("Toggle ESP", &g_overlayCtx.espEnabled);
            if (g_overlayCtx.espEnabled)
            {                  
                Separator();
                Checkbox("Snap lines",        &g_overlayCtx.espRenderSnapLines);
                Checkbox("Show Zombie info",  &g_overlayCtx.espRenderZombieInfo);
                Spacing();
            }
            if (Checkbox("Toggle third person", &g_overlayCtx.thirdPerson))
            {
                cg_t* cg = g_addrBook.clientGame;
                if (cg) {
                    cg->bThirdPerson                      = g_overlayCtx.thirdPerson;
                    cg->renderingThirdPerson              = static_cast<thirdPersonType>(g_overlayCtx.thirdPerson);
                    cg->predictedPlayerState.bThirdPerson = g_overlayCtx.thirdPerson;
                }
            }
			EndTabItem();
		}

        if (BeginTabItem("Misc"))
        {
            if (Checkbox("God mode", &g_overlayCtx.godMode))
            {
                if (g_overlayCtx.godMode)
                    guac_hook(&g_hookCtx.godmodeHandle, (void*)ADDR_FN_GODMODE, (void*)&zirconium::godModeHook, NULL);
                else
                    guac_unhook(&g_hookCtx.godmodeHandle);
            }
            if (g_overlayCtx.isLanGame)
            {
                if (Checkbox("Invisibility from zombies", &g_overlayCtx.ignoreMe))
                {
                    g_addrBook.gentityPlayer->sentient->bIgnoreMe = g_overlayCtx.ignoreMe;
                }
            }
            else
            {
                BeginDisabled();
                Checkbox("Invisibility from zombies", &g_overlayCtx.ignoreMe);
                EndDisabled();
            }
            Separator();

            static vec3_t tpPos = {0};
            InputFloat3("Teleport to position:", reinterpret_cast<float*>(&tpPos));
            if (Button("Teleport"))
            {
                cg_t* cg = g_addrBook.clientGame;
                if (cg) {
                    std::memcpy(&cg->predictedPlayerState.origin, &tpPos, sizeof(tpPos));
                }
            }
            Separator();
        
            if (!g_overlayCtx.isLanGame)
                BeginDisabled();

            InputScalar("Money", ImGuiDataType_U32, g_addrBook.money);
            if(SliderFloat("Gravity", &g_overlayCtx.gravity, -800.0F, 1600.0F))
            {
                g_addrBook.dvarGravity->current.value = g_overlayCtx.gravity;
            }

            if(SliderFloat("Jump height", &g_overlayCtx.jumpHeight, -100.0F, 250000.0F))
            {
                g_addrBook.dvarJumpHeight->current.value = g_overlayCtx.jumpHeight;
            }
            if(SliderInt("Speed", &g_overlayCtx.speed, 0, 1000))
            {
                g_addrBook.dvarSpeed->current.integer = g_overlayCtx.speed;
            }

            InputScalar("Primary Main Ammo", ImGuiDataType_U32, g_addrBook.primaryMainAmmo);
            InputScalar("Primary Reserve Ammo", ImGuiDataType_U32, g_addrBook.primaryReserveAmmo);
            InputScalar("Secondary Main Ammo", ImGuiDataType_U32, g_addrBook.secondaryMainAmmo);
            InputScalar("Secondary Reserve Ammo", ImGuiDataType_U32, g_addrBook.secondaryReserveAmmo);
            InputScalar("Grenades", ImGuiDataType_U32, g_addrBook.grenades);
            InputScalar("Claymores", ImGuiDataType_U32, g_addrBook.claymores);
            InputScalar("Monkey Bombs", ImGuiDataType_U32, g_addrBook.monkeyBombs);

            if (!g_overlayCtx.isLanGame)
                EndDisabled();
            EndTabItem();
        }
        if (BeginTabItem("Settings"))
        {
            Checkbox("LAN mode (server-side data)", &g_overlayCtx.isLanGame);
            if (IsItemHovered())
                SetTooltip("Enable when playing solo/LAN. Gives access to health bars,\nHP-based aimbot, invisibility, etc.\nThe data required for this sort of thing isn't exposed as a client so you won't get this unless you're solo :(");
            Separator();
            Checkbox("ESP/Aimbot debug", &g_overlayCtx.visualDebug);
            EndTabItem();
        }
        if (BeginTabItem("About")) {
			Text("Zirconium v1.0");
            Text("Made by Robert Motrogeanu for educational purposes.");
			Text("Source available here: https://www.github.com/robertmotr/zirconium");
            EndTabItem();
        }

        PushStyleColor(ImGuiCol_Tab,        ImVec4(0.55f, 0.0f, 0.0f, 1.0f));
        PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.75f, 0.0f, 0.0f, 1.0f));
        PushStyleColor(ImGuiCol_TabActive,  ImVec4(1.0f,  0.0f, 0.0f, 1.0f));
        if (BeginTabItem("Eject Zirconium")) 
        {
            Spacing();
            TextUnformatted("Unloads the mod menu and (hopefully) restores the game to its original state.");
            Spacing();
            Separator();
            Spacing();
            PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
            PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            if (Button("Eject Zirconium"))
                g_hookCtx.ejectRequested = true;
            PopStyleColor(3);
            EndTabItem();
        }
        PopStyleColor(3);

		EndTabBar();
	}
    End();
}

