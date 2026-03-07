#!/Users/mac/Documents/live-css/dev-browser/venv/bin/python3
"""
webbrowse_no_controls.py -- Full Crissy browser engine, no toolbar/tabs.

Clone of webbrowse.py with all auth / cf_bridge / WKWebView capabilities
intact.  The only difference is that the toolbar, tab bar, status bar,
and menu bar are hidden so the window shows an uncluttered content view.

Launched by webview_popup.mm (via NSTask) whenever a billing/auth URL is
intercepted in the imgui browser.

Usage:
    webbrowse_no_controls.py --url URL [--w W] [--h H] [--x X] [--y Y]
"""

import sys
import os
import socket
import time
import traceback
import argparse
import subprocess
import atexit
import signal


def _excepthook(exc_type, exc_value, exc_tb):
    traceback.print_exception(exc_type, exc_value, exc_tb)

sys.excepthook = _excepthook

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)

# ── Suppress noisy Qt / macOS framework warnings ─────────────────────
_qt_rules = os.environ.get('QT_LOGGING_RULES', '')
for _rule in ('qt.qpa.fonts=false', 'qt.webenginecontext.info=false'):
    if _rule.split('=')[0] not in _qt_rules:
        _qt_rules = (_qt_rules + ';' if _qt_rules else '') + _rule
os.environ['QT_LOGGING_RULES'] = _qt_rules
os.environ.setdefault('NSAppSleepDisabled', '1')

# ── Chromium / QtWebEngine performance flags ──────────────────────────
# Identical to webbrowse.py.  Must be set before QApplication is created.
_CHROMIUM_FLAGS = [
    '--enable-gpu-rasterization',
    '--enable-zero-copy',
    '--ignore-gpu-blocklist',
    '--enable-accelerated-video-decode',
    '--num-raster-threads=4',
    '--enable-features=CanvasOopRasterization,UseSkiaRenderer,NetworkServiceInProcess2,BackForwardCache,LazyImageLoading,ScrollUnification,SpeculationRules,Prerender2,PrefetchProxy',
    '--disable-features=TranslateUI,MediaRouter,PaintHolding,ElasticOverscroll',
    '--enable-quic',
    '--enable-smooth-scrolling',
    '--process-per-site',
    '--memory-model=high',
    '--disable-background-timer-throttling',
    '--disable-renderer-backgrounding',
    '--disable-backgrounding-occluded-windows',
    '--disable-partial-raster',
    '--disable-checker-imaging',
    '--enable-prefer-compositing-to-lcd-text',
    '--v8-cache-options=bypassHeatCheck',
    '--enable-tcp-fast-open',
]
_existing_flags = os.environ.get('QTWEBENGINE_CHROMIUM_FLAGS', '')
os.environ['QTWEBENGINE_CHROMIUM_FLAGS'] = (
    _existing_flags + ' ' + ' '.join(_CHROMIUM_FLAGS)
).strip()

try:
    subprocess.run(
        ['defaults', 'write', 'org.python.python',
         'ApplePersistenceIgnoreState', 'YES'],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        timeout=2,
    )
except Exception:
    pass

from PyQt6.QtWidgets import QApplication

from modules.cf_bridge import start_bridge as _start_cf_bridge
from modules.main_window import MainWindow


def main():
    parser = argparse.ArgumentParser(
        description='Crissy secure popup -- full engine, no controls')
    parser.add_argument('--url', required=True,
                        help='URL to open immediately')
    parser.add_argument('--w',   type=int, default=980,
                        help='Window width  (default 980)')
    parser.add_argument('--h',   type=int, default=820,
                        help='Window height (default 820)')
    parser.add_argument('--x',   type=int, default=None)
    parser.add_argument('--y',   type=int, default=None)
    args = parser.parse_args()

    initial_geom = None
    if args.x is not None and args.y is not None:
        initial_geom = (args.x, args.y, args.w, args.h)

    app = QApplication(sys.argv)

    # Start the hidden Chromium cf_bridge -- same as webbrowse.py.
    # This provides real Cloudflare clearance cookies for billing/auth pages.
    _start_cf_bridge(app)

    window = MainWindow(frameless=False, initial_geometry=initial_geom)

    # Signal to createWindow in _page.py that all new-window requests
    # must open as floating popup windows (no visible tab bar to click).
    window._popup_mode = True

    # ── Hide all chrome: toolbar, tab bar, status bar, menu bar ──────
    window.navbar.hide()
    window.tabs.tabBar().hide()
    window.statusBar().hide()
    window.menuBar().hide()

    # Resize to popup dimensions when no explicit geometry was given.
    if initial_geom is None:
        window.resize(args.w, args.h)
        window.setGeometry(
            # Center on primary screen
            QApplication.primaryScreen().geometry().center().x() - args.w // 2,
            QApplication.primaryScreen().geometry().center().y() - args.h // 2,
            args.w, args.h,
        )

    # Derive a human-readable title from the URL host.
    try:
        from urllib.parse import urlparse
        _host = urlparse(args.url).netloc or args.url
        window.setWindowTitle(_host)
    except Exception:
        window.setWindowTitle('Secure Window')

    window.navigate_to_url_str(args.url)
    window.showMaximized()
    window.raise_()
    window.activateWindow()

    sys.exit(app.exec())


if __name__ == '__main__':
    main()
