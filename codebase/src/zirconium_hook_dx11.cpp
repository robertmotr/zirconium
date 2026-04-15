#ifdef ZIRCONIUM_USE_TRAMPOLINE

#include "pch.h"

#include "zirconium_hook.h"
#include "zirconium_globals.h"
#include "zirconium_memory.h"

__declspec(naked) void __stdcall zirconium::hook_hookedPresent(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags) {
    __asm {
        pushad
        pushfd

        mov eax, dword ptr ss : [ebp + 0x10]
        push eax

        mov eax, dword ptr ss : [ebp + 0xC]
        push eax

        mov eax, dword ptr ss : [ebp + 0x8]
        push eax

        call hook_handler

        popfd
        popad

        push dword ptr ss : [ebp + 0x10]
        push dword ptr ss : [ebp + 0xC]

        jmp [g_hookCtx.resumeAddr]
    }
}

bool __stdcall zirconium::hook_install() {
    // presentAddr is already resolved by hook_startThread
    if (!g_hookCtx.presentAddr) {
        LOG_ERROR("hook_install: presentAddr not resolved.");
        return false;
    }
    g_hookCtx.oPresent = (BYTE*)g_hookCtx.presentAddr + HOOK_OFFSET;
    g_hookCtx.resumeAddr = g_hookCtx.oPresent + TRAMPOLINE_SZ;
    LOG_INFO("Trampoline hook site: 0x%p (Present + 0x%X)", (void*)g_hookCtx.oPresent, HOOK_OFFSET);

    zirconium::memory_pauseAllThreads();

    DWORD oldProtect;
    if (!VirtualProtect(g_hookCtx.oPresent, TRAMPOLINE_SZ, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LOG_ERROR("Failed to change memory protection on Present.");
        zirconium::memory_resumeAllThreads();
        return false;
    }

    memcpy(g_hookCtx.originalBytes, g_hookCtx.oPresent, TRAMPOLINE_SZ);

    DWORD hookRelativeAddr = (DWORD)&zirconium::hook_hookedPresent - ((DWORD)g_hookCtx.oPresent + JMP_SZ);
    g_hookCtx.oPresent[0] = JMP;
    memcpy(g_hookCtx.oPresent + 1, &hookRelativeAddr, sizeof(DWORD));

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

#endif // ZIRCONIUM_USE_TRAMPOLINE
