import 'package:flutter/material.dart';
import 'package:mobile_scanner/mobile_scanner.dart';

import '../widgets/glass_card.dart';

class ScannerScreen extends StatefulWidget {
  const ScannerScreen({super.key});

  @override
  State<ScannerScreen> createState() => _ScannerScreenState();
}

class _ScannerScreenState extends State<ScannerScreen> {
  bool _done = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.transparent,
      appBar: AppBar(title: const Text('Scan Enrollment QR')),
      body: Column(
        children: [
          Expanded(
            child: ClipRRect(
              borderRadius: BorderRadius.circular(24),
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: MobileScanner(
                  onDetect: (capture) {
                    if (_done) {
                      return;
                    }
                    if (capture.barcodes.isEmpty) {
                      return;
                    }
                    final value = capture.barcodes.first.rawValue;
                    if (value == null || value.isEmpty) {
                      return;
                    }
                    _done = true;
                    Navigator.of(context).pop(value);
                  },
                ),
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(16),
            child: GlassCard(
              child: const Text(
                'Scan an otpauth:// QR code from your account provider.',
                textAlign: TextAlign.center,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
