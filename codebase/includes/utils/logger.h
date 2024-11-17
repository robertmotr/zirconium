#pragma once

#include <string>
#include <streambuf>
#include <iostream>
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

/*
* Converts a pointer to a string as a hex address.
* @param ptr The pointer to convert.
* @return std::string representation of the pointer as a hex address.
*/
template<typename T>
std::string __stdcall toHex(T* ptr) {
    std::string str;
	str += "0x";

	std::stringstream stream;
	stream << std::hex << ptr;
	str += stream.str();
	return str;
}

/*
* Converts a variable to a string as a hex address.
* @param var The variable to convert.
* @return std::string representation of the variable as a hex address.
*/
template<typename T>
std::string __stdcall toHex(T var) {
	std::string str;
	str += "0x";

	std::stringstream stream;
	stream << std::hex << var;
	str += stream.str();
	return str;
}

// Macro to print file, line, and any number of arguments
#define LOG(...) logHelper(__FILE__, __LINE__, __VA_ARGS__)