#pragma once

#include <string_view>

struct TranslationManagerConfig {
    bool filterChineseSourceWrites = false;
};

class TranslationManager {
public:
    TranslationManager() = delete;

    static void Configure(const TranslationManagerConfig& config);
    static void SetEnabled(bool enabled);
    static bool IsEnabled();
    static bool Initialize(const wchar_t* dictionaryName);
    static void Clear();

    static wchar_t* Translate(const wchar_t* sourceText, bool writeUntranslated);
    static char* Translate(const char* sourceText, bool writeUntranslated, unsigned int inputCodePage, unsigned int outputCodePage);
    static std::wstring_view Translate(std::wstring_view sourceText, bool writeUntranslated);
};
