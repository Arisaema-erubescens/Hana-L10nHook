#pragma once

#include <string>

namespace StringUtils {

std::wstring Escape(const std::wstring& text);
std::wstring Unescape(const std::wstring& text);

} // namespace StringUtils
