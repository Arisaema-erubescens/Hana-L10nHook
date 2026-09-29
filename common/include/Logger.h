#pragma once

namespace Logger {

void Write(const wchar_t* format, ...);
void Write(const char* utf8Format, ...);

} // namespace Logger
