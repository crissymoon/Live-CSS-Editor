import 'package:flutter/material.dart';

import '../controllers/app_controller.dart';
import '../widgets/glass_card.dart';

class VerifyTwoFAScreen extends StatefulWidget {
  const VerifyTwoFAScreen({
    required this.controller,
    required this.challengeToken,
    super.key,
  });

  final AppController controller;
  final String challengeToken;

  @override
  State<VerifyTwoFAScreen> createState() => _VerifyTwoFAScreenState();
}

class _VerifyTwoFAScreenState extends State<VerifyTwoFAScreen> {
  final TextEditingController _codeController = TextEditingController();
  bool _submitting = false;

  @override
  void dispose() {
    _codeController.dispose();
    super.dispose();
  }

  Future<void> _verify() async {
    final code = _codeController.text.trim();
    if (code.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Enter the verification code first.')),
      );
      return;
    }

    setState(() => _submitting = true);
    try {
      await widget.controller.verifyTwoFA(
        challengeToken: widget.challengeToken,
        code: code,
      );
      if (!mounted) {
        return;
      }
      Navigator.of(context).pop(true);
    } catch (e) {
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(e.toString())),
      );
    } finally {
      if (mounted) {
        setState(() => _submitting = false);
      }
    }
  }

  Future<void> _resend() async {
    try {
      await widget.controller.resendTwoFA(widget.challengeToken);
      if (!mounted) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('A fresh 2FA code has been sent.')),
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

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: Center(
        child: SingleChildScrollView(
          padding: const EdgeInsets.all(20),
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 420),
            child: GlassCard(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  const Text(
                    'Two-Factor Verification',
                    style: TextStyle(fontSize: 24, fontWeight: FontWeight.w700),
                  ),
                  const SizedBox(height: 10),
                  const Text(
                    'Enter the one-time code that xcm_auth sent to your email.',
                  ),
                  const SizedBox(height: 20),
                  TextField(
                    controller: _codeController,
                    keyboardType: TextInputType.number,
                    decoration: const InputDecoration(labelText: '2FA Code'),
                  ),
                  const SizedBox(height: 18),
                  ElevatedButton(
                    onPressed: _submitting ? null : _verify,
                    child: Text(_submitting ? 'Verifying...' : 'Verify and Continue'),
                  ),
                  TextButton(
                    onPressed: _submitting ? null : _resend,
                    child: const Text('Resend code'),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}
