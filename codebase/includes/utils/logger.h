#pragma once

#include "pch.h"

std::string HResultToString(HRESULT hr);

// Base case: only one argument left 
template<typename T>
void logArgs(T&& t);

// Recursive case: more than one argument 
template<typename T, typename... Args>
void logArgs(T&& t, Args&&... args);

template<typename... Args>
void logHelper(const char* file, int line, Args&&... args);

// Macro to print file, line, and any number of arguments
#define LOG(...) logHelper(__FILE__, __LINE__, __VA_ARGS__)
