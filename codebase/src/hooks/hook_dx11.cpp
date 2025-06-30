#include "hook.h"
#include <d3d11.h>
#include <dxgi.h>

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
    void* presentAddr = vmt[PRESENT_INDEX]; 

    pSwapChain->Release();
    pDevice->Release();
    pContext->Release();
    DestroyWindow(hWnd);
    UnregisterClass(L"DummyWindow", wc.hInstance);

    return presentAddr;
}

static bool __stdcall initDX11(IDXGISwapChain* swapChain) {
	if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&hookVars::device))) {
			if (FAILED(hookVars::device->GetImmediateContext(&hookVars::deviceContext)) {
				LOG("GetImmediateContext failed.");
				return false;
			}
			
			DXGI_SWAP_CHAIN_DESC swapChainDesc;
			if (FAILED(swapChain->GetDesc(&swapChainDesc)) {
				LOG("Get swapChainDesc failed.");
				return false;
			}
			renderVars::g_hwnd = swapChainDesc.OutputWindow;

			ID3D11Texture2D* backBuffer = nullptr;
			if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer) 
				|| backBuffer == nullptr)
			{
				LOG("GetBuffer() failed/backBuffer == null");
				return false;
			}

			if (FAILED(hookVars::device->CreateRenderTargetView(backBuffer, NULL, renderVars::renderTargetView)) {
				LOG("CreateRenderTargetView failed.");
				return false;
			}
			backBuffer->Release();

			if (!initOverlay(hookVars::device, hookVars::deviceContext)) {
				LOG("ERROR: initOverlay failed.");
				return false;
			}
	}
	else {
		LOG("swapChain->GetDevice failed.");
		return false;
	}
	return true;
}

__declspec(naked) void __stdcall hookedPresent(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags) {
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
        mov Flags, eax

        mov eax, dword ptr ss : [ebp + 0xC] // SyncInterval
        mov SyncInterval, eax

        mov eax, dword ptr ss : [ebp + 0x8] // swapChain
        mov swapChain, eax
    }

	if (swapChain == nullptr) {
        LOG("ERROR: Swap chain is null.");
    }
	
    // verify params are correct
    LOG("swapChain: 0x", (void*)swapChain);
    LOG("SyncInterval:", SyncInterval);
    LOG("Flags:", Flags);

    LOG("Present instructions (inside hookedPresent):");
    for (int i = 0; i < TRAMPOLINE_SZ; i++) {
        LOG("0x", (void*)hookVars::oPresent[i]);
    }

    if (!renderVars::initialized) {
		if (initDX11(swapChain)) {
			LOG("SUCCESS: initDX11 completed successfully.");
		}
		else {
			LOG("ERROR: initDX11 failed.");
		}
	}

	renderOverlay(hookVars::device, hookVars::deviceContext);

    __asm {
        popfd
        popad

        // trampoline (original instructions overwritten)
        push dword ptr ss : [ebp + 0x10] // flags
        push dword ptr ss : [ebp + 0xC] // syncinterval

		jmp [hookVars::resumeAddr]
    }
}

/*
* Install the hook on the Present function.
* @return true if the hook was installed successfully, false otherwise
*/
static bool __stdcall installHook() {
    pauseAllThreads();

    hookVars::oPresent = (BYTE*)getPresentAddress();
    if (!hookVars::oPresent) {
        LOG("ERROR: Failed to get Present address.");
        return false;
    }
    hookVars::oPresent += HOOK_OFFSET;
    LOG("Present address (+ 0x1F):", (void*)hookVars::oPresent);
    hookVars::resumeAddr = hookVars::oPresent + TRAMPOLINE_SZ;
    LOG("Resume addr:", (void*)hookVars::resumeAddr);
    LOG("Hooked Present address:", &hookedPresent);
	    
    DWORD oldProtect;
    if (!VirtualProtect(hookVars::oPresent, TRAMPOLINE_SZ, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LOG("ERROR: Failed to change memory protection on Present.");
        return false;
    }

	// overwrite the first bytes of Present with a jump to our hook
    DWORD hookRelativeAddr = (DWORD)&hookedPresent - ((DWORD)hookVars::oPresent + JMP_SZ);
    hookVars::oPresent[0] = JMP;
    memcpy(hookVars::oPresent + 1, &hookRelativeAddr, sizeof(DWORD));
    
    // pad remaining bytes with NOPs
    for (int i = JMP_SZ; i < TRAMPOLINE_SZ; i++) {
		hookVars::oPresent[i] = NOP;
    }
    
    if (!VirtualProtect(hookVars::oPresent, TRAMPOLINE_SZ, oldProtect, &oldProtect)) {
	LOG("ERROR: Failed to restore memory protection on Present.");
	return false;
    }

    _mm_sfence(); // barrier to ensure that the changes are visible to other threads
    // this is unnecessary because we pause all threads but just in case
    
    FlushInstructionCache(GetCurrentProcess(), hookVars::oPresent, TRAMPOLINE_SZ);
    // flush to ensure that new instructions are visible to CPU
	
    LOG("New Present instructions:");
    for (int i = 0; i < TRAMPOLINE_SZ; i++) {
		LOG("0x", (void*)hookVars::oPresent[i]);
    }

    resumeAllThreads();
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
        LOG("ERROR: Failed to open log file on the Desktop.");
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
    while (1);
}
