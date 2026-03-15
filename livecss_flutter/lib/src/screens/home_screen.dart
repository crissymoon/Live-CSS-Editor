import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../controllers/app_controller.dart';
import '../models/totp_account.dart';
import '../services/otp_paste_prompt.dart';
import '../theme/crystal_theme.dart';
import '../widgets/glass_card.dart';
import 'scanner_screen.dart';
import 'settings_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({required this.controller, super.key});

  final AppController controller;

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final Set<String> _revealedAccountIds = <String>{};

  @override
  Widget build(BuildContext context) {
    final accounts = widget.controller.accounts;

    return Scaffold(
      backgroundColor: Colors.transparent,
      appBar: AppBar(
        title: const Text('Crystal Authenticator'),
        actions: [
          IconButton(
            onPressed: () {
              Navigator.of(context).push(
                MaterialPageRoute(
                  builder: (_) => SettingsScreen(controller: widget.controller),
                ),
              );
            },
            icon: const Icon(Icons.security),
          ),
          IconButton(
            onPressed: () async {
              await widget.controller.signOut();
            },
            icon: const Icon(Icons.logout),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _showAddAccountOptions,
        backgroundColor: CrystalTheme.ice,
        foregroundColor: const Color(0xFF0A1021),
        icon: const Icon(Icons.add),
        label: const Text('Add 2FA Account'),
      ),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 80),
        children: [
          _topBanner(),
          const SizedBox(height: 12),
          if (accounts.isEmpty) _emptyState(),
          ...accounts.map((account) {
            try {
              return _buildAccountCard(account);
            } catch (_) {
              return const SizedBox.shrink();
            }
          }),
        ],
      ),
    );
  }

  Widget _topBanner() {
    final user = widget.controller.session?.user;

    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Your encrypted OTP vault',
            style: TextStyle(fontSize: 22, fontWeight: FontWeight.w700),
          ),
          const SizedBox(height: 8),
          Text(
            'xcm_auth: ${user?.email ?? '-'}',
            style: const TextStyle(color: CrystalTheme.textMuted),
          ),
          const SizedBox(height: 6),
          Text(
            'Accounts: ${widget.controller.accounts.length}',
            style: const TextStyle(color: CrystalTheme.textMuted),
          ),
        ],
      ),
    );
  }

  Widget _emptyState() {
    return GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: const [
          Text(
            'No authenticators yet',
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
          ),
          SizedBox(height: 8),
          Text('Add your first otpauth QR or manual secret key to start generating OTPs.'),
        ],
      ),
    );
  }

  Widget _buildAccountCard(TotpAccount account) {
    final code = widget.controller.codeFor(account);
    final seconds = widget.controller.secondsRemaining(account);
    final progress = widget.controller.periodProgress(account);
    final obscureByDefault = widget.controller.settings.obscureCodes;
    final hidden = obscureByDefault && !_revealedAccountIds.contains(account.id);

    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: GestureDetector(
        onTap: obscureByDefault
            ? () {
                setState(() {
                  if (_revealedAccountIds.contains(account.id)) {
                    _revealedAccountIds.remove(account.id);
                  } else {
                    _revealedAccountIds.add(account.id);
                  }
                });
              }
            : null,
        child: GlassCard(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                account.issuer,
                style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
              ),
              const SizedBox(height: 4),
              Text(account.label, style: const TextStyle(color: CrystalTheme.textMuted)),
              const SizedBox(height: 12),
              Row(
                children: [
                  Expanded(
                    child: Text(
                      hidden ? '••••••' : _prettyCode(code),
                      style: const TextStyle(
                        fontSize: 32,
                        fontWeight: FontWeight.w800,
                        letterSpacing: 2,
                      ),
                    ),
                  ),
                  Text(
                    '${seconds}s',
                    style: const TextStyle(color: CrystalTheme.textMuted),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              ClipRRect(
                borderRadius: BorderRadius.circular(99),
                child: LinearProgressIndicator(
                  minHeight: 8,
                  value: progress,
                  backgroundColor: const Color(0x22FFFFFF),
                  valueColor: const AlwaysStoppedAnimation<Color>(CrystalTheme.ice),
                ),
              ),
              const SizedBox(height: 10),
              Row(
                children: [
                  TextButton.icon(
                    onPressed: hidden
                        ? null
                        : () async {
                            await Clipboard.setData(ClipboardData(text: code));
                            if (!mounted) {
                              return;
                            }
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(content: Text('${account.issuer} code copied.')),
                            );
                          },
                    icon: const Icon(Icons.copy),
                    label: const Text('Copy'),
                  ),
                  const SizedBox(width: 8),
                  TextButton.icon(
                    onPressed: () => widget.controller.removeAccount(account.id),
                    icon: const Icon(Icons.delete_outline),
                    label: const Text('Remove'),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  String _prettyCode(String raw) {
    if (raw.length != 6) {
      return raw;
    }
    return '${raw.substring(0, 3)} ${raw.substring(3)}';
  }

  Future<void> _showAddAccountOptions() async {
    showModalBottomSheet<void>(
      context: context,
      backgroundColor: const Color(0xEE0D142D),
      builder: (context) {
        return SafeArea(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                ListTile(
                  leading: const Icon(Icons.qr_code_scanner),
                  title: const Text('Scan QR code'),
                  onTap: () async {
                    Navigator.of(context).pop();
                    await _scanAndAdd();
                  },
                ),
                ListTile(
                  leading: const Icon(Icons.content_paste),
                  title: const Text('Paste otpauth URI'),
                  subtitle: const Text('Import directly from clipboard'),
                  onTap: () async {
                    Navigator.of(context).pop();
                    await _pasteAndAdd();
                  },
                ),
                ListTile(
                  leading: const Icon(Icons.edit_note),
                  title: const Text('Enter secret manually'),
                  onTap: () async {
                    Navigator.of(context).pop();
                    await _manualAdd();
                  },
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Future<void> _scanAndAdd() async {
    final raw = await Navigator.of(context).push<String>(
      MaterialPageRoute(builder: (_) => const ScannerScreen()),
    );

    if (raw == null || raw.isEmpty) {
      return;
    }

    try {
      final account = widget.controller.parseOtpAuthUri(raw);
      await widget.controller.addAccount(account);
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Added ${account.issuer} authenticator.')),
      );
    } catch (e) {
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(e.toString())),
      );
    }
  }

  Future<void> _pasteAndAdd() async {
    final nativePromptValue = await showNativeOtpPastePrompt();
    final entered = nativePromptValue ?? await _promptForOtpAuthUri();
    final raw = entered?.trim() ?? '';

    if (raw.isEmpty) {
      return;
    }

    await _addFromOtpAuthRaw(raw);
  }

  Future<String?> _promptForOtpAuthUri() async {
    final controller = TextEditingController();
    final didSave = await showDialog<bool>(
      context: context,
      builder: (context) {
        return AlertDialog(
          backgroundColor: const Color(0xFF101936),
          title: const Text('Paste otpauth URI'),
          content: TextField(
            controller: controller,
            // maxLines: null + TextInputType.multiline renders as <textarea> on
            // Flutter Web, which has stable paste handling. A fixed maxLines
            // with <input type="text"> causes a known Flutter Web RangeError
            // when pasting long URLs.
            maxLines: null,
            keyboardType: TextInputType.multiline,
            autocorrect: false,
            enableSuggestions: false,
            decoration: const InputDecoration(
              hintText: 'otpauth://totp/...',
              helperText: 'Tap and hold the field, then choose Paste.',
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () => Navigator.of(context).pop(true),
              child: const Text('Import'),
            ),
          ],
        );
      },
    );

    if (didSave != true) {
      controller.dispose();
      return null;
    }

    final value = controller.text;
    controller.dispose();
    return value;
  }

  Future<void> _addFromOtpAuthRaw(String raw) async {
    try {
      debugPrint('[totp-ui] import requested rawLength=${raw.length} rawPrefix=${raw.substring(0, raw.length > 48 ? 48 : raw.length)}');
      final account = widget.controller.parseOtpAuthUri(raw);
      debugPrint(
        '[totp-ui] parsed issuer=${account.issuer} label=${account.label} secretLength=${account.secret.length} digits=${account.digits} period=${account.period} algorithm=${account.algorithm}',
      );
      await widget.controller.addAccount(account);
      debugPrint('[totp-ui] account saved id=${account.id}');
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Added ${account.issuer} authenticator from pasted text.')),
      );
    } catch (e, stack) {
      debugPrint('[totp-ui] import failed error=$e');
      debugPrintStack(stackTrace: stack, label: '[totp-ui] import stack');
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(e.toString())),
      );
    }
  }

  Future<void> _manualAdd() async {
    final issuerController = TextEditingController();
    final labelController = TextEditingController();
    final secretController = TextEditingController();

    final didSave = await showDialog<bool>(
      context: context,
      builder: (context) {
        return AlertDialog(
          backgroundColor: const Color(0xFF101936),
          title: const Text('Add TOTP Secret'),
          content: SingleChildScrollView(
            child: Column(
              children: [
                TextField(
                  controller: issuerController,
                  decoration: const InputDecoration(labelText: 'Issuer (example: GitHub)'),
                ),
                const SizedBox(height: 10),
                TextField(
                  controller: labelController,
                  decoration: const InputDecoration(labelText: 'Account label (example: you@email.com)'),
                ),
                const SizedBox(height: 10),
                TextField(
                  controller: secretController,
                  decoration: const InputDecoration(labelText: 'Base32 secret key'),
                ),
              ],
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(false),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () => Navigator.of(context).pop(true),
              child: const Text('Add'),
            ),
          ],
        );
      },
    );

    if (didSave != true) {
      issuerController.dispose();
      labelController.dispose();
      secretController.dispose();
      return;
    }

    try {
      final uri =
          'otpauth://totp/${Uri.encodeComponent('${issuerController.text.trim()}:${labelController.text.trim()}')}?secret=${Uri.encodeQueryComponent(secretController.text.trim())}&issuer=${Uri.encodeQueryComponent(issuerController.text.trim())}';
      final account = widget.controller.parseOtpAuthUri(uri);
      await widget.controller.addAccount(account);
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Added ${account.issuer} authenticator.')),
      );
    } catch (e) {
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(e.toString())),
      );
    } finally {
      issuerController.dispose();
      labelController.dispose();
      secretController.dispose();
    }
  }
}
