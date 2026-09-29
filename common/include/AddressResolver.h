#pragma once

#include <Windows.h>

namespace AddressResolver {

void* GetFunctionAddress(const wchar_t* moduleName, const char* functionName);
void* FindPattern(const wchar_t* moduleName, const char* pattern);

} // namespace AddressResolver
