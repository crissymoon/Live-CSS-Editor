"""
wkwebview_widget.py -- WKWebView embedded in a PyQt6 QWidget.

Uses macOS's native WebKit (WKWebView) instead of QtWebEngine.
WKWebView has full H.264/AAC/HEVC codec support via VideoToolbox,
persistent cookies via WKWebsiteDataStore, JavaScript injection
via WKUserContentController, and ad blocking via WKContentRuleList.

The WKBrowserTab class exposes the same interface as the old
BrowserTab(QWebEngineView) so MainWindow needs minimal changes.
"""

import os
import objc
import WebKit
from AppKit import NSView, NSWindow, NSApplication
from Foundation import (
    NSURL, NSURLRequest, NSMakeRect, NSObject,
    NSString, NSLog, NSDictionary,
)

from .cdm_sdk import CDM_SDK_JS

# ── Performance-injection helpers ─────────────────────────────────────────────
_SRC_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src')


def _load_src_js(name: str) -> str:
    """Load a JS file from dev-browser/src/ at injection time."""
    with open(os.path.join(_SRC_DIR, name), 'r', encoding='utf-8') as fh:
        return fh.read()


# Lightweight rAF-based Hz detector and tick bus (replaces the ESM GSAP ticker
# from ticker-inject.js so it can be injected into WKWebView directly).
_TICKER_JS = r"""
(function () {
    'use strict';

    // Exported globals (read by CDM SDK and other injections)
    window.__xcmHz            = 60;
    window.__xcmFrameMs       = 1000 / 60;
    window.__xcmFrameBudget   = (1000 / 60) * 0.55;  // 55% of frame for JS
    window.__xcmIsHighHz      = false;
    // Scroll-idle debounce: wait 4 full frame periods before treating scroll
    // as ended.  Previously this was 0.3x one frame (~5 ms at 60 Hz) which
    // fired the scroll-end handler dozens of times during active momentum
    // scrolling, triggering layout work mid-scroll.
    window.__xcmIdleThreshold = Math.ceil(1000 / 60) * 4;

    var SAMPLE_COUNT = 30;
    var _samples = [], _last = 0;

    function _snap(hz) {
        if (hz >= 130) return 144;
        if (hz >= 105) return 120;
        if (hz >=  82) return 90;
        if (hz >=  50) return 60;
        return 30;
    }

    function _calibrate(hz) {
        var frameMs                = 1000 / hz;
        window.__xcmHz            = hz;
        window.__xcmFrameMs       = frameMs;
        window.__xcmFrameBudget   = frameMs * 0.55;
        window.__xcmIsHighHz      = hz > 75;
        // 4 frame periods: long enough that momentum scrolling never
        // accidentally triggers the scroll-end path between frames.
        window.__xcmIdleThreshold = Math.ceil(frameMs) * 4;
    }

    // Phase 1: sample 30 rAF intervals to detect the actual display Hz.
    function _sampleRaf(ts) {
        if (_last) _samples.push(ts - _last);
        _last = ts;
        if (_samples.length < SAMPLE_COUNT) {
            requestAnimationFrame(_sampleRaf);
        } else {
            _samples.sort(function (a, b) { return a - b; });
            var median = _samples[Math.floor(_samples.length / 2)];
            _calibrate(_snap(Math.round(1000 / median)));
            // Hand off to the continuous tick loop.
            _rafId = requestAnimationFrame(_loop);
        }
    }
    requestAnimationFrame(_sampleRaf);

    // Phase 2: single shared rAF loop.
    // Passes { ts, deadline } to every listener so they know how much
    // budget remains in the current frame without calling performance.now().
    var _listeners = [];
    var _rafId     = null;

    function _loop(ts) {
        _rafId = requestAnimationFrame(_loop);
        // Deadline = start of this frame + JS budget (55% of frame period).
        var deadline = ts + window.__xcmFrameBudget;
        for (var i = 0; i < _listeners.length; i++) {
            try { _listeners[i](ts, deadline); } catch (e) {}
        }
    }

    // Register a per-frame callback.  Returns an unsubscribe function.
    // Listeners receive (timestamp, frameDeadline) on each tick.
    window.__xcmTick = function (fn) {
        _listeners.push(fn);
        // Only start the loop if the sampling phase has finished; otherwise
        // the loop starts automatically after _calibrate runs.
        if (_rafId === null && _samples.length >= SAMPLE_COUNT) {
            _rafId = requestAnimationFrame(_loop);
        }
        return function () {
            var idx = _listeners.indexOf(fn);
            if (idx !== -1) _listeners.splice(idx, 1);
            if (!_listeners.length && _rafId !== null) {
                cancelAnimationFrame(_rafId);
                _rafId = null;
            }
        };
    };
})();
"""

from PyQt6.QtCore import Qt, QUrl, QTimer, pyqtSignal
from PyQt6.QtGui import QWindow
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QApplication,
    QMessageBox, QInputDialog, QFileDialog,
)

# Keep track of popup windows so they are not garbage-collected.
_popup_windows = []


# ---------------------------------------------------------------------------
# Module-level singletons for shared resources
# ---------------------------------------------------------------------------

# Shared WKProcessPool -- all WKWebView instances created with the same pool
# run in the SAME WebKit network/rendering process instead of spawning a new
# one per tab.  This means JIT-compiled JS, DNS lookups, HTTP/2 connections,
# and the in-memory resource cache are all shared, which significantly speeds
# up page loads after the first one.
_SHARED_PROCESS_POOL = WebKit.WKProcessPool.alloc().init()

# ---------------------------------------------------------------------------
# WKContentRuleList ad/tracker blocker (compiled once, added to every tab)
# ---------------------------------------------------------------------------
# Re-use the same domain list as browser_profile._AD_DOMAINS.
# WKContentRuleList uses Safari-style JSON rules that block network requests
# at the WebKit layer before any data is fetched, saving bandwidth and CPU.
_WK_CONTENT_RULES = None
_WK_RULES_READY = False

def _build_wk_rules():
    """Build and compile a WKContentRuleList from browser_profile._AD_DOMAINS.
    Must be called from the main thread (ObjC requirement)."""
    global _WK_CONTENT_RULES, _WK_RULES_READY
    if _WK_RULES_READY:
        return
    _WK_RULES_READY = True
    try:
        from .browser_profile import _AD_DOMAINS
        import json as _json
        rules = []
        for domain in _AD_DOMAINS:
            # Strip path components (e.g. 'facebook.com/tr' -> 'facebook.com')
            host = domain.split('/')[0]
            rules.append({
                'trigger': {
                    'url-filter': r'.*',
                    'if-domain': ['*.' + host, host],
                    'resource-type': ['script', 'image', 'style-sheet',
                                      'raw', 'font', 'media', 'popup'],
                    'load-type': ['third-party'],
                },
                'action': {'type': 'block'},
            })
        json_str = _json.dumps(rules)
        store = WebKit.WKContentRuleListStore.defaultStore()
        # compileContentRuleListForIdentifier is async; use a semaphore to wait.
        import threading
        done = threading.Event()
        def handler(ruleList, error):
            global _WK_CONTENT_RULES
            if ruleList:
                _WK_CONTENT_RULES = ruleList
            done.set()
        store.compileContentRuleListForIdentifier_encodedContentRuleList_completionHandler_(
            'xcm_adblock', json_str, handler)
        # Give it a moment but do not block forever.
        done.wait(timeout=3.0)
    except Exception:
        pass

# Expand NSURLCache so macOS's URL loading system has a large disk + memory
# cache.  Default is ~10 MB memory / ~100 MB disk which saturates fast.
# Setting this at module import time covers all NSURLSession traffic,
# including the WKWebView network stack.
try:
    from Foundation import NSURLCache
    _cache = NSURLCache.alloc().initWithMemoryCapacity_diskCapacity_diskPath_(
        256 * 1024 * 1024,   # 256 MB in-memory cache
        1024 * 1024 * 1024,  # 1 GB on-disk cache
        os.path.expanduser('~/.xcaliburmoon_urlcache'),
    )
    NSURLCache.setSharedURLCache_(_cache)
except Exception:
    pass


# ---------------------------------------------------------------------------
# Console noise filter (JS-side, since WKWebView has no Python callback)
# ---------------------------------------------------------------------------
_CONSOLE_FILTER_JS = r"""
(function(){
    var _SUPPRESS = [
        "Can't find variable: module",
        'could not be converted to a proto attribute',
        'Using the tech directly can be dangerous',
        'Refused to connect to',
        "because the document's frame is sandboxed",
        'was preloaded using link preload but not used',
        'TypeError: network error',
        'Network Error',
        'NetworkError',
        'Network request failed',
        'network error occurred',
        'TMS load event sent successfully',
        'External tag load event sent successfully',
        'Play was interrupted',
        'MEDIA_ERR_SRC_NOT_SUPPORTED',
        'no supported source was found',
        'BooleanExpression with operator',
        'had an expression that evaluated to null',
        'chrome-extension://invalid/',
        'net::ERR_FAILED',
        'net::ERR_ACCESS_DENIED',
        'net::ERR_BLOCKED_BY_CLIENT',
        'net::ERR_ABORTED',
        'JQMIGRATE',
        'module is not defined',
        'robustness level be specified',
        'opt-out',
        'Failed to load resource',
        'Adaptive Video Streaming Service',
        'play() request was interrupted',
        'DRM will only work via',
        'Bad Gateway',
        'Transcoding failed',
        'Mixed Content',
        'Refused to connect',
        'has no supported sources',
        'NotSupportedError',
        'defaultTracking',
        'Blocked a frame with origin',
        'Log success',
        'complianz',
        'load event sent successfully',
        'Access-Control-Allow-Origin',
        'ERR_BLOCKED',
        'The operation was aborted',
        'cancelled',
    ];
    function _isSuppressed(msg) {
        for (var i = 0; i < _SUPPRESS.length; i++) {
            if (msg.indexOf(_SUPPRESS[i]) !== -1) return true;
        }
        return false;
    }

    // Extract source file / line / col from a stack trace string.
    // WebKit format: "    functionName@https://host/path/file.js:42:15"
    function _parseStack(stack) {
        if (!stack) return null;
        var lines = stack.split('\n');
        for (var i = 0; i < lines.length; i++) {
            var m = lines[i].match(/@(.+?):(\d+):(\d+)\s*$/);
            if (m) {
                var url = m[1];
                // Shorten to just filename and query-stripped
                var parts = url.split('/');
                var fname = (parts[parts.length - 1] || url).split('?')[0];
                return {src: fname, url: url, line: +m[2], col: +m[3]};
            }
        }
        return null;
    }

    // Get source of the caller of a console.* call.
    // depth=0 -> _getCallerSource frame, depth=1 -> _wrap, depth=2 -> actual caller
    function _getCallerSource() {
        try {
            var stack = new Error().stack || '';
            var lines = stack.split('\n');
            // Skip frames belonging to this injected script (_getCallerSource, _wrap)
            for (var i = 3; i < lines.length; i++) {
                var line = lines[i];
                if (line.indexOf('_xcm_console') !== -1) continue;
                var m = line.match(/@(.+?):(\d+):(\d+)\s*$/);
                if (m) {
                    var url = m[1];
                    var parts = url.split('/');
                    var fname = (parts[parts.length - 1] || url).split('?')[0];
                    return {src: fname, url: url, line: +m[2], col: +m[3]};
                }
            }
        } catch(e) {}
        return null;
    }

    function _send(level, msg, srcInfo) {
        try {
            if (window.webkit && window.webkit.messageHandlers
                    && window.webkit.messageHandlers._xcm_console) {
                var payload = {level: level, text: msg};
                if (srcInfo) {
                    payload.src  = srcInfo.src;
                    payload.url  = srcInfo.url;
                    payload.line = srcInfo.line;
                    payload.col  = srcInfo.col;
                }
                window.webkit.messageHandlers._xcm_console.postMessage(payload);
            }
        } catch(e) {}
    }

    function _wrap(orig, level) {
        return function() {
            var args = Array.prototype.slice.call(arguments);
            var msg = args.map(function(a) {
                if (a === null) return 'null';
                if (a === undefined) return 'undefined';
                if (typeof a === 'object') {
                    try { return JSON.stringify(a); } catch(e) { return String(a); }
                }
                return String(a);
            }).join(' ');
            if (_isSuppressed(msg)) { return orig.apply(console, arguments); }
            var srcInfo = _getCallerSource();
            _send(level, msg, srcInfo);
            return orig.apply(console, arguments);
        };
    }
    console.log   = _wrap(console.log,   'log');
    console.warn  = _wrap(console.warn,  'warn');
    console.error = _wrap(console.error, 'error');
    console.info  = _wrap(console.info,  'info');
    console.debug = _wrap(console.debug, 'debug');

    // Capture uncaught errors.
    window.addEventListener('error', function(e) {
        var msg = (e.message || 'Script error');
        if (_isSuppressed(msg)) return;
        var srcInfo = null;
        if (e.filename) {
            var parts = e.filename.split('/');
            srcInfo = {
                src:  (parts[parts.length-1] || e.filename).split('?')[0],
                url:  e.filename,
                line: e.lineno  || 0,
                col:  e.colno   || 0,
            };
        } else if (e.error && e.error.stack) {
            srcInfo = _parseStack(e.error.stack);
        }
        _send('error', '[uncaught] ' + msg, srcInfo);
    });

    // Capture unhandled promise rejections.
    window.addEventListener('unhandledrejection', function(e) {
        var reason = e.reason;
        var msg = reason
            ? (reason.message || String(reason))
            : 'Promise rejected';
        if (_isSuppressed(msg)) return;
        var srcInfo = (reason && reason.stack) ? _parseStack(reason.stack) : null;
        _send('error', '[unhandled] ' + msg, srcInfo);
    });
})();
"""

# JS to disable right-click context menu (no "Inspect Element" leak).
_DISABLE_CONTEXTMENU_JS = r"""
document.addEventListener('contextmenu', function(e) { e.preventDefault(); }, true);
"""


# ---------------------------------------------------------------------------
# ObjC delegate classes
# ---------------------------------------------------------------------------
# WKNavigationDelegate receives navigation callbacks from WKWebView.
# WKUIDelegate handles JS alert/confirm/prompt and new-window requests.
# We define them as NSObject subclasses that conform to the protocols.

class _NavigationDelegate(NSObject):
    """Receives WKNavigationDelegate callbacks and forwards to the Qt widget."""

    # Tell PyObjC this class implements the protocol.
    __objc_protocols__ = [objc.protocolNamed('WKNavigationDelegate')]

    def initWithBrowserTab_(self, tab):
        self = objc.super(_NavigationDelegate, self).init()
        if self is None:
            return None
        self._tab = tab
        return self

    # -- Navigation events -------------------------------------------------

    def webView_didStartProvisionalNavigation_(self, webView, navigation):
        tab = self._tab
        if tab:
            tab._sig_loadStarted.emit()

    def webView_didCommitNavigation_(self, webView, navigation):
        tab = self._tab
        if tab:
            url_str = str(webView.URL().absoluteString()) if webView.URL() else ''
            tab._sig_urlChanged.emit(QUrl(url_str))
            tab._sig_loadProgress.emit(50)

    def webView_didFinishNavigation_(self, webView, navigation):
        tab = self._tab
        if tab:
            url_str = str(webView.URL().absoluteString()) if webView.URL() else ''
            tab._sig_urlChanged.emit(QUrl(url_str))
            tab._sig_loadProgress.emit(100)
            tab._sig_loadFinished.emit(True)
            title = str(webView.title()) if webView.title() else ''
            if title:
                tab._sig_titleChanged.emit(title)

    def webView_didFailNavigation_withError_(self, webView, navigation, error):
        tab = self._tab
        if tab:
            tab._sig_loadFinished.emit(False)

    def webView_didFailProvisionalNavigation_withError_(self, webView, navigation, error):
        tab = self._tab
        if tab:
            tab._sig_loadFinished.emit(False)

    # -- Allow all navigation (no blocking) --------------------------------
    def webView_decidePolicyForNavigationAction_decisionHandler_(
            self, webView, navigationAction, decisionHandler):
        decisionHandler(WebKit.WKNavigationActionPolicyAllow)

    # -- Allow all responses (no content-type blocking) --------------------
    def webView_decidePolicyForNavigationResponse_decisionHandler_(
            self, webView, navigationResponse, decisionHandler):
        decisionHandler(WebKit.WKNavigationResponsePolicyAllow)

    # -- HTTPS certificate errors: allow all (dev) -------------------------
    def webView_didReceiveAuthenticationChallenge_completionHandler_(
            self, webView, challenge, completionHandler):
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            try:
                trust = challenge.protectionSpace().serverTrust()
            except Exception:
                trust = None
        if trust is not None:
            from Foundation import NSURLCredential, NSURLSessionAuthChallengeUseCredential
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                cred = NSURLCredential.credentialForTrust_(trust)
            completionHandler(NSURLSessionAuthChallengeUseCredential, cred)
        else:
            from Foundation import NSURLSessionAuthChallengePerformDefaultHandling
            completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, None)


# ---------------------------------------------------------------------------
# Helper: open a URL in a new MainWindow tab
# ---------------------------------------------------------------------------
def _open_url_in_new_tab(browser_tab, url_str):
    """Walk up the Qt parent chain from a WKBrowserTab to find the MainWindow
    and open *url_str* in a brand new tab."""
    from PyQt6.QtCore import QUrl
    w = browser_tab
    while w is not None:
        if hasattr(w, 'add_new_tab'):
            w.add_new_tab(QUrl(url_str))
            return
        w = w.parent() if hasattr(w, 'parent') else None
    # Fallback: load in the same tab if we cannot find MainWindow.
    browser_tab.setUrl(QUrl(url_str))


class _UIDelegate(NSObject):
    """Receives WKUIDelegate callbacks for JS dialogs and popups."""

    __objc_protocols__ = [objc.protocolNamed('WKUIDelegate')]

    def initWithBrowserTab_(self, tab):
        self = objc.super(_UIDelegate, self).init()
        if self is None:
            return None
        self._tab = tab
        return self

    # -- JS alert ----------------------------------------------------------
    def webView_runJavaScriptAlertPanelWithMessage_initiatedByFrame_completionHandler_(
            self, webView, message, frame, completionHandler):
        msg = str(message) if message else ''
        QMessageBox.information(
            QApplication.activeWindow(), 'Page Alert', msg)
        completionHandler()

    # -- JS confirm --------------------------------------------------------
    def webView_runJavaScriptConfirmPanelWithMessage_initiatedByFrame_completionHandler_(
            self, webView, message, frame, completionHandler):
        msg = str(message) if message else ''
        result = QMessageBox.question(
            QApplication.activeWindow(), 'Page Confirm', msg,
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        completionHandler(result == QMessageBox.StandardButton.Yes)

    # -- JS prompt ---------------------------------------------------------
    def webView_runJavaScriptTextInputPanelWithPrompt_defaultText_initiatedByFrame_completionHandler_(
            self, webView, prompt, defaultText, frame, completionHandler):
        text, ok = QInputDialog.getText(
            QApplication.activeWindow(), 'Page Input',
            str(prompt) if prompt else '',
            text=str(defaultText) if defaultText else '',
        )
        completionHandler(text if ok else None)

    # -- Fullscreen entry / exit ------------------------------------------
    # These informal-protocol methods are called by WKWebView when the page
    # calls element.requestFullscreen() / document.exitFullscreen().
    def webViewDidEnterFullscreen_(self, webView):
        pass   # WKWebView handles the animation itself; nothing extra needed.

    def webViewDidExitFullscreen_(self, webView):
        pass

    # -- File upload (<input type="file">) ---------------------------------
    # Without this method the file picker does nothing and videos/images
    # can never be attached to forms (LinkedIn posts, etc.).
    def webView_runOpenPanelWithParameters_initiatedByFrame_completionHandler_(
            self, webView, parameters, frame, completionHandler):
        try:
            multiple   = bool(parameters.allowsMultipleSelection()) if parameters else False
            allow_dirs = bool(parameters.allowsDirectories())       if parameters else False
        except Exception:
            multiple   = False
            allow_dirs = False

        parent = QApplication.activeWindow()

        if allow_dirs:
            path = QFileDialog.getExistingDirectory(parent, 'Choose Folder')
            if path:
                completionHandler([NSURL.fileURLWithPath_(path)])
            else:
                completionHandler(None)
            return

        if multiple:
            paths, _ = QFileDialog.getOpenFileNames(parent, 'Choose Files')
        else:
            path, _ = QFileDialog.getOpenFileName(parent, 'Choose File')
            paths = [path] if path else []

        if not paths:
            completionHandler(None)
            return

        completionHandler([NSURL.fileURLWithPath_(p) for p in paths if p])

    # -- New window / popup requests (OAuth, target=_blank, etc.) ----------

    # OAuth domains that require a real popup with window.opener preserved.
    _OAUTH_DOMAINS = (
        'accounts.google.com', 'login.microsoftonline.com',
        'login.live.com', 'www.linkedin.com', 'linkedin.com',
        'appleid.apple.com', 'github.com', 'api.twitter.com',
        'www.facebook.com', 'auth0.com',
    )

    def webView_createWebViewWithConfiguration_forNavigationAction_windowFeatures_(
            self, webView, configuration, navigationAction, windowFeatures):
        req_url = navigationAction.request().URL()
        url_str = str(req_url) if req_url else ''
        host = str(req_url.host()) if req_url and req_url.host() else ''

        # If windowFeatures specifies explicit dimensions the page is requesting
        # a deliberate popup window (modal viewer, share dialog etc).  Always
        # honour those with a real NSWindow so the opener relationship and
        # sizing are preserved.
        wf_width  = windowFeatures.width()  if windowFeatures else None
        wf_height = windowFeatures.height() if windowFeatures else None
        has_geometry = (wf_width is not None or wf_height is not None)

        # Decide: is this an OAuth/popup that needs window.opener, or a
        # regular link that should open in a new browser tab?
        is_popup = has_geometry  # explicit dimensions -> keep as popup
        if not url_str or url_str == 'about:blank':
            is_popup = True  # window.open() with no URL -- OAuth / postMessage
        else:
            for domain in self._OAUTH_DOMAINS:
                if host == domain or host.endswith('.' + domain):
                    is_popup = True
                    break
            lower = url_str.lower()
            if any(kw in lower for kw in (
                '/oauth', '/signin', '/sign-in', '/login',
                '/auth', '/authorize', '/connect/',
            )):
                is_popup = True

        # ── Regular link (no dimensions, no auth): open in a new tab ─────
        if not is_popup:
            tab = self._tab
            if tab and url_str:
                from PyQt6.QtCore import QTimer, QUrl
                QTimer.singleShot(0, lambda u=url_str, t=tab: _open_url_in_new_tab(t, u))
            elif url_str:
                webView.loadRequest_(navigationAction.request())
            return None

        # ── OAuth popup: create a real WKWebView in an NSWindow ───
        popup_wk = WebKit.WKWebView.alloc().initWithFrame_configuration_(
            NSMakeRect(0, 0, 900, 700), configuration)

        # Copy the parent UA so the popup is not treated differently.
        if webView.customUserAgent():
            popup_wk.setCustomUserAgent_(webView.customUserAgent())

        # Wrap in a native NSWindow so it shows on screen.
        from AppKit import (
            NSWindowStyleMaskTitled, NSWindowStyleMaskClosable,
            NSWindowStyleMaskMiniaturizable, NSWindowStyleMaskResizable,
            NSBackingStoreBuffered, NSScreen,
        )
        style = (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                 | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
        screen_frame = NSScreen.mainScreen().frame()
        # Centre the popup on screen.
        x = (screen_frame.size.width - 900) / 2
        y = (screen_frame.size.height - 700) / 2
        win = NSWindow.alloc().initWithContentRect_styleMask_backing_defer_(
            NSMakeRect(x, y, 900, 700), style, NSBackingStoreBuffered, False)
        win.setContentView_(popup_wk)
        win.setTitle_('Sign In')
        win.setReleasedWhenClosed_(False)

        # Close-tracking delegate so we can clean up references.
        closer = _PopupCloseDelegate.alloc().initWithWindow_(win)
        win.setDelegate_(closer)

        # Navigation delegate for the popup (allow all, handle cert).
        popup_nav = _NavigationDelegate.alloc().initWithBrowserTab_(None)
        popup_wk.setNavigationDelegate_(popup_nav)

        win.makeKeyAndOrderFront_(None)
        NSApplication.sharedApplication().activateIgnoringOtherApps_(True)

        # Prevent GC.
        _popup_windows.append({
            'window': win, 'wkview': popup_wk,
            'nav_delegate': popup_nav, 'close_delegate': closer,
        })
        return popup_wk


# ---------------------------------------------------------------------------
# Popup window close delegate (releases GC references)
# ---------------------------------------------------------------------------
class _PopupCloseDelegate(NSObject):
    """NSWindowDelegate that cleans up popup tracking on close."""

    def initWithWindow_(self, win):
        self = objc.super(_PopupCloseDelegate, self).init()
        if self is None:
            return None
        self._win = win
        return self

    def windowWillClose_(self, notification):
        # Remove from tracking list so the objects can be freed.
        win = self._win
        for i, entry in enumerate(_popup_windows):
            if entry['window'] is win:
                _popup_windows.pop(i)
                break


# ---------------------------------------------------------------------------
# WKScriptMessageHandler for console capture
# ---------------------------------------------------------------------------
class _ConsoleMessageHandler(NSObject):
    """Receives postMessage calls from the JS console wrapper and
    forwards them to the Qt ConsolePanel."""

    def initWithBrowserTab_(self, tab):
        self = objc.super(_ConsoleMessageHandler, self).init()
        if self is None:
            return None
        self._tab = tab
        return self

    def userContentController_didReceiveScriptMessage_(self, controller, message):
        tab = self._tab
        if not tab or not tab._console_panel:
            return
        body = message.body()
        # body is an NSDictionary (PyObjC bridged) or plain dict
        if hasattr(body, 'objectForKey_'):
            # NSDictionary -- convert to plain Python dict
            body = {str(k): body.objectForKey_(k) for k in body}
        if isinstance(body, dict):
            level  = str(body.get('level', 'log'))
            text   = str(body.get('text',  ''))
            src    = str(body['src'])  if body.get('src')  else None
            url    = str(body['url'])  if body.get('url')  else None
            line   = int(body['line']) if body.get('line') else None
            col    = int(body['col'])  if body.get('col')  else None
        else:
            level, text, src, url, line, col = 'log', str(body), None, None, None, None
        tab._console_panel.append_message(level, text, src=src, url=url, line=line, col=col)


# ---------------------------------------------------------------------------
# Title observer (KVO on WKWebView.title)
# ---------------------------------------------------------------------------
class _TitleObserver(NSObject):
    """KVO observer that watches WKWebView.title and emits a Qt signal."""

    def initWithBrowserTab_(self, tab):
        self = objc.super(_TitleObserver, self).init()
        if self is None:
            return None
        self._tab = tab
        return self

    def observeValueForKeyPath_ofObject_change_context_(
            self, keyPath, obj, change, context):
        if str(keyPath) == 'title' and self._tab:
            title = str(obj.title()) if obj.title() else ''
            if title:
                self._tab._sig_titleChanged.emit(title)
        elif str(keyPath) == 'estimatedProgress' and self._tab:
            pct = int(obj.estimatedProgress() * 100)
            self._tab._sig_loadProgress.emit(pct)


# ---------------------------------------------------------------------------
# WKBrowserTab -- PyQt6 QWidget wrapping a WKWebView
# ---------------------------------------------------------------------------
class WKBrowserTab(QWidget):
    """Drop-in replacement for BrowserTab(QWebEngineView).

    Exposes the same signals and methods that MainWindow expects:
        Signals: urlChanged, loadStarted, loadProgress, loadFinished, titleChanged
        Methods: setUrl, url, back, forward, reload, setHtml, page
    """

    # Signals matching QWebEngineView's interface
    urlChanged    = pyqtSignal(QUrl)
    loadStarted   = pyqtSignal()
    loadProgress   = pyqtSignal(int)
    loadFinished   = pyqtSignal(bool)
    titleChanged   = pyqtSignal(str)

    # Internal signals (used by ObjC delegates on any thread)
    _sig_urlChanged   = pyqtSignal(QUrl)
    _sig_loadStarted  = pyqtSignal()
    _sig_loadProgress = pyqtSignal(int)
    _sig_loadFinished = pyqtSignal(bool)
    _sig_titleChanged = pyqtSignal(str)

    def __init__(self, profile_config=None, parent=None):
        super().__init__(parent)

        # Wire internal signals to public signals (thread-safe)
        self._sig_urlChanged.connect(self.urlChanged.emit)
        self._sig_loadStarted.connect(self.loadStarted.emit)
        self._sig_loadProgress.connect(self.loadProgress.emit)
        self._sig_loadFinished.connect(self.loadFinished.emit)
        self._sig_titleChanged.connect(self.titleChanged.emit)

        # -- Create WKWebView configuration --------------------------------
        config = WebKit.WKWebViewConfiguration.alloc().init()

        # Share the process pool so all tabs run in the same WebKit process.
        # This gives them a shared DNS cache, HTTP/2 connection pool, JIT
        # cache, and in-memory resource cache -- the biggest single speedup.
        config.setProcessPool_(_SHARED_PROCESS_POOL)

        # Persistent data store for cookies, localStorage, etc.
        store_path = os.path.expanduser('~/.xcaliburmoon_wk_profile')
        os.makedirs(store_path, exist_ok=True)

        # Use default data store (persistent across sessions).
        data_store = WebKit.WKWebsiteDataStore.defaultDataStore()
        config.setWebsiteDataStore_(data_store)

        # Disable ITP at the data store level (controls cross-origin XHR/fetch
        # blocking used during uploads to api.linkedin.com and CDN endpoints).
        # Try several key names used across WebKit versions.
        for _dk in ('_resourceLoadStatisticsEnabled',
                    'resourceLoadStatisticsEnabled',
                    'isResourceLoadStatisticsEnabled'):
            try:
                data_store.setValue_forKey_(False, _dk)
            except Exception:
                pass

        # User content controller (for JS injection)
        self._content_controller = WebKit.WKUserContentController.alloc().init()
        # Attach compiled ad-block rules if available.
        if _WK_CONTENT_RULES is not None:
            self._content_controller.addContentRuleList_(_WK_CONTENT_RULES)
        config.setUserContentController_(self._content_controller)

        # Preferences
        prefs = config.preferences()
        prefs.setValue_forKey_(True, 'javaScriptEnabled')
        prefs.setValue_forKey_(True, 'javaScriptCanOpenWindowsAutomatically')
        # Developer extras (Web Inspector)
        prefs.setValue_forKey_(True, 'developerExtrasEnabled')
        # Note: _resourceLoadStatisticsEnabled and allowCrossOriginSubresources
        # do NOT exist as KVC keys on WKPreferences on this system. ITP is
        # disabled at the data store level above instead.
        # Enable JS fullscreen API (element.requestFullscreen() on videos).
        for _fsk in ('_fullScreenEnabled', 'fullScreenEnabled'):
            try:
                prefs.setValue_forKey_(True, _fsk)
            except Exception:
                pass

        # Media playback preferences
        config.setMediaTypesRequiringUserActionForPlayback_(0)  # WKAudiovisualMediaTypeNone
        config.setAllowsAirPlayForMediaPlayback_(True)

        # Increase WebKit's internal memory cache ceilings via private KVC
        # keys (ignored silently on versions that don't support them).
        for _ck, _cv in (
            ('_memoryLimit',                    512 * 1024 * 1024),  # 512 MB
            ('_pageCacheEnabled',               True),
            ('_allowUniversalAccessFromFileURLs', False),            # keep tight
        ):
            try:
                prefs.setValue_forKey_(_cv, _ck)
            except Exception:
                pass

        # Configuration-level caching keys (complements prefs above).
        for _ck2, _cv2 in (
            ('_offlineApplicationCacheIsEnabled', True),
            ('_serviceWorkerEntitlementDisabled', False),
            ('_serviceWorkersEnabled',            True),
        ):
            try:
                prefs.setValue_forKey_(_cv2, _ck2)
            except Exception:
                pass

        # Bump the speculative tile coverage so pages pre-render more
        # off-screen area (eliminates white flashes during fast scrolls).
        try:
            prefs.setValue_forKey_(True, '_tiledScrollingIndicatorVisible')
        except Exception:
            pass

        # -- Create WKWebView ----------------------------------------------
        self._wkview = WebKit.WKWebView.alloc().initWithFrame_configuration_(
            NSMakeRect(0, 0, 800, 600), config)
        self._wkview.setAllowsBackForwardNavigationGestures_(True)
        # Prevent pinch-to-zoom and any programmatic magnification change that
        # could be triggered by editor-framework focus animations on sites like
        # LinkedIn.  Default is already False but set it explicitly.
        try:
            self._wkview.setAllowsMagnification_(False)
        except Exception:
            pass

        # -- Configure NSScrollView for smooth, non-snapping scrolling ----
        # NSScrollElasticityAllowed = 0 keeps the natural macOS momentum
        # deceleration and overscroll bounce that users expect.
        # setScrollsDynamically_ ensures pixel-level live scrolling.
        try:
            sv = self._wkview.scrollView()
            if sv:
                sv.setVerticalScrollElasticity_(0)    # NSScrollElasticityAllowed
                sv.setHorizontalScrollElasticity_(0)  # NSScrollElasticityAllowed
                sv.setScrollsDynamically_(True)
        except Exception:
            pass

        # Allow WKWebView to render on a background thread where possible.
        # This keeps the main thread free for input handling.
        try:
            self._wkview.setValue_forKey_(True, '_shouldSuspendWhenInBackground')
        except Exception:
            pass
        # Disable the content inset automatic adjustment that can cause a
        # brief layout shift (1 px vertical jump) on first paint.
        try:
            self._wkview.setValue_forKey_(False, '_haveSetObscuredInsets')
        except Exception:
            pass
        # Use a larger tile size for the GPU compositor so fewer tiles need
        # to be (re-)painted during fast scrolls.
        try:
            self._wkview.setValue_forKey_(512, '_tileSize')
        except Exception:
            pass

        # Custom user agent
        ua = ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) '
              'AppleWebKit/605.1.15 (KHTML, like Gecko) '
              'Version/17.6 Safari/605.1.15')
        self._wkview.setCustomUserAgent_(ua)

        # -- Delegates (prevent GC by storing references) ------------------
        self._nav_delegate = _NavigationDelegate.alloc().initWithBrowserTab_(self)
        self._wkview.setNavigationDelegate_(self._nav_delegate)

        self._ui_delegate = _UIDelegate.alloc().initWithBrowserTab_(self)
        self._wkview.setUIDelegate_(self._ui_delegate)

        # -- KVO observers for title and progress --------------------------
        self._title_observer = _TitleObserver.alloc().initWithBrowserTab_(self)
        self._wkview.addObserver_forKeyPath_options_context_(
            self._title_observer, 'title', 0x01, None)  # NSKeyValueObservingOptionNew
        self._wkview.addObserver_forKeyPath_options_context_(
            self._title_observer, 'estimatedProgress', 0x01, None)

        # -- Embed WKWebView's NSView in a Qt widget -----------------------
        view_ptr = objc.pyobjc_id(self._wkview)
        self._foreign_window = QWindow.fromWinId(view_ptr)
        self._container = QWidget.createWindowContainer(
            self._foreign_window, self)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self._container)

        # -- Register WKScriptMessageHandler for console capture ----------
        self._console_handler = _ConsoleMessageHandler.alloc().initWithBrowserTab_(self)
        self._content_controller.addScriptMessageHandler_name_(
            self._console_handler, '_xcm_console')

        # -- Inject stealth + consent killer scripts -----------------------
        self._inject_userscripts()

        # -- Console panel reference (set by MainWindow) -------------------
        self._console_panel = None

        # -- Main window reference (set by MainWindow after creation) ------
        self._main_window = None

        # -- Track current URL (updated by delegate) -----------------------
        self._current_url = QUrl()
        self.urlChanged.connect(self._on_url_changed)

        # -- Enable fullscreen: set NSWindowCollectionBehavior after show --
        QTimer.singleShot(500, self._enable_host_window_fullscreen)

    def _enable_host_window_fullscreen(self):
        """Set NSWindowCollectionBehaviorFullScreenPrimary on the host Qt
        window so that element.requestFullscreen() can succeed in WKWebView."""
        try:
            from AppKit import NSWindowCollectionBehaviorFullScreenPrimary
            win_id = self.window().winId()
            if not win_id:
                return
            # winId() returns the NSView* as an integer.
            ns_view = objc.objc_object(c_void_p=int(win_id))
            ns_window = ns_view.window()
            if ns_window:
                cb = ns_window.collectionBehavior()
                ns_window.setCollectionBehavior_(
                    cb | NSWindowCollectionBehaviorFullScreenPrimary
                )
        except Exception:
            pass

    def _on_url_changed(self, qurl):
        self._current_url = qurl

    # ── Public API matching QWebEngineView ─────────────────────

    def setUrl(self, qurl):
        url_str = qurl.toString()
        nsurl = NSURL.URLWithString_(url_str)
        if nsurl:
            req = NSURLRequest.requestWithURL_(nsurl)
            self._wkview.loadRequest_(req)

    def url(self):
        return self._current_url

    def back(self):
        self._wkview.goBack()

    def forward(self):
        self._wkview.goForward()

    def reload(self):
        self._wkview.reload()

    def setHtml(self, html, base_url=None):
        base = NSURL.URLWithString_(base_url.toString()) if base_url else None
        self._wkview.loadHTMLString_baseURL_(html, base)

    def page(self):
        """Return a shim object that provides runJavaScript()
        and setDevToolsPage() for compatibility."""
        return self._page_shim

    @property
    def _page_shim(self):
        """Lazy shim so page().runJavaScript() works."""
        if not hasattr(self, '_shim'):
            self._shim = _PageShim(self)
        return self._shim

    def stop(self):
        self._wkview.stopLoading()

    def title(self):
        t = self._wkview.title()
        return str(t) if t else ''

    def _run_js(self, js: str):
        """Run JavaScript in the page with no return value."""
        self._wkview.evaluateJavaScript_completionHandler_(js, None)

    # ── JavaScript injection ───────────────────────────────────

    def _inject_userscripts(self):
        """Inject the consent-kill and stealth scripts.
        No video-fix script needed since WKWebView has native H.264."""
        from .js_constants import CONSENT_KILL_JS, STEALTH_JS

        # Consent killer at document end
        consent = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                CONSENT_KILL_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentEnd,
                True)
        self._content_controller.addUserScript_(consent)

        # Stealth script at document start, all frames
        stealth = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                STEALTH_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)
        self._content_controller.addUserScript_(stealth)

        # Console capture + noise filter.
        console_filter = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _CONSOLE_FILTER_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)
        self._content_controller.addUserScript_(console_filter)

        # Scroll-snap suppression -- injected at document START, all frames,
        # so it loads before any page stylesheet has a chance to set
        # scroll-snap-type. The MutationObserver re-applies it if the page
        # replaces <head> or removes the style element dynamically.
        _SCROLL_SNAP_KILL_JS = """
(function(){
    var _STYLE_ID = '__xcmScrollKill__';
    function _applyStyle() {
        if (document.getElementById(_STYLE_ID)) return;
        var s = document.createElement('style');
        s.id = _STYLE_ID;
        s.textContent = [
            '*, *::before, *::after {',
            '  scroll-snap-type: none !important;',
            '  scroll-snap-align: none !important;',
            '  scroll-snap-stop: normal !important;',
            '  scroll-behavior: auto !important;',
            '}',
            'html, body {',
            '  overscroll-behavior: contain !important;',
            '}'
        ].join('');
        (document.head || document.documentElement).appendChild(s);
    }
    _applyStyle();
    var _mo = new MutationObserver(_applyStyle);
    var _root = document.documentElement || document.body;
    if (_root) { _mo.observe(_root, {childList: true, subtree: false}); }
})();
"""
        scroll_snap_kill = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _SCROLL_SNAP_KILL_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)  # False = all frames (iframes too)
        self._content_controller.addUserScript_(scroll_snap_kill)

        # Resource-hint injector -- at document end, main frame only.
        # Scans every src/href on the page, extracts the unique origins, and
        # inserts <link rel="dns-prefetch"> + <link rel="preconnect"> elements.
        # This starts DNS resolution and TCP/TLS handshakes for sub-resources
        # that the browser has not yet discovered, shaving tens to hundreds of
        # milliseconds off subsequent requests on the same page or links to
        # the same origin.  A MutationObserver keeps the hints up to date as
        # SPAs swap content dynamically.
        _RESOURCE_HINTS_JS = """
(function(){
    var _seen = {};
    function _addHints(origin) {
        if (!origin || _seen[origin]) return;
        _seen[origin] = true;
        var head = document.head || document.documentElement;
        var dns = document.createElement('link');
        dns.rel = 'dns-prefetch'; dns.href = origin;
        head.appendChild(dns);
        var pc = document.createElement('link');
        pc.rel = 'preconnect'; pc.href = origin; pc.crossOrigin = 'anonymous';
        head.appendChild(pc);
    }
    function _scanEl(el) {
        var src = el.src || el.href || el.getAttribute('data-src') || '';
        if (!src) return;
        try {
            var u = new URL(src, location.href);
            if (u.origin && u.origin !== location.origin) _addHints(u.origin);
        } catch(e) {}
    }
    function _scanAll() {
        document.querySelectorAll('[src],[href]').forEach(_scanEl);
    }
    _scanAll();
    var _mo2 = new MutationObserver(function(muts) {
        muts.forEach(function(m) {
            m.addedNodes.forEach(function(n) {
                if (n.nodeType !== 1) return;
                _scanEl(n);
                n.querySelectorAll && n.querySelectorAll('[src],[href]').forEach(_scanEl);
            });
        });
    });
    var _body = document.body || document.documentElement;
    if (_body) { _mo2.observe(_body, {childList: true, subtree: true}); }
})();
"""
        resource_hints = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _RESOURCE_HINTS_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentEnd,
                True)  # main frame only -- sub-frames handle their own hints
        self._content_controller.addUserScript_(resource_hints)

        # Inject at DocumentCreation so the patch is in place before any site
        # JS runs.  Forces all programmatic focus() calls to use preventScroll
        # so editor frameworks (LinkedIn, Twitter, etc.) cannot trigger the
        # heavy scroll-zoom that WKWebView performs when an input is scrolled
        # into view via an automated focus call.
        _PREVENT_FOCUS_ZOOM_JS = """
(function() {
    'use strict';
    if (window.__xcmFocusPatchDone__) return;
    window.__xcmFocusPatchDone__ = true;
    var _origFocus = HTMLElement.prototype.focus;
    HTMLElement.prototype.focus = function (opts) {
        // Always use preventScroll:true for programmatic focus so WKWebView
        // does not auto-scroll/zoom the compositing layer.  The user's own
        // tap will still physically show the element because that sets the
        // real hit-test origin inside WebKit.
        var safe = Object.assign({}, opts || {}, { preventScroll: true });
        return _origFocus.call(this, safe);
    };
})();
"""
        prevent_focus_zoom = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _PREVENT_FOCUS_ZOOM_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)  # apply to all frames including LinkedIn sub-frames
        self._content_controller.addUserScript_(prevent_focus_zoom)

        # Input watcher: single-writer Int32Array atom for all pointer,
        # wheel, touch, keyboard, and scroll events.  Must be injected
        # BEFORE the CDM SDK so the atom exists when CDM SDK initialises.
        # All other modules read scroll/input state from this atom; nothing
        # else writes to it, eliminating the multi-boolean race condition.
        input_script = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _load_src_js('input-watcher.js'),
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)
        self._content_controller.addUserScript_(input_script)

        # CDM SDK: cooperative scheduler, read/write batcher, CSS containment,
        # passive-listener enforcement, MutationObserver feed-item tagging.
        # Replaces the old lightweight CDM_PERF_JS.
        cdm_sdk = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                CDM_SDK_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)
        self._content_controller.addUserScript_(cdm_sdk)

        # ── Performance injections from dev-browser/src/ ─────────────────────
        # Ticker: rAF Hz detector + tick bus (sets __xcmHz, __xcmFrameMs,
        # __xcmIdleThreshold, __xcmTick).  DocumentStart so other scripts
        # can read __xcmIdleThreshold during their first IO callback.
        ticker_script = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _TICKER_JS,
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)
        self._content_controller.addUserScript_(ticker_script)

        # Lazy: patches createElement for lazy/async media loading, IO-based
        # video, MutationObserver off main thread, CLS prevention via
        # aspect-ratio preservation.  DocumentStart, all frames.
        lazy_script = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _load_src_js('lazy-inject.js'),
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                False)
        self._content_controller.addUserScript_(lazy_script)

        # Virtualizer: applies content-visibility:auto + contain-intrinsic-size
        # to feed rows (article, ytd-*, Reddit/Twitter/LinkedIn selectors).
        # DocumentStart, main frame only.
        virt_script = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _load_src_js('virtualizer-inject.js'),
                WebKit.WKUserScriptInjectionTimeAtDocumentStart,
                True)
        self._content_controller.addUserScript_(virt_script)

        # Compress: proxies images through 127.0.0.1:7779/img for WebP
        # compression; falls back to OffscreenCanvas if server not running.
        # DocumentEnd, main frame only.
        compress_script = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _load_src_js('compress-inject.js'),
                WebKit.WKUserScriptInjectionTimeAtDocumentEnd,
                True)
        self._content_controller.addUserScript_(compress_script)

        # Stats monitor: rAF frame-time ring buffer, PerformanceObserver
        # long-task tracking, and Spector-style per-frame resource capture.
        # DocumentEnd, main frame only.  Exposes window.__xcm.stats.
        stats_script = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _load_src_js('stats-inject.js'),
                WebKit.WKUserScriptInjectionTimeAtDocumentEnd,
                True)
        self._content_controller.addUserScript_(stats_script)

        # WebGL GPU pre-warmer: creates a shared WebGL2 OffscreenCanvas,
        # decodes images via createImageBitmap (off main thread), and uploads
        # them as WebGL textures before they scroll into view.  Prevents
        # WKWebView compositor stalls from unready textures during scroll.
        # Also includes Spector-style captureFrame() for WebGL call inspection.
        # DocumentEnd, main frame only.  Exposes window.__xcm.gpu.
        gpu_script = WebKit.WKUserScript.alloc() \
            .initWithSource_injectionTime_forMainFrameOnly_(
                _load_src_js('webgl-engine.js'),
                WebKit.WKUserScriptInjectionTimeAtDocumentEnd,
                True)
        self._content_controller.addUserScript_(gpu_script)

    # ── Cleanup ────────────────────────────────────────────────

    def take_focus(self):
        """Make the WKWebView the Cocoa first responder so keyboard input
        and scrolling work immediately without requiring a click."""
        try:
            ns_window = self._wkview.window()
            if ns_window:
                ns_window.makeFirstResponder_(self._wkview)
        except Exception:
            pass

    def cleanup(self):
        """Stop any in-flight load, remove KVO observers, and release the
        WKWebView.  Call before destroying the widget."""
        # Stop the load first so no more navigation delegate callbacks fire.
        try:
            self._wkview.stopLoading()
        except Exception:
            pass
        try:
            self._wkview.removeObserver_forKeyPath_(self._title_observer, 'title')
            self._wkview.removeObserver_forKeyPath_(self._title_observer, 'estimatedProgress')
        except Exception:
            pass
        self._nav_delegate._tab = None
        self._ui_delegate._tab = None
        self._title_observer._tab = None
        # Remove script message handlers to break reference cycles.
        for _name in ('_xcm_console', '_xcm_catassist'):
            try:
                self._content_controller.removeScriptMessageHandlerForName_(_name)
            except Exception:
                pass
        self._console_handler._tab = None
        self._cat_handler._tab = None
        # Detach the foreign-window container before Qt destroys it.
        # On macOS, deleting a createWindowContainer widget while it still
        # has a parent can corrupt the host window's NSView hierarchy and
        # blank the entire main window.
        try:
            self._container.hide()
            self._container.setParent(None)
        except Exception:
            pass


class _PageShim:
    """Minimal shim providing page().runJavaScript() and page().title()
    for MainWindow / AIChatPanel compatibility."""

    def __init__(self, tab):
        self._tab = tab

    def runJavaScript(self, js, callback=None):
        """Execute JavaScript in the WKWebView."""
        if callback:
            def _handler(result, error):
                try:
                    callback(result)
                except Exception:
                    pass
            self._tab._wkview.evaluateJavaScript_completionHandler_(js, _handler)
        else:
            self._tab._wkview.evaluateJavaScript_completionHandler_(js, None)

    def title(self):
        """Return the page title (used by ai_panel.py)."""
        return self._tab.title()

    def setDevToolsPage(self, page):
        """No-op. WKWebView uses Safari Web Inspector instead."""
        pass

    def setPage(self, val):
        """No-op shim for closeEvent compatibility."""
        pass

    def deleteLater(self):
        """No-op shim for closeEvent compatibility."""
        pass


# ---------------------------------------------------------------------------
# WKPersistentProfile -- replaces PersistentProfile for WKWebView setup
# ---------------------------------------------------------------------------
class WKPersistentProfile:
    """Configuration holder for WKWebView-based tabs.
    Unlike QWebEngineProfile, WKWebView's configuration is per-view,
    so this class just provides a setup_profile() that returns a
    config dict (not a Qt object)."""

    @staticmethod
    def setup_profile():
        """Return a configuration dict. WKBrowserTab reads this.\n        Also compiles the WKContentRuleList ad blocker."""
        _build_wk_rules()
        return {
            'engine': 'wkwebview',
            'profile_path': os.path.expanduser('~/.xcaliburmoon_wk_profile'),
        }
