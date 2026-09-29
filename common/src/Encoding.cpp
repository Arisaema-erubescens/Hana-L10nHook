#include "Encoding.h"

#include <Windows.h>

#include <array>
#include <cstring>
#include <limits>
#include <string>

namespace {

constexpr unsigned int GbkCodePage = 936;
constexpr std::size_t kConversionBufferCount = 64;

thread_local std::array<std::wstring, kConversionBufferCount> g_wideBuffers;
thread_local std::size_t g_wideBufferIndex = 0;
thread_local std::array<std::string, kConversionBufferCount> g_multibyteBuffers;
thread_local std::size_t g_multibyteBufferIndex = 0;

DWORD MultiByteFlags(unsigned int codePage) {
    return codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
}

DWORD Utf16Flags(unsigned int codePage) {
    return codePage == CP_UTF8 ? WC_ERR_INVALID_CHARS : 0;
}

std::wstring& NextWideBuffer() {
    std::wstring& buffer = g_wideBuffers[g_wideBufferIndex];
    g_wideBufferIndex = (g_wideBufferIndex + 1) % kConversionBufferCount;
    buffer.clear();
    return buffer;
}

std::string& NextMultibyteBuffer() {
    std::string& buffer = g_multibyteBuffers[g_multibyteBufferIndex];
    g_multibyteBufferIndex = (g_multibyteBufferIndex + 1) % kConversionBufferCount;
    buffer.clear();
    return buffer;
}

bool TryMultiByteToUtf16(unsigned int codePage, const char* text, std::wstring& output) {
    output.clear();
    if (text == nullptr) {
        return false;
    }

    const size_t rawLength = std::strlen(text);
    if (rawLength > static_cast<size_t>((std::numeric_limits<int>::max)())) {
        return false;
    }

    const int length = static_cast<int>(rawLength);
    if (length == 0) {
        return true;
    }

    const int size = MultiByteToWideChar(
        codePage,
        MultiByteFlags(codePage),
        text,
        length,
        nullptr,
        0
    );
    if (size <= 0) {
        return false;
    }

    output.resize(static_cast<size_t>(size));
    const int written = MultiByteToWideChar(
        codePage,
        MultiByteFlags(codePage),
        text,
        length,
        output.data(),
        size
    );
    return written == size;
}

bool TryUtf16ToMultiByte(unsigned int codePage, const wchar_t* text, std::string& output) {
    output.clear();
    if (text == nullptr) {
        return false;
    }

    const size_t rawLength = wcslen(text);
    if (rawLength > static_cast<size_t>((std::numeric_limits<int>::max)())) {
        return false;
    }

    const int length = static_cast<int>(rawLength);
    if (length == 0) {
        return true;
    }

    BOOL usedDefaultChar = FALSE;
    BOOL* usedDefaultCharPtr = codePage == CP_UTF8 ? nullptr : &usedDefaultChar;
    const int size = WideCharToMultiByte(
        codePage,
        Utf16Flags(codePage),
        text,
        length,
        nullptr,
        0,
        nullptr,
        usedDefaultCharPtr
    );
    if (size <= 0) {
        return false;
    }

    output.resize(static_cast<size_t>(size));
    usedDefaultChar = FALSE;
    const int written = WideCharToMultiByte(
        codePage,
        Utf16Flags(codePage),
        text,
        length,
        output.data(),
        size,
        nullptr,
        usedDefaultCharPtr
    );
    return written == size && !usedDefaultChar;
}

} // namespace

namespace Encoding {

const wchar_t* MultiByteToUtf16(unsigned int codePage, const char* text) {
    std::wstring& output = NextWideBuffer();
    return TryMultiByteToUtf16(codePage, text, output) ? output.c_str() : nullptr;
}

const char* Utf16ToMultiByte(unsigned int codePage, const wchar_t* text) {
    std::string& output = NextMultibyteBuffer();
    return TryUtf16ToMultiByte(codePage, text, output) ? output.c_str() : nullptr;
}

std::wstring MultiByteToUtf16(unsigned int codePage, const std::string& text) {
    const wchar_t* converted = MultiByteToUtf16(codePage, text.c_str());
    if (converted == nullptr) {
        return {};
    }
    return std::wstring(converted);
}

std::string Utf16ToMultiByte(unsigned int codePage, const std::wstring& text) {
    const char* converted = Utf16ToMultiByte(codePage, text.c_str());
    if (converted == nullptr) {
        return {};
    }
    return std::string(converted);
}

const wchar_t* GbkToUtf16(const char* text) {
    return MultiByteToUtf16(GbkCodePage, text);
}

const char* Utf16ToGbk(const wchar_t* text) {
    return Utf16ToMultiByte(GbkCodePage, text);
}

const wchar_t* Utf8ToUtf16(const char* text) {
    return MultiByteToUtf16(CP_UTF8, text);
}

const char* Utf16ToUtf8(const wchar_t* text) {
    return Utf16ToMultiByte(CP_UTF8, text);
}

const char* GbkToUtf8(const char* text) {
    const wchar_t* utf16 = GbkToUtf16(text);
    return utf16 != nullptr ? Utf16ToUtf8(utf16) : nullptr;
}

const char* Utf8ToGbk(const char* text) {
    const wchar_t* utf16 = Utf8ToUtf16(text);
    return utf16 != nullptr ? Utf16ToGbk(utf16) : nullptr;
}

std::wstring GbkToUtf16(const std::string& text) {
    return MultiByteToUtf16(GbkCodePage, text);
}

std::string Utf16ToGbk(const std::wstring& text) {
    return Utf16ToMultiByte(GbkCodePage, text);
}

std::wstring Utf8ToUtf16(const std::string& text) {
    return MultiByteToUtf16(CP_UTF8, text);
}

std::string Utf16ToUtf8(const std::wstring& text) {
    return Utf16ToMultiByte(CP_UTF8, text);
}

std::string GbkToUtf8(const std::string& text) {
    const char* converted = GbkToUtf8(text.c_str());
    if (converted == nullptr) {
        return {};
    }
    return std::string(converted);
}

std::string Utf8ToGbk(const std::string& text) {
    const char* converted = Utf8ToGbk(text.c_str());
    if (converted == nullptr) {
        return {};
    }
    return std::string(converted);
}

} // namespace Encoding
