#include "AddressResolver.h"

#include "Encoding.h"
#include "Logger.h"

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct PatternByte {
    unsigned char value = 0;
    bool wildcard = false;
};

struct ReadableRange {
    const unsigned char* begin = nullptr;
    size_t size = 0;
};

std::wstring FunctionNameToWide(const char* functionName) {
    if (functionName == nullptr || functionName[0] == '\0') {
        return L"<null>";
    }

    const wchar_t* converted = GBK2W(functionName);
    if (converted != nullptr) {
        return converted;
    }

    return L"<invalid>";
}

std::wstring NarrowToWideForLog(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return L"<null>";
    }

    std::wstring wide;
    while (*text != '\0') {
        wide += static_cast<unsigned char>(*text);
        ++text;
    }
    return wide;
}

int HexValue(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

bool IsSpace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

bool ParsePattern(const char* pattern, std::vector<PatternByte>& bytes) {
    bytes.clear();
    if (pattern == nullptr) {
        return false;
    }

    bytes.reserve(std::strlen(pattern) / 3 + 1);
    const char* cursor = pattern;
    while (*cursor != '\0') {
        while (IsSpace(*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }

        const char* tokenStart = cursor;
        while (*cursor != '\0' && !IsSpace(*cursor)) {
            ++cursor;
        }

        const size_t tokenLength = static_cast<size_t>(cursor - tokenStart);
        if (tokenLength == 2 && tokenStart[0] == '?' && tokenStart[1] == '?') {
            bytes.push_back({0, true});
            continue;
        }

        if (tokenLength != 2) {
            return false;
        }

        const int high = HexValue(tokenStart[0]);
        const int low = HexValue(tokenStart[1]);
        if (high < 0 || low < 0) {
            return false;
        }

        bytes.push_back({
            static_cast<unsigned char>((high << 4) | low),
            false
        });
    }

    return !bytes.empty();
}

HMODULE ResolveModule(const wchar_t* moduleName, const wchar_t* operationName) {
    if (moduleName == nullptr || moduleName[0] == L'\0') {
        Logger::Write(L"[AddressResolver] invalid module name. operation=%s", operationName);
        return nullptr;
    }

    HMODULE module = GetModuleHandleW(moduleName);
    if (module == nullptr) {
        module = LoadLibraryW(moduleName);
        if (module == nullptr) {
            Logger::Write(
                L"[AddressResolver] LoadLibrary failed. operation=%s module=%s address=%p error=%lu",
                operationName,
                moduleName,
                nullptr,
                GetLastError()
            );
            return nullptr;
        }
    }

    return module;
}

size_t GetModuleImageSize(HMODULE module) {
    if (module == nullptr) {
        return 0;
    }

    const auto base = reinterpret_cast<const unsigned char*>(module);
    const auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    if (dosHeader->e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) ||
        dosHeader->e_lfanew > 0x100000) {
        return 0;
    }

    const auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    return static_cast<size_t>(ntHeaders->OptionalHeader.SizeOfImage);
}

bool MatchesPatternAt(const unsigned char* address, const std::vector<PatternByte>& pattern) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (!pattern[i].wildcard && address[i] != pattern[i].value) {
            return false;
        }
    }
    return true;
}

size_t FindRequiredPatternByte(const std::vector<PatternByte>& pattern) {
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (!pattern[i].wildcard) {
            return i;
        }
    }
    return pattern.size();
}

bool IsReadableProtect(DWORD protect) {
    if ((protect & PAGE_GUARD) != 0 || (protect & PAGE_NOACCESS) != 0) {
        return false;
    }

    protect &= 0xFF;
    return protect == PAGE_READONLY ||
           protect == PAGE_READWRITE ||
           protect == PAGE_WRITECOPY ||
           protect == PAGE_EXECUTE_READ ||
           protect == PAGE_EXECUTE_READWRITE ||
           protect == PAGE_EXECUTE_WRITECOPY;
}

void AddReadableRange(
    std::vector<ReadableRange>& ranges,
    const unsigned char* begin,
    size_t size
) {
    if (size == 0) {
        return;
    }

    if (!ranges.empty()) {
        ReadableRange& last = ranges.back();
        if (last.begin + last.size == begin) {
            last.size += size;
            return;
        }
    }

    ranges.push_back({begin, size});
}

std::vector<ReadableRange> GetReadableRanges(HMODULE module, size_t imageSize) {
    std::vector<ReadableRange> ranges;
    if (module == nullptr || imageSize == 0) {
        return ranges;
    }

    const auto imageBegin = reinterpret_cast<const unsigned char*>(module);
    const unsigned char* const imageEnd = imageBegin + imageSize;
    const unsigned char* cursor = imageBegin;

    while (cursor < imageEnd) {
        MEMORY_BASIC_INFORMATION mbi = {};
        const SIZE_T queried = VirtualQuery(cursor, &mbi, sizeof(mbi));
        if (queried == 0) {
            break;
        }

        const auto regionBegin = reinterpret_cast<const unsigned char*>(mbi.BaseAddress);
        const unsigned char* const regionEnd = regionBegin + mbi.RegionSize;
        const unsigned char* const readableBegin = cursor > regionBegin ? cursor : regionBegin;
        const unsigned char* const readableEnd = regionEnd < imageEnd ? regionEnd : imageEnd;

        if (mbi.State == MEM_COMMIT &&
            IsReadableProtect(mbi.Protect) &&
            readableBegin < readableEnd) {
            AddReadableRange(
                ranges,
                readableBegin,
                static_cast<size_t>(readableEnd - readableBegin)
            );
        }

        if (regionEnd <= cursor) {
            break;
        }
        cursor = regionEnd;
    }

    return ranges;
}

} // namespace

namespace AddressResolver {

void* GetFunctionAddress(const wchar_t* moduleName, const char* functionName) {
    if (moduleName == nullptr || moduleName[0] == L'\0' ||
        functionName == nullptr || functionName[0] == '\0') {
        Logger::Write(L"[AddressResolver] invalid module or function name.");
        return nullptr;
    }

    const std::wstring functionNameWide = FunctionNameToWide(functionName);

    HMODULE module = ResolveModule(moduleName, L"GetFunctionAddress");
    if (module == nullptr) {
        Logger::Write(
            L"[AddressResolver] resolve module failed. module=%s function=%s address=%p",
            moduleName,
            functionNameWide.c_str(),
            nullptr
        );
        return nullptr;
    }

    void* address = reinterpret_cast<void*>(GetProcAddress(module, functionName));
    if (address == nullptr) {
        Logger::Write(
            L"[AddressResolver] GetProcAddress failed. module=%s function=%s address=%p error=%lu",
            moduleName,
            functionNameWide.c_str(),
            nullptr,
            GetLastError()
        );
        return nullptr;
    }

    Logger::Write(
        L"[AddressResolver] resolved. module=%s function=%s address=%p error=%lu",
        moduleName,
        functionNameWide.c_str(),
        address,
        0
    );
    return address;
}

void* FindPattern(const wchar_t* moduleName, const char* pattern) {
    std::vector<PatternByte> patternBytes;
    if (!ParsePattern(pattern, patternBytes)) {
        Logger::Write(
            L"[AddressResolver] invalid pattern. module=%s pattern=%s address=%p",
            moduleName != nullptr ? moduleName : L"<null>",
            NarrowToWideForLog(pattern).c_str(),
            nullptr
        );
        return nullptr;
    }

    HMODULE module = ResolveModule(moduleName, L"FindPattern");
    if (module == nullptr) {
        return nullptr;
    }

    const size_t imageSize = GetModuleImageSize(module);
    if (imageSize == 0 || imageSize < patternBytes.size()) {
        Logger::Write(
            L"[AddressResolver] invalid image size. module=%s pattern=%s imageSize=%llu address=%p",
            moduleName,
            NarrowToWideForLog(pattern).c_str(),
            static_cast<unsigned long long>(imageSize),
            nullptr
        );
        return nullptr;
    }

    const std::vector<ReadableRange> ranges = GetReadableRanges(module, imageSize);
    const size_t requiredByteIndex = FindRequiredPatternByte(patternBytes);
    for (const ReadableRange& range : ranges) {
        if (range.size < patternBytes.size()) {
            continue;
        }

        const size_t lastOffset = range.size - patternBytes.size();
        for (size_t offset = 0; offset <= lastOffset; ++offset) {
            const unsigned char* address = range.begin + offset;
            if (requiredByteIndex < patternBytes.size() &&
                address[requiredByteIndex] != patternBytes[requiredByteIndex].value) {
                continue;
            }

            if (MatchesPatternAt(address, patternBytes)) {
                void* result = const_cast<unsigned char*>(address);
                Logger::Write(
                    L"[AddressResolver] pattern resolved. module=%s pattern=%s address=%p error=%lu",
                    moduleName,
                    NarrowToWideForLog(pattern).c_str(),
                    result,
                    0
                );
                return result;
            }
        }
    }

    Logger::Write(
        L"[AddressResolver] pattern not found. module=%s pattern=%s address=%p error=%lu",
        moduleName,
        NarrowToWideForLog(pattern).c_str(),
        nullptr,
        0
    );
    return nullptr;
}

} // namespace AddressResolver
