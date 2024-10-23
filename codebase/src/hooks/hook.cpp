#include "hook.h"

/*
* Exception handler for catching unhandled exceptions.
    *
    * @param ExceptionInfo The exception information.
    * @return The exception code.
*/
LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    LOG("Exception caught! Code: 0x", ExceptionInfo->ExceptionRecord->ExceptionCode);

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        LOG("ERROR: Access violation occurred!");
    }

    LOG("Press Enter to close the console...");
    std::cin.get();

    return EXCEPTION_EXECUTE_HANDLER;
}

/*
    Hooked EndScene function for DX9 rendering pipeline.
    This function intercepts the call to the original EndScene method in the
    IDirect3DDevice9 vtable, allowing us to insert custom rendering logic (e.g.,
    ImGui overlay) before handing control back to DirectX for normal rendering.

    How this works:
        1. Inline assembly is used to get the CPU's state (registers and flags).
        2. The device pointer (LPDIRECT3DDEVICE9) is retrieved from the stack.
        In the case of DX9, the device pointer is passed to EndScene and
        is located at an offset on the stack. We access it manually.
        3. `renderOverlay(pDevice)` is called to execute our custom rendering logic.
        4. We restore the CPU's state using inline assembly, making sure to return
        control back to the original EndScene function(`oEndScene`), which ensures
        DirectX continues rendering as expected.

    Notes:
        - The function is marked as `__declspec(naked)` to avoid the compiler generating
        prologue and epilogue code (such as setting up the stack frame). This is essential
        because we're manually manipulating the stack and registers.
        - The 0x2C offset is specific to the calling convention and the layout of arguments
        on the stack when EndScene is called. This offset points to the IDirect3DDevice9
        pointer passed to EndScene. 
*/

HRESULT __stdcall customEndScene(LPDIRECT3DDEVICE9 pDevice) {
    LOG("Custom End Scene called.");

    if (pDevice == nullptr) {
		LOG("ERROR: pDevice is null inside customEndScene");
        return E_FAIL;
	}

	return hookVars::oEndScene(pDevice);
}

__declspec(naked) HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
    __asm
    {
		; Preserve registers and flags
        push ebp
        mov ebp, esp
        pushad
        pushfd

        ; Push the parameter(pDevice)
        push[ebp + 8]

        call CustomEndScene

        ; Restore CPU state
        popfd
        popad
        pop ebp

        ret
    }
}

/*
* Finds the EndScene fn addr by creating a dummy device and getting the vtable.
	*
	* @return EndScene's address in process memory as a DWORD otherwise NULL if failed.
*/
DWORD findEndScene() {
    HMODULE hD3D = 0;
    while (!hD3D)
        hD3D = GetModuleHandleA("d3d9.dll");

	if (!hD3D) {
		LOG("ERROR: GetModuleHandleA failed on d3d9.dll.");
		LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
		return NULL;
	}

	HWND hwnd = GetDesktopWindow();
	if (hwnd == NULL) {
		LOG("ERROR: GetDesktopWindow failed.");
		LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
		return NULL;
	}

	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) {
        LOG("ERROR: Direct3DCreate9 failed.");
        LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
        return NULL;
    }

	D3DDISPLAYMODE d3ddm;
	if (FAILED(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))) {
		LOG("ERROR: GetAdapterDisplayMode failed.");
		LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
		return NULL;
	}

	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = d3ddm.Format;

	LPDIRECT3DDEVICE9 pDevice = nullptr;
	if (SUCCEEDED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice))) {
		LOG("Created device successfully.");
	}
    else {
        LOG("ERROR: CreateDevice failed.");
        LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
        return NULL;
	}

	DWORD* vTable = (DWORD*)*(DWORD*)pDevice;
	DWORD endSceneAddr = vTable[42];
	LOG("EndScene address found as: 0x", (void*)endSceneAddr);

	pDevice->Release();
	pD3D->Release();

	return endSceneAddr;
}

/*
* Installs the hook by trampoline hooking the EndScene function with our hook.
    *
    * @return true if successful, false otherwise.
*/
bool __stdcall installHook() {

	pauseAllThreads();

	DWORD endSceneAddr = findEndScene();
    if (endSceneAddr == NULL) {
        LOG("ERROR: GetProcAddress failed on getting EndScene.");
        LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
        return false;
    }

    DWORD protect;
    if (!VirtualProtect((void*)endSceneAddr, TRAMPOLINE_SZ, PAGE_EXECUTE_READWRITE, &protect)) {
        LOG("ERROR: Changing read/write perms on EndScene address failed.");
        LOG("ERROR: Got error", dwordErrorToString(GetLastError()));
        return false;
    }
	LOG("VirtualProtect perms changed successfully on EndScene ASM.");

    memcpy(hookVars::oldEndSceneAsm, (void*)endSceneAddr, TRAMPOLINE_SZ);

	hookVars::execMem = VirtualAlloc(0, TRAMPOLINE_SZ + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!hookVars::execMem) {
		LOG("ERROR: VirtualAlloc failed.");
		LOG("ERROR: Got error", dwordErrorToString(GetLastError()));
		return false;
	}

	// print out the bytes we've written to the original EndScene
	LOG("Bytes written to original EndScene: ");
	for (int i = 0; i < TRAMPOLINE_SZ; i++) {
		LOG("0x", std::hex, (int)*(BYTE*)(endSceneAddr + i));
	}

    LOG("Installed hook successfully");
    resumeAllThreads();
    return true;
}

/*
* Starts a debug console and installs the hook. Waits for VK_END to close. This thread entry point is used to avoid blocking the main thread.
* (Not sure if this is necessary though)
    *
    * @param hModule The handle to the DLL module.
*/
void __stdcall startThread(HMODULE hModule) {
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

    LOG("Hook thread started, checking process details.");

    ULONG64 bitMask = 0;
    if (!GetProcessMitigationPolicy(GetCurrentProcess(), ProcessMitigationOptionsMask, &bitMask, sizeof(ULONG64))) {
        LOG("ERROR: Process mitigation policy call failed.");
        LOG("ERROR: Error code: ", dwordErrorToString(GetLastError()));
        return;
    }

    LOG("----- Process mitigation options: -----\n", mitMaskToString(bitMask));
    LOG("---------------------------------------");

    SetUnhandledExceptionFilter(ExceptionHandler);
    
    if (!installHook()) {
        LOG("Failed to install hook.");
        return;
    }
    LOG("Hook installed successfully.");

    // TODO
    //LOG("Hook installed successfully. Waiting for VK_END to unhook...");
    //while (!GetAsyncKeyState(VK_END)) {
    //    Sleep(1);
    //}
    // TODO?
    // LOG("VK_END pressed. Unhooking...");
    // Unhook and cleanup
    // TODO: Implement unhooking

    /*FreeLibraryAndExitThread(hModule, 0);*/
    return;
}