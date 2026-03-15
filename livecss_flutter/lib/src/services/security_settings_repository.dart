import 'dart:convert';

import '../models/totp_account.dart';
import 'app_storage.dart';
import 'app_storage_base.dart';

class SecuritySettingsRepository {
  SecuritySettingsRepository({AppStorage? storage})
      : _storage = storage ?? createAppStorage();

  static const String _settingsKey = 'security_settings';
  final AppStorage _storage;

  Future<SecuritySettings> loadSettings() async {
    final raw = await _storage.read(key: _settingsKey);
    if (raw == null || raw.isEmpty) {
      return const SecuritySettings();
    }

    final decoded = jsonDecode(raw);
    if (decoded is! Map<String, dynamic>) {
      return const SecuritySettings();
    }
    return SecuritySettings.fromJson(decoded);
  }

  Future<void> saveSettings(SecuritySettings settings) async {
    await _storage.write(key: _settingsKey, value: jsonEncode(settings.toJson()));
  }
}
