import 'dart:ui';

import 'package:flutter/material.dart';

import '../theme/crystal_theme.dart';

class CrystalBackground extends StatelessWidget {
  const CrystalBackground({required this.child, super.key});

  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        Container(
          decoration: const BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: [
                CrystalTheme.midnight,
                CrystalTheme.indigo,
                CrystalTheme.ocean,
                Color(0xFF140B2F),
              ],
            ),
          ),
        ),
        const _GlowOrb(
          size: 260,
          top: -80,
          left: -40,
          color: Color(0x553C6BFF),
        ),
        const _GlowOrb(
          size: 220,
          bottom: -60,
          right: -30,
          color: Color(0x4451E2D2),
        ),
        BackdropFilter(
          filter: ImageFilter.blur(sigmaX: 8, sigmaY: 8),
          child: Container(color: Colors.transparent),
        ),
        child,
      ],
    );
  }
}

class _GlowOrb extends StatelessWidget {
  const _GlowOrb({
    required this.size,
    required this.color,
    this.top,
    this.left,
    this.right,
    this.bottom,
  });

  final double size;
  final double? top;
  final double? left;
  final double? right;
  final double? bottom;
  final Color color;

  @override
  Widget build(BuildContext context) {
    return Positioned(
      top: top,
      left: left,
      right: right,
      bottom: bottom,
      child: IgnorePointer(
        child: Container(
          width: size,
          height: size,
          decoration: BoxDecoration(
            shape: BoxShape.circle,
            gradient: RadialGradient(
              colors: [color, Colors.transparent],
            ),
          ),
        ),
      ),
    );
  }
}
