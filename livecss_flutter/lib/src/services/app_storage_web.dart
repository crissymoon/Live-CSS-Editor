import 'dart:html' as html;

import 'package:flutter/foundation.dart';

import 'app_storage_base.dart';

class _WebAppStorage implements AppStorage {
  static const String _prefix = 'livecss_auth.';

  String _fullKey(String key) => '$_prefix$key';

  @override
  Future<void> delete({required String key}) async {
    debugPrint('[app-storage-web] delete key=${_fullKey(key)}');
    html.window.localStorage.remove(_fullKey(key));
  }

  @override
  Future<String?> read({required String key}) async {
    final value = html.window.localStorage[_fullKey(key)];
    debugPrint('[app-storage-web] read key=${_fullKey(key)} valueLength=${value?.length ?? 0}');
    return value;
  }

  @override
  Future<void> write({required String key, required String? value}) async {
    final fullKey = _fullKey(key);
    debugPrint('[app-storage-web] write key=$fullKey valueLength=${value?.length ?? 0}');
    if (value == null) {
      html.window.localStorage.remove(fullKey);
      return;
    }
    html.window.localStorage[fullKey] = value;
  }
}

AppStorage createAppStorage() => _WebAppStorage();