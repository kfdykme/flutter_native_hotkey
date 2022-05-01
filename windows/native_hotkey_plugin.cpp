#include "include/native_hotkey/native_hotkey_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>
#include "libloaderapi.h"

typedef const void *register_func_type(const char *key, int32_t callback_id);
typedef void *rust_hotkey_callback_func(int32_t);
typedef const void *init_func_type(rust_hotkey_callback_func func);
typedef const void *keydown_stop_type();
typedef const void *keydown_resume_type();
namespace
{

  enum NativeHotkeyEventCode
  {
    Success = 1,
    ParamError = 2,
    Error = 0
  };

  struct NativeHotkeyEvent_struct
  {
    const char *keydownEvent = "keydownEvent";
    const char *registerKeydownEvent = "registerKeydownEvent";
    const char *register_key = "register_key";
    const char *init_invoke_callback = "init_invoke_callback";
    const char *keydown_stop_keyevent = "keydown_stop_keyevent";
    const char *keydown_resumt_keyevent = "keydown_resume_keyevent";
  };

  NativeHotkeyEvent_struct NativeHotkeyEvent;

  class NativeHotkeyPlugin : public flutter::Plugin
  {
  public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);
    static std::shared_ptr<flutter::MethodChannel<flutter::EncodableValue>> S_CHANNEL;

    NativeHotkeyPlugin();

    virtual ~NativeHotkeyPlugin();

  private:
    // Called when a method is called on this plugin's channel from Dart.
    void HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue> &method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
    void InitRustLibrary();
    void *GetFuncByName(const char *name);

  private:
    std::shared_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_ = nullptr;
    register_func_type *register_key_func_ = nullptr;
    init_func_type *init_func_ = nullptr;
    keydown_resume_type *keydown_resume_keyevent_func_ = nullptr;
    keydown_stop_type *keydown_stop_keyevent_func_ = nullptr;
    HMODULE libkeydown_handle_ = NULL;
  };

  // static
  std::shared_ptr<flutter::MethodChannel<flutter::EncodableValue>> NativeHotkeyPlugin::S_CHANNEL = nullptr;

  // static
  void NativeHotkeyPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarWindows *registrar)
  {
    std::cout << "native_hotkey registrar" << std::endl;

    auto plugin = std::make_unique<NativeHotkeyPlugin>();
    plugin->channel_ =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "native_hotkey",
            &flutter::StandardMethodCodec::GetInstance());
    plugin->channel_->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result)
        {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });
    NativeHotkeyPlugin::S_CHANNEL = plugin->channel_;
    registrar->AddPlugin(std::move(plugin));
  }

  NativeHotkeyPlugin::NativeHotkeyPlugin()
  {
    InitRustLibrary();
  }

  void *NativeHotkeyPlugin::GetFuncByName(const char *name)
  {
    return GetProcAddress(libkeydown_handle_, name);
  }

  void NativeHotkeyPlugin::InitRustLibrary()
  {

    libkeydown_handle_ = LoadLibraryExA("libkeydown", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (libkeydown_handle_)
    {
      std::cout << "load libkeydown success" << std::endl;
    }
    else
    {
      std::cout << "load libkeydown fail" << std::endl;
      return;
    }

    register_key_func_ = (register_func_type *)GetFuncByName("register_key");
    init_func_ = (init_func_type *)GetFuncByName("init_invoke_callback");
    keydown_stop_keyevent_func_ = (keydown_stop_type *)GetFuncByName("keydown_stop_keyevent");
    keydown_resume_keyevent_func_ = (keydown_resume_type *)GetFuncByName("keydown_resume_keyenvent");

    void (*f)(int32_t) = [](int32_t id)
    {
      if (NativeHotkeyPlugin::S_CHANNEL)
      {
        NativeHotkeyPlugin::S_CHANNEL->InvokeMethod("onHotkeyCallback", std::move(std::make_unique<flutter::EncodableValue>(std::to_string(id))));
      }
    };
    init_func_((rust_hotkey_callback_func *)f);
  }

  NativeHotkeyPlugin::~NativeHotkeyPlugin() {}

  void NativeHotkeyPlugin::HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {
    // std::cout << "NativeHotkeyPlugin::HandleMethodCall" << std::endl;
    if (method_call.method_name().compare(NativeHotkeyEvent.keydownEvent) == 0)
    {
      // method_call.arguments()
      // std::cout << method_call.method_name() << std::endl;
      std::string status{};
      const auto *arguments = std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (arguments)
      {
        auto status_it = arguments->find(flutter::EncodableValue("status"));
        if (status_it != arguments->end())
        {
          status = std::get<std::string>(status_it->second);
          if (status.compare("resume") == 0)
          {
            keydown_resume_keyevent_func_();
          }
          if (status.compare("stop") == 0)
          {
            keydown_stop_keyevent_func_();
          }
        }
      }
      result->Success(flutter::EncodableValue(NativeHotkeyEventCode::Success));
      return;
    }

    if (method_call.method_name().compare(NativeHotkeyEvent.registerKeydownEvent) == 0)
    {
      std::string hotkey{};
      int32_t callback_id = -1;
      const auto *arguments = std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (arguments)
      {
        auto hotkey_it = arguments->find(flutter::EncodableValue("hotkey"));
        if (hotkey_it != arguments->end())
        {
          hotkey = std::get<std::string>(hotkey_it->second);
        }
        auto callback_id_it = arguments->find(flutter::EncodableValue("callbackId"));
        if (callback_id_it != arguments->end())
        {
          callback_id = std::get<int32_t>(callback_id_it->second);
        }
        if (!hotkey.empty() && callback_id != -1)
        {
          register_key_func_(hotkey.c_str(), callback_id);
          result->Success(flutter::EncodableValue(NativeHotkeyEventCode::Success));
          return;
        }
        else
        {
          // result->Success(flutter::EncodableValue(NativeHotkeyEventCode::ParamError));
          result->Error(std::to_string(NativeHotkeyEventCode::ParamError));
          return;
        }
      }
    }

    result->NotImplemented();
  }

} // namespace

void NativeHotkeyPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar)
{
  NativeHotkeyPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

void NativeHotKeySendMessage(const char *message)
{
  if (NativeHotkeyPlugin::S_CHANNEL)
  {
    NativeHotkeyPlugin::S_CHANNEL->InvokeMethod("onNativeHotKeySendMessage", std::move(std::make_unique<flutter::EncodableValue>(std::string(message))));
  }
}

void NativeHotKeyOnWindowActiveEvent(int64_t const wparam)
{
  if (wparam == WA_INACTIVE)
  {
    NativeHotKeySendMessage("{ \"event\": \"WM_ACTIVATE\", \"value\": 0 }");
  }
  else
  {
    NativeHotKeySendMessage("{ \"event\": \"WM_ACTIVATE\", \"value\": 1 }");
  }
}