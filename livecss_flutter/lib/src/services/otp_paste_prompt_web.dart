import 'dart:js_interop';

@JS('window.prompt')
external JSString? _prompt(JSString message, JSString defaultValue);

Future<String?> showNativeOtpPastePrompt() async {
  return _prompt('Paste the otpauth:// URI'.toJS, ''.toJS)?.toDart;
}