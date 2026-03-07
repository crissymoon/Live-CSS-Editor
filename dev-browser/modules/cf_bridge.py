"""
cf_bridge.py -- Hidden Chromium WebEngine worker for Cloudflare Turnstile.

WHY THIS EXISTS
---------------
WKWebView uses Apple's JavaScriptCore (JSC) engine and Network.framework TLS.
Cloudflare Turnstile runs probe scripts that detect the JSC engine and the
Apple TLS JA3 fingerprint as non-Chromium and issues a challenge that JSC
cannot solve.

Qt WebEngine embeds real Chromium (Blink + V8 + BoringSSL). The TLS JA3 hash
matches real Chrome and V8 is the engine Turnstile expects. Turnstile passes
automatically with no user interaction.

ARCHITECTURE
------------
  1. A hidden QWebEngineView (1x1 pixels, off-screen window) loads the target
     URL in Chromium.
  2. The cookieAdded signal fires when __cf_clearance is set.
  3. All cookies for the domain are harvested and returned as JSON.
  4. An HTTP server on port 9925 exposes /solve?url=X blocking until done.
     The imgui C++ browser calls this endpoint, gets the cookies back as JSON,
     and injects them into its own WKWebsiteDataStore via a companion endpoint
     on the imgui cmd_server (port 9878, /inject-cookies).

EXTENSIBILITY
-------------
The module is intentionally generic. Additional services that use Turnstile
or other Chromium-specific challenges (Stripe, Google reCAPTCHA v3, etc.) can
be added by simply calling solve_url() with the appropriate URL. The cookie
harvest covers ALL cookies set during the Chromium session for that domain,
not just __cf_clearance.

USAGE FROM PYTHON (also see command_server.py /cf-solve endpoint)
---------
    from modules.cf_bridge import start_bridge, solve_url
    start_bridge(app)  # call once after QApplication is created
    cookies = solve_url("https://platform.claude.com/settings/billing")
    # cookies: [{"name": "__cf_clearance", "value": "...", "domain": ".claude.com", ...}]

USAGE FROM C++ IMGUI BROWSER
------------------------------
    GET http://127.0.0.1:9925/solve?url=https://platform.claude.com/settings/billing
    -> {"ok": true, "cookies": [...], "elapsed": 4.2}
    Then POST http://127.0.0.1:9878/inject-cookies with that JSON body.
"""

from __future__ import annotations

import json
import logging
import threading
import time
import uuid
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Dict, List, Optional
from urllib.parse import parse_qs, urlparse

from PyQt6.QtCore import QObject, QTimer, QUrl, pyqtSignal, pyqtSlot, Qt
from PyQt6.QtNetwork import QNetworkCookie
from PyQt6.QtWebEngineCore import QWebEngineProfile
from PyQt6.QtWebEngineWidgets import QWebEngineView

log = logging.getLogger("cf_bridge")

# ── Configuration ────────────────────────────────────────────────────────────

PORT          = 9925
SOLVE_TIMEOUT = 45       # seconds before giving up and returning empty cookies
POLL_INTERVAL = 500      # ms between cookie checks after page loads
MANUAL_SHOW_AFTER = 5   # seconds without clearance before showing window for manual click

# Cookie names produced by a solved Cloudflare Turnstile / Bot Management pass.
# __cf_clearance -- standard CF Bot Management clearance cookie
# cf_clearance   -- older name (still seen on some properties)
# _cfuvid        -- Cloudflare-specific user intent verification
CF_CLEARANCE_NAMES = {"__cf_clearance", "cf_clearance", "_cfuvid"}

# Additional cookies that are valuable to relay for specific services.
# Stripe and Anthropic session tokens -- passing these back avoids re-login.
RELAY_COOKIE_NAMES = {
    "__stripe_mid", "__stripe_sid",
    "claude_session", "sessionKey", "__Secure-next-auth.session-token",
}

# ── Internal state ───────────────────────────────────────────────────────────

_worker: Optional[_CfWorker] = None
_virt_ctrl: Optional['_VirtController'] = None
_bridge_started = False


# ── Worker (runs on Qt main thread) ─────────────────────────────────────────

class _PendingRequest:
    def __init__(self, url: str):
        self.url        = url
        self.cookies: List[Dict] = []
        self.done_event = threading.Event()
        self.domain     = urlparse(url).hostname or ""

    def add_cookie(self, c: QNetworkCookie):
        self.cookies.append({
            "name":     bytes(c.name()).decode("utf-8", errors="replace"),
            "value":    bytes(c.value()).decode("utf-8", errors="replace"),
            "domain":   c.domain(),
            "path":     c.path(),
            "secure":   c.isSecure(),
            "httpOnly": c.isHttpOnly(),
            "expiresAt": (c.expirationDate().toSecsSinceEpoch()
                          if not c.isSessionCookie() else None),
        })

    def is_cleared(self) -> bool:
        """Return True once a CF clearance cookie for our domain has been set."""
        for ck in self.cookies:
            if ck["name"] in CF_CLEARANCE_NAMES:
                host = ck.get("domain", "").lstrip(".")
                if host in self.domain or self.domain.endswith(host):
                    return True
        return False


class _CfWorker(QObject):
    """Owned by the Qt main thread. Drives the hidden QWebEngineView."""

    _solve_signal = pyqtSignal(str, str)  # (request_id, url)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._view      : Optional[QWebEngineView]    = None
        self._profile   : Optional[QWebEngineProfile] = None
        self._pending   : Dict[str, _PendingRequest]  = {}
        self._active_id : Optional[str]               = None
        self._queue     : List[tuple]                 = []  # (id, url) backlog
        self._poll_timer: Optional[QTimer]            = None
        self._ua        : str                         = ""  # Chrome UA for this profile

        self._solve_signal.connect(self._enqueue, Qt.ConnectionType.QueuedConnection)

    # ── Public thread-safe API ────────────────────────────────────────────────

    def request_solve(self, request_id: str, url: str):
        """Called from any thread. Schedules the solve on the Qt main thread."""
        req = _PendingRequest(url)
        self._pending[request_id] = req
        self._solve_signal.emit(request_id, url)
        return req

    # ── Qt main thread internals ──────────────────────────────────────────────

    def _ensure_view(self):
        if self._view is not None:
            return

        log.info("[cf_bridge] creating hidden Chromium view")
        self._profile = QWebEngineProfile("cf_bridge_v1")
        self._profile.setPersistentCookiesPolicy(
            QWebEngineProfile.PersistentCookiesPolicy.AllowPersistentCookies
        )
        # Harvest every cookie as it is set -- this fires on the Qt main thread.
        self._profile.cookieStore().cookieAdded.connect(self._on_cookie_added)

        # Record the exact UA string Chromium will send with every request.
        # cf_clearance is bound to this UA -- the C++ side must use it too.
        self._ua = self._profile.httpUserAgent()
        log.info("[cf_bridge] Chromium UA: %s", self._ua)

        self._view = QWebEngineView()
        self._view.setPage(
            self._view.page().__class__(self._profile, self._view)
        )
        # Must be shown (even off-screen) so Turnstile's visibility checks pass.
        # move() puts it outside any visible screen area.
        self._view.resize(2, 2)
        self._view.move(-9000, -9000)
        self._view.show()
        self._view.lower()

        self._poll_timer = QTimer(self)
        self._poll_timer.setInterval(POLL_INTERVAL)
        self._poll_timer.timeout.connect(self._on_poll)

        log.info("[cf_bridge] hidden Chromium view ready")

    def _enqueue(self, request_id: str, url: str):
        """Qt main thread: queue the request; start immediately if idle."""
        self._queue.append((request_id, url))
        if self._active_id is None:
            self._start_next()

    def _start_next(self):
        if not self._queue:
            self._active_id = None
            return
        request_id, url = self._queue.pop(0)
        self._active_id = request_id
        self._ensure_view()

        req = self._pending[request_id]
        req._started_at = time.monotonic()
        req._shown      = False   # becomes True if we show the window for manual solve
        log.info("[cf_bridge] loading  %s", url)

        self._view.loadFinished.connect(self._on_load_finished)
        self._view.load(QUrl(url))

    def _on_load_finished(self, ok: bool):
        try:
            self._view.loadFinished.disconnect(self._on_load_finished)
        except RuntimeError:
            pass
        req = self._pending.get(self._active_id)
        if req is None:
            return
        url = self._view.url().toString()
        log.info("[cf_bridge] page loaded ok=%s url=%s", ok, url)
        if not self._poll_timer.isActive():
            self._poll_timer.start()

    def _on_cookie_added(self, cookie: QNetworkCookie):
        name = bytes(cookie.name()).decode("utf-8", errors="replace")
        if self._active_id is None:
            return
        req = self._pending.get(self._active_id)
        if req is None:
            return
        if name in CF_CLEARANCE_NAMES or name in RELAY_COOKIE_NAMES:
            log.info("[cf_bridge] cookie: %s domain=%s", name, cookie.domain())
        req.add_cookie(cookie)

    def _on_poll(self):
        if self._active_id is None:
            self._poll_timer.stop()
            return
        req = self._pending.get(self._active_id)
        if req is None:
            self._poll_timer.stop()
            self._active_id = None
            self._start_next()
            return

        elapsed = time.monotonic() - getattr(req, "_started_at", time.monotonic())
        cleared = req.is_cleared()

        if cleared:
            log.info("[cf_bridge] CF clearance obtained after %.1fs", elapsed)
            self._finish(req)
        elif elapsed > MANUAL_SHOW_AFTER and not getattr(req, "_shown", False):
            # CF is showing an interactive challenge (checkbox CAPTCHA, etc.).
            # Make the window visible so the user can click through it.
            req._shown = True
            log.warning("[cf_bridge] not cleared after %.0fs -- showing window for manual solve",
                        elapsed)
            self._view.resize(520, 640)
            self._view.move(200, 100)
            self._view.show()
            self._view.raise_()
            self._view.activateWindow()
        elif elapsed > SOLVE_TIMEOUT:
            log.warning("[cf_bridge] timeout after %ds -- returning %d cookies",
                        SOLVE_TIMEOUT, len(req.cookies))
            self._finish(req)

    def _finish(self, req: _PendingRequest):
        self._poll_timer.stop()
        # If the window was shown for manual interaction, hide it again.
        if self._view is not None and getattr(req, "_shown", False):
            self._view.resize(2, 2)
            self._view.move(-9000, -9000)
            self._view.lower()
        req.done_event.set()
        self._active_id = None
        self._start_next()


# ── Virt overlay controller (Qt main thread) ────────────────────────────────

class _VirtController(QObject):
    """
    Thread-safe proxy for VirtOverlay.  The HTTP server thread calls the
    public methods; pyqtSignals marshal the actual Qt GUI operations onto
    the Qt main thread.
    """

    _show_sig = pyqtSignal(str, int, int, int, int)  # url, x, y, w, h
    _hide_sig = pyqtSignal()
    _move_sig = pyqtSignal(int, int, int, int)        # x, y, w, h

    def __init__(self, profile: QWebEngineProfile, parent=None) -> None:
        super().__init__(parent)
        from modules.qt_virt import VirtOverlay
        self._overlay = VirtOverlay(profile)
        self._show_sig.connect(self._overlay.show_at,    Qt.ConnectionType.QueuedConnection)
        self._hide_sig.connect(self._overlay.hide_overlay, Qt.ConnectionType.QueuedConnection)
        self._move_sig.connect(self._overlay.move_overlay, Qt.ConnectionType.QueuedConnection)

    # Called from the HTTP thread -------------------------------------------
    def show(self, url: str, x: int, y: int, w: int, h: int) -> None:
        self._show_sig.emit(url, x, y, w, h)

    def hide(self) -> None:
        self._hide_sig.emit()

    def move(self, x: int, y: int, w: int, h: int) -> None:
        self._move_sig.emit(x, y, w, h)


# ── HTTP server (background thread) ─────────────────────────────────────────

class _Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        log.debug("cf_bridge http: " + fmt, *args)

    def do_GET(self):
        parsed = urlparse(self.path)
        qs     = parse_qs(parsed.query)

        if parsed.path == "/health":
            self._respond(200, b"ok")
            return

        if parsed.path == "/solve":
            url_list = qs.get("url", [])
            if not url_list:
                self._respond(400, b'{"error":"missing url parameter"}')
                return
            url = url_list[0]
            t0  = time.monotonic()

            request_id = str(uuid.uuid4())
            req = _worker.request_solve(request_id, url)
            req.done_event.wait(timeout=SOLVE_TIMEOUT + 5)

            elapsed  = time.monotonic() - t0
            body = json.dumps({
                "ok":      req.is_cleared(),
                "cookies": req.cookies,
                "elapsed": round(elapsed, 2),
                "url":     url,
                "ua":      _worker._ua,
            }).encode()
            self._respond(200, body, content_type="application/json")
            return

        self._respond(404, b'{"error":"not found"}')

    def do_POST(self):
        parsed = urlparse(self.path)
        length = int(self.headers.get("Content-Length", 0))
        body   = self.rfile.read(length) if length > 0 else b"{}"
        try:
            data = json.loads(body)
        except Exception:
            data = {}

        if parsed.path == "/virt-show":
            url = data.get("url", "")
            x   = int(data.get("x", 0))
            y   = int(data.get("y", 0))
            w   = int(data.get("w", 800))
            h   = int(data.get("h", 600))
            if url and _virt_ctrl is not None:
                _virt_ctrl.show(url, x, y, w, h)
            self._respond(200, b'{"ok":true}')
            return

        if parsed.path == "/virt-hide":
            if _virt_ctrl is not None:
                _virt_ctrl.hide()
            self._respond(200, b'{"ok":true}')
            return

        if parsed.path == "/virt-move":
            x = int(data.get("x", 0))
            y = int(data.get("y", 0))
            w = int(data.get("w", 800))
            h = int(data.get("h", 600))
            if _virt_ctrl is not None:
                _virt_ctrl.move(x, y, w, h)
            self._respond(200, b'{"ok":true}')
            return

        self._respond(404, b'{"error":"not found"}')

    def _respond(self, status: int, body: bytes, content_type: str = "application/json"):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)


def _run_server():
    server = HTTPServer(("127.0.0.1", PORT), _Handler)
    log.info("[cf_bridge] HTTP server listening on port %d", PORT)
    server.serve_forever()


# ── Public API ───────────────────────────────────────────────────────────────

def start_bridge(app=None):
    """
    Initialise the CF bridge. Must be called after QApplication is created.

    Parameters
    ----------
    app : QApplication, optional
        Not used directly, but calling start_bridge before QApplication
        raises an error, so passing it reminds the caller of the ordering
        requirement.
    """
    global _worker, _bridge_started
    if _bridge_started:
        return
    _bridge_started = True

    global _virt_ctrl
    _worker = _CfWorker()

    # Ensure the CF worker view is created (which also creates the shared profile)
    # before VirtController so both share the same persistent cookie store.
    _worker._ensure_view()
    _virt_ctrl = _VirtController(_worker._profile)

    t = threading.Thread(target=_run_server, daemon=True, name="cf-bridge-http")
    t.start()
    log.info("[cf_bridge] started (Chromium WebEngine + HTTP :%d)", PORT)


def solve_url(url: str, timeout: float = SOLVE_TIMEOUT) -> List[Dict]:
    """
    Block until CF clearance cookies are available for url, then return them.

    Returns a list of cookie dicts. The list is non-empty even on timeout --
    it contains whatever cookies were collected before the deadline.

    Can be called from any thread.
    """
    if _worker is None:
        raise RuntimeError("call start_bridge() first")
    request_id = str(uuid.uuid4())
    req = _worker.request_solve(request_id, url)
    req.done_event.wait(timeout=timeout + 5)
    return req.cookies
