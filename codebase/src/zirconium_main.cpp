#include "pch.h"

#include "zirconium_hook.h"
#include "zirconium_render.h"
#include "zirconium_globals.h"
#include "zirconium_memory.h"

#define GUAC_IMPLEMENTATION
#include "guac.h"

#ifndef ZIRCONIUM_USE_TRAMPOLINE
typedef HRESULT (__stdcall *PresentFn)(IDXGISwapChain*, UINT, UINT);
#endif

zirconium::HookCtx zirconium::g_hookCtx{};

BOOL __stdcall DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)zirconium::hook_startThread, hModule, NULL, NULL);
    }
    return TRUE;
}

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
        DestroyWindow(hWnd);
        UnregisterClass(L"DummyWindow", wc.hInstance);
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
        if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer))) {
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

#ifndef ZIRCONIUM_USE_TRAMPOLINE
static HRESULT __stdcall hwbp_presentDetour(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags)
{
    zirconium::hook_handler(swapChain, SyncInterval, Flags);

    // bypass the HWBP so we can call the real Present without re-triggering the hook
    guac_gate_enter(&zirconium::g_hookCtx.presentHwbpHandle);
    HRESULT hr = ((PresentFn)zirconium::g_hookCtx.presentAddr)(swapChain, SyncInterval, Flags);
    guac_gate_exit(&zirconium::g_hookCtx.presentHwbpHandle);

    return hr;
}
#endif

void __stdcall zirconium::hook_performEject() {
    LOG_INFO("Eject requested, beginning cleanup.");

#ifdef ZIRCONIUM_USE_TRAMPOLINE
    memory_pauseAllThreads();
    hook_uninstall();
    memory_resumeAllThreads();
#else
    guac_unhook(&g_hookCtx.presentHwbpHandle);
    LOG_INFO("HWBP Present hook removed.");
#endif

    // restore game's original window procedure
    if (g_renderCtx.oWndProc && g_renderCtx.hwnd) {
        SetWindowLongPtr(g_renderCtx.hwnd, GWLP_WNDPROC, (LONG_PTR)g_renderCtx.oWndProc);
        LOG_INFO("WndProc restored.");
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_renderCtx.ctx = nullptr;
    g_renderCtx.io = nullptr;
    LOG_INFO("ImGui shut down.");

    if (g_renderCtx.renderTargetView) {
        g_renderCtx.renderTargetView->Release();
        g_renderCtx.renderTargetView = nullptr;
    }

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

static void shutdownThread(const char* reason)
{
    LOG_INFO("Shutting down hook thread: %s", reason);
#ifdef _DEBUG
    FreeConsole();
#endif
    FreeLibraryAndExitThread(zirconium::g_hookCtx.hSelf, 0);
}

void __stdcall zirconium::hook_startThread(HMODULE hModule) {
    g_hookCtx.hSelf = hModule;

#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
#endif

    LOG_INFO("Hook thread started, checking process details.");
    SetUnhandledExceptionFilter(zirconium::ExceptionHandler);

    g_hookCtx.presentAddr = zirconium::hook_getPresentAddress();
    if (!g_hookCtx.presentAddr) {
        LOG_ERROR("Failed to get Present address.");
        shutdownThread("Present address resolution failed");
        return;
    }
    LOG_INFO("Present resolved: 0x%p", g_hookCtx.presentAddr);

#ifdef ZIRCONIUM_USE_TRAMPOLINE
    if (!zirconium::hook_install()) {
        LOG_ERROR("Failed to install trampoline hook.");
        shutdownThread("trampoline hook install failed");
        return;
    }
    LOG_INFO("Trampoline hook installed.");
#else
    guac_status_t status = guac_hook(
        &g_hookCtx.presentHwbpHandle,
        g_hookCtx.presentAddr,
        (void*)&hwbp_presentDetour,
        NULL
    );
    if (status != GUAC_ERROR_NONE) {
        LOG_ERROR("Failed to install HWBP hook: %s", guac_status_string(status));
        shutdownThread("HWBP hook install failed");
        return;
    }
    LOG_INFO("HWBP hook installed on Present (DR%d).", g_hookCtx.presentHwbpHandle);
#endif

    while (!g_hookCtx.cleanupDone) {
        Sleep(50);
    }

    shutdownThread("eject complete");
}
