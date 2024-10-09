#include "pch.h"
#include "vmt_hook.h"

DWORD*                          oEndScene = nullptr; // original EndScene function address
DWORD*                          vtable = nullptr; // IDirect3DDevice9 virtual method table
volatile LPDIRECT3DDEVICE9      pDevice = nullptr; // IDirect3DDevice9 pointer being used in the target application

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
        pointer passed to EndScene. I didn't come up with this asm code as well as finding the offset,
        credits to the UC posts I've researched and the original author.
*/
__declspec(naked) void __stdcall hkEndScene() {

    // asm instructions documented for clarity

    __asm {
        pushad // push all general purpose registers onto stack
        pushfd // push all flags onto stack
        push esi // push esi onto stack

        mov esi, dword ptr ss : [esp + 0x2c] // gets the IDirect3DDevice9 pointer from the stack
        mov pDevice, esi // store the pointer in a global variable
    }

    renderOverlay(pDevice);

    __asm {
        pop esi // pop esi off stack
        popfd // pop flags off stack
        popad // pop all general purpose registers off stack
        jmp oEndScene // jump back to the address stored in the oEndScene variable
    }
}

/*
* Finds the IDirect3DDevice9 pointer and the VMT. Address to VMT is then stored globally in vtable.
    *
    * @return S_OK if successful, E_FAIL otherwise.
*/
HRESULT __stdcall findVMT() {
    LOG("Attempting to find VMT...");
    DWORD hD3D = NULL;
    while (!hD3D) hD3D = (DWORD)GetModuleHandle(L"d3d9.dll");
    LOG("d3d9.dll found.");
    // pointer chain:
    // d3d9.dll -> IDirect3D9 -> IDirect3DDevice9 -> IDirect3DDevice9 VMT
    // honestly i have no idea how this pattern was found, but it works, credits to the original author
    DWORD PPPDevice = patternScan(hD3D, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
    if (PPPDevice == NULL) {
        LOG("Failed to find IDirect3DDevice9 pointer.");
        return E_FAIL;
    }
    LOG("IDirect3DDevice9 pointer found.");
    memcpy(&vtable, (void*)(PPPDevice + 2), 4);
    return S_OK;
}

/*
* Installs the hook by overwriting the EndScene function pointer in the VMT with our hook.
    *
    * @return S_OK if successful, E_FAIL otherwise.
*/
HRESULT __stdcall installHook() {
    if (!vtable) {
        LOG("VMT not found.");
        return E_FAIL;
    }

    LOG("Attempting to install hook...");

    // overwrite vtable entry with our hook
    DWORD oldProtect;
    if (VirtualProtect(&vtable[ENDSCENE_INDEX], sizeof(DWORD), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        // save original EndScene function pointer
        oEndScene = (DWORD*)vtable[ENDSCENE_INDEX];
        vtable[ENDSCENE_INDEX] = (DWORD)&hkEndScene;
        VirtualProtect(&vtable[ENDSCENE_INDEX], sizeof(DWORD), oldProtect, &oldProtect);
        LOG("Successfully installed hook.");
        // dx9 endscene calls should now be redirected to preHkEndScene
    }
    else {
        LOG("Failed to install hook.");
        return E_FAIL;
    }
    return S_OK;
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
    LOG("Hook thread started.");

    if (FAILED(findVMT())) {
        LOG("Failed to find VMT.");
        return;
    }
    if (FAILED(installHook())) {
        LOG("Failed to install hook.");
        return;
    }

    //LOG("Hook installed successfully. Waiting for VK_END to unhook...");
    //while (!GetAsyncKeyState(VK_END)) {
    //    Sleep(1);
    //}
    // TODO?
    // LOG("VK_END pressed. Unhooking...");
    // Unhook and cleanup
    // TODO: Implement unhooking

    FreeLibraryAndExitThread(hModule, 0);
    return;
}