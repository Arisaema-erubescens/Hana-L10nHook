#include "DetoursHook.h"

#include "AddressResolver.h"
#include "Encoding.h"
#include "Logger.h"

#include <Windows.h>
#include <detours.h>

#include <utility>

namespace {

bool BeginTransaction() {
    LONG error = DetourTransactionBegin();
    if (error != NO_ERROR) {
        Logger::Write(L"[DetoursHook] DetourTransactionBegin failed. error=%ld", error);
        return false;
    }

    error = DetourUpdateThread(GetCurrentThread());
    if (error != NO_ERROR) {
        Logger::Write(L"[DetoursHook] DetourUpdateThread failed. error=%ld", error);
        DetourTransactionAbort();
        return false;
    }

    return true;
}

} // namespace

void DetoursHook::SyncOriginal(Entry& entry) {
    if (entry.original != nullptr) {
        *entry.original = entry.current != nullptr ? entry.current : entry.target;
    }
}

DetoursHook::~DetoursHook() {
    Release();
}

DetoursHook::DetoursHook(void** ppOriginal, void* detour) {
    Register(ppOriginal, detour);
}

DetoursHook::DetoursHook(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, void* detour) {
    Register(moduleName, functionName, ppOriginal, detour);
}

DetoursHook::DetoursHook(DetoursHook&& other) noexcept
    : entries_(std::move(other.entries_)) {
}

DetoursHook& DetoursHook::operator=(DetoursHook&& other) noexcept {
    if (this != &other) {
        Release();
        entries_ = std::move(other.entries_);
    }
    return *this;
}

bool DetoursHook::Register(void** ppOriginal, void* detour) {
    if (ppOriginal == nullptr || *ppOriginal == nullptr || detour == nullptr) {
        Logger::Write(L"[DetoursHook] Register 参数无效。original=%p detour=%p",
                      ppOriginal != nullptr ? *ppOriginal : nullptr,
                      detour);
        return false;
    }

    void* target = *ppOriginal;
    for (const Entry& entry : entries_) {
        if (entry.target == target) {
            Logger::Write(L"[DetoursHook] 重复注册目标函数。target=%p", target);
            return false;
        }
    }

    Entry entry;
    entry.original = ppOriginal;
    entry.target = *ppOriginal;
    entry.current = *ppOriginal;
    entry.detour = detour;
    entries_.push_back(entry);
    return true;
}

bool DetoursHook::Register(const std::wstring& moduleName, const std::string& functionName, void** ppOriginal, void* detour) {
    if (ppOriginal == nullptr || detour == nullptr || moduleName.empty() || functionName.empty()) {
        Logger::Write(L"[DetoursHook] Register 参数无效。module=%s detour=%p",
                      moduleName.empty() ? L"<empty>" : moduleName.c_str(),
                      detour);
        return false;
    }

    void* target = AddressResolver::GetFunctionAddress(moduleName.c_str(), functionName.c_str());
    if (target == nullptr) {
        Logger::Write(L"[DetoursHook] 解析目标函数失败。module=%s function=%s",
                      moduleName.c_str(),
                      U82W(functionName.c_str()));
        return false;
    }

    *ppOriginal = target;
    return Register(ppOriginal, detour);
}

bool DetoursHook::Commit() {
    if (entries_.empty()) {
        return true;
    }

    DetourRestoreAfterWith();
    if (!BeginTransaction()) {
        return false;
    }

    std::vector<Entry*> attached;
    attached.reserve(entries_.size());

    for (Entry& entry : entries_) {
        if (entry.installed) {
            continue;
        }

        void* before = entry.current;
        const LONG error = DetourAttach(reinterpret_cast<void**>(&entry.current), entry.detour);
        if (error != NO_ERROR) {
            entry.current = before;
            Logger::Write(L"[DetoursHook] DetourAttach failed. target=%p detour=%p error=%ld",
                          entry.target,
                          entry.detour,
                          error);
            DetourTransactionAbort();
            for (Entry* attachedEntry : attached) {
                attachedEntry->current = attachedEntry->target;
                SyncOriginal(*attachedEntry);
            }
            return false;
        }

        attached.push_back(&entry);
    }

    const LONG error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        Logger::Write(L"[DetoursHook] DetourTransactionCommit failed. error=%ld", error);
        for (Entry* attachedEntry : attached) {
            attachedEntry->current = attachedEntry->target;
            SyncOriginal(*attachedEntry);
        }
        return false;
    }

    for (Entry* entry : attached) {
        entry->installed = true;
        SyncOriginal(*entry);
    }
    return true;
}

void DetoursHook::Release() {
    if (entries_.empty()) {
        return;
    }

    bool hasInstalled = false;
    for (const Entry& entry : entries_) {
        if (entry.installed) {
            hasInstalled = true;
            break;
        }
    }

    if (!hasInstalled) {
        for (Entry& entry : entries_) {
            entry.current = entry.target;
            SyncOriginal(entry);
        }
        entries_.clear();
        return;
    }

    if (!BeginTransaction()) {
        return;
    }

    for (Entry& entry : entries_) {
        if (!entry.installed) {
            continue;
        }

        void* before = entry.current;
        const LONG error = DetourDetach(reinterpret_cast<void**>(&entry.current), entry.detour);
        if (error != NO_ERROR) {
            entry.current = before;
            Logger::Write(L"[DetoursHook] DetourDetach failed. target=%p detour=%p error=%ld",
                          entry.target,
                          entry.detour,
                          error);
            DetourTransactionAbort();
            return;
        }
    }

    const LONG error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        Logger::Write(L"[DetoursHook] DetourTransactionCommit detach failed. error=%ld", error);
        return;
    }

    for (Entry& entry : entries_) {
        entry.installed = false;
        entry.current = entry.target;
        SyncOriginal(entry);
    }
    entries_.clear();
}
