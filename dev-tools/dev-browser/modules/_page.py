"""
CustomWebEnginePage -- QWebEnginePage subclass with:
  - stealth script injection (runs before page JS)
  - suppressed noisy console messages
  - native JS alert / confirm / prompt dialogs
  - popup / new-window routing
"""
from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import QApplication, QMessageBox, QInputDialog
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebEngineCore import (
    QWebEnginePage,
    QWebEngineScript,
)
from ._js_injections import _STEALTH_JS

class CustomWebEnginePage(QWebEnginePage):
    def __init__(self, profile, parent=None):
        super().__init__(profile, parent)
        # Grant all feature permission requests (camera, mic, geo, clipboard, etc.)
        self.featurePermissionRequested.connect(self._grant_permission)

        # Inject stealth script at document creation so it runs before page JS.
        script = QWebEngineScript()
        script.setName('_stealth')
        script.setSourceCode(_STEALTH_JS)
        script.setInjectionPoint(
            QWebEngineScript.InjectionPoint.DocumentCreation
        )
        script.setWorldId(QWebEngineScript.ScriptWorldId.MainWorld)
        # Do not run stealth in subframes: the anti-bot checks (navigator.webdriver,
        # window.chrome, etc.) are only relevant in the top-level context.  Running
        # it in every React lazy-chunk iframe doubles injection overhead for nothing.
        script.setRunsOnSubFrames(False)
        self.scripts().insert(script)

    # ── Suppress noisy JS console messages ─────────────────────
    # Some sites (LinkedIn, etc.) emit repetitive telemetry errors
    # like "Attribute 'exception.tags' of type 'object' could not
    # be converted to a proto attribute" from their OpenTelemetry
    # or Sentry instrumentation.  Filter them out so the debug
    # console stays useful.
    _SUPPRESSED_MSGS = (
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
        'load event sent successfully',
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
        'Access-Control-Allow-Origin',
        'ERR_BLOCKED',
        'The operation was aborted',
        'cancelled',
        # Chrome injects inline speculation rules on every page for prerender;
        # strict page CSPs block them.  The error is harmless since it only
        # means prefetch prerender is disabled for that page.
        'Refused to apply inline speculation rules',
    )

    def javaScriptConsoleMessage(self, level, message, line, source):
        for pattern in self._SUPPRESSED_MSGS:
            if pattern in message:
                return  # swallow silently
        # Forward to ConsolePanel if attached to the parent BrowserTab.
        panel = getattr(self.parent(), '_console_panel', None)
        if panel:
            _LEVEL_MAP = {
                QWebEnginePage.JavaScriptConsoleMessageLevel.InfoMessageLevel: 'info',
                QWebEnginePage.JavaScriptConsoleMessageLevel.WarningMessageLevel: 'warn',
                QWebEnginePage.JavaScriptConsoleMessageLevel.ErrorMessageLevel: 'error',
            }
            lvl = _LEVEL_MAP.get(level, 'log')
            src = None
            if source:
                parts = source.split('/')
                src = (parts[-1] or source).split('?')[0]
            panel.append_message(lvl, message, src=src, line=line)
        super().javaScriptConsoleMessage(level, message, line, source)

    def _grant_permission(self, origin, feature):
        self.setFeaturePermission(
            origin, feature,
            QWebEnginePage.PermissionPolicy.PermissionGrantedByUser,
        )

    # ── JavaScript dialogs ─────────────────────────────────────

    def javaScriptAlert(self, origin, msg):
        dlg = QMessageBox(QApplication.activeWindow())
        dlg.setWindowTitle(origin.host() or 'Page Alert')
        dlg.setText(msg)
        dlg.setIcon(QMessageBox.Icon.Information)
        dlg.setStandardButtons(QMessageBox.StandardButton.Ok)
        dlg.exec()

    def javaScriptConfirm(self, origin, msg):
        dlg = QMessageBox(QApplication.activeWindow())
        dlg.setWindowTitle(origin.host() or 'Page Confirm')
        dlg.setText(msg)
        dlg.setIcon(QMessageBox.Icon.Question)
        dlg.setStandardButtons(
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        dlg.setDefaultButton(QMessageBox.StandardButton.Yes)
        return dlg.exec() == QMessageBox.StandardButton.Yes

    def javaScriptPrompt(self, origin, msg, default_value):
        text, ok = QInputDialog.getText(
            QApplication.activeWindow(),
            origin.host() or 'Page Input',
            msg,
            text=default_value,
        )
        if ok:
            return True, text
        return False, ''

    def createWindow(self, win_type):
        """Route popup/dialog requests to a floating window; tabs for everything else."""
        try:
            WebDialog = QWebEnginePage.WebWindowType.WebDialog
            WebBrowserWindow = QWebEnginePage.WebWindowType.WebBrowserWindow

            # webbrowse_no_controls.py sets _popup_mode=True on MainWindow so
            # that all createWindow calls open as visible floating popups.
            # view() and _main_window are set directly, no fragile parent walk.
            popup_mode = False
            view = self.view()
            print('[createWindow] win_type=%s view=%r' % (win_type, view), flush=True)
            if view is not None:
                mw = getattr(view, '_main_window', None)
                print('[createWindow] mw=%r _popup_mode=%r' % (mw, getattr(mw, '_popup_mode', None)), flush=True)
                if mw is not None:
                    popup_mode = getattr(mw, '_popup_mode', False)

            if win_type in (WebDialog, WebBrowserWindow) or popup_mode:
                # Open as a separate floating QWebEngineView window, not a tab.
                # The profile is taken from this page so cookies/session are shared.
                popup_view = QWebEngineView()
                popup_view.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)
                popup_page = CustomWebEnginePage(self.profile(), popup_view)
                popup_view.setPage(popup_page)
                popup_view.resize(900, 700)
                popup_view.setWindowTitle('Popup')
                popup_view.setWindowFlag(Qt.WindowType.WindowStaysOnTopHint, True)
                popup_page.titleChanged.connect(
                    lambda t, v=popup_view: v.setWindowTitle(t) if t else None
                )
                popup_view.show()
                popup_view.raise_()
                popup_view.activateWindow()
                return popup_page

            # Default: open in a new tab.
            main = self.parent()
            while main and not hasattr(main, 'add_new_tab'):
                main = main.parent()
            if main:
                main.add_new_tab(None, 'New Tab')
                w = main.tabs.currentWidget()
                if w and hasattr(w, 'page'):
                    return w.page()
            return None
        except Exception as exc:
            print('[createWindow] error: %s' % exc, flush=True)
            return None
