#pragma once

#include <Windows.h>

#include <string>

namespace ModuleUtils {

// 获取路径所在目录，返回值保留末尾反斜杠。
std::wstring GetPathDirectory(const wchar_t* path);

// 获取当前进程主程序所在目录，返回值保留末尾反斜杠。
std::wstring GetProcessDirectory();

// 获取指定模块所在目录，返回值保留末尾反斜杠。
std::wstring GetModuleDirectory(HMODULE module);

// Windows 路径比较，忽略大小写。
bool IsSamePath(const std::wstring& left, const std::wstring& right);

// 只从当前进程主程序目录加载 DLL。
// 若同名模块已加载但不在主程序目录，返回 nullptr。
HMODULE LoadLibraryFromProcessDirectory(const wchar_t* moduleName);

} // namespace ModuleUtils
