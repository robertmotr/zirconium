#pragma once

#include "pch.h"
#include "render.h"
#include "mem.h"
#include "globals.h"

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