#pragma once

#include <string>
#include <iostream>
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>

std::string __stdcall HResultToString(HRESULT hr);

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