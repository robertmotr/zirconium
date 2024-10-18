#include "hook.h"

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
    static volatile LPDIRECT3DDEVICE9 pDevice = nullptr;

    __asm {
        push 14h
        mov eax, 10084770
        // bytes overwritten for our jmp to this hook need to be executed here
        // this was determined through IDA 

        push esi
        mov esi, [ebp + 8] // determinmed by IDA that this ptr (dword ptr 8 local var) is pDevice
        mov pDevice, esi
        pop esi
    }

    LOG("Hooked EndScene called.");
    if (pDevice == nullptr) {
        LOG("ERROR: pDevice inside hkEndScene is nullptr.");
        exit(-1);
    }
    renderOverlay(pDevice);
    
    __asm {
        jmp [hookVars::oEndScene + 7] // resume rest of original endscene 
    }
}

/*
* Installs the hook by trampoline hooking the EndScene function with our hook.
    *
    * @return true if successful, false otherwise.
*/
bool __stdcall installHook() {

    HMODULE hD3D = 0;
    while (!hD3D) 
        hD3D = GetModuleHandle(L"d3d9.dll");

    hookVars::oEndScene = GetProcAddress(hD3D, "EndScene");
    if (hookVars::oEndScene == NULL) {
        LOG("ERROR: GetProcAddress failed on getting EndScene.");
        LOG("ERROR: Got error ", dwordErrorToString(GetLastError()));
    }

    DWORD protect;
    if (!VirtualProtect(hookVars::oEndScene, 7, PAGE_EXECUTE_READWRITE, &protect)) {
        LOG("ERROR: Changing read/write perms on EndScene address failed.");
        LOG("ERROR: Got error", dwordErrorToString(GetLastError()));
    }

    memcpy(hookVars::oldEndSceneAsm, hookVars::oEndScene, 7 * sizeof(BYTE));

    // sanity check that our bytes match disassembler output from IDA
    for (unsigned int i = 0; i < OENDSCENE_NUM_BYTES_OVERWRITTEN; i++) {
        if (hookVars::oldEndSceneAsm[i] != hookVars::expectedEndSceneAsm[i]) {
            LOG("ERROR: Getting fn addr for EndScene probably went wrong.");
            LOG("ERROR: Expected", (void*)hookVars::expectedEndSceneAsm[i], 
                       "but got ", (void*)hookVars::oldEndSceneAsm[i]);
            
            if(!VirtualProtect(hookVars::oEndScene, 7, protect, &protect)) {
                LOG("ERROR: Restoring VirtualProtect perms after comparing EndScene bytes failed.");
                LOG("ERROR: Got error", dwordErrorToString(GetLastError()));
            }
            return false;
        }
    }
    
    // rel addr formula = absolute destination - (current EIP + jmp opcode sz)
    hookVars::relJmpAddrToHook = (DWORD)&hkEndScene - ((DWORD)hookVars::oEndScene + 5);
    hookVars::newEndSceneAsm[0] = 0xE9;
    memset(hookVars::newEndSceneAsm + 1, hookVars::relJmpAddrToHook, sizeof(DWORD));
    // for jmp in x86 opcode is first byte 0xE9 to denote jmp, 
    // and then next 4 bytes for rel addr to jump to

    // fill the rest with NOPs to ensure valid instruction bounds 
    // (2 NOPs, 1 byte each, opcode 0x90)
    memset(hookVars::newEndSceneAsm + 5, 0x9090, 2 * sizeof(BYTE));

    // sanity check oEndScene == newEndSceneAsm
    for (unsigned int i = 0; i < 7; i++) {
        if (*((DWORD*)hookVars::oEndScene + i) != hookVars::newEndSceneAsm[i]) {
            LOG("ERROR: oEndScene[", i, "] does not match newEndScene[", i, "] after patching.");
            exit(0);
        }
    }

    LOG("Installed hook successfully");
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

    LOG("Process mitigation options:\n", mitMaskToString(bitMask));

    if (!installHook) {
        LOG("Failed to install hook.");
        return;
    }

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