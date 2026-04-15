#pragma once

#include "pch.h"

#define PRESENT_INDEX       8                       // index 8 for Present in IDXGISwapChain vmt
#define PE_MODULE_NAME      "plutonium-bootstrapper-win32.exe"
#define WINDOW_NAME         "Plutonium T6 Zombies (r5246)"

#ifdef ZIRCONIUM_USE_TRAMPOLINE
#define JMP_SZ              5
#define JMP                 0xE9
#define NOP                 0x90
#define TRAMPOLINE_SZ       6
#define HOOK_OFFSET         0x1F
#endif

namespace zirconium {

    LRESULT CALLBACK dummyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void* hook_getPresentAddress();

    HRESULT __stdcall hook_handler(
        IDXGISwapChain* swapChain,
        UINT            syncInterval,
        UINT            flags
    );

    void __stdcall hook_startThread(HMODULE hModule);
    void __stdcall hook_performEject(void);

#ifdef ZIRCONIUM_USE_TRAMPOLINE
    void __stdcall hook_hookedPresent(
        IDXGISwapChain* swapChain,
        UINT            SyncInterval,
        UINT            Flags
    );

    bool __stdcall hook_install(void);
    bool __stdcall hook_uninstall(void);
#endif

} // namespace zirconium
