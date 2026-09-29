#pragma once

namespace Win32GdiHook {

// Installs the Win32/GDI text entry-point hooks. TranslationManager must be
// initialized before this function is called.
bool Initialize();
void Uninitialize();

} // namespace Win32GdiHook
