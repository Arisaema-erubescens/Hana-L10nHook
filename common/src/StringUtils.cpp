#include "StringUtils.h"

namespace {

wchar_t HexDigit(unsigned int value) {
    return static_cast<wchar_t>(value < 10 ? L'0' + value : L'A' + (value - 10));
}

int HexValue(wchar_t ch) {
    if (ch >= L'0' && ch <= L'9') {
        return ch - L'0';
    }
    if (ch >= L'a' && ch <= L'f') {
        return ch - L'a' + 10;
    }
    if (ch >= L'A' && ch <= L'F') {
        return ch - L'A' + 10;
    }
    return -1;
}

bool ReadHex4(const std::wstring& text, size_t offset, wchar_t& value) {
    if (offset + 4 > text.size()) {
        return false;
    }

    unsigned int code = 0;
    for (size_t i = 0; i < 4; ++i) {
        const int digit = HexValue(text[offset + i]);
        if (digit < 0) {
            return false;
        }
        code = (code << 4) | static_cast<unsigned int>(digit);
    }

    value = static_cast<wchar_t>(code);
    return true;
}

} // namespace

namespace StringUtils {

std::wstring Escape(const std::wstring& text) {
    std::wstring escaped;
    escaped.reserve(text.size());

    for (wchar_t ch : text) {
        switch (ch) {
            case L'\\':
                escaped += L"\\\\";
                break;
            case L'\t':
                escaped += L"\\t";
                break;
            case L'\r':
                escaped += L"\\r";
                break;
            case L'\n':
                escaped += L"\\n";
                break;
            default:
                if (ch < 0x20) {
                    escaped += L"\\u";
                    escaped += HexDigit((ch >> 12) & 0xF);
                    escaped += HexDigit((ch >> 8) & 0xF);
                    escaped += HexDigit((ch >> 4) & 0xF);
                    escaped += HexDigit(ch & 0xF);
                } else {
                    escaped += ch;
                }
                break;
        }
    }

    return escaped;
}

std::wstring Unescape(const std::wstring& text) {
    std::wstring unescaped;
    unescaped.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        const wchar_t ch = text[i];
        if (ch != L'\\' || i + 1 >= text.size()) {
            unescaped += ch;
            continue;
        }

        const wchar_t next = text[++i];
        switch (next) {
            case L'\\':
                unescaped += L'\\';
                break;
            case L't':
                unescaped += L'\t';
                break;
            case L'r':
                unescaped += L'\r';
                break;
            case L'n':
                unescaped += L'\n';
                break;
            case L'u': {
                wchar_t value = 0;
                if (ReadHex4(text, i + 1, value)) {
                    unescaped += value;
                    i += 4;
                } else {
                    unescaped += L'\\';
                    unescaped += next;
                }
                break;
            }
            default:
                unescaped += L'\\';
                unescaped += next;
                break;
        }
    }

    return unescaped;
}

} // namespace StringUtils
