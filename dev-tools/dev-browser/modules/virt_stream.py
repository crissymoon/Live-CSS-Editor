"""
virt_stream.py -- Offscreen Qt WebEngine MJPEG streaming server.

Runs a QWebEngineView positioned off-screen (renders but is not visible on the
desktop).  Captures frames at ~20 fps and serves them as an MJPEG stream over
HTTP on port 9926.  A minimal HTML viewer is served at GET / that forwards
mouse and keyboard events back via POST /event.

The imgui WKWebView navigates to http://127.0.0.1:9926/ to show the
streamed content without any Qt overlay window layering issues.

Routes
------
GET  /           HTML viewer page (full-viewport canvas, no navigation chrome)
GET  /stream     MJPEG multipart/x-mixed-replace stream
POST /navigate   {"url": "https://..."}
POST /event      {"type": "mousemove"|"mousedown"|"mouseup"|"keydown"|"keyup",
                  "x": N, "y": N, "button": 0|1|2,
                  "key": "...", "code": "...", "native": N}
GET  /health
"""
from __future__ import annotations

import json
import logging
import queue
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Optional
from urllib.parse import urlparse

from PyQt6.QtCore import QByteArray, QPointF, QTimer, Qt, QUrl, pyqtSlot
from PyQt6.QtGui import QMouseEvent, QKeyEvent
from PyQt6.QtWebEngineCore import QWebEngineProfile, QWebEnginePage
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWidgets import QApplication, QMainWindow

log = logging.getLogger(__name__)

_STREAM_PORT  = 9926
_FPS          = 20
_JPEG_QUALITY = 75

# ── Off-screen page: redirect window.open back to same view ─────────────────

class _VirtPage(QWebEnginePage):
    def createWindow(self, win_type):
        return self


# ── Off-screen browser window ────────────────────────────────────────────────

class VirtBrowser(QMainWindow):
    """
    A QMainWindow with a QWebEngineView positioned off-screen.
    The window renders normally (backing store is live) but is not visible.
    Frames are captured via grab() on a QTimer and pushed into a queue.
    """

    _W = 1400
    _H = 820

    def __init__(self, profile: Optional[QWebEngineProfile] = None) -> None:
        super().__init__()

        # No title bar / taskbar entry while still being a real window.
        self.setWindowFlags(
            Qt.WindowType.Window |
            Qt.WindowType.FramelessWindowHint |
            Qt.WindowType.WindowDoesNotAcceptFocus |
            Qt.WindowType.WindowStaysOnTopHint
        )
        # Position completely off-screen so the user never sees it.
        self.setGeometry(-(self._W + 200), 0, self._W, self._H)

        page = _VirtPage(profile or QWebEngineProfile.defaultProfile(), self)
        self._view = QWebEngineView(self)
        self._view.setPage(page)
        self._view.resize(self._W, self._H)
        self.setCentralWidget(self._view)

        # Must call show() for the widget to get a backing store and render.
        self.show()
        self.move(-(self._W + 200), 0)  # push off-screen again after show

        self._frame_queue: queue.Queue[bytes] = queue.Queue(maxsize=6)

        self._capture_timer = QTimer(self)
        self._capture_timer.timeout.connect(self._capture_frame)
        self._capture_timer.start(1000 // _FPS)

        log.info("[virt_stream] off-screen browser ready (%dx%d)", self._W, self._H)

    # ── Navigation ───────────────────────────────────────────────────────────

    @pyqtSlot(str)
    def navigate(self, url: str) -> None:
        log.info("[virt_stream] navigate -> %s", url)
        self._view.setUrl(QUrl(url))

    # ── Frame capture ────────────────────────────────────────────────────────

    def _capture_frame(self) -> None:
        px = self._view.grab()
        if px.isNull():
            return
        buf = QByteArray()
        px.save(buf, "JPEG", _JPEG_QUALITY)  # type: ignore[arg-type]
        raw = bytes(buf)
        if not raw:
            return
        # Drop oldest frame if queue is full to avoid stale backlog.
        if self._frame_queue.full():
            try:
                self._frame_queue.get_nowait()
            except queue.Empty:
                pass
        try:
            self._frame_queue.put_nowait(raw)
        except queue.Full:
            pass

    def get_frame(self, timeout: float = 0.3) -> Optional[bytes]:
        try:
            return self._frame_queue.get(timeout=timeout)
        except queue.Empty:
            return None

    # ── Event forwarding ─────────────────────────────────────────────────────

    @pyqtSlot(str, int, int, int)
    def send_mouse_event(self, event_type: str, x: int, y: int, button: int) -> None:
        btn_map = {
            0: Qt.MouseButton.LeftButton,
            1: Qt.MouseButton.MiddleButton,
            2: Qt.MouseButton.RightButton,
        }
        qt_btn = btn_map.get(button, Qt.MouseButton.LeftButton)
        pos    = QPointF(float(x), float(y))
        if event_type == "mousedown":
            evt = QMouseEvent(
                QMouseEvent.Type.MouseButtonPress, pos, pos,
                qt_btn, qt_btn, Qt.KeyboardModifier.NoModifier,
            )
        elif event_type == "mouseup":
            evt = QMouseEvent(
                QMouseEvent.Type.MouseButtonRelease, pos, pos,
                qt_btn, qt_btn, Qt.KeyboardModifier.NoModifier,
            )
        else:
            evt = QMouseEvent(
                QMouseEvent.Type.MouseMove, pos, pos,
                Qt.MouseButton.NoButton, Qt.MouseButton.NoButton,
                Qt.KeyboardModifier.NoModifier,
            )
        target = self._view.focusProxy() or self._view
        QApplication.sendEvent(target, evt)

    @pyqtSlot(str, str, int)
    def send_key_event(self, event_type: str, key_text: str, native_key: int) -> None:
        evt_type = (
            QKeyEvent.Type.KeyPress
            if event_type == "keydown"
            else QKeyEvent.Type.KeyRelease
        )
        evt = QKeyEvent(
            evt_type, native_key,
            Qt.KeyboardModifier.NoModifier, key_text,
        )
        target = self._view.focusProxy() or self._view
        QApplication.sendEvent(target, evt)


# ── HTML viewer ───────────────────────────────────────────────────────────────

_VIEWER_HTML = b"""\
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>VirtStream</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
html, body { width: 100%; height: 100%; overflow: hidden; background: #111; }
#view {
    display: block;
    width: 100%;
    height: 100%;
    object-fit: contain;
    cursor: default;
    user-select: none;
}
</style>
</head>
<body>
<img id="view" draggable="false">
<script>
(function () {
    var img = document.getElementById('view');
    var polling = false;

    // JS-poll single JPEG frames.  No persistent connection so WKWebView can
    // navigate away cleanly at any time.
    function fetchFrame() {
        if (polling) return;
        polling = true;
        var xhr = new XMLHttpRequest();
        xhr.open('GET', '/frame?t=' + Date.now(), true);
        xhr.responseType = 'blob';
        xhr.onload = function () {
            if (xhr.status === 200) {
                var old = img.src;
                img.src = URL.createObjectURL(xhr.response);
                if (old && old.startsWith('blob:')) URL.revokeObjectURL(old);
            }
            polling = false;
        };
        xhr.onerror = function () { polling = false; };
        xhr.send();
    }

    setInterval(fetchFrame, 50);
    fetchFrame();

    function rel(e) {
        var r  = img.getBoundingClientRect();
        var sx = (img.naturalWidth  || r.width)  / r.width;
        var sy = (img.naturalHeight || r.height) / r.height;
        return {
            x: Math.round((e.clientX - r.left) * sx),
            y: Math.round((e.clientY - r.top)  * sy)
        };
    }

    function post(obj) {
        fetch('/event', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(obj)
        });
    }

    img.addEventListener('mousemove', function (e) {
        var p = rel(e);
        post({ type: 'mousemove', x: p.x, y: p.y });
    });
    img.addEventListener('mousedown', function (e) {
        e.preventDefault();
        var p = rel(e);
        post({ type: 'mousedown', x: p.x, y: p.y, button: e.button });
    }, true);
    img.addEventListener('mouseup', function (e) {
        e.preventDefault();
        var p = rel(e);
        post({ type: 'mouseup', x: p.x, y: p.y, button: e.button });
    }, true);
    img.addEventListener('contextmenu', function (e) { e.preventDefault(); });
    img.addEventListener('wheel', function (e) {
        e.preventDefault();
        post({ type: 'scroll', dx: e.deltaX, dy: e.deltaY });
    }, { passive: false });

    document.addEventListener('keydown', function (e) {
        e.preventDefault();
        post({ type: 'keydown', key: e.key, code: e.code, native: e.keyCode });
    });
    document.addEventListener('keyup', function (e) {
        e.preventDefault();
        post({ type: 'keyup', key: e.key, code: e.code, native: e.keyCode });
    });
}());
</script>
</body>
</html>
"""


# ── HTTP handler ──────────────────────────────────────────────────────────────

_browser: Optional[VirtBrowser] = None


class _Handler(BaseHTTPRequestHandler):

    def log_message(self, fmt, *args) -> None:  # silence access log
        pass

    def do_GET(self) -> None:
        parsed = urlparse(self.path)

        if parsed.path == "/":
            self.send_response(200)
            self.send_header("Content-Type",   "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(_VIEWER_HTML)))
            self.send_header("Cache-Control",  "no-cache")
            self.end_headers()
            self.wfile.write(_VIEWER_HTML)
            return

        if parsed.path == "/frame":
            # Single JPEG frame -- polled by JS.  Short-lived connection, no
            # persistent stream so WKWebView can navigate away cleanly.
            frame = _browser.get_frame(timeout=0.1) if _browser else None
            if frame is None:
                # Return a 1x1 transparent GIF so the img element is not broken.
                body = (b"GIF89a\x01\x00\x01\x00\x80\x00\x00\xff\xff\xff\x00"
                        b"\x00\x00!\xf9\x04\x00\x00\x00\x00\x00,\x00\x00\x00"
                        b"\x00\x01\x00\x01\x00\x00\x02\x02D\x01\x00;")
                self.send_response(200)
                self.send_header("Content-Type",   "image/gif")
                self.send_header("Content-Length", str(len(body)))
                self.send_header("Cache-Control",  "no-store")
                self.end_headers()
                self.wfile.write(body)
            else:
                self.send_response(200)
                self.send_header("Content-Type",   "image/jpeg")
                self.send_header("Content-Length", str(len(frame)))
                self.send_header("Cache-Control",  "no-store")
                self.end_headers()
                self.wfile.write(frame)
            return

        if parsed.path == "/health":
            self._ok()
            return

        self.send_response(404)
        self.end_headers()

    def do_POST(self) -> None:
        length = int(self.headers.get("Content-Length", 0))
        body   = self.rfile.read(length) if length > 0 else b"{}"
        try:
            data = json.loads(body)
        except Exception:
            data = {}

        parsed = urlparse(self.path)

        if parsed.path == "/navigate":
            url = data.get("url", "")
            if url and _browser is not None:
                # Marshal onto Qt main thread via QTimer.singleShot
                QTimer.singleShot(0, lambda: _browser.navigate(url))
            self._ok()
            return

        if parsed.path == "/event":
            if _browser is not None:
                t   = data.get("type", "")
                x   = int(data.get("x",  0))
                y   = int(data.get("y",  0))
                if t in ("mousemove", "mousedown", "mouseup"):
                    btn = int(data.get("button", 0))
                    QTimer.singleShot(
                        0, lambda: _browser.send_mouse_event(t, x, y, btn)
                    )
                elif t in ("keydown", "keyup"):
                    key_text = data.get("key",    "")
                    native   = int(data.get("native", 0))
                    QTimer.singleShot(
                        0, lambda: _browser.send_key_event(t, key_text, native)
                    )
                elif t == "scroll":
                    pass  # TODO: QWheelEvent forwarding
            self._ok()
            return

        self.send_response(404)
        self.end_headers()

    def _ok(self) -> None:
        body = b'{"ok":true}'
        self.send_response(200)
        self.send_header("Content-Type",   "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


# ── Public API ─────────────────────────────────────────────────────────────────

def start_virt_stream(
    profile: Optional[QWebEngineProfile] = None,
    port: int = _STREAM_PORT,
) -> None:
    """
    Create the off-screen VirtBrowser and start the MJPEG HTTP server.
    Must be called from the Qt main thread after QApplication exists.
    """
    global _browser
    _browser = VirtBrowser(profile)

    srv = ThreadingHTTPServer(("127.0.0.1", port), _Handler)
    t   = threading.Thread(
        target=srv.serve_forever, daemon=True, name="virt-stream-http"
    )
    t.start()
    log.info("[virt_stream] MJPEG server on 127.0.0.1:%d  viewer: http://127.0.0.1:%d/", port, port)
