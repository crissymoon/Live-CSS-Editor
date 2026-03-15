import '../lib/src/services/totp_code_service.dart';
import '../lib/src/services/totp_repository.dart';

void main() {
  const raw = 'otpauth://totp/XcaliburMoon%20Test%20Site%3ACrissy%20Login?secret=JBSWY3DPEHPK3PXP&issuer=XcaliburMoon+Test+Site&algorithm=SHA1&digits=6&period=30';
  final repository = TotpRepository();
  final account = repository.parseOtpAuthUri(raw);
  final code = TotpCodeService().codeFor(account, now: DateTime.utc(2026, 3, 14, 12, 0, 0));

  print('issuer=${account.issuer}');
  print('label=${account.label}');
  print('secret=${account.secret}');
  print('digits=${account.digits}');
  print('period=${account.period}');
  print('algorithm=${account.algorithm}');
  print('code=$code');
}
