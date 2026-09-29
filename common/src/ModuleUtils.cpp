#include "ModuleUtils.h"

#include "Logger.h"

namespace ModuleUtils {

std::wstring GetPathDirectory(const wchar_t* path) {
    if (path == nullptr || path[0] == L'\0') {
        return {};
    }

    std::wstring directory = path;
    const std::size_t slash = directory.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return {};
    }

    directory.resize(slash + 1);
    return directory;
}

std::wstring GetProcessDirectory() {
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }

    return GetPathDirectory(path);
}

std::wstring GetModuleDirectory(HMODULE module) {
    if (module == nullptr) {
        return {};
    }

    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return {};
    }

    return GetPathDirectory(path);
}

bool IsSamePath(const std::wstring& left, const std::wstring& right) {
    return _wcsicmp(left.c_str(), right.c_str()) == 0;
}

HMODULE LoadLibraryFromProcessDirectory(const wchar_t* moduleName) {
    const std::wstring processDirectory = GetProcessDirectory();
    if (processDirectory.empty() || moduleName == nullptr || moduleName[0] == L'\0') {
        return nullptr;
    }

    HMODULE module = GetModuleHandleW(moduleName);
    if (module != nullptr) {
        if (IsSamePath(GetModuleDirectory(module), processDirectory)) {
            return module;
        }

        Logger::Write(L"[ModuleUtils] 模块已加载但不在程序目录，拒绝使用。module=%s", moduleName);
        return nullptr;
    }

    const std::wstring fullPath = processDirectory + moduleName;
    module = LoadLibraryExW(fullPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (module == nullptr) {
        return nullptr;
    }

    if (!IsSamePath(GetModuleDirectory(module), processDirectory)) {
        Logger::Write(L"[ModuleUtils] 模块加载后目录不匹配，拒绝使用。module=%s", moduleName);
        return nullptr;
    }

    return module;
}

} // namespace ModuleUtils
