#include "mem.h"

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

    // offset doesn�t cause overflow (rare but safety)
    if (offset > UINTPTR_MAX - moduleBase) {
        LOG("ERROR: Offset is too large and causes overflow.");
        return NULL;
    }

    return moduleBase + offset;
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

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    DWORD exceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;
    void* exceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionAddress;
    CONTEXT* context = ExceptionInfo->ContextRecord;

    LOG("Exception Code:", "0x", (void*)exceptionCode);
    LOG("Exception Address:", "0x", (void*)exceptionAddress);

    switch (exceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        LOG("Exception: Access Violation");
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        LOG("Exception: Array Bounds Exceeded");
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        LOG("Exception: Float Divide by Zero");
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        LOG("Exception: Integer Divide by Zero");
        break;
    default:
        LOG("Exception: Unknown");
        break;
    }

    LOG("Register State:");
    LOG("EAX:", "0x", (void*)context->Eax);
    LOG("EBX:", "0x", (void*)context->Ebx);
    LOG("ECX:", "0x", (void*)context->Ecx);
    LOG("EDX:", "0x", (void*)context->Edx);
    LOG("ESI:", "0x", (void*)context->Esi);
    LOG("EDI:", "0x", (void*)context->Edi);
    LOG("EBP:", "0x", (void*)context->Ebp);
    LOG("ESP:", "0x", (void*)context->Esp);
    LOG("EIP:", "0x", (void*)context->Eip);

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymInitialize(process, NULL, TRUE);
    STACKFRAME64 stackFrame;
    memset(&stackFrame, 0, sizeof(STACKFRAME64));

    stackFrame.AddrPC.Offset = context->Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context->Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context->Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;

    LOG("Stack Trace:");
    for (int i = 0; i < 10; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &stackFrame, context, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            break;
        }

        LOG("Frame", i, ":", "0x", (void*)stackFrame.AddrPC.Offset);
    }
    SymCleanup(process);

    return EXCEPTION_EXECUTE_HANDLER;
}