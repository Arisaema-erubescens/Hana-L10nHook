#include <Windows.h>
#include <shellapi.h>

#include <string>
#include <vector>

namespace {

#if defined(L10N_WORLD_MACHINE_LAUNCHER)
constexpr wchar_t kDefaultExecutable[] = L"World Machine64.exe";
constexpr wchar_t kDefaultHook[] = L"WorldMachineHook.dll";
constexpr wchar_t kLauncherTitle[] = L"WorldMachineLauncher";
#else
constexpr wchar_t kDefaultExecutable[] = L"knald.exe";
constexpr wchar_t kDefaultHook[] = L"KnaldHook.dll";
constexpr wchar_t kLauncherTitle[] = L"KnaldLauncher";
#endif

struct LauncherOptions {
    std::wstring executable;
    std::wstring hook;
    std::vector<std::wstring> applicationArgs;
};

std::wstring ModuleDirectory() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0) {
            return {};
        }
        if (length < path.size() - 1) {
            path.resize(length);
            const size_t slash = path.find_last_of(L"\\/");
            return slash == std::wstring::npos ? std::wstring{} : path.substr(0, slash);
        }
        path.resize(path.size() * 2);
    }
}

std::wstring JoinPath(const std::wstring& directory, const wchar_t* name) {
    if (directory.empty()) {
        return name;
    }
    return directory + L"\\" + name;
}

std::wstring DirectoryOf(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring{} : path.substr(0, slash);
}

std::wstring FullPath(const std::wstring& path) {
    std::wstring result(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length = GetFullPathNameW(
            path.c_str(),
            static_cast<DWORD>(result.size()),
            result.data(),
            nullptr
        );
        if (length == 0) {
            return {};
        }
        if (length < result.size() - 1) {
            result.resize(length);
            return result;
        }
        result.resize(length + 1);
    }
}

std::wstring QuoteArgument(const std::wstring& argument) {
    if (argument.find_first_of(L" \t\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring quoted = L"\"";
    size_t backslashes = 0;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'\"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted += L'\"';
            backslashes = 0;
            continue;
        }
        quoted.append(backslashes, L'\\');
        quoted += character;
        backslashes = 0;
    }
    quoted.append(backslashes * 2, L'\\');
    quoted += L'\"';
    return quoted;
}

bool ParseOptions(LauncherOptions& options) {
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments == nullptr) {
        return false;
    }

    const std::wstring moduleDirectory = ModuleDirectory();
    options.executable = JoinPath(moduleDirectory, kDefaultExecutable);
    options.hook = JoinPath(moduleDirectory, kDefaultHook);

    for (int index = 1; index < argumentCount; ++index) {
        const std::wstring argument = arguments[index];
        if ((argument == L"--knald" || argument == L"--exe") && index + 1 < argumentCount) {
            options.executable = arguments[++index];
        } else if (argument == L"--hook" && index + 1 < argumentCount) {
            options.hook = arguments[++index];
        } else {
            options.applicationArgs.push_back(argument);
        }
    }

    LocalFree(arguments);
    options.executable = FullPath(options.executable);
    options.hook = FullPath(options.hook);
    return !options.executable.empty() && !options.hook.empty();
}

std::wstring BuildCommandLine(const LauncherOptions& options) {
    std::wstring commandLine = QuoteArgument(options.executable);
    for (const std::wstring& argument : options.applicationArgs) {
        commandLine += L" ";
        commandLine += QuoteArgument(argument);
    }
    return commandLine;
}

void ShowError(const wchar_t* operation) {
    const DWORD error = GetLastError();
    wchar_t message[512] = {};
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        0,
        message,
        static_cast<DWORD>(_countof(message)),
        nullptr
    );

    std::wstring text = operation;
    text += L" 失败 (";
    text += std::to_wstring(error);
    text += L")\n";
    text += message;
    MessageBoxW(nullptr, text.c_str(), kLauncherTitle, MB_ICONERROR | MB_OK);
}

bool InjectLibrary(HANDLE process, const std::wstring& hookPath) {
    const size_t bytes = (hookPath.size() + 1) * sizeof(wchar_t);
    void* remotePath = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remotePath == nullptr) {
        return false;
    }

    bool success = false;
    if (WriteProcessMemory(process, remotePath, hookPath.c_str(), bytes, nullptr)) {
        const HMODULE kernel32 = GetModuleHandleW(L"Kernel32.dll");
        const auto loadLibrary = kernel32 == nullptr
            ? nullptr
            : GetProcAddress(kernel32, "LoadLibraryW");
        if (loadLibrary != nullptr) {
            HANDLE thread = CreateRemoteThread(
                process,
                nullptr,
                0,
                reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary),
                remotePath,
                0,
                nullptr
            );
            if (thread != nullptr) {
                success = WaitForSingleObject(thread, 30000) == WAIT_OBJECT_0;
                DWORD moduleResult = 0;
                success = success && GetExitCodeThread(thread, &moduleResult) && moduleResult != 0;
                CloseHandle(thread);
            }
        }
    }

    VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
    return success;
}

int Run() {
    LauncherOptions options;
    if (!ParseOptions(options)) {
        ShowError(L"解析启动参数");
        return 2;
    }

    if (GetFileAttributesW(options.executable.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        ShowError(L"找不到目标程序");
        return 2;
    }
    if (GetFileAttributesW(options.hook.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        ShowError(L"找不到 Hook DLL");
        return 2;
    }

    std::wstring commandLine = BuildCommandLine(options);
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    if (!CreateProcessW(
            options.executable.c_str(),
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_SUSPENDED,
            nullptr,
            (DirectoryOf(options.executable).empty()
                ? ModuleDirectory()
                : DirectoryOf(options.executable)).c_str(),
            &startupInfo,
            &processInfo)) {
        ShowError(L"启动目标程序");
        return 3;
    }

    const bool injected = InjectLibrary(processInfo.hProcess, options.hook);
    if (!injected) {
        ShowError(L"加载 Hook DLL");
        TerminateProcess(processInfo.hProcess, 4);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return 4;
    }

    if (ResumeThread(processInfo.hThread) == static_cast<DWORD>(-1)) {
        ShowError(L"恢复目标程序主线程");
        TerminateProcess(processInfo.hProcess, 5);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return 5;
    }

    CloseHandle(processInfo.hThread);
    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hProcess);
    return static_cast<int>(exitCode);
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    return Run();
}
