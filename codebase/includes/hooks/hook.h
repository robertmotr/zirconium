#pragma once

#include "pch.h"
#include "render.h"
#include "mem.h"
#include "globals.h"

// dummy window procedure
LRESULT CALLBACK dummyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
* This function creates a dummy window and a swap chain to get the address of the Present function.
* @return the address of the Present function
*/
void* getPresentAddress();

HRESULT __stdcall hookHandler(IDXGISwapChain* swapChain,
    UINT syncInterval,
    UINT flags);

/*
* Custom Present function that will be called instead of the original Present function.
*/
void __stdcall hookedPresent(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags);

/*
* Installs the hook by overwriting the EndScene function pointer in the VMT with our hook.
    *
    * @return S_OK if successful, E_FAIL otherwise.
*/
bool __stdcall installHook();

/*
* Starts a debug console and installs the hook. Waits for VK_END to close. This thread entry point is used to avoid blocking the main thread.
* (Not sure if this is necessary though)
    *
    * @param hModule The handle to the DLL module.
*/
void __stdcall startThread(HMODULE hModule);