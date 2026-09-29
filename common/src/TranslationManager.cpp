#include "TranslationManager.h"

#include "Encoding.h"
#include "StringUtils.h"

#include <Windows.h>

#include <array>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <winnt.h>
#include <cwctype>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {

std::mutex g_dictionaryMutex;
std::unordered_map<std::wstring, std::wstring> g_dictionary;
std::unordered_map<std::wstring, std::wstring> g_reverseDictionary;
struct FallbackTranslation {
    std::wstring source;
    std::wstring translated;
    bool sourceHasMarkup = false;
};
std::unordered_map<std::wstring, FallbackTranslation> g_normalizedDictionary;
std::unordered_map<std::wstring, FallbackTranslation> g_plainDictionary;
bool g_translationEnabled = true;
TranslationManagerConfig g_config;
std::wstring g_dictionaryPath;

constexpr size_t kTranslationBufferSlotCount = 64;
constexpr wchar_t kWildcardToken[] = L"{*}";
constexpr size_t kWildcardTokenLength = 3;

thread_local std::array<std::wstring, kTranslationBufferSlotCount> g_translationBuffers;
thread_local size_t g_translationBufferIndex = 0;
thread_local std::array<std::string, kTranslationBufferSlotCount> g_multibyteTranslationBuffers;
thread_local size_t g_multibyteTranslationBufferIndex = 0;

struct WildcardEntry {
    std::wstring source;
    std::wstring translated;
    std::vector<std::wstring> sourceParts;
    bool translatedHasWildcard = false;
};

std::vector<WildcardEntry> g_wildcardDictionary;

std::wstring& NextTranslationBuffer() {
    std::wstring& buffer = g_translationBuffers[g_translationBufferIndex];
    g_translationBufferIndex = (g_translationBufferIndex + 1) % kTranslationBufferSlotCount;
    buffer.clear();
    return buffer;
}

std::string& NextMultibyteTranslationBuffer() {
    std::string& buffer = g_multibyteTranslationBuffers[g_multibyteTranslationBufferIndex];
    g_multibyteTranslationBufferIndex = (g_multibyteTranslationBufferIndex + 1) % kTranslationBufferSlotCount;
    buffer.clear();
    return buffer;
}

void ClearTranslationBuffers() {
    for (std::wstring& buffer : g_translationBuffers) {
        buffer.clear();
    }
    g_translationBufferIndex = 0;

    for (std::string& buffer : g_multibyteTranslationBuffers) {
        buffer.clear();
    }
    g_multibyteTranslationBufferIndex = 0;
}

bool Utf8ToWide(const std::string& utf8, std::wstring& wide) {
    if (utf8.empty()) {
        wide.clear();
        return true;
    }

    const int size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.data(),
        static_cast<int>(utf8.size()),
        nullptr,
        0
    );
    if (size <= 0) {
        return false;
    }

    wide.resize(static_cast<size_t>(size));
    const int written = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        utf8.data(),
        static_cast<int>(utf8.size()),
        wide.data(),
        size
    );
    return written == size;
}

bool WideToUtf8(const std::wstring& wide, std::string& utf8) {
    if (wide.empty()) {
        utf8.clear();
        return true;
    }

    const int size = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wide.data(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (size <= 0) {
        return false;
    }

    utf8.resize(static_cast<size_t>(size));
    const int written = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wide.data(),
        static_cast<int>(wide.size()),
        utf8.data(),
        size,
        nullptr,
        nullptr
    );
    return written == size;
}

void RemoveUtf8Bom(std::string& text) {
    constexpr unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == bom[0] &&
        static_cast<unsigned char>(text[1]) == bom[1] &&
        static_cast<unsigned char>(text[2]) == bom[2]) {
        text.erase(0, 3);
    }
}

void RemoveTrailingCarriageReturn(std::string& text) {
    if (!text.empty() && text.back() == '\r') {
        text.pop_back();
    }
}

bool HasTranslatedText(const std::wstring& sourceText, const std::wstring& translatedText) {
    return !translatedText.empty() && translatedText != sourceText;
}

bool HasMarkup(const std::wstring& text) {
    const size_t open = text.find(L'<');
    return open != std::wstring::npos && text.find(L'>', open + 1) != std::wstring::npos;
}

std::wstring StripMarkup(const std::wstring& text) {
    std::wstring result;
    result.reserve(text.size());

    bool inTag = false;
    for (size_t index = 0; index < text.size(); ++index) {
        const wchar_t ch = text[index];
        if (ch == L'<') {
            const size_t close = text.find(L'>', index + 1);
            if (close != std::wstring::npos) {
                const std::wstring tag = text.substr(index, close - index + 1);
                if (tag.size() >= 3 &&
                    (tag.compare(0, 3, L"<br") == 0 || tag.compare(0, 4, L"<BR") == 0)) {
                    result.push_back(L'\n');
                }
                index = close;
                continue;
            }
        }
        if (!inTag) {
            result.push_back(ch);
        }
    }
    return result;
}

std::wstring NormalizeLookupText(const std::wstring& text, bool stripMarkup) {
    const std::wstring input = stripMarkup ? StripMarkup(text) : text;
    std::wstring result;
    result.reserve(input.size());

    bool pendingSpace = false;
    for (const wchar_t ch : input) {
        if (iswspace(ch) != 0) {
            pendingSpace = !result.empty();
            continue;
        }
        if (pendingSpace) {
            result.push_back(L' ');
            pendingSpace = false;
        }
        result.push_back(ch);
    }

    while (!result.empty() && result.back() == L' ') {
        result.pop_back();
    }
    return result;
}

std::wstring AdaptFallbackTranslation(
    const std::wstring& source,
    const FallbackTranslation& entry
) {
    if (!entry.sourceHasMarkup || HasMarkup(source)) {
        return entry.translated;
    }
    return StripMarkup(entry.translated);
}

bool IsChineseCharacter(wchar_t ch) {
    return (ch >= 0x3400 && ch <= 0x4DBF) ||
           (ch >= 0x4E00 && ch <= 0x9FFF) ||
           (ch >= 0xF900 && ch <= 0xFAFF);
}

bool ContainsChineseText(const std::wstring& text) {
    for (const wchar_t ch : text) {
        if (IsChineseCharacter(ch)) {
            return true;
        }
    }
    return false;
}

bool CanWriteUntranslatedSource(const std::wstring& sourceText) {
    if (g_config.filterChineseSourceWrites && ContainsChineseText(sourceText)) {
        return false;
    }
    return true;
}

bool HasWildcard(const std::wstring& text) {
    return text.find(kWildcardToken) != std::wstring::npos;
}

std::vector<std::wstring> SplitWildcardPattern(const std::wstring& pattern) {
    std::vector<std::wstring> parts;
    size_t offset = 0;

    while (true) {
        const size_t token = pattern.find(kWildcardToken, offset);
        if (token == std::wstring::npos) {
            parts.push_back(pattern.substr(offset));
            return parts;
        }

        parts.push_back(pattern.substr(offset, token - offset));
        offset = token + kWildcardTokenLength;
    }
}

bool MatchWildcardPattern(
    const std::vector<std::wstring>& parts,
    const std::wstring& text,
    std::vector<std::wstring>& captures
) {
    captures.clear();
    if (parts.empty() || text.compare(0, parts.front().size(), parts.front()) != 0) {
        return false;
    }

    size_t textOffset = parts.front().size();
    const size_t wildcardCount = parts.size() - 1;

    for (size_t i = 0; i < wildcardCount; ++i) {
        const std::wstring& nextPart = parts[i + 1];

        if (i + 1 == wildcardCount) {
            if (nextPart.empty()) {
                captures.push_back(text.substr(textOffset));
                textOffset = text.size();
                continue;
            }

            if (text.size() < nextPart.size()) {
                return false;
            }

            const size_t suffixOffset = text.size() - nextPart.size();
            if (suffixOffset < textOffset ||
                text.compare(suffixOffset, nextPart.size(), nextPart) != 0) {
                return false;
            }

            captures.push_back(text.substr(textOffset, suffixOffset - textOffset));
            textOffset = text.size();
            continue;
        }

        if (nextPart.empty()) {
            captures.emplace_back();
            continue;
        }

        const size_t nextOffset = text.find(nextPart, textOffset);
        if (nextOffset == std::wstring::npos) {
            return false;
        }

        captures.push_back(text.substr(textOffset, nextOffset - textOffset));
        textOffset = nextOffset + nextPart.size();
    }

    return textOffset == text.size();
}

std::wstring ApplyWildcardCaptures(
    const std::wstring& translatedText,
    const std::vector<std::wstring>& captures
) {
    if (!HasWildcard(translatedText)) {
        return translatedText;
    }

    std::wstring result;
    size_t offset = 0;
    size_t captureIndex = 0;

    while (true) {
        const size_t token = translatedText.find(kWildcardToken, offset);
        if (token == std::wstring::npos) {
            result += translatedText.substr(offset);
            return result;
        }

        result += translatedText.substr(offset, token - offset);
        if (captureIndex < captures.size()) {
            result += captures[captureIndex];
        }

        ++captureIndex;
        offset = token + kWildcardTokenLength;
    }
}

bool NeedsLeadingNewline(const wchar_t* filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size <= 0) {
        return false;
    }

    file.seekg(size - 1, std::ios::beg);
    char last = '\0';
    file.get(last);
    return last != '\n';
}

bool AppendUntranslatedLocked(const std::wstring& sourceText) {
    if (sourceText.empty() || g_dictionaryPath.empty()) {
        return false;
    }

    std::string sourceUtf8;
    const std::wstring escapedSource = StringUtils::Escape(sourceText);
    if (!WideToUtf8(escapedSource, sourceUtf8)) {
        return false;
    }

    const bool needsLeadingNewline = NeedsLeadingNewline(g_dictionaryPath.c_str());
    std::ofstream file(g_dictionaryPath.c_str(), std::ios::binary | std::ios::app);
    if (!file) {
        return false;
    }

    if (needsLeadingNewline) {
        file.put('\n');
    }
    file.write(sourceUtf8.data(), static_cast<std::streamsize>(sourceUtf8.size()));
    file.put('\t');
    file.write(sourceUtf8.data(), static_cast<std::streamsize>(sourceUtf8.size()));
    file.write("\r\n", 2);
    return static_cast<bool>(file);
}

bool WriteDictionaryFile(
    const wchar_t* filePath,
    const std::vector<std::wstring>& order,
    const std::unordered_map<std::wstring, std::wstring>& dictionary
) {
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }

    for (const std::wstring& source : order) {
        const auto it = dictionary.find(source);
        if (it == dictionary.end()) {
            continue;
        }

        std::string sourceUtf8;
        std::string translatedUtf8;
        if (!WideToUtf8(StringUtils::Escape(source), sourceUtf8) ||
            !WideToUtf8(StringUtils::Escape(it->second), translatedUtf8)) {
            return false;
        }

        file.write(sourceUtf8.data(), static_cast<std::streamsize>(sourceUtf8.size()));
        file.put('\t');
        file.write(translatedUtf8.data(), static_cast<std::streamsize>(translatedUtf8.size()));
        file.write("\r\n", 2);
    }

    return static_cast<bool>(file);
}

bool BuildDictionaryPath(const wchar_t* dictionaryName, std::wstring& dictionaryPath) {
    dictionaryPath.clear();
    if (dictionaryName == nullptr || dictionaryName[0] == L'\0') {
        return false;
    }

    wchar_t modulePath[MAX_PATH] = {};
    HMODULE selfModule =
        reinterpret_cast<HMODULE>(&__ImageBase);

    if (selfModule == nullptr) {
        return false;
    }

    const DWORD length = GetModuleFileNameW(selfModule, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return false;
    }

    dictionaryPath.assign(modulePath, length);
    const size_t slash = dictionaryPath.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        dictionaryPath.resize(slash + 1);
    } else {
        dictionaryPath.clear();
    }
    dictionaryPath += dictionaryName;
    return true;
}

bool EnsureParentDirectoryExists(const std::wstring& filePath) {
    const size_t slash = filePath.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return true;
    }

    const std::wstring directory = filePath.substr(0, slash);
    if (directory.empty() || directory.size() >= MAX_PATH) {
        return !directory.empty();
    }

    wchar_t normalized[MAX_PATH] = {};
    wcscpy_s(normalized, directory.c_str());
    for (wchar_t* p = normalized; *p; ++p) {
        if (*p == L'/') {
            *p = L'\\';
        }
    }

    for (wchar_t* p = normalized; *p; ++p) {
        if (*p != L'\\') {
            continue;
        }

        if (p == normalized || *(p - 1) == L':') {
            continue;
        }

        *p = L'\0';
        CreateDirectoryW(normalized, nullptr);
        *p = L'\\';
    }

    return CreateDirectoryW(normalized, nullptr) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
}

} // namespace

void TranslationManager::Configure(const TranslationManagerConfig& config) {
    std::lock_guard<std::mutex> lock(g_dictionaryMutex);
    g_config = config;
}

void TranslationManager::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(g_dictionaryMutex);
    g_translationEnabled = enabled;
    ClearTranslationBuffers();
}

bool TranslationManager::IsEnabled() {
    std::lock_guard<std::mutex> lock(g_dictionaryMutex);
    return g_translationEnabled;
}

bool TranslationManager::Initialize(const wchar_t* dictionaryName) {
    std::wstring dictionaryPath;
    if (!BuildDictionaryPath(dictionaryName, dictionaryPath)) {
        return false;
    }

    std::ifstream file(dictionaryPath.c_str(), std::ios::binary);
    if (!file) {
        return false;
    }

    std::unordered_map<std::wstring, std::wstring> loaded;
    std::vector<std::wstring> order;
    std::string line;
    bool isFirstLine = true;

    while (std::getline(file, line)) {
        RemoveTrailingCarriageReturn(line);
        if (isFirstLine) {
            RemoveUtf8Bom(line);
            isFirstLine = false;
        }
        if (line.empty()) {
            continue;
        }

        const size_t tab = line.find('\t');
        if (tab == std::string::npos) {
            continue;
        }

        std::wstring source;
        std::wstring translated;
        if (!Utf8ToWide(line.substr(0, tab), source) ||
            !Utf8ToWide(line.substr(tab + 1), translated)) {
            return false;
        }

        source = StringUtils::Unescape(source);
        translated = StringUtils::Unescape(translated);
        if (source.empty()) {
            continue;
        }

        const auto existing = loaded.find(source);
        if (existing == loaded.end()) {
            loaded.emplace(source, translated);
            order.push_back(source);
        } else if (!HasTranslatedText(source, existing->second) &&
                   HasTranslatedText(source, translated)) {
            existing->second = translated;
        }
    }

    std::unordered_map<std::wstring, std::wstring> reverseDictionary;
    for (const std::wstring& source : order) {
        const auto it = loaded.find(source);
        if (it != loaded.end() && HasTranslatedText(source, it->second) &&
            reverseDictionary.find(it->second) == reverseDictionary.end()) {
            reverseDictionary.emplace(it->second, source);
        }
    }

    std::unordered_map<std::wstring, FallbackTranslation> normalizedDictionary;
    std::unordered_map<std::wstring, FallbackTranslation> plainDictionary;
    for (const std::wstring& source : order) {
        const auto it = loaded.find(source);
        if (it == loaded.end() || !HasTranslatedText(source, it->second)) {
            continue;
        }

        const FallbackTranslation entry{source, it->second, HasMarkup(source)};
        const std::wstring normalized = NormalizeLookupText(source, false);
        if (!normalized.empty() && normalizedDictionary.find(normalized) == normalizedDictionary.end()) {
            normalizedDictionary.emplace(normalized, entry);
        }

        const std::wstring plain = NormalizeLookupText(source, true);
        if (!plain.empty() && plainDictionary.find(plain) == plainDictionary.end()) {
            plainDictionary.emplace(plain, entry);
        }
    }

    std::vector<WildcardEntry> wildcardDictionary;
    for (const std::wstring& source : order) {
        const auto it = loaded.find(source);
        if (it != loaded.end() && HasWildcard(source) &&
            HasTranslatedText(source, it->second)) {
            wildcardDictionary.push_back({
                source,
                it->second,
                SplitWildcardPattern(source),
                HasWildcard(it->second)
            });
        }
    }

    std::lock_guard<std::mutex> lock(g_dictionaryMutex);
    g_dictionary = std::move(loaded);
    g_reverseDictionary = std::move(reverseDictionary);
    g_normalizedDictionary = std::move(normalizedDictionary);
    g_plainDictionary = std::move(plainDictionary);
    g_wildcardDictionary = std::move(wildcardDictionary);
    g_translationEnabled = true;
    g_dictionaryPath.clear();
    ClearTranslationBuffers();
    return true;
}

void TranslationManager::Clear() {
    std::lock_guard<std::mutex> lock(g_dictionaryMutex);
    g_dictionary.clear();
    g_reverseDictionary.clear();
    g_normalizedDictionary.clear();
    g_plainDictionary.clear();
    g_wildcardDictionary.clear();
    g_translationEnabled = true;
    g_dictionaryPath.clear();
    ClearTranslationBuffers();
}

wchar_t* TranslationManager::Translate(const wchar_t* sourceText, bool writeUntranslated) {
    if (sourceText == nullptr) {
        return nullptr;
    }

    const std::wstring_view translated = Translate(std::wstring_view(sourceText), writeUntranslated);
    return translated.empty() ? nullptr : const_cast<wchar_t*>(translated.data());
}

char* TranslationManager::Translate(
    const char* sourceText,
    bool writeUntranslated,
    unsigned int inputCodePage,
    unsigned int outputCodePage
) {
    if (sourceText == nullptr) {
        return nullptr;
    }

    const wchar_t* source = Encoding::MultiByteToUtf16(inputCodePage, sourceText);
    if (source == nullptr) {
        return nullptr;
    }

    const std::wstring_view translated = Translate(std::wstring_view(source), writeUntranslated);
    if (translated.empty()) {
        return nullptr;
    }

    std::wstring translatedText(translated.data(), translated.size());
    const char* converted = Encoding::Utf16ToMultiByte(outputCodePage, translatedText.c_str());
    if (converted == nullptr) {
        return nullptr;
    }

    std::string& buffer = NextMultibyteTranslationBuffer();
    buffer = converted;
    return buffer.data();
}

std::wstring_view TranslationManager::Translate(std::wstring_view sourceText, bool writeUntranslated) {
    if (sourceText.empty()) {
        return {};
    }

    const std::wstring source(sourceText);
    std::lock_guard<std::mutex> lock(g_dictionaryMutex);
    const bool enabled = g_translationEnabled;
    const auto& dictionary = enabled ? g_dictionary : g_reverseDictionary;
    const auto it = dictionary.find(source);
    if (it != dictionary.end() && HasTranslatedText(source, it->second)) {
        std::wstring& buffer = NextTranslationBuffer();
        buffer = it->second;
        return buffer;
    }

    size_t left = 0;
    size_t right = source.size();

    while (left < right && iswspace(source[left]) != 0) {
        ++left;
    }

    while (right > left && iswspace(source[right - 1]) != 0) {
        --right;
    }

    if (left != 0 || right != source.size()) {
        const std::wstring trimmedSource = source.substr(left, right - left);
        const auto trimmedIt = dictionary.find(trimmedSource);

        if (trimmedIt != dictionary.end() &&
            HasTranslatedText(trimmedSource, trimmedIt->second)) {

            std::wstring& buffer = NextTranslationBuffer();
            buffer.assign(source.substr(0, left));
            buffer += trimmedIt->second;
            buffer.append(source.substr(right));
            return buffer;
        }
    }

    const std::wstring normalizedSource = NormalizeLookupText(source, false);
    const auto normalizedIt = g_normalizedDictionary.find(normalizedSource);
    if (normalizedIt != g_normalizedDictionary.end()) {
        std::wstring& buffer = NextTranslationBuffer();
        buffer = AdaptFallbackTranslation(source, normalizedIt->second);
        return buffer;
    }

    const std::wstring plainSource = NormalizeLookupText(source, true);
    const auto plainIt = g_plainDictionary.find(plainSource);
    if (plainIt != g_plainDictionary.end()) {
        std::wstring& buffer = NextTranslationBuffer();
        buffer = AdaptFallbackTranslation(source, plainIt->second);
        return buffer;
    }

    if (enabled) {
        std::vector<std::wstring> captures;
        for (const auto& wildcardEntry : g_wildcardDictionary) {
            if (MatchWildcardPattern(wildcardEntry.sourceParts, source, captures)) {
                std::wstring& buffer = NextTranslationBuffer();
                buffer = wildcardEntry.translatedHasWildcard
                    ? ApplyWildcardCaptures(wildcardEntry.translated, captures)
                    : wildcardEntry.translated;
                return buffer;
            }
        }

        if (it == dictionary.end() && writeUntranslated) {
            if (CanWriteUntranslatedSource(source) && AppendUntranslatedLocked(source)) {
                g_dictionary.emplace(source, source);
            }
        }
    }

    return {};
}
