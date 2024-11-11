#include "hook.h"
#include <d3d11.h>
#include <dxgi.h>

// Dummy window procedure
LRESULT CALLBACK dummyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/*
* This function creates a dummy window and a swap chain to get the address of the Present function.
* @return the address of the Present function
*/
static void* getPresentAddress() {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = dummyWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"DummyWindow";
    RegisterClass(&wc);

    HWND hWnd = CreateWindow(L"DummyWindow", L"DummyWindow", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    if (hWnd == nullptr) {
        LOG("ERROR: Failed to create dummy window.");
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
        LOG("ERROR: Failed to create device and swap chain.");
        return nullptr;
    }

    void** vmt = *(void***)pSwapChain;
    void* presentAddr = vmt[8]; // index 8 for Present in IDXGISwapChain vmt

    pSwapChain->Release();
    pDevice->Release();
    pContext->Release();
    DestroyWindow(hWnd);
    UnregisterClass(L"DummyWindow", wc.hInstance);

    return presentAddr;
}

/*
* Custom Present function that will be called instead of the original Present function.
*/
__declspec(naked) void __stdcall hookedPresent(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags) {
    LOG("Hooked IDXGISwapChain::Present called");
    __asm {
        pushad                
        pushfd       

        mov eax, [esp + 36 + 4]
		mov swapChain, eax

		mov eax, [esp + 36 + 8]
		mov SyncInterval, eax

		mov eax, [esp + 36 + 0Ch]
		mov Flags, eax
    }

    if (!swapChain) {
		LOG("ERROR: Swap chain is null.");
    }

    if (!hookVars::device || !hookVars::deviceContext) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
            pBackBuffer->GetDevice(&hookVars::device);
            if (hookVars::device) {
                hookVars::device->GetImmediateContext(&hookVars::deviceContext);
                if (!hookVars::deviceContext) {
                    LOG("ERROR: Failed to get device context.");
                }
            }
            else {
                LOG("ERROR: Failed to get device.");
            }
            pBackBuffer->Release();
            LOG("Successfully got device and device context.");
        }
        else {
            LOG("ERROR: Failed to get back buffer.");
        }
    }

    LOG("Calling renderOverlay.");
    renderOverlay(hookVars::device, hookVars::deviceContext);

    __asm {
        popfd                 // restore flags
        popad                 // restore all general-purpose registers
        jmp [hookVars::trampoline]  // continue execution of Present
    }
}

/*
* Install the hook on the Present function.
* @return true if the hook was installed successfully, false otherwise
*/
static bool __stdcall installHook() {
    pauseAllThreads();
    LOG("Paused threads.");

    hookVars::oPresent = (BYTE*)getPresentAddress();
    if (!hookVars::oPresent) {
        LOG("ERROR: Failed to get Present address.");
        return false;
    }

    hookVars::trampoline = (BYTE*)VirtualAlloc(nullptr, 2 * JMP_SZ, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!hookVars::trampoline) {
        LOG("ERROR: VirtualAlloc failed for the trampoline.");
        return false;
    }

    // copy the overwritten bytes from Present to the trampoline
    memcpy(hookVars::trampoline, hookVars::oPresent, JMP_SZ);

    // relative address for the trampoline jump back
    DWORD trampolineBackAddr = ((DWORD)hookVars::oPresent + JMP_SZ) - ((DWORD)hookVars::trampoline + JMP_SZ + 5);
    hookVars::trampoline[JMP_SZ] = JMP;
    memcpy(hookVars::trampoline + JMP_SZ + 1, &trampolineBackAddr, sizeof(DWORD));

    // overwrite the first bytes of Present with a jump to our hook
    DWORD oldProtect;
    if (VirtualProtect(hookVars::oPresent, JMP_SZ, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        DWORD hookRelativeAddr = (DWORD)&hookedPresent - ((DWORD)hookVars::oPresent + JMP_SZ);
        *(BYTE*)hookVars::oPresent = JMP;
        memcpy((BYTE*)hookVars::oPresent + 1, &hookRelativeAddr, sizeof(DWORD));
    }
    else {
        LOG("ERROR: Failed to change memory protection on Present.");
        return false;
    }

    LOG("Trampoline hook installed successfully.");
    resumeAllThreads();
    LOG("Resumed threads.");
    return true;
}

/*
* Starts the hook thread.
* @param hModule the handle to the DLL module
*/
void __stdcall startThread(HMODULE hModule) {
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

    std::string desktopPath = std::getenv("USERPROFILE");
    desktopPath += "\\Desktop\\output.log";
    std::ofstream logFile(desktopPath, std::ios::app);

    if (!logFile) {
        std::cerr << "Failed to open log file on the Desktop." << std::endl;
        return;
    }

    DualOutputBuf dualBuf(std::cout.rdbuf(), logFile.rdbuf());
    std::streambuf* originalCoutBuffer = std::cout.rdbuf(&dualBuf);
    std::streambuf* originalCerrBuffer = std::cerr.rdbuf(&dualBuf);

    LOG("Hook thread started, checking process details.");
    SetUnhandledExceptionFilter(ExceptionHandler);

    ULONG64 bitMask = 0;
    if (!GetProcessMitigationPolicy(GetCurrentProcess(), ProcessMitigationOptionsMask, &bitMask, sizeof(ULONG64))) {
        LOG("ERROR: Process mitigation policy call failed.");
        return;
    }

    LOG("----- Process mitigation options: -----\n", mitMaskToString(bitMask));
    LOG("---------------------------------------");

    if (!installHook()) {
        LOG("Failed to install hook.");
        return;
    }
    LOG("Hook installed successfully.");
}
