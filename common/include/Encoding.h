#pragma once

#include <string>

namespace Encoding {

const wchar_t* MultiByteToUtf16(unsigned int codePage, const char* text);
const char* Utf16ToMultiByte(unsigned int codePage, const wchar_t* text);
std::wstring MultiByteToUtf16(unsigned int codePage, const std::string& text);
std::string Utf16ToMultiByte(unsigned int codePage, const std::wstring& text);

const wchar_t* GbkToUtf16(const char* text);
const char* Utf16ToGbk(const wchar_t* text);
const wchar_t* Utf8ToUtf16(const char* text);
const char* Utf16ToUtf8(const wchar_t* text);
const char* GbkToUtf8(const char* text);
const char* Utf8ToGbk(const char* text);
std::wstring GbkToUtf16(const std::string& text);
std::string Utf16ToGbk(const std::wstring& text);
std::wstring Utf8ToUtf16(const std::string& text);
std::string Utf16ToUtf8(const std::wstring& text);
std::string GbkToUtf8(const std::string& text);
std::string Utf8ToGbk(const std::string& text);

} // namespace Encoding

// 返回线程局部环形缓冲指针，转换失败时返回 nullptr。
#define GBK2W(text) (::Encoding::GbkToUtf16(text))
#define W2GBK(text) (::Encoding::Utf16ToGbk(text))
#define U82W(text) (::Encoding::Utf8ToUtf16(text))
#define W2U8(text) (::Encoding::Utf16ToUtf8(text))
#define GBK2U8(text) (::Encoding::GbkToUtf8(text))
#define U82GBK(text) (::Encoding::Utf8ToGbk(text))
