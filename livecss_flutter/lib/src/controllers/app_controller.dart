import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:local_auth/local_auth.dart';

import '../models/auth_models.dart';
import '../models/totp_account.dart';
import '../services/auth_repository.dart';
import '../services/totp_code_service.dart';
import '../services/totp_repository.dart';
import '../services/security_settings_repository.dart';

class AppController extends ChangeNotifier {
  AppController({
    required AuthRepository authRepository,
    required TotpRepository totpRepository,
    required SecuritySettingsRepository settingsRepository,
    required TotpCodeService codeService,
    LocalAuthentication? localAuth,
  })  : _authRepository = authRepository,
        _totpRepository = totpRepository,
        _settingsRepository = settingsRepository,
        _codeService = codeService,
        _localAuth = localAuth ?? LocalAuthentication();

  final AuthRepository _authRepository;
  final TotpRepository _totpRepository;
  final SecuritySettingsRepository _settingsRepository;
  final TotpCodeService _codeService;
  final LocalAuthentication _localAuth;

  AuthSession? _session;
  List<TotpAccount> _accounts = const [];
  SecuritySettings _settings = const SecuritySettings();

  bool _loading = true;
  bool _locked = false;
  String? _lastError;
  DateTime _backgroundedAt = DateTime.now();
  Timer? _tick;

  AuthSession? get session => _session;
  List<TotpAccount> get accounts => _accounts;
  SecuritySettings get settings => _settings;
  bool get loading => _loading;
  bool get locked => _locked;
  String? get lastError => _lastError;

  void startTicker() {
    _tick?.cancel();
    _tick = Timer.periodic(const Duration(seconds: 1), (_) {
      if (_accounts.isNotEmpty && !_locked) {
        notifyListeners();
      }
    });
  }

  void disposeTicker() {
    _tick?.cancel();
    _tick = null;
  }

  Future<void> initialize() async {
    _loading = true;
    notifyListeners();

    _settings = await _settingsRepository.loadSettings();
    _accounts = await _totpRepository.loadAccounts();
    _session = await _authRepository.loadSession();

    if (_settings.biometricsEnabled) {
      _locked = true;
      await unlockWithBiometrics();
    }

    _loading = false;
    startTicker();
    notifyListeners();
  }

  Future<LoginData> login(String identifier, String password) async {
    _lastError = null;
    notifyListeners();

    final loginData = await _authRepository.login(
      identifier: identifier,
      password: password,
    );

    if (!loginData.twoFARequired) {
      _session = await _authRepository.createSessionFromLogin(loginData);
      notifyListeners();
    }

    return loginData;
  }

  Future<AuthSession> verifyTwoFA({
    required String challengeToken,
    required String code,
  }) async {
    _session = await _authRepository.verifyTwoFA(
      challengeToken: challengeToken,
      code: code,
    );
    notifyListeners();
    return _session!;
  }

  Future<void> resendTwoFA(String challengeToken) async {
    await _authRepository.resendTwoFA(challengeToken);
  }

  Future<void> refreshSession() async {
    final current = _session;
    if (current == null) {
      return;
    }

    try {
      _session = await _authRepository.refresh(current);
      _lastError = null;
    } catch (e) {
      _lastError = e.toString();
      await signOut();
    }
    notifyListeners();
  }

  Future<void> signOut() async {
    final current = _session;
    if (current != null) {
      await _authRepository.logout(current);
    } else {
      await _authRepository.clearSession();
    }

    _session = null;
    notifyListeners();
  }

  String codeFor(TotpAccount account) {
    try {
      return _codeService.codeFor(account);
    } catch (_) {
      return '------';
    }
  }

  int secondsRemaining(TotpAccount account, {DateTime? now}) {
    final instant = now ?? DateTime.now();
    final step = account.period > 0 ? account.period : 30;
    final elapsed = instant.second % step;
    return step - elapsed;
  }

  double periodProgress(TotpAccount account, {DateTime? now}) {
    final instant = now ?? DateTime.now();
    final step = account.period > 0 ? account.period : 30;
    final elapsedMs = (instant.millisecond + instant.second * 1000) %
        (step * 1000);
    return (elapsedMs / (step * 1000)).clamp(0.0, 1.0);
  }

  Future<void> addAccount(TotpAccount account) async {
    _accounts = [..._accounts, account];
    await _totpRepository.saveAccounts(_accounts);
    notifyListeners();
  }

  TotpAccount parseOtpAuthUri(String uri) {
    return _totpRepository.parseOtpAuthUri(uri);
  }

  Future<void> removeAccount(String id) async {
    _accounts = _accounts.where((entry) => entry.id != id).toList();
    await _totpRepository.saveAccounts(_accounts);
    notifyListeners();
  }

  Future<void> updateSettings(SecuritySettings settings) async {
    _settings = settings;
    await _settingsRepository.saveSettings(settings);

    if (!settings.biometricsEnabled) {
      _locked = false;
    }

    notifyListeners();
  }

  Future<void> onAppBackgrounded() async {
    _backgroundedAt = DateTime.now();
  }

  Future<void> onAppResumed() async {
    final timeout = Duration(seconds: _settings.autoLockSeconds);
    final shouldLock = _settings.biometricsEnabled &&
        DateTime.now().difference(_backgroundedAt) >= timeout;

    if (shouldLock) {
      _locked = true;
      notifyListeners();
    }
  }

  Future<bool> unlockWithBiometrics() async {
    if (!_settings.biometricsEnabled) {
      _locked = false;
      notifyListeners();
      return true;
    }

    try {
      final canCheck = await _localAuth.canCheckBiometrics;
      final isSupported = await _localAuth.isDeviceSupported();
      if (!canCheck || !isSupported) {
        _locked = false;
        notifyListeners();
        return false;
      }

      final didAuth = await _localAuth.authenticate(
        localizedReason: 'Unlock your authenticator vault',
        options: const AuthenticationOptions(
          biometricOnly: true,
          stickyAuth: true,
          useErrorDialogs: true,
        ),
      );

      _locked = !didAuth;
      notifyListeners();
      return didAuth;
    } catch (_) {
      _locked = false;
      notifyListeners();
      return false;
    }
  }

  @override
  void dispose() {
    disposeTicker();
    super.dispose();
  }
}
