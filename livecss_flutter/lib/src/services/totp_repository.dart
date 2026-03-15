import 'dart:convert';
import 'dart:math';

import 'package:flutter/foundation.dart';

import '../models/totp_account.dart';
import 'app_storage.dart';
import 'app_storage_base.dart';

class TotpRepository {
  TotpRepository({AppStorage? storage}) : _storage = storage ?? createAppStorage();

  static const String _accountsKey = 'totp_accounts';
  final AppStorage _storage;

  Future<List<TotpAccount>> loadAccounts() async {
    final raw = await _storage.read(key: _accountsKey);
    debugPrint('[totp-repo] loadAccounts rawLength=${raw?.length ?? 0}');
    if (raw == null || raw.isEmpty) {
      return const [];
    }

    final decoded = jsonDecode(raw);
    if (decoded is! List<dynamic>) {
      return const [];
    }

    final accounts = <TotpAccount>[];
    for (final item in decoded.whereType<Map<String, dynamic>>()) {
      try {
        final account = TotpAccount.fromJson(item);
        if (account.secret.isNotEmpty) {
          debugPrint('[totp-repo] loaded account issuer=${account.issuer} secretLength=${account.secret.length}');
          accounts.add(account);
        }
      } catch (error, stack) {
        debugPrint('[totp-repo] skipped corrupt account error=$error item=$item');
        debugPrintStack(stackTrace: stack, label: '[totp-repo] load account stack');
        // Skip corrupted entries.
      }
    }
    return accounts;
  }

  Future<void> saveAccounts(List<TotpAccount> accounts) async {
    final payload = accounts.map((account) => account.toJson()).toList();
    debugPrint('[totp-repo] saveAccounts count=${accounts.length} payloadLength=${jsonEncode(payload).length}');
    await _storage.write(key: _accountsKey, value: jsonEncode(payload));
  }

  TotpAccount parseOtpAuthUri(String uriString) {
    final normalized = _normalizeOtpAuthInput(uriString);
    debugPrint('[totp-repo] parse rawLength=${uriString.length} normalized=$normalized');
    final uri = Uri.tryParse(normalized);
    if (uri == null || uri.scheme.toLowerCase() != 'otpauth' || uri.host.toLowerCase() != 'totp') {
      throw FormatException('This QR code is not a valid TOTP enrollment URI.');
    }

    final path = uri.path.replaceFirst('/', '');
    if (path.isEmpty) {
      throw FormatException('The account label is missing from the URI.');
    }

    final accountLabel = _safeDecodeComponent(path);
    final issuer = uri.queryParameters['issuer'] ??
        (accountLabel.contains(':') ? accountLabel.split(':').first : 'Authenticator');
    final label = accountLabel.contains(':')
        ? accountLabel.split(':').sublist(1).join(':').trim()
        : accountLabel;
    final secret = _normalizeSecret(uri.queryParameters['secret'] ?? '');
    debugPrint(
      '[totp-repo] parsed issuer=$issuer label=$label secretLength=${secret.length} digits=${uri.queryParameters['digits']} period=${uri.queryParameters['period']} algorithm=${uri.queryParameters['algorithm']}',
    );
    if (secret.isEmpty) {
      throw FormatException('The secret key is missing from the enrollment URI.');
    }
    if (!_isValidBase32(secret)) {
      throw FormatException('The secret key in the enrollment URI is not valid Base32.');
    }

    final digits = _positiveIntOrDefault(uri.queryParameters['digits'], 6);
    final period = _positiveIntOrDefault(uri.queryParameters['period'], 30);
    final algorithm = (uri.queryParameters['algorithm'] ?? 'SHA1').toUpperCase();

    return TotpAccount(
      id: _nextId(),
      issuer: issuer,
      label: label,
      secret: secret,
      digits: digits,
      period: period,
      algorithm: algorithm,
    );
  }

  String _normalizeOtpAuthInput(String input) {
    final trimmed = input.trim();
    if (trimmed.isEmpty) {
      return trimmed;
    }

    final lower = trimmed.toLowerCase();
    final start = lower.indexOf('otpauth://');
    if (start >= 0) {
      return trimmed.substring(start).trim();
    }

    return trimmed;
  }

  int _positiveIntOrDefault(String? raw, int fallback) {
    final parsed = int.tryParse(raw ?? '');
    if (parsed == null || parsed <= 0) {
      return fallback;
    }
    return parsed;
  }

  String _safeDecodeComponent(String value) {
    try {
      return Uri.decodeComponent(value);
    } on FormatException {
      return value;
    }
  }

  String _normalizeSecret(String raw) {
    final unpadded = raw.replaceAll(RegExp(r'[\s\-=]'), '').toUpperCase();
    debugPrint('[totp-repo] normalizeSecret rawLength=${raw.length} unpaddedLength=${unpadded.length}');
    if (unpadded.isEmpty) {
      return unpadded;
    }
    final padded = unpadded.padRight(((unpadded.length + 7) ~/ 8) * 8, '=');
    debugPrint('[totp-repo] normalizeSecret paddedLength=${padded.length}');
    return padded;
  }

  bool _isValidBase32(String value) {
    return RegExp(r'^[A-Z2-7]+=*$').hasMatch(value);
  }

  String _nextId() {
    final seed = DateTime.now().microsecondsSinceEpoch;
    // Keep max well within web Random.nextInt bounds.
    final random = Random(seed).nextInt(0x7fffffff);
    return '${seed}_$random';
  }
}
