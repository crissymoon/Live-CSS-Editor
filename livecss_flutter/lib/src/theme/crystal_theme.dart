import 'package:flutter/material.dart';

class CrystalTheme {
  static const Color midnight = Color(0xFF0B0820);
  static const Color indigo = Color(0xFF101B43);
  static const Color ocean = Color(0xFF08253A);
  static const Color glass = Color(0x1AFFFFFF);
  static const Color glassHigh = Color(0x2EFFFFFF);
  static const Color border = Color(0x33FFFFFF);
  static const Color ice = Color(0xFFA8D8FF);
  static const Color lavender = Color(0xFFC8B4FF);
  static const Color text = Color(0xFFE6F0FF);
  static const Color textMuted = Color(0xFF96AACA);

  static ThemeData get material {
    final base = ThemeData.dark(useMaterial3: true);

    return base.copyWith(
      scaffoldBackgroundColor: Colors.transparent,
      colorScheme: base.colorScheme.copyWith(
        primary: ice,
        secondary: lavender,
        surface: const Color(0xFF111936),
      ),
      textTheme: base.textTheme.apply(
        bodyColor: text,
        displayColor: text,
      ),
      cardTheme: CardThemeData(
        color: glass,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      ),
      appBarTheme: const AppBarTheme(
        backgroundColor: Colors.transparent,
        elevation: 0,
        foregroundColor: text,
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: glass,
        labelStyle: const TextStyle(color: textMuted),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(16),
          borderSide: const BorderSide(color: border),
        ),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(16),
          borderSide: const BorderSide(color: border),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(16),
          borderSide: const BorderSide(color: ice),
        ),
      ),
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: ice,
          foregroundColor: const Color(0xFF061325),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
          padding: const EdgeInsets.symmetric(horizontal: 18, vertical: 14),
        ),
      ),
      snackBarTheme: const SnackBarThemeData(
        backgroundColor: Color(0xFF1A2348),
        contentTextStyle: TextStyle(color: text),
      ),
    );
  }
}
