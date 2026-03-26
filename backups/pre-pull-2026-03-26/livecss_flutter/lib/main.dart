import 'dart:io';
import 'package:flutter/material.dart';
import 'package:webview_flutter/webview_flutter.dart';

void main() {
  runApp(const LiveCSSApp());
}

// Local dev server URL.
// Android emulator reaches the Mac host via 10.0.2.2.
// iOS simulator uses localhost directly.
// Real device over Wi-Fi: pass --dart-define=DEV_HOST=192.168.x.x at build time.
const String _devHost = String.fromEnvironment('DEV_HOST', defaultValue: '');

String get _baseUrl {
  if (_devHost.isNotEmpty) {
    return 'http://$_devHost:8080/my_project/index.php';
  }
  if (Platform.isAndroid) {
    return 'http://10.0.2.2:8080/my_project/index.php';
  }
  return 'http://localhost:8080/my_project/index.php';
}

class LiveCSSApp extends StatelessWidget {
  const LiveCSSApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Live CSS',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF6a9fb5),
          brightness: Brightness.dark,
        ),
        useMaterial3: true,
      ),
      home: const AppShell(),
    );
  }
}

class AppShell extends StatefulWidget {
  const AppShell({super.key});

  @override
  State<AppShell> createState() => _AppShellState();
}

class _AppShellState extends State<AppShell> {
  late final WebViewController _controller;
  bool _loading = true;
  bool _hasError = false;
  String _errorMessage = '';

  @override
  void initState() {
    super.initState();

    _controller = WebViewController()
      ..setJavaScriptMode(JavaScriptMode.unrestricted)
      ..setBackgroundColor(const Color(0xFF111111))
      ..setNavigationDelegate(
        NavigationDelegate(
          onPageStarted: (_) {
            setState(() {
              _loading = true;
              _hasError = false;
            });
          },
          onPageFinished: (_) {
            setState(() { _loading = false; });
          },
          onWebResourceError: (WebResourceError error) {
            if (error.isForMainFrame == true) {
              setState(() {
                _loading = false;
                _hasError = true;
                _errorMessage = error.description;
              });
            }
          },
        ),
      )
      ..loadRequest(Uri.parse(_baseUrl));
  }

  @override
  Widget build(BuildContext context) {
    return PopScope(
      canPop: false,
      onPopInvokedWithResult: (didPop, _) async {
        if (!didPop) {
          if (await _controller.canGoBack()) {
            await _controller.goBack();
          }
        }
      },
      child: Scaffold(
        backgroundColor: const Color(0xFF111111),
        body: SafeArea(
          child: Stack(
            children: [
              if (!_hasError)
                WebViewWidget(controller: _controller),

              if (_loading && !_hasError)
                const _LoadingOverlay(),

              if (_hasError)
                _ErrorView(
                  message: _errorMessage,
                  url: _baseUrl,
                  onRetry: () {
                    setState(() {
                      _hasError = false;
                      _loading = true;
                    });
                    _controller.reload();
                  },
                ),
            ],
          ),
        ),
      ),
    );
  }
}

class _LoadingOverlay extends StatelessWidget {
  const _LoadingOverlay();

  @override
  Widget build(BuildContext context) {
    return Container(
      color: const Color(0xFF111111),
      child: const Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            SizedBox(
              width: 40,
              height: 40,
              child: CircularProgressIndicator(
                strokeWidth: 2.5,
                color: Color(0xFF6a9fb5),
              ),
            ),
            SizedBox(height: 20),
            Text(
              'LOADING',
              style: TextStyle(
                color: Color(0xFF888888),
                fontSize: 12,
                letterSpacing: 2.0,
                fontFamily: 'monospace',
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _ErrorView extends StatelessWidget {
  final String message;
  final String url;
  final VoidCallback onRetry;

  const _ErrorView({
    required this.message,
    required this.url,
    required this.onRetry,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      color: const Color(0xFF111111),
      padding: const EdgeInsets.all(32),
      child: Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text(
              'Could not connect to the local server.',
              textAlign: TextAlign.center,
              style: TextStyle(color: Color(0xFFd4d4d4), fontSize: 15),
            ),
            const SizedBox(height: 12),
            Text(
              url,
              textAlign: TextAlign.center,
              style: const TextStyle(
                color: Color(0xFF888888),
                fontSize: 11,
                fontFamily: 'monospace',
              ),
            ),
            const SizedBox(height: 8),
            Text(
              message,
              textAlign: TextAlign.center,
              style: const TextStyle(color: Color(0xFF666666), fontSize: 11),
            ),
            const SizedBox(height: 32),
            TextButton(
              onPressed: onRetry,
              style: TextButton.styleFrom(
                foregroundColor: const Color(0xFF6a9fb5),
              ),
              child: const Text('Retry'),
            ),
          ],
        ),
      ),
    );
  }
}
