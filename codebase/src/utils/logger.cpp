#include "logger.h"

static void enableAnsiColors() {
    static bool enabled = false;
    if (enabled) return;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    enabled = true;
}

#define ANSI_RESET   "\033[0m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_RED     "\033[31m"
#define ANSI_YELLOW  "\033[33m"

void logPrint(const char* level, const char* file, int line, const char* fmt, ...) {
    enableAnsiColors();

    // extract filename only (strip directory path)
    const char* filename = strrchr(file, '\\');
    if (!filename) filename = strrchr(file, '/');
    filename = filename ? filename + 1 : file;

    // timestamp
    SYSTEMTIME st;
    GetLocalTime(&st);

    // format user message
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    // pick color for log level
    const char* levelColor = "";
    if (strcmp(level, "ERROR") == 0) levelColor = ANSI_RED;
    else if (strcmp(level, "DEBUG") == 0) levelColor = ANSI_YELLOW;

    std::cout << ANSI_MAGENTA << "zirconium" << ANSI_RESET << " ["
              << std::setfill('0') << std::setw(2) << st.wHour << ":"
              << std::setw(2) << st.wMinute << ":"
              << std::setw(2) << st.wSecond
              << "] [" << levelColor << level << ANSI_RESET << "] [" << filename << ":" << line << "]: "
              << msg << std::endl;
}

/*
* Converts an HRESULT error code to a human-readable std::string.
 *
 * @param hr The HRESULT error code to convert.
 * @return std::string representation of the error code.
*/
std::string __stdcall HresultErrorToString(HRESULT hr) {
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
	else {
		return "Unknown error";
	}
}

/*
* Converts a DWORD error code (such as from GetLastError()) to a human-readable std::string.
 *
 * @param dw The DWORD error code to convert.
 * @return std::string representation of the error code.
*/
std::string __stdcall dwordErrorToString(DWORD dw) {
    if (dw == 0)
        return "No error"; 

    LPVOID lpMsgBuf;
    DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    FormatMessage(
        dwFlags,
        nullptr,
        dw,
        0, // Default language
        (LPWSTR)&lpMsgBuf,
        0, nullptr);

    std::string message = (LPSTR)lpMsgBuf;

    LocalFree(lpMsgBuf);

    return message;
}

/*
*Converts a ULONG64 mask returned by GetProcessMitigation to a human - readable string.
* @param: mask ULONG64 mask to convert.
* @return std::string representation of the policies in place.
*/
std::string __stdcall mitMaskToString(ULONG64 & mask) {
    std::string str; str.append("\n");

    if (mask & (1ULL << 0))
        str.append("- DEP (Data Execution Prevention) is enabled.\n");
    else
        str.append("- DEP (Data Execution Prevention) is disabled.\n");

    if (mask & (1ULL << 1))
        str.append("- ASLR (Address Space Layout Randomization) is enabled.\n");
    else
        str.append("- ASLR (Address Space Layout Randomization) is disabled.\n");

    if (mask & (1ULL << 2))
        str.append("- Dynamic Code policy is enforced.\n");
    else
        str.append("- Dynamic Code policy is not enforced.\n");

    if (mask & (1ULL << 3))
        str.append("- Strict handle checks are enabled.\n");
    else
        str.append("- Strict handle checks are disabled.\n");

    if (mask & (1ULL << 4))
        str.append("- System call disable policy is enforced.\n");
    else
        str.append("- System call disable policy is not enforced.\n");

    if (mask & (1ULL << 5))
        str.append("- Mitigation for extension points is enforced.\n");
    else
        str.append("- Mitigation for extension points is not enforced.\n");

    if (mask & (1ULL << 6))
        str.append("- Prohibit remote dynamic code is enabled.\n");
    else
        str.append("- Prohibit remote dynamic code is disabled.\n");

    if (mask & (1ULL << 7))
        str.append("- Control Flow Guard (CFG) is enabled.\n");
    else
        str.append("- Control Flow Guard (CFG) is disabled.\n");

    if (mask & (1ULL << 8))
        str.append("- Arbitrary code guard is enabled.\n");
    else
        str.append("- Arbitrary code guard is disabled.\n");

    return str;
}

