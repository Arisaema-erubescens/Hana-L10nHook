#pragma once

#include <cstddef>
#include <string>
#include <vector>

class DetoursHook {
public:
    DetoursHook() = default;
    ~DetoursHook();

    DetoursHook(const DetoursHook&) = delete;
    DetoursHook& operator=(const DetoursHook&) = delete;

    DetoursHook(DetoursHook&& other) noexcept;
    DetoursHook& operator=(DetoursHook&& other) noexcept;

    // 构造并注册一个 Detours Hook。
    // ppOriginal 输入目标函数地址，Commit 成功后会被 Detours 改写为 trampoline 地址。
    explicit DetoursHook(void** ppOriginal, void* detour);

    // 构造并按模块名/导出名注册一个 Detours Hook。
    explicit DetoursHook(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, void* detour);

    // 注册一个已解析目标函数的 Detours Hook。
    bool Register(void** ppOriginal, void* detour);

    // 按模块名/导出名解析目标函数并注册 Detours Hook。
    bool Register(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, void* detour);

    // 用一个 Detours 事务安装当前对象注册的所有 Hook。
    bool Commit();

    // 卸载当前对象已安装的 Hook，并清空注册项。
    void Release();

private:
    struct Entry {
        void** original = nullptr;
        void* target = nullptr;
        void* current = nullptr;
        void* detour = nullptr;
        bool installed = false;
    };

    std::vector<Entry> entries_;

    static void SyncOriginal(Entry& entry);

public:
    DetoursHook(std::nullptr_t, void* detour) = delete;
    DetoursHook(int, void* detour) = delete;
    DetoursHook(void** ppOriginal, std::nullptr_t) = delete;
    DetoursHook(void** ppOriginal, int) = delete;
    DetoursHook(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, std::nullptr_t) = delete;
    DetoursHook(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, int) = delete;

    bool Register(std::nullptr_t, void* detour) = delete;
    bool Register(int, void* detour) = delete;
    bool Register(void** ppOriginal, std::nullptr_t) = delete;
    bool Register(void** ppOriginal, int) = delete;
    bool Register(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, std::nullptr_t) = delete;
    bool Register(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, int) = delete;
};
