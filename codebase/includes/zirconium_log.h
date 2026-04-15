#pragma once

#include <string>
#include <iostream>
#include <iomanip>
#include <cstdarg>
#include <cstring>
#include <sstream>
#include <Windows.h>

void logPrint(const char* level, const char* file, int line, const char* fmt, ...);

#ifdef _DEBUG
#define LOG_ERROR(fmt, ...) logPrint("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  logPrint("INFO",  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) logPrint("DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) ((void)0)
#define LOG_INFO(fmt, ...)  ((void)0)
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif