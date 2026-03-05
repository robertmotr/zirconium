#pragma once

#include "pch.h"

namespace zirconium {
    
    /*
    * Exception handler for catching unhandled exceptions.
        *
        * @param ExceptionInfo The exception information.
        * @return The exception code.
    */
    LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);

    /*
    * Finds complete address given a module name and offset, i.e "client.dll" + 0x1234 = 0x12345678.
    * @param moduleName The name of the module to search for.
    * @param offset The offset from the module base address.
    * @return The final calculated address.
    */
    uintptr_t memory_resolveModuleAddress(const char *moduleName, const uintptr_t offset);

    /*
    * Pauses all threads except the current one.
    */
    void __stdcall memory_pauseAllThreads();

    /*
    * Resumes all threads except the current one.
    */
    void __stdcall memory_resumeAllThreads();

}