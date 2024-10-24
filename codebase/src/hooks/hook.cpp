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
*/

__declspec(naked) void __stdcall hkEndScene() {
    LOG("Hooked EndScene called.");
    __asm
    {
        mov eax, [esp + 8] // according to IDA pdevice lies at esp + 8
        mov hookVars::pDevice, eax 
    }
    
    if(hookVars::pDevice != nullptr) {
        renderOverlay(hookVars::pDevice);
    }
    else {
        LOG("ERROR: Got nullptr for pDevice inside hkEndScene.");
        // crash here or smth idk
    }
    
    __asm {
        mov eax, &oldEndSceneAsm
        jmp eax
    }
}

/*
* Finds the EndScene fn addr by creating a dummy device and getting the vtable.
	*
	* @return EndScene's address in process memory as a DWORD otherwise NULL if failed.
*/
BYTE* __stdcall findEndScene() {
    HMODULE hD3D = 0;
    while (!hD3D)
        hD3D = GetModuleHandleA("d3d9.dll");

	if (!hD3D) {
		LOG("ERROR: GetModuleHandleA failed on d3d9.dll.");
		LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
        return nullptr;
	}

	HWND hwnd = GetDesktopWindow();
	if (hwnd == NULL) {
		LOG("ERROR: GetDesktopWindow failed.");
		LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
        return nullptr;
	}

	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) {
        LOG("ERROR: Direct3DCreate9 failed.");
        LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
        return nullptr;
    }

	D3DDISPLAYMODE d3ddm;
	if (FAILED(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))) {
		LOG("ERROR: GetAdapterDisplayMode failed.");
		LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
        return nullptr;
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
        return nullptr;
	}

	DWORD* vTable = (DWORD*)*(DWORD*)pDevice;
	DWORD endSceneAddr = vTable[42];
	LOG("EndScene address found as: 0x", (void*)endSceneAddr);

	pDevice->Release();
	pD3D->Release();

	return (BYTE*)endSceneAddr;
}

/*
* Installs the hook by trampoline hooking the EndScene function with our hook.
    *
    * @return true if successful, false otherwise.
*/
bool __stdcall installHook() {

	pauseAllThreads();

	BYTE* endSceneAddr = findEndScene();
    if (endSceneAddr == nullptr) {
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

    // backup oEndScene
    memcpy(hookVars::oldEndSceneAsm, (void*)endSceneAddr, TRAMPOLINE_SZ);

    // overwrite endscene with a jump to hook + 2 NOPs to align with instruction boundaries
    *endSceneAddr = JMP_OPCODE;
    
    DWORD relativeJumpToHook = (DWORD)&hkEndScene - (*endSceneAddr + 5);
    *(DWORD*)(endSceneAddr + 1) = relativeJumpToHook;

    *(endSceneAddr + 5) = NOP_OPCODE;
    *(endSceneAddr + 6) = NOP_OPCODE;

    // print out the bytes we've written to the original EndScene
    LOG("Bytes written to original EndScene: ");
    for (int i = 0; i < HOOK_SZ; i++) {
        LOG("0x", (BYTE*)(endSceneAddr + i));
    }

    // change permissions for oldEndSceneAsm so we can use it as a trampoline
    if (!VirtualProtect(hookVars::oldEndSceneAsm, TRAMPOLINE_SZ + JMP_SZ, PAGE_EXECUTE_READWRITE, &protect)) {
        LOG("ERROR: Unable to change memory permissions at oldEndSceneAsm.");
        LOG("ERROR: Got error:", dwordErrorToString(GetLastError()));
        // crash or smth idk
    }

    // write the jump such that we jump to oEndScene + TRAMPOLINE_SZ bytes (otherwise, itd be looping infinitely)
    DWORD relativeJumpToEndScene = (hookVars::oEndScene + TRAMPOLINE_SZ) - 
                                   (hookVars::oldEndSceneAsm + TRAMPOLINE_SZ + 1 + TRAMPOLINE_SZ);
    hookVars::oldEndSceneAsm[TRAMPOLINE_SZ + 1] = JMP_OPCODE;
    hookVars::oldEndSceneAsm[TRAMPOLINE_SZ + 2] = relativeJumpToEndScene;

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