#include "mem.h"

/*
* Compares a pattern to a specified memory region.
    *
    * @param data The memory region to search.
    * @param pattern The pattern to search for.
    * @param mask The mask specifying which bytes in the pattern to compare.
    * @return True if the pattern was found, false otherwise.
*/
bool compare(const uint8_t* data, const uint8_t* pattern, const char* mask) {
    for (; *mask; ++mask, ++data, ++pattern) {
        if (*mask == 'x' && *data != *pattern) {
            return false;
        }
    }
    return true;
}

/*
    * Searches for a pattern in a specified memory region.
    *
    * @param startAddress The starting address of the memory region to search.
    * @param regionSize The size of the memory region to search.
    * @param pattern The pattern to search for.
    * @param mask The mask specifying which bytes in the pattern to compare.
    * @return The address of the first occurrence of the pattern, or NULL if not found.
    */
uintptr_t patternScan(uintptr_t startAddress, size_t regionSize, const uint8_t* pattern, const char* mask) {
    size_t patternLength = strlen(mask);

    for (size_t i = 0; i <= regionSize - patternLength; i++) {
        uintptr_t currentAddress = startAddress + i;
        if (compare(reinterpret_cast<const uint8_t*>(currentAddress), pattern, mask)) {
            return currentAddress;
        }
    }

    return NULL;
}

/*
* Finds complete address given a module name and offset, i.e "client.dll" + 0x1234 = 0x12345678.
* @param moduleName The name of the module to search for.
* @param offset The offset from the module base address.
* @return The final calculated address.
*/
uintptr_t resolveModuleAddress(const char* moduleName, const uintptr_t offset) {
    uintptr_t moduleBase = NULL;
    if (moduleName == nullptr) {
        moduleBase = (uintptr_t)GetModuleHandle(NULL);
	}
    else {
        moduleBase = (uintptr_t)GetModuleHandleA(moduleName);
    }

    if (!moduleBase) {
        LOG("ERROR: Failed to get module base address for module: ", moduleName);
        LOG("ERROR: Error code: ", dwordErrorToString(GetLastError()));

        // Optional: Attempt to load the module dynamically
        moduleBase = reinterpret_cast<uintptr_t>(LoadLibraryA(moduleName));
        if (!moduleBase) {
            LOG("ERROR: Dynamic loading of module also failed.");
            return NULL; 
        }
        else {
            LOG("Dynamic loading of module succeeded.");
        }
    }

    // offset doesn’t cause overflow (rare but safety)
    if (offset > UINTPTR_MAX - moduleBase) {
        LOG("ERROR: Offset is too large and causes overflow.");
        return NULL;
    }

    return moduleBase + offset;
}

/*
* Smart function for replacing len bytes from src at dst.
* @param src Pointer to an array of bytes that will replace dst.
* @param dst Destination address to be patched with.
* @param len # of bytes to replace
* @return true on success, false otherwise. See logging messages for details
*/
bool patchBytes(BYTE *src, BYTE *dst, unsigned int len) {
    if (src == nullptr) {
        LOG("ERROR: Provided nullptr as src argument to patchBytes.");
        return false;
    }
    else if (dst == nullptr) {
        LOG("ERROR: Provided nullptr as dst argument to patchBytes.");
    }

    if (len == 0) {
        LOG("ERROR: Provided 0 as argument to length of bytes to patch.");
        return false;
    }

    PMEMORY_BASIC_INFORMATION srcDetails;
    PMEMORY_BASIC_INFORMATION dstDetails;
    // TODO
}

/*
* Writes n NOP (0x90) instructions to the specified address.
* @param address The address to write the NOP instructions to.
* @param n The number of NOP instructions to write.
*/
bool writeNOP(void* address, unsigned int n) {
    if (address == nullptr) {
		LOG("ERROR: Provided nullptr as address argument to writeNOP.");
        return false;
    }

	if (n == 0) {
		LOG("ERROR: Provided 0 as argument to number of NOP instructions to write.");
		return false;
	}

	DWORD oldProtect;
	if (!VirtualProtect(address, n, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		LOG("ERROR: Failed to change memory protection.");
		LOG("ERROR CODE: ", dwordErrorToString(GetLastError()));
		return false;
	}

	memset(address, 0x90, n);
    return true;
}

/*
* Executes a sequence of bytes.
* @param bytes Pointer to bytes to execute
* @param len The number of bytes to execute
*/
bool execBytes(BYTE* bytes) {
    if (bytes == nullptr) {
        LOG("ERROR: Provided nullptr as bytes argument to execBytes.");
        return false;
    }

    typedef void(__stdcall *ExecFunc)();
    ExecFunc func = (ExecFunc)bytes;
	LOG("Executing bytes at address: 0x", (void*)bytes);
    func();
    LOG("Executed bytes.");

    return true;
}

/*
* Pauses all threads in the current process except the calling thread.
*/
void __stdcall pauseAllThreads() {
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD currentProcessId = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot.\n";
        return;
    }

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID == currentProcessId && threadEntry.th32ThreadID != currentThreadId) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                if (hThread) {
                    SuspendThread(hThread);  
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &threadEntry));
    }

    CloseHandle(hSnapshot);
}

/*
* Resumes all threads in the current process except the calling thread.
*/
void __stdcall resumeAllThreads() {
    DWORD currentThreadId = GetCurrentThreadId();
    DWORD currentProcessId = GetCurrentProcessId();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot.\n";
        return;
    }

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID == currentProcessId && threadEntry.th32ThreadID != currentThreadId) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                if (hThread) {
                    ResumeThread(hThread);  
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &threadEntry));
    }

    CloseHandle(hSnapshot);
}
