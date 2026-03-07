#!/usr/bin/env python3
"""
webbrowse_no_controls.py -- Standalone secure-window browser, no address bar.

Launched by the imgui browser (server_manager.cpp) whenever a payment or
auth URL is detected.  Displays a brief security animation, then navigates
to the target URL in a full-content Chromium (Qt WebEngine) window.

Usage:
    python webbrowse_no_controls.py --url URL [--cookies-json JSON]
"""
import sys
import os
import argparse
import json

try:
    import PyQt6
except ImportError:
    print("[webbrowse] PyQt6 not installed - opening URL in system browser instead", file=sys.stderr, flush=True)
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", required=True)
    parser.add_argument("--cookies-json", default="")
    args = parser.parse_args()
    import webbrowser
    webbrowser.open(args.url)
    sys.exit(0)

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)

# ── Animation HTML shown for ANIM_MS before navigating ───────────────────────

ANIM_MS = 1400

_ANIM_HTML = """<!doctype html>
<html><head><meta charset="utf-8">
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{width:100%;height:100%;background:#0a0f1a;display:flex;
  align-items:center;justify-content:center;
  font-family:-apple-system,BlinkMacSystemFont,sans-serif;color:#94a3b8}
.card{display:flex;flex-direction:column;align-items:center;gap:22px}
.sw{position:relative;width:64px;height:64px}
.ring{position:absolute;top:-14px;left:-14px;width:92px;height:92px;
  border-radius:50%;border:2px solid rgba(56,189,248,.12);
  border-top-color:rgba(56,189,248,.85);
  animation:spin 1.1s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
svg{width:64px;height:64px}
.lbl{font-size:13px;letter-spacing:.07em;text-transform:uppercase}
.sub{color:#475569;font-size:11px;text-align:center;max-width:300px;line-height:1.7}
.dots span{display:inline-block;width:6px;height:6px;border-radius:50%;
  background:#38bdf8;margin:0 3px;animation:p 1.4s ease-in-out infinite}
.dots span:nth-child(2){animation-delay:.2s}
.dots span:nth-child(3){animation-delay:.4s}
@keyframes p{0%,80%,100%{transform:scale(.6);opacity:.3}40%{transform:scale(1);opacity:1}}
</style></head><body>
<div class="card">
  <div class="sw">
    <div class="ring"></div>
    <svg viewBox="0 0 24 24" fill="none" stroke="#38bdf8" stroke-width="1.5"
         stroke-linecap="round" stroke-linejoin="round">
      <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
      <polyline points="9 12 11 14 15 10"/>
    </svg>
  </div>
  <div class="lbl">Opening secure window</div>
  <div class="dots"><span></span><span></span><span></span></div>
  <div class="sub">This page requires a dedicated secure session</div>
</div>
</body></html>"""


def _make_app(argv):
    from PyQt6.QtWidgets import QApplication
    app = QApplication(argv)
    app.setApplicationName("Secure Window")

    try:
        from AppKit import NSApplication, NSApplicationActivationPolicyRegular
        NSApplication.sharedApplication().setActivationPolicy_(
            NSApplicationActivationPolicyRegular)
    except Exception:
        pass

    return app


def _open_window(url: str, cookies: list):
    from PyQt6.QtCore import QTimer, QUrl, Qt
    from PyQt6.QtNetwork import QNetworkCookie
    from PyQt6.QtWebEngineCore import QWebEngineProfile, QWebEnginePage
    from PyQt6.QtWebEngineWidgets import QWebEngineView
    from PyQt6.QtWidgets import QMainWindow

    _live_popups: list = []

    class _PopupPage(QWebEnginePage):
        """Handles window.open() calls (OAuth redirects) inside the secure window."""
        def createWindow(self, _wt):
            pop = _PopupPage(self.profile(), self)
            win = QMainWindow()
            pop._win = win
            win.setWindowTitle("Sign In")
            win.resize(520, 680)
            win.setWindowFlags(Qt.WindowType.Window |
                               Qt.WindowType.WindowStaysOnTopHint)
            view = QWebEngineView(win)
            view.setPage(pop)
            win.setCentralWidget(view)
            win.show()
            win.raise_()
            win.activateWindow()
            _live_popups.append(win)
            return pop

    profile = QWebEngineProfile("xcm-secure-win")
    profile.setHttpUserAgent(
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/131.0.0.0 Safari/537.36"
    )

    if cookies:
        store = profile.cookieStore()
        for c in cookies:
            name   = c.get("name",  "").encode()
            value  = c.get("value", "").encode()
            domain = c.get("domain", "")
            if not name or not domain:
                continue
            qc = QNetworkCookie(name, value)
            qc.setDomain(domain)
            qc.setPath(c.get("path", "/"))
            qc.setSecure(bool(c.get("secure", False)))
            qc.setHttpOnly(bool(c.get("httpOnly", False)))
            exp = c.get("expiresAt")
            if exp:
                from PyQt6.QtCore import QDateTime
                qc.setExpirationDate(QDateTime.fromSecsSinceEpoch(int(exp)))
            store.setCookie(qc)

    page = _PopupPage(profile)
    view = QWebEngineView()
    view.setPage(page)

    win = QMainWindow()
    win.setWindowTitle("Secure Window")
    win.resize(1080, 800)
    win.setWindowFlags(Qt.WindowType.Window |
                       Qt.WindowType.WindowStaysOnTopHint)
    win.setCentralWidget(view)

    view.setHtml(_ANIM_HTML)

    def _navigate():
        view.load(QUrl(url))

    QTimer.singleShot(ANIM_MS, _navigate)

    win.show()
    win.raise_()
    win.activateWindow()

    return win


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", required=True, help="URL to open")
    parser.add_argument("--cookies-json", default="",
                        help="JSON array of cookie dicts from WKWebView")
    args = parser.parse_args()

    cookies = []
    if args.cookies_json:
        try:
            cookies = json.loads(args.cookies_json)
        except Exception as e:
            print(f"[webbrowse_no_controls] cookies parse error: {e}", flush=True)

    app = _make_app(sys.argv[:1])
    _open_window(args.url, cookies)

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
