"""
puppeteer_bridge.py  --  Python bridge to the Puppeteer CDP server.

Architecture
------------
This module replaces the WKWebView / QWebEngineView rendering backends with
native Chrome controlled via CDP (Chrome DevTools Protocol).

Why this eliminates jank
~~~~~~~~~~~~~~~~~~~~~~~~
QWebEngineView routes trackpad input through Qt's event loop before it
reaches the Chromium renderer.  WKWebView does the same through AppKit.
Both add a synchronous main-thread round-trip between hardware input and
the GPU compositor.

With this backend Chrome runs as a standalone subprocess.  Trackpad events
arrive directly at Chrome's GPU compositor, bypassing all toolkit overhead.
The WebGL spring compositor (chrome-gl-compositor.js) runs inside Chrome's
own renderer process.

How it works
~~~~~~~~~~~~
  1.  PuppeteerBridge launches puppeteer-server.js (Node.js).
  2.  It connects to the server's WebSocket on :9922.
  3.  It accepts navigation commands and forwards them as JSON.
  4.  It receives page events (url, title, load, console, error) and emits
      them as Qt signals so MainWindow can update its toolbar.
  5.  PuppeteerBridgeTab is a lightweight QWidget placeholder that occupies
      the tab's slot in QTabWidget while Chrome's native window shows behind
      the Qt shell's content area.

Window management
~~~~~~~~~~~~~~~~~
Chrome is launched with --app=about:blank (frameless, no address bar) and
positioned to fill the content area of the Qt window via the "setBounds"
command.  The Qt window provides the toolbar, tabs, and status bar.

Use the bridge
~~~~~~~~~~~~~~
    bridge = PuppeteerBridge(parent_window)
    bridge.navigate('https://example.com')
    bridge.url_changed.connect(lambda url: ...)
    bridge.title_changed.connect(lambda title: ...)

    # Position Chrome behind the Qt content area:
    bridge.set_bounds(x, y, w, h)

    # Inject JS into the current page:
    bridge.evaluate('window.__xcm.stats.showHud()')

    # Log of recent input events (from input-watcher.js):
    bridge.evaluate('JSON.stringify(window.__xcm.input.log(20))')
"""

import os
import sys
import json
import shutil
import threading
import subprocess
import atexit
import time
import datetime

from PyQt6.QtCore import (
    QObject, pyqtSignal, QThread, QTimer, Qt, QRect,
)
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QLabel, QPushButton, QApplication,
)

_HERE      = os.path.dirname(os.path.abspath(__file__))
_SRC_DIR   = os.path.join(os.path.dirname(_HERE), 'src')
_SERVER_JS = os.path.join(_SRC_DIR, 'puppeteer-server.js')
_WS_PORT   = 9922


def _ts():
    """Return a compact timestamp string for log lines."""
    return datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]


def _log(tag: str, msg: str):
    print(f'[{_ts()}][{tag}] {msg}', flush=True)

# ── Lazy websocket-client import ─────────────────────────────────────────────
# websocket-client ships as 'websocket' on pip.
def _import_websocket():
    try:
        import websocket
        return websocket
    except ImportError:
        return None


# ── WebSocket reader thread ───────────────────────────────────────────────────
class _WsReader(QThread):
    """Runs in a background thread; emits the 'message' signal when data arrives."""
    message = pyqtSignal(dict)
    connected    = pyqtSignal()
    disconnected = pyqtSignal()

    def __init__(self, port: int):
        super().__init__()
        self._port   = port
        self._ws     = None
        self._stop   = threading.Event()

    def run(self):
        ws_mod = _import_websocket()
        if ws_mod is None:
            _log('ws-reader', 'websocket-client not installed -- run: pip install websocket-client')
            return

        url = f'ws://127.0.0.1:{self._port}'
        retries = 0

        while not self._stop.is_set() and retries < 30:
            try:
                self._ws = ws_mod.WebSocket()
                _log('ws-reader', f'connecting to {url} (attempt {retries + 1})')
                self._ws.connect(url, timeout=2)
                _log('ws-reader', f'connected on attempt {retries + 1}')
                self.connected.emit()
                retries = 0

                while not self._stop.is_set():
                    try:
                        raw = self._ws.recv()
                        if raw:
                            try:
                                obj = json.loads(raw)
                                self.message.emit(obj)
                            except json.JSONDecodeError as e:
                                _log('ws-reader', f'JSON decode error: {e} | raw={raw[:120]}')
                    except Exception as e:
                        _log('ws-reader', f'recv error: {type(e).__name__}: {e}')
                        break

                self.disconnected.emit()
            except Exception as e:
                _log('ws-reader', f'connect failed (attempt {retries + 1}): {type(e).__name__}: {e}')

            retries += 1
            time.sleep(0.5)

        if retries >= 30:
            _log('ws-reader', 'gave up after 30 attempts -- is puppeteer-server.js running?')

    def send(self, obj: dict):
        if self._ws:
            try:
                self._ws.send(json.dumps(obj))
            except Exception as e:
                _log('ws-reader', f'send error: {e}')

    def stop(self):
        self._stop.set()
        if self._ws:
            try:
                self._ws.close()
            except Exception:
                pass


# ── Main bridge class ─────────────────────────────────────────────────────────
class PuppeteerBridge(QObject):
    """
    Manages the puppeteer-server.js subprocess and the WebSocket connection.

    Signals
    -------
    url_changed(str)          -- page URL changed
    title_changed(str)        -- page title changed
    load_started()            -- navigation started
    load_finished()           -- page fully loaded
    load_progress(int)        -- 0-100 percent
    console_message(str, str) -- (level, text)
    error_occurred(str)       -- error message
    ready()                   -- Chrome is up and ready
    """

    url_changed    = pyqtSignal(str)
    title_changed  = pyqtSignal(str)
    load_started   = pyqtSignal()
    load_finished  = pyqtSignal()
    load_progress  = pyqtSignal(int)
    console_message = pyqtSignal(str, str)
    error_occurred = pyqtSignal(str)
    ready          = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._proc      = None
        self._reader    = None
        self._current_url   = ''
        self._current_title = ''
        self._started   = False

    # ── Lifecycle ─────────────────────────────────────────────────────────────
    def start(self) -> bool:
        """
        Launch puppeteer-server.js and connect the WebSocket reader.
        Returns True if the subprocess started successfully.
        """
        node = shutil.which('node')
        if not node:
            print('[puppeteer_bridge] node not found', flush=True)
            return False

        if not os.path.isfile(_SERVER_JS):
            print(f'[puppeteer_bridge] server not found: {_SERVER_JS}', flush=True)
            return False

        node_modules = os.path.join(_SRC_DIR, 'node_modules')
        if not os.path.isdir(node_modules):
            print('[puppeteer_bridge] node_modules missing -- '
                  'run: cd dev-browser/src && npm install', flush=True)
            return False

        env = os.environ.copy()
        if 'CHROME_BIN' in os.environ:
            env['CHROME_BIN'] = os.environ['CHROME_BIN']

        self._proc = subprocess.Popen(
            [node, _SERVER_JS],
            cwd=_SRC_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env,
        )
        atexit.register(self.stop)

        # Stdout logger (non-blocking).
        def _log():
            for line in self._proc.stdout:
                print('[puppeteer]', line.decode('utf-8', errors='replace').rstrip(), flush=True)
        threading.Thread(target=_log, daemon=True).start()

        print(f'[puppeteer_bridge] server started (PID {self._proc.pid})', flush=True)

        # Start the WebSocket reader.
        self._reader = _WsReader(_WS_PORT)
        self._reader.message.connect(self._on_message)
        self._reader.connected.connect(self._on_connected)
        self._reader.disconnected.connect(self._on_disconnected)
        self._reader.start()

        self._started = True
        return True

    def stop(self):
        """Terminate the Chrome subprocess and the WebSocket thread."""
        if self._reader:
            self._reader.stop()
            self._reader.wait(2000)
            self._reader = None
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self._proc.kill()
            self._proc = None
        self._started = False
        print('[puppeteer_bridge] stopped', flush=True)

    def is_running(self) -> bool:
        return (self._proc is not None and self._proc.poll() is None)

    # ── Internal WebSocket message handler ───────────────────────────────────
    def _on_message(self, obj: dict):
        event = obj.get('event')

        # Log every event so nothing is silently dropped.
        if event == 'loadProgress':
            # High-frequency -- only log at 0 / 50 / 100%.
            pct = int(obj.get('percent', 0))
            if pct in (0, 50, 70, 100):
                _log('bridge', f'loadProgress {pct}%')
        elif event == 'console':
            level = obj.get('level', 'log')
            text  = obj.get('text', '')
            _log('page-console', f'[{level}] {text}')
        elif event == 'cfChallenge':
            _log('CF-CHALLENGE', f'Cloudflare challenge detected | url={obj.get("url")} status={obj.get("status")}')
        elif event == 'cfClearance':
            _log('CF-CLEARANCE', '__cf_clearance cookie received -- challenge solved')
        elif event == 'cookies':
            cookies = obj.get('cookies', [])
            names   = [c.get('name') for c in cookies]
            header  = obj.get('cookieHeader', '')
            _log('cookies', f'{len(cookies)} cookies | names={names}')
            _log('cookies', f'cookie-header ({len(header)} chars): {header[:200]}')
        elif event == 'screenshot':
            _log('bridge', f'screenshot received ({len(obj.get("data",""))} base64 chars)')
        elif event == 'evalResult':
            err = obj.get('error')
            cid = obj.get('id', '')
            if err:
                _log('eval', f'id={cid} ERROR: {err}')
            else:
                result_repr = repr(obj.get('result'))[:120]
                _log('eval', f'id={cid} result={result_repr}')
        elif event == 'error':
            _log('page-error', obj.get('msg', ''))
        elif event == 'url':
            _log('bridge', f'url -> {obj.get("url")}')
        elif event == 'title':
            _log('bridge', f'title -> {obj.get("title")}')
        elif event == 'load':
            _log('bridge', 'load (page fully loaded)')
        elif event == 'loadStart':
            _log('bridge', 'loadStart')
        elif event == 'ready':
            _log('bridge', 'Chrome ready')
        else:
            _log('bridge', f'unknown event: {obj}')

        if event == 'ready':
            self.ready.emit()
        elif event == 'url':
            self._current_url = obj.get('url', '')
            self.url_changed.emit(self._current_url)
        elif event == 'title':
            self._current_title = obj.get('title', '')
            self.title_changed.emit(self._current_title)
        elif event == 'load':
            self.load_finished.emit()
        elif event == 'loadStart':
            self.load_started.emit()
        elif event == 'loadProgress':
            self.load_progress.emit(int(obj.get('percent', 0)))
        elif event == 'console':
            self.console_message.emit(obj.get('level', 'log'), obj.get('text', ''))
        elif event == 'error':
            self.error_occurred.emit(obj.get('msg', ''))
        elif event == 'evalResult':
            pass

    def _on_connected(self):
        _log('bridge', 'WebSocket connected')

    def _on_disconnected(self):
        _log('bridge', 'WebSocket disconnected')

    # ── Public command API ────────────────────────────────────────────────────
    def _send(self, obj: dict):
        if self._reader:
            self._reader.send(obj)

    def navigate(self, url: str):
        if not url.startswith(('http://', 'https://', 'file://', 'about:', 'data:')):
            url = 'https://' + url
        _log('bridge', f'navigate -> {url}')
        self._send({'cmd': 'navigate', 'url': url})

    def back(self):
        _log('bridge', 'back')
        self._send({'cmd': 'back'})

    def forward(self):
        _log('bridge', 'forward')
        self._send({'cmd': 'forward'})

    def reload(self):
        _log('bridge', 'reload')
        self._send({'cmd': 'reload'})

    def stop_load(self):
        _log('bridge', 'stop')
        self._send({'cmd': 'stop'})

    def evaluate(self, js: str, cid: str = ''):
        self._send({'cmd': 'evaluate', 'js': js, 'id': cid})

    def screenshot(self):
        """Request a screenshot; result arrives via evalResult event."""
        self._send({'cmd': 'screenshot'})

    def set_bounds(self, x: int, y: int, w: int, h: int):
        """Position Chrome's native window to fill the content area."""
        self._send({'cmd': 'setBounds', 'x': x, 'y': y, 'w': w, 'h': h})
        self._send({'cmd': 'resize', 'w': w, 'h': h})

    def zoom_set(self, factor: float):
        self._send({'cmd': 'zoomSet', 'factor': factor})

    def find_in_page(self, text: str, forward: bool = True):
        self._send({'cmd': 'findInPage', 'text': text, 'forward': forward})

    def stop_find(self):
        self._send({'cmd': 'stopFind'})

    def clear_cache(self):
        self._send({'cmd': 'clearCache'})

    def set_user_agent(self, ua: str):
        self._send({'cmd': 'setUserAgent', 'ua': ua})

    # ── Convenience: show the WebGL HUD inside Chrome pages ──────────────────
    def show_gl_hud(self):
        self.evaluate("window.__xcmGlComp && window.__xcmGlComp.showHud()")

    def hide_gl_hud(self):
        self.evaluate("window.__xcmGlComp && window.__xcmGlComp.hideHud()")

    def gl_snapshot(self, cid: str = 'gl_snap'):
        self.evaluate("JSON.stringify(window.__xcmGlComp && window.__xcmGlComp.snapshot())", cid)

    def input_log(self, n: int = 32, cid: str = 'input_log'):
        self.evaluate(f"JSON.stringify(window.__xcm && window.__xcm.input && window.__xcm.input.log({n}))", cid)

    # ── Properties ────────────────────────────────────────────────────────────
    @property
    def current_url(self) -> str:
        return self._current_url

    @property
    def current_title(self) -> str:
        return self._current_title


# ── PuppeteerBridgeTab: QWidget placeholder inside the tab strip ──────────────
class PuppeteerBridgeTab(QWidget):
    """
    A placeholder QWidget that sits inside MainWindow's QTabWidget while
    Chrome's native window provides the actual page rendering behind/beside it.

    Emits the same signals as QWebEngineView (urlChanged, titleChanged,
    loadStarted, loadFinished, loadProgress) so it is plug-and-play with
    the existing MainWindow tab-wiring code.
    """

    # Same signature as QWebEngineView signals so MainWindow.add_new_tab wiring
    # connects without modification.
    urlChanged    = pyqtSignal(object)  # QUrl
    titleChanged  = pyqtSignal(str)
    loadStarted   = pyqtSignal()
    loadFinished  = pyqtSignal(bool)
    loadProgress  = pyqtSignal(int)

    def __init__(self, bridge: 'PuppeteerBridge', parent=None):
        super().__init__(parent)
        self._bridge = bridge
        self._setup_ui()
        self._connect_signals()

    def _setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self._status = QLabel('Connecting to Chrome...')
        self._status.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._status.setStyleSheet(
            'color: #6366f1; background: #0a0a14; font-family: monospace;'
            'font-size: 13px; padding: 40px;'
        )
        layout.addWidget(self._status)
        self.setStyleSheet('background: #0a0a14;')

    def _connect_signals(self):
        b = self._bridge
        b.ready.connect(lambda: self._set_status('Chrome ready -- content visible in Chrome window'))

        b.url_changed.connect(self._on_url_changed)
        b.title_changed.connect(self._on_title_changed)
        b.load_started.connect(self._on_load_started)
        b.load_finished.connect(self._on_load_finished)
        b.load_progress.connect(self.loadProgress)
        b.error_occurred.connect(lambda m: self._set_status(f'Error: {m}'))

    def _on_url_changed(self, url_str: str):
        from PyQt6.QtCore import QUrl
        self._set_status(f'Loading: {url_str}')
        self.urlChanged.emit(QUrl(url_str))

    def _on_title_changed(self, title: str):
        self.titleChanged.emit(title)

    def _on_load_started(self):
        self.loadStarted.emit()

    def _on_load_finished(self):
        self._set_status('Chrome window shows live page -- use log: window.__xcm.input.log(32)')
        self.loadFinished.emit(True)

    def _set_status(self, text: str):
        self._status.setText(text)

    # ── QWebEngineView-compatible navigation API ──────────────────────────────
    def setUrl(self, qurl):                             # noqa: N802
        self.load(qurl)

    def load(self, url):
        if hasattr(url, 'toString'):
            url = url.toString()
        self._bridge.navigate(str(url))

    def url(self):
        from PyQt6.QtCore import QUrl
        return QUrl(self._bridge.current_url)

    def title(self):
        return self._bridge.current_title

    def back(self):    self._bridge.back()
    def forward(self): self._bridge.forward()
    def reload(self):  self._bridge.reload()
    def stop(self):    self._bridge.stop_load()

    def runJavaScript(self, js, callback=None):  # noqa: N802
        self._bridge.evaluate(js)


# ── Global singleton helpers ──────────────────────────────────────────────────
_global_bridge: PuppeteerBridge | None = None


def get_bridge() -> PuppeteerBridge:
    """Return (creating if needed) the process-level PuppeteerBridge."""
    global _global_bridge
    if _global_bridge is None:
        _global_bridge = PuppeteerBridge()
        _global_bridge.start()
    return _global_bridge


def start_bridge() -> bool:
    """Convenience: start the bridge singleton.  Safe to call multiple times."""
    b = get_bridge()
    return b.is_running()
