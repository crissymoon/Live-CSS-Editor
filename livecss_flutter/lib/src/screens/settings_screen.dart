import 'package:flutter/material.dart';

import '../controllers/app_controller.dart';
import '../models/totp_account.dart';
import '../widgets/glass_card.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({required this.controller, super.key});

  final AppController controller;

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  late bool _biometricsEnabled;
  late bool _obscureCodes;
  late int _autoLockSeconds;
  bool _saving = false;

  static const List<int> _lockOptions = [15, 30, 60, 120, 300];

  @override
  void initState() {
    super.initState();
    final settings = widget.controller.settings;
    _biometricsEnabled = settings.biometricsEnabled;
    _obscureCodes = settings.obscureCodes;
    _autoLockSeconds = settings.autoLockSeconds;
  }

  Future<void> _save() async {
    setState(() => _saving = true);

    var next = SecuritySettings(
      biometricsEnabled: _biometricsEnabled,
      obscureCodes: _obscureCodes,
      autoLockSeconds: _autoLockSeconds,
    );

    if (_biometricsEnabled) {
      final unlocked = await widget.controller.unlockWithBiometrics();
      if (!unlocked) {
        next = next.copyWith(biometricsEnabled: false);
        _biometricsEnabled = false;
      }
    }

    await widget.controller.updateSettings(next);

    if (!mounted) {
      return;
    }

    setState(() => _saving = false);
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Security settings updated.')),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      appBar: AppBar(title: const Text('Security Settings')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          GlassCard(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'Vault Protection',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.w700),
                ),
                const SizedBox(height: 16),
                SwitchListTile(
                  value: _biometricsEnabled,
                  contentPadding: EdgeInsets.zero,
                  title: const Text('Require biometrics'),
                  subtitle: const Text('Face ID / Touch ID / fingerprint unlock.'),
                  onChanged: (value) => setState(() => _biometricsEnabled = value),
                ),
                const Divider(color: Color(0x22FFFFFF)),
                SwitchListTile(
                  value: _obscureCodes,
                  contentPadding: EdgeInsets.zero,
                  title: const Text('Obscure OTP codes by default'),
                  subtitle: const Text('Tap a code card to reveal when needed.'),
                  onChanged: (value) => setState(() => _obscureCodes = value),
                ),
                const SizedBox(height: 12),
                const Text('Auto-lock timeout'),
                const SizedBox(height: 8),
                DropdownButtonFormField<int>(
                  value: _autoLockSeconds,
                  decoration: const InputDecoration(),
                  items: _lockOptions
                      .map(
                        (seconds) => DropdownMenuItem<int>(
                          value: seconds,
                          child: Text('$seconds seconds'),
                        ),
                      )
                      .toList(),
                  onChanged: (value) {
                    if (value != null) {
                      setState(() => _autoLockSeconds = value);
                    }
                  },
                ),
                const SizedBox(height: 20),
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton(
                    onPressed: _saving ? null : _save,
                    child: Text(_saving ? 'Saving...' : 'Save Security Preferences'),
                  ),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          GlassCard(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'Connected Session',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.w700),
                ),
                const SizedBox(height: 10),
                Text('Signed in as: ${widget.controller.session?.user.email ?? '-'}'),
                const SizedBox(height: 12),
                OutlinedButton.icon(
                  onPressed: () async {
                    await widget.controller.refreshSession();
                    if (!mounted) {
                      return;
                    }
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('Session refreshed.')),
                    );
                  },
                  icon: const Icon(Icons.refresh),
                  label: const Text('Refresh API Session'),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
