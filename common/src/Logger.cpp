#include "Logger.h"

#include "Encoding.h"

#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

namespace {

void WriteLine(const wchar_t* message) {
    OutputDebugStringW(message != nullptr ? message : L"<null>");
    OutputDebugStringW(L"\r\n");
}

bool FormatWide(const wchar_t* format, va_list args, std::wstring& output) {
    output.clear();
    if (format == nullptr) {
        return false;
    }

    va_list countArgs;
    va_copy(countArgs, args);
    const int length = _vscwprintf(format, countArgs);
    va_end(countArgs);
    if (length < 0) {
        return false;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1);
    const int written = vswprintf_s(
        buffer.data(),
        buffer.size(),
        format,
        args
    );
    if (written < 0) {
        return false;
    }

    output.assign(buffer.data(), static_cast<size_t>(written));
    return true;
}

bool FormatUtf8(const char* format, va_list args, std::string& output) {
    output.clear();
    if (format == nullptr) {
        return false;
    }

    va_list countArgs;
    va_copy(countArgs, args);
    const int length = _vscprintf(format, countArgs);
    va_end(countArgs);
    if (length < 0) {
        return false;
    }

    std::vector<char> buffer(static_cast<size_t>(length) + 1);
    const int written = vsnprintf_s(
        buffer.data(),
        buffer.size(),
        _TRUNCATE,
        format,
        args
    );
    if (written < 0) {
        return false;
    }

    output.assign(buffer.data(), static_cast<size_t>(written));
    return true;
}

} // namespace

namespace Logger {

void Write(const wchar_t* format, ...) {
    va_list args;
    va_start(args, format);

    std::wstring message;
    const bool formatted = FormatWide(format, args, message);

    va_end(args);

    if (formatted) {
        WriteLine(message.c_str());
        return;
    }

    WriteLine(L"[Logger] invalid wide format message.");
}

void Write(const char* utf8Format, ...) {
    va_list args;
    va_start(args, utf8Format);

    std::string message;
    const bool formatted = FormatUtf8(utf8Format, args, message);

    va_end(args);

    if (formatted) {
        const wchar_t* wide = U82W(message.c_str());
        if (wide != nullptr) {
            WriteLine(wide);
            return;
        }

        WriteLine(L"[Logger] invalid UTF-8 message.");
        return;
    }

    WriteLine(L"[Logger] invalid UTF-8 format message.");
}

} // namespace Logger
