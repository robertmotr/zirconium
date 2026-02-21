#pragma once

#include <string>
#include <streambuf>
#include <iostream>
#include <iomanip>
#include <cstdarg>
#include <cstring>
#include <sstream>
#include <Windows.h>

class DualOutputBuf : public std::streambuf {
public:
    DualOutputBuf(std::streambuf* consoleBuf, std::streambuf* fileBuf)
        : consoleBuf(consoleBuf), fileBuf(fileBuf) {}

protected:
    virtual int overflow(int c) override {
        if (c == EOF) return EOF;
        if (consoleBuf->sputc(c) == EOF || fileBuf->sputc(c) == EOF) return EOF;
        return c;
    }

    virtual int sync() override {
        if (consoleBuf->pubsync() == -1 || fileBuf->pubsync() == -1) return -1;
        return 0;
    }

private:
    std::streambuf* consoleBuf;
    std::streambuf* fileBuf;
};

std::string __stdcall HresultErrorToString(HRESULT hr);
std::string __stdcall dwordErrorToString(DWORD dw);
std::string __stdcall mitMaskToString(ULONG64& mask);

template<typename T>
std::string __stdcall toHex(T* ptr) {
    std::string str;
	str += "0x";

	std::stringstream stream;
	stream << std::hex << ptr;
	str += stream.str();
	return str;
}

template<typename T>
std::string __stdcall toHex(T var) {
	std::string str;
	str += "0x";

	std::stringstream stream;
	stream << std::hex << var;
	str += stream.str();
	return str;
}

// Log levels — higher number = more verbose
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_DEBUG 2

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

void logPrint(const char* level, const char* file, int line, const char* fmt, ...);

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) logPrint("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) logPrint("INFO", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) logPrint("DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif
