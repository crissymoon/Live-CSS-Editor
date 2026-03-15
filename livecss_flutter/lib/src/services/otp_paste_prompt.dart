import 'otp_paste_prompt_stub.dart'
    if (dart.library.html) 'otp_paste_prompt_web.dart' as impl;

Future<String?> showNativeOtpPastePrompt() {
  return impl.showNativeOtpPastePrompt();
}