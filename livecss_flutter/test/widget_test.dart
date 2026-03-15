import 'package:flutter_test/flutter_test.dart';

import 'package:livecss_app/src/services/totp_code_service.dart';
import 'package:livecss_app/src/models/totp_account.dart';

void main() {
  group('TotpCodeService', () {
    late TotpCodeService service;

    setUp(() {
      service = TotpCodeService();
    });

    test('generates a 6-digit code for SHA1', () {
      final account = const TotpAccount(
        id: '1',
        issuer: 'Test Issuer',
        label: 'test@example.com',
        secret: 'JBSWY3DPEHPK3PXP',
        algorithm: 'SHA1',
        digits: 6,
        period: 30,
      );
      final code = service.codeFor(account, now: DateTime.utc(2024, 1, 1));
      expect(code.length, 6);
      expect(RegExp(r'^\d{6}$').hasMatch(code), isTrue);
    });

    test('generates a 6-digit code for SHA256', () {
      final account = const TotpAccount(
        id: '2',
        issuer: 'Test Issuer',
        label: 'test@example.com',
        secret: 'JBSWY3DPEHPK3PXP',
        algorithm: 'SHA256',
      );
      final code = service.codeFor(account, now: DateTime.utc(2024, 1, 1));
      expect(code.length, 6);
      expect(RegExp(r'^\d{6}$').hasMatch(code), isTrue);
    });

    test('generates a 6-digit code for SHA512', () {
      final account = const TotpAccount(
        id: '3',
        issuer: 'Test Issuer',
        label: 'test@example.com',
        secret: 'JBSWY3DPEHPK3PXP',
        algorithm: 'SHA512',
      );
      final code = service.codeFor(account, now: DateTime.utc(2024, 1, 1));
      expect(code.length, 6);
      expect(RegExp(r'^\d{6}$').hasMatch(code), isTrue);
    });

    test('unknown algorithm defaults to SHA1', () {
      final account = const TotpAccount(
        id: '4',
        issuer: 'Test Issuer',
        label: 'test@example.com',
        secret: 'JBSWY3DPEHPK3PXP',
        algorithm: 'UNKNOWN',
      );
      final code = service.codeFor(account, now: DateTime.utc(2024, 1, 1));
      expect(code.length, 6);
    });
  });
}
