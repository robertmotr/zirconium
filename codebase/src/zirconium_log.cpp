#include "pch.h"

#include "zirconium_log.h"

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

