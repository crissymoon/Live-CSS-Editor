#!/Users/mac/Documents/live-css/dev-browser/venv/bin/python3
"""
Crissy's Browser - entry point.

Module layout:
    modules/command_server.py   -- HTTP command server + CMD_QUEUE
    modules/bookmark_manager.py -- BookmarkManager
    modules/password_manager.py -- PasswordManager (username + password)
    modules/browser_profile.py  -- BrowserTab, CustomWebEnginePage, PersistentProfile
    modules/main_window.py      -- MainWindow
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
    """Print the full traceback and keep the process alive (no SIGABRT)."""
    traceback.print_exception(exc_type, exc_value, exc_tb)

sys.excepthook = _excepthook

_HERE = os.path.dirname(os.path.abspath(__file__))

# ── Suppress noisy Qt / macOS framework warnings ────────────────────
# qt.qpa.fonts: "Populating font family aliases took N ms" when a page
#   requests a missing font like JetBrains Mono.
# ApplePersistenceIgnoreState: macOS complaining about savedState.
_qt_rules = os.environ.get('QT_LOGGING_RULES', '')
for _rule in ('qt.qpa.fonts=false', 'qt.webenginecontext.info=false'):
    if _rule.split('=')[0] not in _qt_rules:
        _qt_rules = (_qt_rules + ';' if _qt_rules else '') + _rule
os.environ['QT_LOGGING_RULES'] = _qt_rules
os.environ.setdefault('NSAppSleepDisabled', '1')

# ── Chromium / QtWebEngine performance flags ─────────────────────────
# Must be set before QApplication is created.
# GPU rasterization: paint/composite ops run on the GPU instead of CPU.
# Zero-copy: tiles are mapped directly from GPU memory (no CPU round-trip).
# QUIC: enables HTTP/3 where servers support it (lower latency).
# process-per-site: tabs sharing the same site reuse the renderer process
#   which cuts memory pressure and speeds up JS startup.
# memory-model=high: tells Chromium it can use more RAM for caches/JIT.
_CHROMIUM_FLAGS = [
    '--enable-gpu-rasterization',
    '--enable-zero-copy',
    '--ignore-gpu-blocklist',
    '--enable-accelerated-video-decode',
    '--num-raster-threads=4',
    # ScrollUnification: routes scroll events through the compositor thread,
    # eliminating the main-thread round-trip that causes jank on fast trackpads.
    '--enable-features=CanvasOopRasterization,UseSkiaRenderer,NetworkServiceInProcess2,BackForwardCache,LazyImageLoading,ScrollUnification,SpeculationRules,Prerender2,PrefetchProxy',
    # PaintHolding must be disabled: it holds the first paint for up to 500 ms
    # waiting for more content, which causes the visible white-flash / skip
    # whenever a new page loads or an SPA does a route change.
    # ElasticOverscroll must be disabled: Chromium's rubber-band effect fights
    # macOS's own native elastic overscroll, causing double-bounce snapping.
    '--disable-features=TranslateUI,MediaRouter,PaintHolding,ElasticOverscroll',
    '--enable-quic',
    # Smooth scrolling must stay ENABLED on macOS.
    # After the user lifts fingers from the trackpad, macOS sends many small
    # inertia/momentum scroll events.  With smooth scrolling disabled,
    # Chromium applies each event as an immediate abrupt position jump,
    # producing the visible snapping and skipping.  With smooth scrolling
    # enabled, Chromium interpolates those events into fluid motion.
    # Page JS that calls window.scrollTo({behavior:'smooth'}) is neutralised
    # separately by the _SCROLL_FIX_JS script which patches the scroll APIs.
    '--enable-smooth-scrolling',
    '--process-per-site',
    '--memory-model=high',
    '--disable-background-timer-throttling',
    '--disable-renderer-backgrounding',
    '--disable-backgrounding-occluded-windows',
    # Prevent partial/incomplete tiles from being shown during fast scrolling.
    # Without this, Chromium may composite a frame before all raster tiles are
    # ready, causing a brief white/checkerboard flash in the scroll direction.
    '--disable-partial-raster',
    '--disable-checker-imaging',
    # Prefer composited scrolling layers so the compositor thread can scroll
    # without waiting for the main thread to repaint.
    '--enable-prefer-compositing-to-lcd-text',
    # V8 bytecode cache: write script bytecode to disk on first execution
    # instead of waiting for the "heat check" (second visit) threshold.
    # Every script is cached after the first page load -- no warm-up delay.
    '--v8-cache-options=bypassHeatCheck',
    # TCP Fast Open: reuse TLS session tickets to skip the SYN/ACK round-trip
    # on repeat connections.  Saves ~50ms per new connection to known hosts.
    '--enable-tcp-fast-open',
]
_existing_flags = os.environ.get('QTWEBENGINE_CHROMIUM_FLAGS', '')
os.environ['QTWEBENGINE_CHROMIUM_FLAGS'] = (
    _existing_flags + ' ' + ' '.join(_CHROMIUM_FLAGS)
).strip()
# Suppress ApplePersistenceIgnoreState via defaults-write (takes effect next run).
try:
    subprocess.run(
        ['defaults', 'write', 'org.python.python',
         'ApplePersistenceIgnoreState', 'YES'],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        timeout=2,
    )
except Exception:
    pass

# ── WKWebView engine ─────────────────────────────────────────────────
# All the QtWebEngine-specific setup (DYLD interposer, codec proxy,
# Widevine CDM, Chromium flags, cbproxy: scheme) is no longer needed.
# WKWebView uses macOS's native WebKit which has full H.264/AAC/HEVC
# support via VideoToolbox, and uses FairPlay for DRM content.

from PyQt6.QtWidgets import QApplication

from modules.command_server import start_command_server
from modules.main_window import MainWindow


_LOGIN_URL = 'http://localhost:8080/pb_admin/login.php'
_AUTH_SCRIPT = os.path.join(os.path.dirname(_HERE), 'pb_admin', 'start-auth.sh')


def _port_open(port):
    try:
        c = socket.create_connection(('127.0.0.1', port), timeout=0.2)
        c.close()
        return True
    except OSError:
        return False


def _start_image_cache_server():
    """Start src/image-cache-server.js (Node.js) on 127.0.0.1:7779.

    Silently skips if node is not installed, the port is already in use,
    the server JS file is missing, or npm install has not been run yet.
    """
    import shutil
    node = shutil.which('node')
    if not node:
        print('[webbrowse] node not found -- image cache server will not start', flush=True)
        return
    if _port_open(7779):
        print('[webbrowse] image cache server already running on :7779', flush=True)
        return
    server_js = os.path.join(_HERE, 'src', 'image-cache-server.js')
    if not os.path.isfile(server_js):
        print(f'[webbrowse] image-cache-server.js not found at {server_js}', flush=True)
        return
    node_modules = os.path.join(_HERE, 'src', 'node_modules')
    if not os.path.isdir(node_modules):
        print('[webbrowse] src/node_modules missing -- run: cd dev-browser/src && npm install', flush=True)
        return
    proc = subprocess.Popen(
        [node, server_js],
        cwd=os.path.join(_HERE, 'src'),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    atexit.register(lambda p=proc: p.terminate() if p.poll() is None else None)
    print(f'[webbrowse] image cache server started (PID {proc.pid}) on :7779', flush=True)


def _launch_auth_servers():
    """Start start-auth.sh if the servers are not already running.
    Polls port 8080 with 0.2s intervals for up to 10 seconds."""
    if _port_open(8080) and _port_open(9100):
        print('[webbrowse] servers already running on :8080 / :9100', flush=True)
        return
    if not os.path.isfile(_AUTH_SCRIPT):
        print(f'[webbrowse] start-auth.sh not found at {_AUTH_SCRIPT}', flush=True)
        return
    proc = subprocess.Popen(
        ['bash', _AUTH_SCRIPT],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    atexit.register(lambda p=proc: p.terminate() if p.poll() is None else None)
    print('[webbrowse] waiting for PHP server on port 8080...', flush=True)
    for i in range(50):
        if _port_open(8080):
            print(f'[webbrowse] PHP server ready (after ~{i * 0.2:.1f}s)', flush=True)
            return
        time.sleep(0.2)
    print('[webbrowse] timed out waiting for PHP server - continuing anyway', flush=True)


def main():
    parser = argparse.ArgumentParser(description="Crissy's Browser")
    parser.add_argument("--port",      type=int, default=9877,
                        help="Command server port (default 9877)")
    parser.add_argument("--url",       type=str, default=None,
                        help="Initial URL to load")
    parser.add_argument("--frameless", action="store_true",
                        help="Remove window decorations (overlay mode)")
    parser.add_argument("--x",  type=int, default=None)
    parser.add_argument("--y",  type=int, default=None)
    parser.add_argument("--w",  type=int, default=1400)
    parser.add_argument("--h",  type=int, default=900)
    args = parser.parse_args()

    _start_image_cache_server()
    _launch_auth_servers()

    start_command_server(args.port)

    initial_geom = None
    if args.x is not None and args.y is not None:
        initial_geom = (args.x, args.y, args.w, args.h)

    app = QApplication(sys.argv)
    window = MainWindow(frameless=args.frameless, initial_geometry=initial_geom)

    initial_url = args.url if args.url else _LOGIN_URL
    window.navigate_to_url_str(initial_url)

    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
