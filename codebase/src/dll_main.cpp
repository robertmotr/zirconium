#include "hook.h"

#ifndef _MENU_ONLY // then its either Release/Debug which has the full DLL	
	BOOL __stdcall DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {

		if (fdwReason == DLL_PROCESS_ATTACH) {
            DisableThreadLibraryCalls(hModule);
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)startThread, hModule, NULL, NULL);
		}
		//else if (fdwReason == DLL_PROCESS_DETACH) {
		//	LOG("DLL_PROCESS_DETACH called.");
		//}
		//else if (fdwReason == DLL_THREAD_ATTACH)
		//{
		//	LOG("DLL_THREAD_ATTACH called.");
		//}
		//else if (fdwReason == DLL_THREAD_DETACH)
		//{
		//	LOG("DLL_THREAD_DETACH called.");
		//}
		return TRUE;
	}
#else // exe build for testing menu

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    int __stdcall cleanup(LPDIRECT3DDEVICE9 ptrDevice, IDirect3D9 *pD3D, HWND hwnd, WNDCLASS &wc) {
        // Cleanup
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        ptrDevice->Release();
        pD3D->Release();
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 0;
    }

    int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
        LOG_INFO("Using Menu Only mode");
        LOG_INFO("Starting ImGui overlay...");

        const wchar_t CLASS_NAME[] = L"DX9WindowClass"; // Changed to wide character string

        WNDCLASS wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);
        HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"DirectX 9 Example", 
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            1920, 1080, nullptr, nullptr, hInstance, nullptr);

		renderVars::g_hwnd = hwnd;

        IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if (!pD3D) return false;

        D3DPRESENT_PARAMETERS d3dpp = {};
        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;  // Let Direct3D choose the format
        d3dpp.BackBufferWidth = 1920;             // Match the window size
        d3dpp.BackBufferHeight = 1080;             // Match the window size
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // No vsync
        d3dpp.hDeviceWindow = hwnd;               // Set the window to be used by Direct3D

        renderVars::g_pD3DPP = &d3dpp;

        LPDIRECT3DDEVICE9 ptrDevice;
        if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &ptrDevice))) {
            LOG_ERROR("Failed to create Direct3D device.");
            pD3D->Release();
            DestroyWindow(hwnd);
            return 1;
        }

		renderVars::g_pDevice = ptrDevice;

        ShowWindow(hwnd, nCmdShow);

        // DX9 render loop
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        while (true) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    cleanup(ptrDevice, pD3D, hwnd, wc);
                    return 0;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (renderVars::g_pD3DPP->Windowed) {
                GetClientRect(hwnd, &renderVars::rect);
                renderVars::g_pD3DPP->BackBufferWidth = renderVars::rect.right - renderVars::rect.left;
                renderVars::g_pD3DPP->BackBufferHeight = renderVars::rect.bottom - renderVars::rect.top;
            }

            ptrDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            ptrDevice->BeginScene();

            renderOverlay(ptrDevice);

            ptrDevice->EndScene();
            HRESULT res = ptrDevice->Present(NULL, NULL, NULL, NULL);
            if (res == D3DERR_DEVICELOST) {
                while (res == D3DERR_DEVICELOST) {
                    Sleep(100);
                    res = ptrDevice->TestCooperativeLevel();
                }
                if (res == D3DERR_DEVICENOTRESET) {
                    ptrDevice->Reset(&d3dpp);
                    ImGui_ImplDX9_InvalidateDeviceObjects();
                    ImGui_ImplDX9_CreateDeviceObjects();
                }
            }
        }
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return 0;

        switch (msg) {
        case WM_SIZE:
            if (renderVars::g_pDevice != nullptr && wParam != SIZE_MINIMIZED) {
                renderVars::g_pD3DPP->BackBufferWidth = LOWORD(lParam);
                renderVars::g_pD3DPP->BackBufferHeight = HIWORD(lParam);
                renderVars::rect = { 0, 0, (LONG)renderVars::g_pD3DPP->BackBufferWidth, (LONG)renderVars::g_pD3DPP->BackBufferHeight };
                renderVars::g_pDevice->Reset(renderVars::g_pD3DPP);
                ImGui_ImplDX9_InvalidateDeviceObjects();
                ImGui_ImplDX9_CreateDeviceObjects();
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }
#endif