#include "Win32GdiHook.h"

#include "DetoursHook.h"
#include "Encoding.h"
#include "Logger.h"
#include "TranslationManager.h"

#include <Windows.h>
#include <uxtheme.h>

#include <cstddef>
#include <cwchar>
#include <cstring>
#include <string>
#include <string_view>

namespace {

using TextOutW_t = BOOL (WINAPI*)(HDC, int, int, LPCWSTR, int);
using TextOutA_t = BOOL (WINAPI*)(HDC, int, int, LPCSTR, int);
using ExtTextOutW_t = BOOL (WINAPI*)(HDC, int, int, UINT, const RECT*, LPCWSTR, UINT, const INT*);
using ExtTextOutA_t = BOOL (WINAPI*)(HDC, int, int, UINT, const RECT*, LPCSTR, UINT, const INT*);
using DrawTextW_t = int (WINAPI*)(HDC, LPCWSTR, int, LPRECT, UINT);
using DrawTextA_t = int (WINAPI*)(HDC, LPCSTR, int, LPRECT, UINT);
using DrawTextExW_t = int (WINAPI*)(HDC, LPWSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS);
using DrawTextExA_t = int (WINAPI*)(HDC, LPSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS);
using DrawThemeText_t = decltype(&DrawThemeText);
using DrawThemeTextEx_t = decltype(&DrawThemeTextEx);
using SetWindowTextW_t = BOOL (WINAPI*)(HWND, LPCWSTR);
using SetWindowTextA_t = BOOL (WINAPI*)(HWND, LPCSTR);
using SetDlgItemTextW_t = BOOL (WINAPI*)(HWND, int, LPCWSTR);
using SetDlgItemTextA_t = BOOL (WINAPI*)(HWND, int, LPCSTR);

TextOutW_t g_originalTextOutW = nullptr;
TextOutA_t g_originalTextOutA = nullptr;
ExtTextOutW_t g_originalExtTextOutW = nullptr;
ExtTextOutA_t g_originalExtTextOutA = nullptr;
DrawTextW_t g_originalDrawTextW = nullptr;
DrawTextA_t g_originalDrawTextA = nullptr;
DrawTextExW_t g_originalDrawTextExW = nullptr;
DrawTextExA_t g_originalDrawTextExA = nullptr;
DrawThemeText_t g_originalDrawThemeText = nullptr;
DrawThemeTextEx_t g_originalDrawThemeTextEx = nullptr;
SetWindowTextW_t g_originalSetWindowTextW = nullptr;
SetWindowTextA_t g_originalSetWindowTextA = nullptr;
SetDlgItemTextW_t g_originalSetDlgItemTextW = nullptr;
SetDlgItemTextA_t g_originalSetDlgItemTextA = nullptr;

DetoursHook g_detours;

bool IsCommandSurfaceText(HDC hdc, std::wstring_view text) {
    if (hdc == nullptr || (text != L"Object" && text != L"Import")) {
        return false;
    }
#if defined(_WIN64)
    const ULONG_PTR value = reinterpret_cast<ULONG_PTR>(hdc);
    return (value & (static_cast<ULONG_PTR>(1) << 63)) != 0;
#else
    return false;
#endif
}
int EffectiveLength(LPCWSTR text, int length) {
    if (text == nullptr) return 0;
    return length < 0 ? static_cast<int>(wcslen(text)) : length;
}

int EffectiveLength(LPCSTR text, int length) {
    if (text == nullptr) return 0;
    return length < 0 ? static_cast<int>(strlen(text)) : length;
}

bool TranslateWide(HDC hdc, LPCWSTR text, int length, std::wstring& translated,
                   LPCWSTR& output, int& outputLength) {
    output = text;
    outputLength = EffectiveLength(text, length);
    if (text == nullptr || outputLength <= 0) return false;
    if (IsCommandSurfaceText(hdc, std::wstring_view(text, static_cast<size_t>(outputLength)))) {
        return false;
    }

    const std::wstring_view result = TranslationManager::Translate(
        std::wstring_view(text, static_cast<size_t>(outputLength)), true);
    if (result.empty()) return false;

    translated.assign(result.data(), result.size());
    output = translated.c_str();
    outputLength = static_cast<int>(translated.size());
    return true;
}

bool TranslateAnsi(HDC hdc, LPCSTR text, int length, std::string& translated,
                   LPCSTR& output, int& outputLength) {
    output = text;
    outputLength = EffectiveLength(text, length);
    if (text == nullptr || outputLength <= 0) return false;

    std::string source(text, static_cast<size_t>(outputLength));
    source.push_back('\0');
    const wchar_t* wideSource = Encoding::MultiByteToUtf16(CP_ACP, source.c_str());
    const std::wstring sourceWide = wideSource != nullptr ? wideSource : L"";
    if (IsCommandSurfaceText(hdc, sourceWide)) {
        return false;
    }
    char* result = TranslationManager::Translate(source.c_str(), true, CP_ACP, CP_ACP);
    if (result == nullptr) return false;

    translated = result;
    output = translated.c_str();
    outputLength = static_cast<int>(translated.size());
    return true;
}

BOOL WINAPI HookedTextOutW(HDC hdc, int x, int y, LPCWSTR text, int count) {
    try { std::wstring t; LPCWSTR o = text; int n = count; TranslateWide(hdc, text, count, t, o, n); return g_originalTextOutW(hdc, x, y, o, n); }
    catch (...) { return g_originalTextOutW(hdc, x, y, text, count); }
}

BOOL WINAPI HookedTextOutA(HDC hdc, int x, int y, LPCSTR text, int count) {
    try { std::string t; LPCSTR o = text; int n = count; TranslateAnsi(hdc, text, count, t, o, n); return g_originalTextOutA(hdc, x, y, o, n); }
    catch (...) { return g_originalTextOutA(hdc, x, y, text, count); }
}

BOOL WINAPI HookedExtTextOutW(HDC hdc, int x, int y, UINT options, const RECT* rect,
                              LPCWSTR text, UINT count, const INT* spacing) {
    try { std::wstring t; LPCWSTR o = text; int n = static_cast<int>(count); TranslateWide(hdc, text, n, t, o, n); return g_originalExtTextOutW(hdc, x, y, options, rect, o, static_cast<UINT>(n), spacing); }
    catch (...) { return g_originalExtTextOutW(hdc, x, y, options, rect, text, count, spacing); }
}

BOOL WINAPI HookedExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* rect,
                              LPCSTR text, UINT count, const INT* spacing) {
    try { std::string t; LPCSTR o = text; int n = static_cast<int>(count); TranslateAnsi(hdc, text, n, t, o, n); return g_originalExtTextOutA(hdc, x, y, options, rect, o, static_cast<UINT>(n), spacing); }
    catch (...) { return g_originalExtTextOutA(hdc, x, y, options, rect, text, count, spacing); }
}

int WINAPI HookedDrawTextW(HDC hdc, LPCWSTR text, int count, LPRECT rect, UINT format) {
    try { std::wstring t; LPCWSTR o = text; int n = count; TranslateWide(hdc, text, count, t, o, n); return g_originalDrawTextW(hdc, o, n, rect, format); }
    catch (...) { return g_originalDrawTextW(hdc, text, count, rect, format); }
}

int WINAPI HookedDrawTextA(HDC hdc, LPCSTR text, int count, LPRECT rect, UINT format) {
    try { std::string t; LPCSTR o = text; int n = count; TranslateAnsi(hdc, text, count, t, o, n); return g_originalDrawTextA(hdc, o, n, rect, format); }
    catch (...) { return g_originalDrawTextA(hdc, text, count, rect, format); }
}

int WINAPI HookedDrawTextExW(HDC hdc, LPWSTR text, int count, LPRECT rect, UINT format,
                            LPDRAWTEXTPARAMS params) {
    try { std::wstring t; LPCWSTR o = text; int n = count; TranslateWide(hdc, text, count, t, o, n); return g_originalDrawTextExW(hdc, const_cast<LPWSTR>(o), n, rect, format, params); }
    catch (...) { return g_originalDrawTextExW(hdc, text, count, rect, format, params); }
}

int WINAPI HookedDrawTextExA(HDC hdc, LPSTR text, int count, LPRECT rect, UINT format,
                            LPDRAWTEXTPARAMS params) {
    try { std::string t; LPCSTR o = text; int n = count; TranslateAnsi(hdc, text, count, t, o, n); return g_originalDrawTextExA(hdc, const_cast<LPSTR>(o), n, rect, format, params); }
    catch (...) { return g_originalDrawTextExA(hdc, text, count, rect, format, params); }
}

HRESULT WINAPI HookedDrawThemeText(HTHEME theme, HDC hdc, int partId, int stateId,
                                   LPCWSTR text, int count, DWORD flags, DWORD flags2, LPCRECT rect) {
    try {
        std::wstring translated;
        LPCWSTR output = text;
        int outputLength = count;
        TranslateWide(hdc, text, count, translated, output, outputLength);
        return g_originalDrawThemeText(
            theme, hdc, partId, stateId, output, outputLength, flags, flags2, rect
        );
    } catch (...) {
        return g_originalDrawThemeText(theme, hdc, partId, stateId, text, count, flags, flags2, rect);
    }
}

HRESULT WINAPI HookedDrawThemeTextEx(HTHEME theme, HDC hdc, int partId, int stateId,
                                     LPCWSTR text, int count, DWORD flags, LPRECT rect,
                                     const DTTOPTS* options) {
    try {
        std::wstring translated;
        LPCWSTR output = text;
        int outputLength = count;
        TranslateWide(hdc, text, count, translated, output, outputLength);
        return g_originalDrawThemeTextEx(
            theme, hdc, partId, stateId, output, outputLength, flags, rect, options
        );
    } catch (...) {
        return g_originalDrawThemeTextEx(
            theme, hdc, partId, stateId, text, count, flags, rect, options
        );
    }
}
BOOL WINAPI HookedSetWindowTextW(HWND hwnd, LPCWSTR text) {
    try { std::wstring t; LPCWSTR o = text; int n = -1; TranslateWide(nullptr, text, -1, t, o, n); return g_originalSetWindowTextW(hwnd, o); }
    catch (...) { return g_originalSetWindowTextW(hwnd, text); }
}

BOOL WINAPI HookedSetWindowTextA(HWND hwnd, LPCSTR text) {
    try { std::string t; LPCSTR o = text; int n = -1; TranslateAnsi(nullptr, text, -1, t, o, n); return g_originalSetWindowTextA(hwnd, o); }
    catch (...) { return g_originalSetWindowTextA(hwnd, text); }
}

BOOL WINAPI HookedSetDlgItemTextW(HWND hwnd, int controlId, LPCWSTR text) {
    try { std::wstring t; LPCWSTR o = text; int n = -1; TranslateWide(nullptr, text, -1, t, o, n); return g_originalSetDlgItemTextW(hwnd, controlId, o); }
    catch (...) { return g_originalSetDlgItemTextW(hwnd, controlId, text); }
}

BOOL WINAPI HookedSetDlgItemTextA(HWND hwnd, int controlId, LPCSTR text) {
    try { std::string t; LPCSTR o = text; int n = -1; TranslateAnsi(nullptr, text, -1, t, o, n); return g_originalSetDlgItemTextA(hwnd, controlId, o); }
    catch (...) { return g_originalSetDlgItemTextA(hwnd, controlId, text); }
}

template <typename T>
bool RegisterHook(const wchar_t* module, const char* function, T& original, void* detour) {
    return g_detours.Register(module, function, reinterpret_cast<void**>(&original), detour);
}

} // namespace

namespace Win32GdiHook {

bool Initialize() {
    std::size_t registered = 0;
    registered += RegisterHook(L"gdi32.dll", "TextOutW", g_originalTextOutW, reinterpret_cast<void*>(HookedTextOutW));
    registered += RegisterHook(L"gdi32.dll", "TextOutA", g_originalTextOutA, reinterpret_cast<void*>(HookedTextOutA));
    registered += RegisterHook(L"gdi32.dll", "ExtTextOutW", g_originalExtTextOutW, reinterpret_cast<void*>(HookedExtTextOutW));
    registered += RegisterHook(L"gdi32.dll", "ExtTextOutA", g_originalExtTextOutA, reinterpret_cast<void*>(HookedExtTextOutA));
    registered += RegisterHook(L"user32.dll", "DrawTextW", g_originalDrawTextW, reinterpret_cast<void*>(HookedDrawTextW));
    registered += RegisterHook(L"user32.dll", "DrawTextA", g_originalDrawTextA, reinterpret_cast<void*>(HookedDrawTextA));
    registered += RegisterHook(L"user32.dll", "DrawTextExW", g_originalDrawTextExW, reinterpret_cast<void*>(HookedDrawTextExW));
    registered += RegisterHook(L"user32.dll", "DrawTextExA", g_originalDrawTextExA, reinterpret_cast<void*>(HookedDrawTextExA));
    registered += RegisterHook(L"uxtheme.dll", "DrawThemeText", g_originalDrawThemeText, reinterpret_cast<void*>(HookedDrawThemeText));
    registered += RegisterHook(L"uxtheme.dll", "DrawThemeTextEx", g_originalDrawThemeTextEx, reinterpret_cast<void*>(HookedDrawThemeTextEx));
    registered += RegisterHook(L"user32.dll", "SetWindowTextW", g_originalSetWindowTextW, reinterpret_cast<void*>(HookedSetWindowTextW));
    registered += RegisterHook(L"user32.dll", "SetWindowTextA", g_originalSetWindowTextA, reinterpret_cast<void*>(HookedSetWindowTextA));
    registered += RegisterHook(L"user32.dll", "SetDlgItemTextW", g_originalSetDlgItemTextW, reinterpret_cast<void*>(HookedSetDlgItemTextW));
    registered += RegisterHook(L"user32.dll", "SetDlgItemTextA", g_originalSetDlgItemTextA, reinterpret_cast<void*>(HookedSetDlgItemTextA));

    if (registered == 0 || !g_detours.Commit()) {
        g_detours.Release();
        Logger::Write(L"[Win32GdiHook] no Win32/GDI text hook was installed");
        return false;
    }

    Logger::Write(L"[Win32GdiHook] installed %zu Win32/GDI text hooks", registered);
    return true;
}

void Uninitialize() {
    g_detours.Release();
    g_originalTextOutW = nullptr;
    g_originalTextOutA = nullptr;
    g_originalExtTextOutW = nullptr;
    g_originalExtTextOutA = nullptr;
    g_originalDrawTextW = nullptr;
    g_originalDrawTextA = nullptr;
    g_originalDrawTextExW = nullptr;
    g_originalDrawTextExA = nullptr;
    g_originalDrawThemeText = nullptr;
    g_originalDrawThemeTextEx = nullptr;
    g_originalSetWindowTextW = nullptr;
    g_originalSetWindowTextA = nullptr;
    g_originalSetDlgItemTextW = nullptr;
    g_originalSetDlgItemTextA = nullptr;
}

} // namespace Win32GdiHook
