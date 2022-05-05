library native_hotkey;

import 'dart:convert';

import 'package:flutter/services.dart';
import 'package:native_hotkey/constants.dart';



class NativeHotkey {

  late MethodChannel mMethodChannel;

  NativeHotkey() {
    mMethodChannel = const MethodChannel(kMethodChannelName);
  } 

  Map<int,Function> callbacks = new Map();

  Future<void> init() async { 
    mMethodChannel.setMethodCallHandler((call)  {
      // print("${call.method} ${call.arguments}");
      var result = Future<dynamic>(() {
        return false;
      });
      if (call.method == 'onNativeHotKeySendMessage') {
        Map<String, dynamic> eventData = jsonDecode(call.arguments);
        if (eventData['event'] == "WM_ACTIVATE") {
          if (eventData['value'] == 0) {
            result = mMethodChannel.invokeMethod<bool>('keydownEvent',<String, Object>{'status': 'stop'});
          } else if (eventData['value'] == 1) {
            result = mMethodChannel.invokeMethod<bool>('keydownEvent',<String, Object>{'status': 'resume'});
          }
        }
      }
      if (call.method == 'onHotkeyCallback') {
        String callbackIdStr = call.arguments;
        // print("onNativeHotKey ${callbackIdStr}");
        Function? callback = callbacks[int.parse(callbackIdStr)];
        if (callback != null) {
          callback.call();
        }
      }
      return result;
    });

    // int? res = await mMethodChannel.invokeMethod<int>('registerKeydownEvent', <String, Object>{'hotkey': 'ctrl-s', 'callbackId': 1}); 
  }

  Future<int?> setHotkeyListener(String hotkey, Function func) {
    int callbackId = callbacks.length+1;
    callbacks[callbackId] = func;
    var res = mMethodChannel.invokeMethod<int>('registerKeydownEvent', <String, Object>{'hotkey': hotkey, 'callbackId': callbackId});
    return res;
  }
  
  static NativeHotkey? _instance = null;

  static NativeHotkey get instance {
    _instance ??= NativeHotkey();
    return _instance!;
  }
}