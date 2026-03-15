import 'package:flutter/material.dart';

import '../controllers/app_controller.dart';
import 'verify_2fa_screen.dart';
import '../widgets/glass_card.dart';

class LoginScreen extends StatefulWidget {
  const LoginScreen({required this.controller, super.key});

  final AppController controller;

  @override
  State<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  final TextEditingController _identifierController = TextEditingController();
  final TextEditingController _passwordController = TextEditingController();
  bool _submitting = false;

  void _showMessage(String message) {
    final messenger = ScaffoldMessenger.maybeOf(context);
    if (messenger != null) {
      messenger.showSnackBar(SnackBar(content: Text(message)));
      return;
    }

    showDialog<void>(
      context: context,
      builder: (dialogContext) {
        return AlertDialog(
          title: const Text('Notice'),
          content: Text(message),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(dialogContext).pop(),
              child: const Text('OK'),
            ),
          ],
        );
      },
    );
  }

  @override
  void dispose() {
    _identifierController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _submit() async {
    final identifier = _identifierController.text.trim();
    final password = _passwordController.text;

    if (identifier.isEmpty || password.isEmpty) {
      _showMessage('Email/username and password are required.');
      return;
    }

    setState(() => _submitting = true);

    try {
      final loginData = await widget.controller.login(identifier, password);
      if (loginData.twoFARequired && (loginData.challengeToken?.isNotEmpty ?? false)) {
        if (!mounted) {
          return;
        }

        await Navigator.of(context).push<bool>(
          MaterialPageRoute(
            builder: (_) => VerifyTwoFAScreen(
              controller: widget.controller,
              challengeToken: loginData.challengeToken!,
            ),
          ),
        );
      }
    } catch (e) {
      if (!mounted) {
        return;
      }
      _showMessage(e.toString());
    } finally {
      if (mounted) {
        setState(() => _submitting = false);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Center(
      child: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 420),
          child: GlassCard(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                const Text(
                  'Crystal Authenticator',
                  style: TextStyle(fontSize: 28, fontWeight: FontWeight.w800),
                ),
                const SizedBox(height: 10),
                const Text(
                  'Sign in to your xcm_auth account, then manage your local authenticator vault.',
                ),
                const SizedBox(height: 20),
                TextField(
                  controller: _identifierController,
                  keyboardType: TextInputType.emailAddress,
                  decoration: const InputDecoration(labelText: 'Email or Username'),
                ),
                const SizedBox(height: 12),
                TextField(
                  controller: _passwordController,
                  obscureText: true,
                  decoration: const InputDecoration(labelText: 'Password'),
                ),
                const SizedBox(height: 20),
                ElevatedButton(
                  onPressed: _submitting ? null : _submit,
                  child: Text(_submitting ? 'Signing In...' : 'Sign In'),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
