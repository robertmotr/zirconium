#pragma once

#include <string>
#include <iostream>
#include <Windows.h>
#include <d3d9.h>

/*
* Converts an HRESULT error code to a human-readable std::string.
 *
 * @param hr The HRESULT error code to convert.
 * @return std::string representation of the error code.
*/
std::string __stdcall HresultErrorToString(HRESULT hr);

/*
* Converts a DWORD error code (such as from GetLastError()) to a human-readable std::string.
 *
 * @param dw The DWORD error code to convert.
 * @return std::string representation of the error code.
*/
std::string __stdcall dwordErrorToString(DWORD dw);

/*
* Converts a ULONG64 mask returned by GetProcessMitigation to a human-readable string.
* @param: mask ULONG64 mask to convert.
* @return std::string representation of the policies in place.
*/
std::string __stdcall mitMaskToString(ULONG64& mask);

// Base case: only one argument left 
template<typename T>
void __stdcall logArgs(T&& t) {
    std::cout << std::forward<T>(t);
}

// Recursive case: more than one argument 
template<typename T, typename... Args>
void __stdcall logArgs(T&& t, Args&&... args) {
    std::cout << std::forward<T>(t) << " ";
    logArgs(std::forward<Args>(args)...);
}

// helper for log macro
template<typename... Args>
void __stdcall logHelper(const char* file, int line, Args&&... args) {
    std::cout << file << ":" << line << " ";
    logArgs(std::forward<Args>(args)...);
    std::cout << std::endl;
}

// Macro to print file, line, and any number of arguments
#define LOG(...) logHelper(__FILE__, __LINE__, __VA_ARGS__)