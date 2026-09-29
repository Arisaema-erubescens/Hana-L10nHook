#include <windows.h>
#include "Logger.h"
#include "Hook.h"

// 1 = 默认在新线程中异步初始化；0 = 在 DllMain 中同步执行初始化逻辑
#ifndef XPROXY_ENABLE_ASYNC_INIT
#define XPROXY_ENABLE_ASYNC_INIT 1
#endif

// 1 = 启用进程过滤，仅允许 AllowedProcessNames 中的进程加载；0 = 不启用进程过滤
#ifndef XPROXY_ENABLE_PROCESS_FILTER
#define XPROXY_ENABLE_PROCESS_FILTER 1
#endif

#if XPROXY_ENABLE_PROCESS_FILTER
static bool IsAllowedProcess();
#if defined(XPROXY_KNALD_ONLY)
static const wchar_t* const AllowedProcessNames[] = {
    L"knald.exe",
};
#elif defined(XPROXY_WORLDMACHINE_ONLY)
static const wchar_t* const AllowedProcessNames[] = {
    L"World Machine64.exe",
};
#else
static const wchar_t* const AllowedProcessNames[] = {
    L"EViews9_x64.exe",
    L"EViews10_x64.exe",
    L"EViews11_x64.exe",
    L"EViews12_x64.exe",
    L"EViews13.exe",
    L"EViews14.exe",
    L"KeyframePro.exe",
    L"rv.exe",
    //L"my1.exe",
};
#endif
#endif
static bool g_hookInitialized = false;

static DWORD Initialize() {
    if (!Hook::Initialize()) {
        Logger::Write(u8"Hook 初始化错误");
        return 1;
    }

    g_hookInitialized = true;
    return 0;
}

static void Uninitialize() {
    if (!g_hookInitialized) {
        return;
    }

    Hook::Uninitialize();
    g_hookInitialized = false;
}

#if XPROXY_ENABLE_ASYNC_INIT
static DWORD WINAPI InitializeThreadProc(PVOID lpThreadParameter) {
    HMODULE selfHold = static_cast<HMODULE>(lpThreadParameter);
    if (selfHold != nullptr) {
        FreeLibraryAndExitThread(selfHold, Initialize());
    }
    return Initialize();
}
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, PVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH: {
#if XPROXY_ENABLE_PROCESS_FILTER
            if (!IsAllowedProcess()) {
                break;
            }
#endif
            // 禁用线程创建/结束通知，减少 DllMain 调用次数
            DisableThreadLibraryCalls(hModule);

#if XPROXY_ENABLE_ASYNC_INIT
            // 在单独线程中执行初始化逻辑，避免阻塞加载锁
            HMODULE selfHold = nullptr;
            const DWORD selfHoldFlags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
            if (!GetModuleHandleExW(selfHoldFlags, reinterpret_cast<LPCWSTR>(&DllMain), &selfHold)) {
                return FALSE;
            }

            HANDLE initThread = CreateThread(nullptr, 0, InitializeThreadProc, selfHold, 0, nullptr);
            if (!initThread) {
                FreeLibrary(selfHold);
                return FALSE;
            }
            CloseHandle(initThread);

            // 立即返回；模块引用已在 DllMain 中先拿到，再交给初始化线程在结束时释放，避免卸载窗口
#else
            // 同步执行初始化逻辑；如初始化包含复杂 API/耗时操作，请优先改回异步模式
            if (Initialize() != 0) {
                return FALSE;
            }
#endif
            break;
        }

        case DLL_PROCESS_DETACH:{
            if (lpReserved != nullptr) {
                // 进程正在退出, 系统会强制终止所有线程并释放所有内存，无需任何清理。
                break;
            }

            Uninitialize();
            break;
        }
        
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // 已调用 DisableThreadLibraryCalls，通常不会收到这些通知
            break;
    }

    return TRUE;
}

#if XPROXY_ENABLE_PROCESS_FILTER
static bool IsAllowedProcess() {
	wchar_t path[MAX_PATH] = { 0 };
	DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
	if (len == 0 || len >= MAX_PATH) {
		return false;
	}

	const wchar_t* fileName = path;
	for (const wchar_t* p = path; *p; ++p) {
		if (*p == L'\\' || *p == L'/') {
			fileName = p + 1;
		}
	}

#if !defined(XPROXY_KNALD_ONLY) && !defined(XPROXY_WORLDMACHINE_ONLY)
    if (_wcsnicmp(fileName, L"EViews", 6) == 0) {
        return true;
    }
#endif

    for (const wchar_t* allowed : AllowedProcessNames) {
		if (lstrcmpiW(fileName, allowed) == 0) {
			return true;
		}
	}

	return false;
}
#endif
