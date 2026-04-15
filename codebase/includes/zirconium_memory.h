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
    * Pauses all threads except the current one.
    */
    void __stdcall memory_pauseAllThreads();

    /*
    * Resumes all threads except the current one.
    */
    void __stdcall memory_resumeAllThreads();

}