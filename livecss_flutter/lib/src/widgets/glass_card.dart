import 'dart:ui';

import 'package:flutter/material.dart';

import '../theme/crystal_theme.dart';

class GlassCard extends StatelessWidget {
  const GlassCard({required this.child, this.padding, super.key});

  final Widget child;
  final EdgeInsetsGeometry? padding;

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: BorderRadius.circular(22),
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: 20, sigmaY: 20),
        child: Material(
          type: MaterialType.transparency,
          child: Container(
            padding: padding ?? const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: CrystalTheme.glass,
              borderRadius: BorderRadius.circular(22),
              border: Border.all(color: CrystalTheme.border),
              boxShadow: const [
                BoxShadow(
                  color: Color(0x66000000),
                  blurRadius: 22,
                  offset: Offset(0, 8),
                ),
              ],
            ),
            child: child,
          ),
        ),
      ),
    );
  }
}
