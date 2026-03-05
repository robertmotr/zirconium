#include "pch.h"

#include "zirconium_hook.h"
#include "zirconium_render.h"
#include "zirconium_globals.h"

zirconium::HookCtx zirconium::g_hookCtx{};

LRESULT CALLBACK zirconium::dummyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void* zirconium::hook_getPresentAddress() {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = dummyWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"DummyWindow";
    RegisterClass(&wc);

    HWND hWnd = CreateWindow(L"DummyWindow", L"DummyWindow", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    if (hWnd == nullptr) {
        LOG_ERROR("Failed to create dummy window.");
        return nullptr;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, nullptr, &pContext
    );

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create device and swap chain.");
        return nullptr;
    }

    void** vmt = *(void***)pSwapChain;
    void* presentAddr = vmt[PRESENT_INDEX];

    pSwapChain->Release();
    pDevice->Release();
    pContext->Release();
    DestroyWindow(hWnd);
    UnregisterClass(L"DummyWindow", wc.hInstance);

    return presentAddr;
}

static bool __stdcall initDX11(IDXGISwapChain* swapChain) {
	if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&zirconium::g_hookCtx.device))) {
        zirconium::g_hookCtx.device->GetImmediateContext(&zirconium::g_hookCtx.deviceContext);
        if (zirconium::g_hookCtx.deviceContext == nullptr) {
			LOG_ERROR("GetImmediateContext failed, deviceContext is null.");
			return false;
        }
		LOG_INFO("Got device and device context.");

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		if (FAILED(swapChain->GetDesc(&swapChainDesc))) {
			LOG_ERROR("GetDesc failed on swap chain.");
			return false;
		}
		LOG_INFO("Got swapChainDesc.");
		zirconium::g_renderCtx.hwnd = swapChainDesc.OutputWindow;

		ID3D11Texture2D* backBuffer = nullptr;
		if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer)))
		{
			LOG_ERROR("GetBuffer() failed, backBuffer is null.");
			return false;
		}
		LOG_INFO("Got back buffer.");

		if (FAILED(zirconium::g_hookCtx.device->CreateRenderTargetView(backBuffer, NULL, &zirconium::g_renderCtx.renderTargetView))) {
			LOG_ERROR("CreateRenderTargetView failed.");
			return false;
		}
		LOG_INFO("Created render target view.");
		backBuffer->Release();

		if (!zirconium::render_initOverlay(zirconium::g_hookCtx.device, zirconium::g_hookCtx.deviceContext)) {
			LOG_ERROR("initOverlay failed.");
			return false;
		}
		LOG_INFO("initOverlay completed successfully.");
	}
	else {
		LOG_ERROR("swapChain->GetDevice failed.");
		return false;
	}
	return true;
}

HRESULT __stdcall zirconium::hook_handler(IDXGISwapChain* swapChain,
    UINT SyncInterval,
    UINT Flags)
{
    // eject path: clean up everything, then signal hook_startThread to unload the DLL
    if (g_hookCtx.ejectRequested && !g_hookCtx.cleanupDone) {
        zirconium::hook_performEject();
        g_hookCtx.cleanupDone = true;
        return TRUE;
    }
    if (g_hookCtx.cleanupDone) {
        return TRUE;
    }

    if (swapChain == nullptr) {
        LOG_ERROR("Swap chain is null.");
    }

    if (!g_renderCtx.initialized) {
        if (initDX11(swapChain)) {
            LOG_INFO("initDX11 completed successfully.");
        }
        else {
            LOG_ERROR("initDX11 failed.");
        }
    }

    zirconium::render_frame(g_hookCtx.device, g_hookCtx.deviceContext);
    return TRUE;
}

__declspec(naked) void __stdcall zirconium::hook_hookedPresent(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags) {
    /* IDA disassembly of CDGXISwapChain::Present:
    dxgi.dll:6A85E1F0 ; Attributes: bp-based frame fuzzy-sp
    dxgi.dll:6A85E1F0 ; public: virtual long __stdcall CDXGISwapChain::Present(unsigned int, unsigned int)
    dxgi.dll:6A85E1F0 ?Present@CDXGISwapChain@@UAGJII@Z proc near
    dxgi.dll:6A85E1F0
    dxgi.dll:6A85E1F0 var_3C          = dword ptr -3Ch
    dxgi.dll:6A85E1F0 var_38          = byte ptr -38h
    dxgi.dll:6A85E1F0 var_34          = dword ptr -34h
    dxgi.dll:6A85E1F0 var_30          = byte ptr -30h
    dxgi.dll:6A85E1F0 var_2C          = byte ptr -2Ch
    dxgi.dll:6A85E1F0 var_4           = dword ptr -4
    dxgi.dll:6A85E1F0 arg_0           = dword ptr  8 // member function 'this' ptr (swapChain)
    dxgi.dll:6A85E1F0 arg_4           = dword ptr  0Ch // SyncInterval
    dxgi.dll:6A85E1F0 arg_8           = dword ptr  10h // Flags
    dxgi.dll:6A85E1F0
    dxgi.dll:6A85E1F0                 mov     edi, edi        ; CODE XREF: [thunk]:CDXGISwapChain::Present`adjustor{56}' (uint,uint)
    dxgi.dll:6A85E1F2                 push    ebp
    dxgi.dll:6A85E1F3                 mov     ebp, esp
    dxgi.dll:6A85E1F5                 and     esp, 0FFFFFFF8h

    plutonium disassembly:
    present starts at 0x6A85E1F0

    6A85E20F | FF75 10                  | push dword ptr ss:[ebp+10] // flags
    6A85E212 | FF75 0C                  | push dword ptr ss:[ebp+C] // syncinterval
    6A85E215 | 56                       | push esi // swapchain
    call to internal present function...

    6 byte difference overwrite 5 for jmp 1 for nop
    */

    __asm {
        pushad // general
        pushfd // flags

        mov eax, dword ptr ss : [ebp + 0x10] // Flags
        push eax

        mov eax, dword ptr ss : [ebp + 0xC] // SyncInterval
        push eax

        mov eax, dword ptr ss : [ebp + 0x8] // swapChain
        push eax

        call hook_handler

        popfd
        popad

        // trampoline (original instructions overwritten)
        push dword ptr ss : [ebp + 0x10] // flags
        push dword ptr ss : [ebp + 0xC] // syncinterval

		jmp [g_hookCtx.resumeAddr]
    }
}

bool __stdcall zirconium::hook_install() {
    // resolve Present address BEFORE pausing threads — D3D11CreateDeviceAndSwapChain
    // needs GPU driver / DWM threads alive, suspending them first causes a deadlock
    LOG_INFO("Resolving Present address...");
    g_hookCtx.oPresent = (BYTE*)zirconium::hook_getPresentAddress();
    if (!g_hookCtx.oPresent) {
        LOG_ERROR("Failed to get Present address.");
        return false;
    }
    g_hookCtx.oPresent += HOOK_OFFSET;
    g_hookCtx.resumeAddr = g_hookCtx.oPresent + TRAMPOLINE_SZ;
    LOG_INFO("Present resolved: 0x%p, hook site: 0x%p", (void*)(g_hookCtx.oPresent - HOOK_OFFSET), (void*)g_hookCtx.oPresent);

    // now pause threads only for the actual byte-patching
    zirconium::memory_pauseAllThreads();

    DWORD oldProtect;
    if (!VirtualProtect(g_hookCtx.oPresent, TRAMPOLINE_SZ, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LOG_ERROR("Failed to change memory protection on Present.");
        zirconium::memory_resumeAllThreads();
        return false;
    }

	// save original bytes before patching so we can restore them on eject
    memcpy(g_hookCtx.originalBytes, g_hookCtx.oPresent, TRAMPOLINE_SZ);

	// overwrite the first bytes of Present with a jump to our hook
    DWORD hookRelativeAddr = (DWORD)&zirconium::hook_hookedPresent - ((DWORD)g_hookCtx.oPresent + JMP_SZ);
    g_hookCtx.oPresent[0] = JMP;
    memcpy(g_hookCtx.oPresent + 1, &hookRelativeAddr, sizeof(DWORD));

    // pad remaining bytes with NOPs
    for (int i = JMP_SZ; i < TRAMPOLINE_SZ; i++) {
		g_hookCtx.oPresent[i] = NOP;
    }

    if (!VirtualProtect(g_hookCtx.oPresent, TRAMPOLINE_SZ, oldProtect, &oldProtect)) {
	    LOG_ERROR("Failed to restore memory protection on Present.");
	    zirconium::memory_resumeAllThreads();
	    return false;
    }

    _mm_sfence();
    FlushInstructionCache(GetCurrentProcess(), g_hookCtx.oPresent, TRAMPOLINE_SZ);

    zirconium::memory_resumeAllThreads();
    return true;
}

/*
* Restores the original bytes at the hook site, removing the trampoline JMP.
*/
bool __stdcall zirconium::hook_uninstall() {
    if (!g_hookCtx.oPresent) return false;

    DWORD oldProtect;
    if (!VirtualProtect(g_hookCtx.oPresent, TRAMPOLINE_SZ, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LOG_ERROR("hook_uninstall: VirtualProtect failed.");
        return false;
    }
    memcpy(g_hookCtx.oPresent, g_hookCtx.originalBytes, TRAMPOLINE_SZ);
    VirtualProtect(g_hookCtx.oPresent, TRAMPOLINE_SZ, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), g_hookCtx.oPresent, TRAMPOLINE_SZ);
    LOG_INFO("Hook uninstalled, original bytes restored.");
    return true;
}

/*
* Tears down all resources acquired during injection: removes the Present hook,
* restores the original WndProc, shuts down ImGui, and releases D3D refs.
* Called from hook_handler() on the render thread when ejectRequested is set.
*/
void __stdcall zirconium::hook_performEject() {
    LOG_INFO("Eject requested, beginning cleanup.");

    // restore Present bytes with threads paused for safety
    memory_pauseAllThreads();
    hook_uninstall();
    memory_resumeAllThreads();

    // restore game's original window procedure
    if (g_renderCtx.oWndProc && g_renderCtx.hwnd) {
        SetWindowLongPtr(g_renderCtx.hwnd, GWLP_WNDPROC, (LONG_PTR)g_renderCtx.oWndProc);
        LOG_INFO("WndProc restored.");
    }

    // shut down ImGui backends and destroy context
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_renderCtx.ctx = nullptr;
    g_renderCtx.io = nullptr;
    LOG_INFO("ImGui shut down.");

    // release the render target view we created
    if (g_renderCtx.renderTargetView) {
        g_renderCtx.renderTargetView->Release();
        g_renderCtx.renderTargetView = nullptr;
    }

    // release the D3D refs we acquired via GetDevice/GetImmediateContext
    if (g_hookCtx.deviceContext) {
        g_hookCtx.deviceContext->Release();
        g_hookCtx.deviceContext = nullptr;
    }
    if (g_hookCtx.device) {
        g_hookCtx.device->Release();
        g_hookCtx.device = nullptr;
    }

    g_renderCtx.initialized = false;
    LOG_INFO("Cleanup complete.");
}

/*
* Starts the hook thread.
* @param hModule the handle to the DLL module (passed as lpParameter from CreateThread)
*/
void __stdcall zirconium::hook_startThread(HMODULE hModule) {
    g_hookCtx.hSelf = hModule;

    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

    std::string desktopPath = std::getenv("USERPROFILE");
    desktopPath += "\\Desktop\\output.log";
    std::ofstream logFile(desktopPath, std::ios::app);

    if (!logFile) {
        LOG_ERROR("Failed to open log file on the Desktop.");
        FreeLibraryAndExitThread(g_hookCtx.hSelf, 0);
        return;
    }

    DualOutputBuf dualBuf(std::cout.rdbuf(), logFile.rdbuf());
    std::streambuf* originalCoutBuffer = std::cout.rdbuf(&dualBuf);
    std::streambuf* originalCerrBuffer = std::cerr.rdbuf(&dualBuf);

    LOG_INFO("Hook thread started, checking process details.");
    SetUnhandledExceptionFilter(zirconium::ExceptionHandler);

    ULONG64 bitMask = 0;
    if (!GetProcessMitigationPolicy(GetCurrentProcess(), ProcessMitigationOptionsMask, &bitMask, sizeof(ULONG64))) {
        LOG_ERROR("Process mitigation policy call failed.");
        std::cout.rdbuf(originalCoutBuffer);
        std::cerr.rdbuf(originalCerrBuffer);
        logFile.close();
        FreeConsole();
        FreeLibraryAndExitThread(g_hookCtx.hSelf, 0);
        return;
    }

    LOG_INFO("----- Process mitigation options: -----\n%s", mitMaskToString(bitMask).c_str());
    LOG_INFO("---------------------------------------");

    if (!zirconium::hook_install()) {
        LOG_ERROR("Failed to install hook.");
        std::cout.rdbuf(originalCoutBuffer);
        std::cerr.rdbuf(originalCerrBuffer);
        logFile.close();
        FreeConsole();
        FreeLibraryAndExitThread(g_hookCtx.hSelf, 0);
        return;
    }

    LOG_INFO("Hook installed. Waiting for eject signal...");

    // wait until the render thread signals that cleanup is complete
    while (!g_hookCtx.cleanupDone) {
        Sleep(50);
    }

    LOG_INFO("Eject complete. Unloading DLL.");
    std::cout.rdbuf(originalCoutBuffer);
    std::cerr.rdbuf(originalCerrBuffer);
    logFile.close();
    FreeConsole();
    FreeLibraryAndExitThread(g_hookCtx.hSelf, 0);
}
