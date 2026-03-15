import 'package:flutter/material.dart';

import 'controllers/app_controller.dart';
import 'screens/home_screen.dart';
import 'screens/login_screen.dart';
import 'screens/lock_screen.dart';
import 'theme/crystal_theme.dart';
import 'widgets/crystal_background.dart';

class CrystalAuthenticatorApp extends StatefulWidget {
  const CrystalAuthenticatorApp({required this.controller, super.key});

  final AppController controller;

  @override
  State<CrystalAuthenticatorApp> createState() => _CrystalAuthenticatorAppState();
}

class _CrystalAuthenticatorAppState extends State<CrystalAuthenticatorApp>
    with WidgetsBindingObserver {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    widget.controller.initialize();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.paused ||
        state == AppLifecycleState.inactive ||
        state == AppLifecycleState.hidden) {
      widget.controller.onAppBackgrounded();
    }

    if (state == AppLifecycleState.resumed) {
      widget.controller.onAppResumed();
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    widget.controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: widget.controller,
      builder: (context, _) {
        return MaterialApp(
          title: 'Crystal Authenticator',
          debugShowCheckedModeBanner: false,
          theme: CrystalTheme.material,
          home: CrystalBackground(
            child: Scaffold(
              backgroundColor: Colors.transparent,
              body: SafeArea(
                child: _buildHome(),
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildHome() {
    if (widget.controller.loading) {
      return const Center(child: CircularProgressIndicator());
    }

    if (widget.controller.locked) {
      return LockScreen(controller: widget.controller);
    }

    if (widget.controller.session == null) {
      return LoginScreen(controller: widget.controller);
    }

    return HomeScreen(controller: widget.controller);
  }
}
