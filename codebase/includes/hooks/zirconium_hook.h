#pragma once

#include "pch.h"

#define JMP_SZ              5 					// jmp size
#define JMP                 0xE9 				// jmp opcode
#define NOP                 0x90 				// nop opcode
#define TRAMPOLINE_SZ       6 					// size of the trampoline 
#define HOOK_OFFSET  	    0x1F 				// offset from g_hookCtx.oPresent. This is where we hook our initial jmp
#define PRESENT_INDEX       8 					// index 8 for Present in IDXGISwapChain vmt

#define PE_MODULE_NAME      "plutonium-bootstrapper-win32.exe" 	// name of the module to hook
#define WINDOW_NAME         "Plutonium T6 Zombies (r5246)" 	// name of the window to hook

namespace zirconium {

    LRESULT CALLBACK dummyWndProc(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    );

    /*
    * Creates a dummy window and a swap chain to get the address of the Present function.
    * @return the address of the Present function
    */
    void* hook_getPresentAddress();

    HRESULT __stdcall hook_handler(
        IDXGISwapChain* swapChain,
        UINT            syncInterval,
        UINT            flags
    );

    /*
    * Custom Present function that will be called instead of the original Present function.
    */
    void __stdcall hook_hookedPresent(
        IDXGISwapChain* swapChain,
        UINT            SyncInterval,
        UINT            Flags
    );

    /*
    * Installs the hook by overwriting the EndScene function pointer in the VMT with our hook.
        *
        * @return S_OK if successful, E_FAIL otherwise.
    */
    bool __stdcall hook_install(void);

    /*
    * Restores the original bytes at the hook site, removing the trampoline JMP.
        *
        * @return true if successful, false otherwise
    */
    bool __stdcall hook_uninstall(void);

    /*
    * Starts a debug console and installs the hook. Waits for VK_END to close. This thread entry point is used to avoid blocking the main thread.
    * (Not sure if this is necessary though)
        *
        * @param hModule The handle to the DLL module.
    */
    void __stdcall hook_startThread(HMODULE hModule);

    /*
    * Tears down all resources acquired during injection: removes the Present hook,
    * restores the original WndProc, shuts down ImGui, and releases D3D refs.
    * Called from hookHandler() on the render thread when ejectRequested is set.
    */
    void __stdcall hook_performEject(void);

}