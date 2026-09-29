#include "QStringBridge.h"

#include "AddressResolver.h"
#include "Encoding.h"
#include "Logger.h"

#include <Windows.h>

#include <cstring>
#include <limits>
#include <new>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace {

constexpr std::int64_t kMaxQStringLength = 10000;
constexpr wchar_t kEmptyQString[] = L"";

struct ExportNameCandidate {
    std::string_view name;
};

struct PatternCandidate {
    std::string_view pattern;
    std::ptrdiff_t offset = 0;
};

using ResolveCandidate = std::variant<ExportNameCandidate, PatternCandidate>;

#if defined(_WIN64)
using QStringSizeType = std::int64_t;

constexpr std::size_t kQt6FieldTextPtr = 0x08;
constexpr std::size_t kQt6FieldLength = 0x10;
constexpr std::size_t kQt6QStringObjectSize = 24u;

constexpr std::size_t kQt5DptrFieldLength = 0x04;
constexpr std::size_t kQt5DptrFieldTextOffset = 0x10;
constexpr std::size_t kQt5QStringObjectSize = 8u;

const std::vector<ResolveCandidate> kQStringConstructorCandidates = {
    ExportNameCandidate{ "??0QString@QT@@QEAA@PEBVQChar@1@_J@Z" },
    ExportNameCandidate{ "??0QString@QT@@QEAA@PEBVQChar@1@H@Z" },
    ExportNameCandidate{ "??0QString@@QEAA@PEBVQChar@@_J@Z" },
    ExportNameCandidate{ "??0QString@@QEAA@PEBVQChar@@H@Z" },
    PatternCandidate{ "48 89 5C 24 18 48 89 4C 24 08 55 56 57 48 83 EC 20 49 8B D8 48 8B F2 48 8B F9 33 ED 48 89 29 48 89 69 08 48 89 69 10 48 85 D2 75 ?? 48 89 29 48 89 69 08 48 89 69 10 E9 ?? ?? ?? ?? 48 85 DB 79 ?? 48 8B CE E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 75 ?? 48 8B 0F 48 89 2F 48 8D 05 ?? ?? ?? ?? 48 89 47 08 48 89 6F 10 48 85 C9" },
};

const std::vector<ResolveCandidate> kQStringDestructorCandidates = {
    ExportNameCandidate{ "??1QString@QT@@QEAA@XZ" },
    ExportNameCandidate{ "??1QString@@QEAA@XZ" },
    PatternCandidate{ "48 8B 11 48 85 D2 74 ?? B8 FF FF FF FF F0 0F C1 02 83 F8 01 75 ?? 48 8B 09 48 FF 25 ?? ?? ?? ?? C3" },
    PatternCandidate{ "48 8B 11 48 85 D2 74 ?? B8 FF FF FF FF F0 0F C1 02 83 F8 01 75 ?? 48 8B 09 8D 50 01 44 8D 40 07 E9 ?? ?? ?? ?? C3" },
};

const std::vector<ResolveCandidate> kQStringUtf16Candidates = {
    ExportNameCandidate{ "?utf16@QString@QT@@QEBAPEBGXZ" },
    ExportNameCandidate{ "?utf16@QString@@QEBAPEBGXZ" },
    PatternCandidate{ "40 53 48 83 EC 20 48 83 39 00 48 8B D9 75 ?? 48 8B 51 10 41 B8 01 00 00 00 E8 ?? ?? ?? ?? 48 8B 43 08 48 83 C4 20 5B C3 48 8B 41 08 48 83 C4 20 5B C3" },
};

const std::vector<ResolveCandidate> kQStringSizeCandidates = {
    ExportNameCandidate{ "?size@QString@QT@@QEBA_JXZ" },
    ExportNameCandidate{ "?size@QString@QT@@QEBAHXZ" },
    ExportNameCandidate{ "?size@QString@@QEBA_JXZ" },
    ExportNameCandidate{ "?size@QString@@QEBAHXZ" },
    PatternCandidate{ "48 8B 41 10 C3" },
};
#elif defined(_M_IX86)
using QStringSizeType = std::int32_t;

constexpr std::size_t kQt6FieldTextPtr = 0x04;
constexpr std::size_t kQt6FieldLength = 0x08;
constexpr std::size_t kQt6QStringObjectSize = 12u;

constexpr std::size_t kQt5DptrFieldLength = 0x04;
constexpr std::size_t kQt5DptrFieldTextOffset = 0x0C;
constexpr std::size_t kQt5QStringObjectSize = 4u;

const std::vector<ResolveCandidate> kQStringConstructorCandidates = {
    ExportNameCandidate{ "??0QString@QT@@QAE@PBVQChar@1@H@Z" },
    ExportNameCandidate{ "??0QString@@QAE@PBVQChar@@H@Z" },
};

const std::vector<ResolveCandidate> kQStringDestructorCandidates = {
    ExportNameCandidate{ "??1QString@QT@@QAE@XZ" },
    ExportNameCandidate{ "??1QString@@QAE@XZ" },
};

const std::vector<ResolveCandidate> kQStringUtf16Candidates = {
    ExportNameCandidate{ "?utf16@QString@QT@@QBEPBGXZ" },
    ExportNameCandidate{ "?utf16@QString@@QBEPBGXZ" },
};

const std::vector<ResolveCandidate> kQStringSizeCandidates = {
    ExportNameCandidate{ "?size@QString@QT@@QBEHXZ" },
    ExportNameCandidate{ "?size@QString@@QBEHXZ" },
};
#else
#error QStringBridge only supports x86/x64 Windows targets.
#endif

std::size_t ComputeModuleOffset(HMODULE module, std::uintptr_t address) {
    if (module == nullptr || address == 0) {
        return 0;
    }
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    return address >= base ? static_cast<std::size_t>(address - base) : 0u;
}

void LogResolveSuccess(const char* targetName,
                       const char* method,
                       const std::wstring& moduleName,
                       std::uintptr_t address,
                       std::string_view detail) {
    HMODULE module = GetModuleHandleW(moduleName.c_str());
    const char* moduleUtf8 = W2U8(moduleName.c_str());
    Logger::Write(u8"[QStringBridge] 解析成功 | 目标=%s | 方式=%s | 模块偏移=%s+0x%zX | 地址=%p | 命中=%s",
                  targetName,
                  method,
                  moduleUtf8 != nullptr ? moduleUtf8 : "<invalid>",
                  ComputeModuleOffset(module, address),
                  reinterpret_cast<void*>(address),
                  detail.empty() ? "-" : detail.data());
}

void LogResolveFailure(const char* targetName,
                       const std::wstring& moduleName,
                       std::size_t candidateCount) {
    const char* moduleUtf8 = W2U8(moduleName.c_str());
    Logger::Write(u8"[QStringBridge] 解析失败 | 目标=%s | 模块=%s | 尝试候选数=%zu",
                  targetName,
                  moduleUtf8 != nullptr ? moduleUtf8 : "<invalid>",
                  candidateCount);
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

bool IsReadableRange(const void* ptr, std::size_t size) {
    if (ptr == nullptr || size == 0) {
        return false;
    }

    const auto begin = reinterpret_cast<std::uintptr_t>(ptr);
    const auto end = begin + size;
    if (end < begin) {
        return false;
    }

    std::uintptr_t cursor = begin;
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (VirtualQuery(reinterpret_cast<const void*>(cursor), &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        if (mbi.State != MEM_COMMIT || !IsReadableProtect(mbi.Protect)) {
            return false;
        }

        const auto regionEnd = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (regionEnd <= cursor) {
            return false;
        }
        cursor = regionEnd;
    }

    return true;
}

template<typename FnT>
FnT ResolveCandidateInOrder(const char* targetName,
                            const std::wstring& moduleName,
                            const std::vector<ResolveCandidate>& candidates) {
    for (const ResolveCandidate& candidate : candidates) {
        FnT resolved = std::visit([&](const auto& item) -> FnT {
            using CandidateT = std::decay_t<decltype(item)>;

            if constexpr (std::is_same_v<CandidateT, ExportNameCandidate>) {
                if (item.name.empty()) {
                    return nullptr;
                }
                FnT fn = reinterpret_cast<FnT>(
                    AddressResolver::GetFunctionAddress(moduleName.c_str(), item.name.data())
                );
                if (fn != nullptr) {
                    LogResolveSuccess(targetName,
                                      u8"导出精确匹配",
                                      moduleName,
                                      reinterpret_cast<std::uintptr_t>(fn),
                                      item.name);
                }
                return fn;
            } else if constexpr (std::is_same_v<CandidateT, PatternCandidate>) {
                if (item.pattern.empty()) {
                    return nullptr;
                }
                void* address = AddressResolver::FindPattern(moduleName.c_str(), item.pattern.data());
                if (address == nullptr) {
                    return nullptr;
                }
                const auto resolvedAddress = reinterpret_cast<std::uintptr_t>(address) + item.offset;
                LogResolveSuccess(targetName,
                                  u8"特征码",
                                  moduleName,
                                  resolvedAddress,
                                  item.pattern);
                return reinterpret_cast<FnT>(resolvedAddress);
            }

            return nullptr;
        }, candidate);

        if (resolved != nullptr) {
            return resolved;
        }
    }

    LogResolveFailure(targetName, moduleName, candidates.size());
    return nullptr;
}

} // namespace

QStringBridge::ScopedQString::ScopedQString(const QStringBridge* bridge, void* object) noexcept
    : bridge_(bridge), object_(object) {
}

QStringBridge::ScopedQString::~ScopedQString() {
    Reset();
}

QStringBridge::ScopedQString::ScopedQString(ScopedQString&& other) noexcept
    : bridge_(other.bridge_), object_(other.object_) {
    other.bridge_ = nullptr;
    other.object_ = nullptr;
}

QStringBridge::ScopedQString& QStringBridge::ScopedQString::operator=(ScopedQString&& other) noexcept {
    if (this != &other) {
        Reset();
        bridge_ = other.bridge_;
        object_ = other.object_;
        other.bridge_ = nullptr;
        other.object_ = nullptr;
    }
    return *this;
}

const void* QStringBridge::ScopedQString::Get() const noexcept {
    return object_;
}

void* QStringBridge::ScopedQString::Get() noexcept {
    return object_;
}

QStringBridge::ScopedQString::operator bool() const noexcept {
    return object_ != nullptr;
}

void QStringBridge::ScopedQString::Reset() noexcept {
    if (bridge_ != nullptr && object_ != nullptr) {
        bridge_->Destroy(object_);
    }
    bridge_ = nullptr;
    object_ = nullptr;
}

void* QStringBridge::ScopedQString::Detach() noexcept {
    void* object = object_;
    bridge_ = nullptr;
    object_ = nullptr;
    return object;
}

bool QStringBridge::Initialize(const Config& config) {
    Reset();

    config_ = config;
    if (config_.module_name.empty()) {
        config_.module_name = L"Qt6Core.dll";
    }
    if (config_.object_size == 0) {
        config_.object_size = InferObjectSize(config_.module_name);
    }

    if (config_.load_if_missing) {
        LoadLibraryW(config_.module_name.c_str());
    }

    constructor_ = ResolveCandidateInOrder<QStringConstructor>("QString::ctor", config_.module_name, kQStringConstructorCandidates);
    destructor_ = ResolveCandidateInOrder<QStringDestructor>("QString::dtor", config_.module_name, kQStringDestructorCandidates);
    utf16_ = ResolveCandidateInOrder<QStringUtf16>("QString::utf16", config_.module_name, kQStringUtf16Candidates);
    size_ = ResolveCandidateInOrder<QStringSize>("QString::size", config_.module_name, kQStringSizeCandidates);

    initialized_ = true;
    const char* moduleUtf8 = W2U8(config_.module_name.c_str());
    Logger::Write(u8"[QStringBridge] 初始化结果 | 模块=%s | 对象大小=%zu | 构造=%d | 析构=%d | utf16=%d | size=%d | 可创建=%d",
                  moduleUtf8 != nullptr ? moduleUtf8 : "<invalid>",
                  config_.object_size,
                  constructor_ != nullptr ? 1 : 0,
                  destructor_ != nullptr ? 1 : 0,
                  utf16_ != nullptr ? 1 : 0,
                  size_ != nullptr ? 1 : 0,
                  CanCreate() ? 1 : 0);
    return CanCreate();
}

void QStringBridge::Reset() {
    config_ = Config{};
    constructor_ = nullptr;
    destructor_ = nullptr;
    utf16_ = nullptr;
    size_ = nullptr;
    initialized_ = false;
}

bool QStringBridge::IsInitialized() const {
    return initialized_;
}

bool QStringBridge::CanCreate() const {
    return initialized_ &&
           config_.object_size != 0 &&
           constructor_ != nullptr &&
           destructor_ != nullptr;
}

const wchar_t* QStringBridge::Extract(const void* qstring, int& length) const {
	length = 0;
	if (qstring == nullptr) {
		return nullptr;
	}

	if (const wchar_t* text = ExtractViaApi(qstring, length)) {
		return text;
	}
	if (const wchar_t* text = ExtractQt6Layout(qstring, length)) {
		return text;
	}
	if (const wchar_t* text = ExtractQt5Layout(qstring, length)) {
		return text;
	}

	return nullptr;
}

std::wstring QStringBridge::Extract(const void* qstring) const {
    int length = 0;
    const wchar_t* text = Extract(qstring, length);
    if (text == nullptr || length <= 0) {
        return {};
    }
    return std::wstring(text, text + length);
}

void* QStringBridge::Create(std::wstring_view text) const {
    if (!CanCreate() || text.empty()) {
        return nullptr;
    }

    if (text.size() > static_cast<std::size_t>((std::numeric_limits<QStringSizeType>::max)())) {
        return nullptr;
    }

    void* qstring = operator new(config_.object_size, std::nothrow);
    if (qstring == nullptr) {
        return nullptr;
    }

    std::memset(qstring, 0, config_.object_size);
    constructor_(qstring, text.data(), static_cast<QStringSizeType>(text.size()));
    return qstring;
}

void* QStringBridge::Create(const wchar_t* text) const {
    if (text == nullptr) {
        return nullptr;
    }
    return Create(std::wstring_view(text));
}

QStringBridge::ScopedQString QStringBridge::CreateScoped(std::wstring_view text) const {
    return ScopedQString(this, Create(text));
}

QStringBridge::ScopedQString QStringBridge::CreateScoped(const wchar_t* text) const {
    return ScopedQString(this, Create(text));
}

void QStringBridge::Destroy(void* qstring) const {
    if (qstring == nullptr) {
        return;
    }
    if (destructor_ != nullptr) {
        destructor_(qstring);
    }
    operator delete(qstring);
}

const wchar_t* QStringBridge::ExtractViaApi(const void* qstring, int& length) const {
    if (utf16_ == nullptr || size_ == nullptr) {
        return nullptr;
    }

    __try {
        const QStringSizeType lenValue = size_(qstring);
        if (lenValue == 0) {
            length = 0;
            return kEmptyQString;
        }
        if (lenValue <= 0 || lenValue > kMaxQStringLength) {
            return nullptr;
        }

        const unsigned short* ptr16 = utf16_(qstring);
        if (ptr16 == nullptr) {
            return nullptr;
        }

        const int len = static_cast<int>(lenValue);
        const wchar_t* wide = reinterpret_cast<const wchar_t*>(ptr16);
        if (!IsReadableRange(wide, static_cast<std::size_t>(len) * sizeof(wchar_t))) {
            return nullptr;
        }

        length = len;
        return wide;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

const wchar_t* QStringBridge::ExtractQt6Layout(const void* qstring, int& length) const {
    __try {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(qstring);
        const auto textPtr = *reinterpret_cast<const std::uintptr_t*>(bytes + kQt6FieldTextPtr);
        const auto lenValue = *reinterpret_cast<const QStringSizeType*>(bytes + kQt6FieldLength);

        if (lenValue == 0) {
            length = 0;
            return kEmptyQString;
        }
        if (lenValue <= 0 || lenValue > kMaxQStringLength || textPtr == 0) {
            return nullptr;
        }

        const int len = static_cast<int>(lenValue);
        const wchar_t* wide = reinterpret_cast<const wchar_t*>(textPtr);
        if (!IsReadableRange(wide, static_cast<std::size_t>(len) * sizeof(wchar_t))) {
            return nullptr;
        }

        length = len;
        return wide;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

const wchar_t* QStringBridge::ExtractQt5Layout(const void* qstring, int& length) const {
    __try {
        const auto dptr = *reinterpret_cast<const std::uintptr_t*>(qstring);
        if (dptr == 0) {
            return nullptr;
        }

        const int len = *reinterpret_cast<const int*>(dptr + kQt5DptrFieldLength);
        if (len == 0) {
            length = 0;
            return kEmptyQString;
        }
        if (len <= 0 || len > kMaxQStringLength) {
            return nullptr;
        }

        const auto textOffset = *reinterpret_cast<const std::intptr_t*>(dptr + kQt5DptrFieldTextOffset);
        if (textOffset <= 0 || textOffset > 0x1000) {
            return nullptr;
        }

        const wchar_t* wide = reinterpret_cast<const wchar_t*>(dptr + static_cast<std::uintptr_t>(textOffset));
        if (!IsReadableRange(wide, static_cast<std::size_t>(len) * sizeof(wchar_t))) {
            return nullptr;
        }

        length = len;
        return wide;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool QStringBridge::IsReadableRange(const void* ptr, std::size_t size) {
    return ::IsReadableRange(ptr, size);
}

std::size_t QStringBridge::InferObjectSize(const std::wstring& module_name) {
    return module_name.find(L"Qt5") != std::wstring::npos ? kQt5QStringObjectSize : kQt6QStringObjectSize;
}
