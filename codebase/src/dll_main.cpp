#include "pch.h"
#include "vmt_hook.h"

BOOL __stdcall DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        LOG("DLL_PROCESS_ATTACH called.");
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)startThread, NULL, NULL, NULL);
	}
	else if (fdwReason == DLL_PROCESS_DETACH) {
		LOG("DLL_PROCESS_DETACH called.");
	}
	else if (fdwReason == DLL_THREAD_ATTACH)
	{
		LOG("DLL_THREAD_ATTACH called.");
	}
	else if (fdwReason == DLL_THREAD_DETACH)
	{
		LOG("DLL_THREAD_DETACH called.");
	}
    return TRUE;
}