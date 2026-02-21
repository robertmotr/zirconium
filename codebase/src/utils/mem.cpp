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
        LOG_ERROR("Failed to get module base address for module: %s", moduleName);
        LOG_ERROR("Error code: %s", dwordErrorToString(GetLastError()).c_str());

        // Optional: Attempt to load the module dynamically
        moduleBase = reinterpret_cast<uintptr_t>(LoadLibraryA(moduleName));
        if (!moduleBase) {
            LOG_ERROR("Dynamic loading of module also failed.");
            return NULL;
        }
        else {
            LOG_INFO("Dynamic loading of module succeeded.");
        }
    }

    // offset doesnt cause overflow (rare but safety)
    if (offset > UINTPTR_MAX - moduleBase) {
        LOG_ERROR("Offset is too large and causes overflow.");
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
        LOG_ERROR("Failed to create snapshot.");
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
        LOG_ERROR("Failed to create snapshot.");
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
        LOG_DEBUG("Base Address: %p", mbi.BaseAddress);
        LOG_DEBUG("Allocation Base: %p", mbi.AllocationBase);
        LOG_DEBUG("Region Size: %zu bytes", mbi.RegionSize);

        switch (mbi.State) {
        case MEM_COMMIT:
            LOG_DEBUG("State: Committed");
            break;
        case MEM_FREE:
            LOG_DEBUG("State: Free");
            break;
        case MEM_RESERVE:
            LOG_DEBUG("State: Reserved");
            break;
        }

        switch (mbi.Type) {
        case MEM_IMAGE:
            LOG_DEBUG("Type: Image (code segment)");
            break;
        case MEM_MAPPED:
            LOG_DEBUG("Type: Mapped");
            break;
        case MEM_PRIVATE:
            LOG_DEBUG("Type: Private (heap, stack, etc.)");
            break;
        }

        if (mbi.Protect & PAGE_EXECUTE) LOG_DEBUG("Protection: Execute");
        if (mbi.Protect & PAGE_EXECUTE_READ) LOG_DEBUG("Protection: Execute/Read");
        if (mbi.Protect & PAGE_EXECUTE_READWRITE) LOG_DEBUG("Protection: Execute/Read/Write");
        if (mbi.Protect & PAGE_READONLY) LOG_DEBUG("Protection: Read-Only");
        if (mbi.Protect & PAGE_READWRITE) LOG_DEBUG("Protection: Read/Write");
    }
    else {
        LOG_ERROR("Failed to query memory information.");
    }
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    DWORD exceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;
    void* exceptionAddress = ExceptionInfo->ExceptionRecord->ExceptionAddress;
    CONTEXT* context = ExceptionInfo->ContextRecord;

    LOG_ERROR("Exception Code: 0x%08X", exceptionCode);
    LOG_ERROR("Exception Address: %p", exceptionAddress);

    switch (exceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        LOG_ERROR("Exception: Access Violation");
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        LOG_ERROR("Exception: Array Bounds Exceeded");
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        LOG_ERROR("Exception: Float Divide by Zero");
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        LOG_ERROR("Exception: Integer Divide by Zero");
        break;
    default:
        LOG_ERROR("Exception: Unknown (0x%08X)", exceptionCode);
        break;
    }

    LOG_ERROR("Register State:");
    LOG_ERROR("EAX: 0x%08X", context->Eax);
    LOG_ERROR("EBX: 0x%08X", context->Ebx);
    LOG_ERROR("ECX: 0x%08X", context->Ecx);
    LOG_ERROR("EDX: 0x%08X", context->Edx);
    LOG_ERROR("ESI: 0x%08X", context->Esi);
    LOG_ERROR("EDI: 0x%08X", context->Edi);
    LOG_ERROR("EBP: 0x%08X", context->Ebp);
    LOG_ERROR("ESP: 0x%08X", context->Esp);
    LOG_ERROR("EIP: 0x%08X", context->Eip);

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

    LOG_ERROR("Stack Trace:");
    for (int i = 0; i < 10; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &stackFrame, context, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            break;
        }

        LOG_ERROR("Frame %d: 0x%08X", i, (DWORD)stackFrame.AddrPC.Offset);
    }
    SymCleanup(process);

    return EXCEPTION_EXECUTE_HANDLER;
}
