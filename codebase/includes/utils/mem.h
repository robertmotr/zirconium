#pragma once

#include "pch.h"

/*
* Exception handler for catching unhandled exceptions.
    *
    * @param ExceptionInfo The exception information.
    * @return The exception code.
*/
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

/*
* Compares a pattern to a specified memory region.
    *
    * @param data The memory region to search.
    * @param pattern The pattern to search for.
    * @param mask The mask specifying which bytes in the pattern to compare.
    * @return True if the pattern was found, false otherwise.
*/
bool compare(const uint8_t* data, const uint8_t* pattern, const char* mask);

/*
    * Searches for a pattern in a specified memory region.
    *
    * @param startAddress The starting address of the memory region to search.
    * @param regionSize The size of the memory region to search.
    * @param pattern The pattern to search for.
    * @param mask The mask specifying which bytes in the pattern to compare.
    * @return The address of the first occurrence of the pattern, or NULL if not found.
    */
uintptr_t patternScan(uintptr_t startAddress, size_t regionSize, const uint8_t* pattern, const char* mask);

/*
* Finds complete address given a module name and offset, i.e "client.dll" + 0x1234 = 0x12345678.
* @param moduleName The name of the module to search for.
* @param offset The offset from the module base address.
* @return The final calculated address.
*/
uintptr_t resolveModuleAddress(const char *moduleName, const uintptr_t offset);

/*
* Smart function for replacing len bytes from src at dst.
* @param src Pointer to an array of bytes that will replace dst.
* @param dst Destination address to be patched with.
* @param len # of bytes to replace
* @return true on success, false otherwise. See logging messages for details
*/
bool patchBytes(BYTE* src, BYTE* dst, unsigned int len);

/*
* Executes a sequence of bytes.
* @param instructions Pointer to instructions to be executed
* @param execRegion The region to execute the instructions in
* @param len The number of bytes to execute
* @param returnAddress The address to jump back to after execution
*/
bool execBytes(BYTE* bytes);

/*
* Writes n NOP (0x90) instructions to the specified address.
* @param address The address to write the NOP instructions to.
* @param n The number of NOP instructions to write.
*/
bool writeNOP(void* address, unsigned int n);

/*
* Pauses all threads except the current one.
*/
void __stdcall pauseAllThreads();

/*
* Resumes all threads except the current one.
*/
void __stdcall resumeAllThreads();

void checkAddressSegment(void* address);
