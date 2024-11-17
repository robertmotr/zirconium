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
    void* presentAddr = vmt[PRESENT_INDEX]; 

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
    
	REMEMBER: first 5 bytes are overwritten, these are the original instructions
    
    plutonium disassembly:
	present starts at 0x6A85E1F0

    6A85E20F | FF75 10                  | push dword ptr ss:[ebp+10] // flags                                                                                              |
    6A85E212 | FF75 0C                  | push dword ptr ss:[ebp+C] // syncinterval                                                                                                              |
    6A85E215 | 56                       | push esi // swapchain    
	call to internal present function...

    notice that theres a 6 byte difference, so overwrite 5 for jmp 
    and 1 for nop
    */
    
    __asm {
        push ebp
        mov ebp, esp 
        sub esp, 0x10 

        push ebx 
		push ecx

		mov eax, [ebp + 0x8] // swapChain
		mov ebx, [ebp + 0xC] // SyncInterval
		mov ecx, [ebp + 0x10] // Flags

        pushad                
        pushfd       
    }

    if (swapChain == nullptr) {
		LOG("ERROR: Swap chain is null.");
    }

    if (!hookVars::device || !hookVars::deviceContext) {
		LOG("Getting device and device context.");
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
            if (pBackBuffer == nullptr) {
                LOG("ERROR: Failed to get back buffer (nullptr).");
            }
            
            pBackBuffer->GetDevice(&hookVars::device);
            if (hookVars::device == nullptr) {
                LOG("ERROR: Failed to get device context (nullptr)");
            }
            hookVars::device->GetImmediateContext(&hookVars::deviceContext);
			if (hookVars::deviceContext == nullptr) {
				LOG("ERROR: Failed to get device context.");
			}
            pBackBuffer->Release();
            LOG("Successfully got device and device context.");
        }
        else {
            LOG("ERROR: Failed to get back buffer. GetBuffer() returned an error.");
        }
    }

    renderOverlay(hookVars::device, hookVars::deviceContext);

    __asm {
        popfd                 // restore flags
        popad                 // restore all general-purpose registers

		pop ecx
		pop ebx

        // fn epilogue
        mov esp, ebp 
        pop ebp 
        jmp [hookVars::trampoline]  // continue execution of Present
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

    // enough to hold the overwritten bytes + a jump back to the original fn
    hookVars::trampoline = (BYTE*)VirtualAlloc(nullptr, TRAMPOLINE_SZ + JMP_SZ , MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!hookVars::trampoline) {
        LOG("ERROR: VirtualAlloc failed for the trampoline.");
        return false;
    }

    // copy the overwritten bytes from Present to the trampoline
    memcpy(hookVars::trampoline, hookVars::oPresent, TRAMPOLINE_SZ);

    // relative address for the trampoline jump back
    DWORD trampolineBackAddr = ((DWORD)hookVars::oPresent + TRAMPOLINE_SZ) - ((DWORD)hookVars::trampoline + JMP_SZ + TRAMPOLINE_SZ);
    hookVars::trampoline[JMP_SZ] = JMP;
    memcpy(hookVars::trampoline + JMP_SZ + 1, &trampolineBackAddr, sizeof(DWORD));

    // overwrite the first bytes of Present with a jump to our hook
    DWORD oldProtect;
    if (VirtualProtect(hookVars::oPresent, TRAMPOLINE_SZ, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        DWORD hookRelativeAddr = (DWORD)&hookedPresent - ((DWORD)hookVars::oPresent + JMP_SZ);
        hookVars::oPresent[0] = JMP;
        memcpy((BYTE*)hookVars::oPresent + 1, &hookRelativeAddr, sizeof(DWORD));
        VirtualProtect(hookVars::oPresent, TRAMPOLINE_SZ, oldProtect, &oldProtect);
    }
    else {
        LOG("ERROR: Failed to change memory protection on Present.");
        return false;
    }
    _mm_sfence();
	FlushInstructionCache(GetCurrentProcess(), hookVars::oPresent, TRAMPOLINE_SZ);
	FlushInstructionCache(GetCurrentProcess(), hookVars::trampoline, TRAMPOLINE_SZ + JMP_SZ);

    // print trampoline addr, etc
	LOG("Trampoline address:", (void*)hookVars::trampoline);
	LOG("Original Present address:", (void*)hookVars::oPresent);
	LOG("Hooked Present address:", &hookedPresent);

    // print trampoline instructions
	LOG("Trampoline instructions:");
	for (int i = 0; i < TRAMPOLINE_SZ; i++) {
		LOG("0x", (void*)hookVars::trampoline[i]);
	}

	// print new Present instructions
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
