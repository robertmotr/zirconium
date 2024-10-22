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
    uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
	if (!moduleBase) {
        LOG("getting module base failed!");
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
* @param returnAddress The address to jump back to after execution
*/
void execBytes(BYTE* instructions, void* execRegion, unsigned int len, void* returnAddress) {
    if (instructions == nullptr || execRegion == nullptr || returnAddress == nullptr) {
        LOG("ERROR: Provided nullptr as instructions/execRegion/returnAddress argument to execBytes.");
        return;
    }

    if (len == 0) {
        LOG("ERROR: Provided 0 as argument to length of bytes to execute");
        return;
    }

    LOG("Executing bytes at address: ", execRegion);

    // Step 5: Call the memory to execute the bytes followed by the jump back
    __asm {
		jmp [execRegion]
    }
    return;
}

/*
* Pauses all threads except the current one.
*/
void pauseAllThreads() {
    DWORD currentThreadId = GetCurrentThreadId();  // ID of the current thread
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);  // Get snapshot of all threads

    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot." << std::endl;
        return;
    }

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    // Iterate through the list of threads
    if (Thread32First(snapshot, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID == GetCurrentProcessId() && threadEntry.th32ThreadID != currentThreadId) {
                // Open the thread for suspend/resume
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                if (thread != NULL) {
                    SuspendThread(thread);  // Suspend the thread
                    CloseHandle(thread);
                }
            }
        } while (Thread32Next(snapshot, &threadEntry));
    }

    CloseHandle(snapshot);
}

/*
* Resumes all threads except the current one.
*/
void resumeAllThreads() {
    DWORD currentThreadId = GetCurrentThreadId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot." << std::endl;
        return;
    }

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    // Iterate through the list of threads
    if (Thread32First(snapshot, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID == GetCurrentProcessId() && threadEntry.th32ThreadID != currentThreadId) {
                // Open the thread for suspend/resume
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                if (thread != NULL) {
                    ResumeThread(thread);  // Resume the thread
                    CloseHandle(thread);
                }
            }
        } while (Thread32Next(snapshot, &threadEntry));
    }

    CloseHandle(snapshot);
}

void checkAddressSegment(void* address) {
    MEMORY_BASIC_INFORMATION mbi;

    // Query information about the memory region that contains 'address'
    if (VirtualQuery(address, &mbi, sizeof(mbi))) {
        LOG("Base Address: %p", mbi.BaseAddress);
        LOG("Allocation Base: %p", mbi.AllocationBase);
        LOG("Region Size: %zu bytes", mbi.RegionSize);

        LOG("State: ");
        switch (mbi.State) {
        case MEM_COMMIT:
            LOG("Committed");
            break;
        case MEM_FREE:
            LOG("Free");
            break;
        case MEM_RESERVE:
            LOG("Reserved");
            break;
        }

        LOG("Type: ");
        switch (mbi.Type) {
        case MEM_IMAGE:
            LOG("Image (code segment)");
            break;
        case MEM_MAPPED:
            LOG("Mapped");
            break;
        case MEM_PRIVATE:
            LOG("Private (heap, stack, etc.)");
            break;
        }

        LOG("Protection: ");
        if (mbi.Protect & PAGE_EXECUTE) LOG("Execute");
        if (mbi.Protect & PAGE_EXECUTE_READ) LOG("Execute/Read");
        if (mbi.Protect & PAGE_EXECUTE_READWRITE) LOG("Execute/Read/Write");
        if (mbi.Protect & PAGE_READONLY) LOG("Read-Only");
        if (mbi.Protect & PAGE_READWRITE) LOG("Read/Write");
    }
    else {
        LOG("Failed to query memory information.");
    }
}