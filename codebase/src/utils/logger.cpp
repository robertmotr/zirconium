#include "logger.h"
#include <string>
#include <d3d9.h>
#include <Windows.h>

// Base case: only one argument left 
template<typename T>
void logArgs(T&& t) {
    std::cout << std::forward<T>(t);
}

// Recursive case: more than one argument 
template<typename T, typename... Args>
void logArgs(T&& t, Args&&... args) {
    std::cout << std::forward<T>(t) << " ";
    logArgs(std::forward<Args>(args)...);
}

// helper for log macro
template<typename... Args>
void logHelper(const char* file, int line, Args&&... args) {
    std::cout << file << ":" << line << " ";
    logArgs(std::forward<Args>(args)...);
    std::cout << std::endl;
}

/*
* Converts an HRESULT error code to a human-readable std::string.
 *
 * @param hr The HRESULT error code to convert.
 * @return std::string representation of the error code.
*/
std::string HResultToString(HRESULT hr) {
    char* errorMsg = nullptr;

    // Use FormatMessage to get a standard error message
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errorMsg,
        0,
        nullptr
    );

    if (errorMsg) {
        std::string message(errorMsg);
        LocalFree(errorMsg);
        return message;
    }

    // If no system error message was found, check for DirectX-specific errors
    switch (hr) {
    case D3DERR_DEVICELOST:
        return "D3DERR_DEVICELOST: The device has been lost.";
    case D3DERR_DEVICENOTRESET:
        return "D3DERR_DEVICENOTRESET: The device cannot be reset.";
    case D3DERR_DRIVERINTERNALERROR:
        return "D3DERR_DRIVERINTERNALERROR: Internal driver error.";
    case D3DERR_INVALIDCALL:
        return "D3DERR_INVALIDCALL: Invalid function call.";
    case D3DERR_OUTOFVIDEOMEMORY:
        return "D3DERR_OUTOFVIDEOMEMORY: Out of video memory.";
    default:
        return "Unknown error.";
    }
}
