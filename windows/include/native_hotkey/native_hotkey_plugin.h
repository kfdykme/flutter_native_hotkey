#ifndef FLUTTER_PLUGIN_NATIVE_HOTKEY_PLUGIN_H_
#define FLUTTER_PLUGIN_NATIVE_HOTKEY_PLUGIN_H_

#include <flutter_plugin_registrar.h>

#ifdef FLUTTER_PLUGIN_IMPL
#define FLUTTER_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FLUTTER_PLUGIN_EXPORT __declspec(dllimport)
#endif

#if defined(__cplusplus)
extern "C" {
#endif 
FLUTTER_PLUGIN_EXPORT void NativeHotkeyPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar);
FLUTTER_PLUGIN_EXPORT void NativeHotKeySendMessage(const char* message);

FLUTTER_PLUGIN_EXPORT void NativeHotKeyOnWindowActiveEvent(int64_t const wparam);
#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_PLUGIN_NATIVE_HOTKEY_PLUGIN_H_
