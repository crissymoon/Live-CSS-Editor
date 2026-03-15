import 'package:flutter_secure_storage/flutter_secure_storage.dart';

import 'app_storage_base.dart';

class _IoAppStorage implements AppStorage {
  _IoAppStorage({FlutterSecureStorage? storage})
      : _storage = storage ?? const FlutterSecureStorage();

  final FlutterSecureStorage _storage;

  @override
  Future<void> delete({required String key}) {
    return _storage.delete(key: key);
  }

  @override
  Future<String?> read({required String key}) {
    return _storage.read(key: key);
  }

  @override
  Future<void> write({required String key, required String? value}) {
    return _storage.write(key: key, value: value);
  }
}

AppStorage createAppStorage() => _IoAppStorage();