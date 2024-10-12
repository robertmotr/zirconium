#pragma once

#include "render.h"
#include "scan_mem.h"

#define                                     ENDSCENE_INDEX 42 // index of EndScene() in IDirect3DDevice9 vtable

extern DWORD*								oEndScene; // original EndScene function address
extern DWORD*								vtable; // IDirect3DDevice9 virtual method table
extern volatile LPDIRECT3DDEVICE9           pDevice; // IDirect3DDevice9 pointer being used in the target application

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
void __stdcall hkEndScene();

/*
* Finds the IDirect3DDevice9 pointer and the VMT. Address to VMT is then stored globally in vtable.
    *
    * @return S_OK if successful, E_FAIL otherwise.
*/
HRESULT __stdcall findVMT();

/*
* Installs the hook by overwriting the EndScene function pointer in the VMT with our hook.
    *
    * @return S_OK if successful, E_FAIL otherwise.
*/
HRESULT __stdcall installHook();

/*
* Starts a debug console and installs the hook. Waits for VK_END to close. This thread entry point is used to avoid blocking the main thread.
* (Not sure if this is necessary though)
    *
    * @param hModule The handle to the DLL module.
*/
void __stdcall startThread(HMODULE hModule);