import 'dart:typed_data';

import 'package:crypto/crypto.dart';
import 'package:flutter/foundation.dart';

import '../models/totp_account.dart';

class TotpCodeService {
  String codeFor(TotpAccount account, {DateTime? now}) {
    try {
      final instant = (now ?? DateTime.now()).toUtc().millisecondsSinceEpoch;
      final interval = account.period > 0 ? account.period : 30;
      final length = (account.digits >= 4 && account.digits <= 10)
          ? account.digits
          : 6;
      final counter = (instant ~/ 1000) ~/ interval;
      debugPrint(
        '[totp-code] generate issuer=${account.issuer} secretLength=${account.secret.length} interval=$interval length=$length algorithm=${account.algorithm} counter=$counter',
      );
      final secretBytes = _decodeBase32(account.secret);
      debugPrint('[totp-code] decoded secretBytes=${secretBytes.length}');
      final hmac = Hmac(_algorithm(account.algorithm), secretBytes);
      final digest = hmac.convert(_counterBytes(counter)).bytes;
      final offset = digest.last & 0x0f;
      debugPrint('[totp-code] digestLength=${digest.length} offset=$offset');
      if (offset + 3 >= digest.length) {
        debugPrint('[totp-code] invalid offset boundary offset=$offset digestLength=${digest.length}');
        return '------';
      }

      final binary = ((digest[offset] & 0x7f) << 24) |
          ((digest[offset + 1] & 0xff) << 16) |
          ((digest[offset + 2] & 0xff) << 8) |
          (digest[offset + 3] & 0xff);
      final code = (binary % _pow10(length)).toString().padLeft(length, '0');

      return code;
    } catch (error, stack) {
      debugPrint('[totp-code] generate failed error=$error');
      debugPrintStack(stackTrace: stack, label: '[totp-code] stack');
      return '------';
    }
  }

  Hash _algorithm(String value) {
    switch (value.toUpperCase()) {
      case 'SHA256':
        return sha256;
      case 'SHA512':
        return sha512;
      case 'SHA1':
      default:
        return sha1;
    }
  }

  Uint8List _decodeBase32(String value) {
    const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567';
    final unpadded = value
        .toUpperCase()
        .replaceAll(RegExp(r'[\s\-=]'), '');
    final normalized = unpadded.isEmpty
        ? unpadded
        : unpadded.padRight(((unpadded.length + 7) ~/ 8) * 8, '=');
    debugPrint('[totp-code] decodeBase32 inputLength=${value.length} unpaddedLength=${unpadded.length} normalizedLength=${normalized.length}');

    if (normalized.isEmpty) {
      throw const FormatException('Empty Base32 secret');
    }

    var buffer = 0;
    var bitsLeft = 0;
    final bytes = <int>[];

    for (final codeUnit in normalized.codeUnits) {
      final char = String.fromCharCode(codeUnit);
      if (char == '=') {
        break;
      }
      final index = alphabet.indexOf(char);
      if (index < 0) {
        debugPrint('[totp-code] invalid base32 char=$char');
        throw FormatException('Invalid Base32 character: $char');
      }

      buffer = (buffer << 5) | index;
      bitsLeft += 5;

      while (bitsLeft >= 8) {
        bytes.add((buffer >> (bitsLeft - 8)) & 0xff);
        bitsLeft -= 8;
      }
    }

    return Uint8List.fromList(bytes);
  }

  Uint8List _counterBytes(int counter) {
    final result = Uint8List(8);
    var value = counter;
    for (var index = 7; index >= 0; index--) {
      result[index] = value & 0xff;
      value = value >> 8;
    }
    return result;
  }

  int _pow10(int exponent) {
    var result = 1;
    for (var index = 0; index < exponent; index++) {
      result *= 10;
    }
    return result;
  }
}
