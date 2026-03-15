import 'dart:ui';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import 'src/app.dart';
import 'src/controllers/app_controller.dart';
import 'src/services/api_client.dart';
import 'src/services/auth_repository.dart';
import 'src/services/security_settings_repository.dart';
import 'src/services/totp_code_service.dart';
import 'src/services/totp_repository.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();

  // Catch Flutter framework errors (build / layout / paint).
  FlutterError.onError = (details) {
    debugPrint('[livecss] FlutterError: ${details.exceptionAsString()}');
  };

  // Catch platform / async errors that escape the framework.
  PlatformDispatcher.instance.onError = (error, stack) {
    debugPrint('[livecss] PlatformError: $error');
    return true; // handled -- do not crash
  };

  final apiClient = ApiClient();
  final authRepository = AuthRepository(apiClient: apiClient);
  final totpRepository = TotpRepository();
  final settingsRepository = SecuritySettingsRepository();
  final codeService = TotpCodeService();

  final controller = AppController(
    authRepository: authRepository,
    totpRepository: totpRepository,
    settingsRepository: settingsRepository,
    codeService: codeService,
  );

  runApp(CrystalAuthenticatorApp(controller: controller));
}
