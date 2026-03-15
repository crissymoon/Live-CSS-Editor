import 'package:flutter/material.dart';

import '../controllers/app_controller.dart';
import '../widgets/glass_card.dart';

class LockScreen extends StatefulWidget {
  const LockScreen({required this.controller, super.key});

  final AppController controller;

  @override
  State<LockScreen> createState() => _LockScreenState();
}

class _LockScreenState extends State<LockScreen> {
  bool _unlocking = false;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 420),
        child: GlassCard(
          child: Padding(
            padding: const EdgeInsets.all(8),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Icon(Icons.lock, size: 48),
                const SizedBox(height: 16),
                const Text(
                  'Vault Locked',
                  style: TextStyle(fontSize: 24, fontWeight: FontWeight.w700),
                ),
                const SizedBox(height: 8),
                const Text(
                  'Authenticate with biometrics to access your 2FA codes.',
                  textAlign: TextAlign.center,
                ),
                const SizedBox(height: 20),
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton.icon(
                    onPressed: _unlocking
                        ? null
                        : () async {
                            setState(() => _unlocking = true);
                            await widget.controller.unlockWithBiometrics();
                            if (mounted) {
                              setState(() => _unlocking = false);
                            }
                          },
                    icon: const Icon(Icons.fingerprint),
                    label: Text(_unlocking ? 'Unlocking...' : 'Unlock'),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
