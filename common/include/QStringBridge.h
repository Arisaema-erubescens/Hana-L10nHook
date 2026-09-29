#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// QStringBridge：桥接目标进程里的 Qt QString ABI。
// 优先解析 QString 导出符号，必要时使用已知布局/特征码兜底。
// 仅负责管理通过本桥接创建出来的临时 QString 对象。
class QStringBridge {
public:
    // ScopedQString：管理 QStringBridge 创建出的临时 QString 生命周期。
    // 离开作用域时自动调用 QString 析构函数并释放对象外壳内存。
    class ScopedQString {
    public:
        ScopedQString() = default;
        ~ScopedQString();

        ScopedQString(ScopedQString&& other) noexcept;
        ScopedQString& operator=(ScopedQString&& other) noexcept;

        ScopedQString(const ScopedQString&) = delete;
        ScopedQString& operator=(const ScopedQString&) = delete;

        // 获取底层 QString 对象指针，不转移所有权。
        const void* Get() const noexcept;
        void* Get() noexcept;

        // 放弃托管并返回底层 QString 指针。
        // 调用方接管销毁责任，必须继续使用同一个 QStringBridge 销毁。
        void* Detach() noexcept;

        // 销毁当前托管的 QString 对象并清空句柄。
        void Reset() noexcept;

        // 当前是否托管了有效的 QString 对象。
        explicit operator bool() const noexcept;

    private:
        friend class QStringBridge;
        ScopedQString(const QStringBridge* bridge, void* object) noexcept;

        const QStringBridge* bridge_ = nullptr;
        void* object_ = nullptr;
    };

    struct Config {
        // 目标进程中的 QtCore 模块名，例如 Qt6Core.dll 或 Qt5Core.dll。
        std::wstring module_name = L"Qt6Core.dll";

        // 模块尚未加载时是否尝试 LoadLibrary。
        bool load_if_missing = true;

        // QString 对象外壳大小。为 0 时按 Qt 主版本和目标架构自动推导。
        std::size_t object_size = 0;
    };

    // 解析 QString 构造/析构/utf16/size，并准备布局兜底提取能力。
    bool Initialize(const Config& config = Config{});

    // 清空已解析函数和配置。仍被 ScopedQString 托管的对象销毁前，桥接实例必须保持有效。
    void Reset();

    // 是否已执行初始化流程。
    bool IsInitialized() const;

    // 从 QString 提取 UTF-16 文本。返回指针归原 QString 所有，不应保存到原对象生命周期之外。
    const wchar_t* Extract(const void* qstring, int& length) const;

    // 从 QString 提取文本并复制为 std::wstring。
    std::wstring Extract(const void* qstring) const;

    // 使用已解析的 QString(const QChar*, length) 创建堆上的 QString 对象。
    void* Create(std::wstring_view text) const;
    void* Create(const wchar_t* text) const;

    // 创建由 ScopedQString 自动管理的临时 QString 对象。
    ScopedQString CreateScoped(std::wstring_view text) const;
    ScopedQString CreateScoped(const wchar_t* text) const;

    // 销毁通过本桥接创建的 QString 对象。
    void Destroy(void* qstring) const;

    // 当前是否具备创建临时 QString 的能力。
    bool CanCreate() const;

private:
    using QStringSizeType = std::intptr_t;
    using QStringConstructor = void* (*)(void* pThis, const wchar_t* text, QStringSizeType length);
    using QStringDestructor = void (*)(void* pThis);
    using QStringUtf16 = const unsigned short* (*)(const void* pThis);
    using QStringSize = QStringSizeType (*)(const void* pThis);

    const wchar_t* ExtractViaApi(const void* qstring, int& length) const;
    const wchar_t* ExtractQt6Layout(const void* qstring, int& length) const;
    const wchar_t* ExtractQt5Layout(const void* qstring, int& length) const;
    static bool IsReadableRange(const void* ptr, std::size_t size);
    static std::size_t InferObjectSize(const std::wstring& module_name);

    Config config_{};
    QStringConstructor constructor_ = nullptr;
    QStringDestructor destructor_ = nullptr;
    QStringUtf16 utf16_ = nullptr;
    QStringSize size_ = nullptr;
    bool initialized_ = false;
};
