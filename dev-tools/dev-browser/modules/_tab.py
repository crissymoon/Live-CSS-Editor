"""
BrowserTab -- QWebEngineView subclass with:
  - scroll event de-duplication filter (macOS inertia fix)
  - keyboard shortcut routing (Cmd+C/X/V/A/Z)
"""
from PyQt6.QtCore import Qt, QEvent, QTimer
from PyQt6.QtWidgets import QApplication
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebEngineCore import QWebEnginePage
from ._page import CustomWebEnginePage

class BrowserTab(QWebEngineView):
    def __init__(self, profile, parent=None):
        super().__init__(parent)
        self.setPage(CustomWebEnginePage(profile, self))
        self._scroll_filter_installed = False
        # Track last forwarded phase so duplicate ScrollEnd can be dropped.
        self._last_wheel_phase = None
        # Reinstall the scroll filter after each navigation: Chromium spawns
        # a new renderer child widget on every page load so focusProxy()
        # changes and the previously installed filter no longer fires.
        self.loadFinished.connect(self._on_load_finished_scroll)

    def _on_load_finished_scroll(self):
        """Reset and reinstall the scroll filter after each page load.
        Chromium creates a new renderer child widget per navigation,
        changing focusProxy(), so the old filter installation is stale."""
        self._scroll_filter_installed = False
        QTimer.singleShot(0, self._install_scroll_filter)

    def _install_scroll_filter(self):
        if self._scroll_filter_installed:
            return
        proxy = self.focusProxy()
        if proxy and proxy is not self:
            proxy.installEventFilter(self)
            self._scroll_filter_installed = True

    def showEvent(self, event):
        super().showEvent(event)
        self._install_scroll_filter()

    def focusInEvent(self, event):
        super().focusInEvent(event)
        self._install_scroll_filter()

    def eventFilter(self, watched, event):
        if event.type() != QEvent.Type.Wheel:
            return False

        px    = event.pixelDelta()
        phase = event.phase()

        # RULE 1: consume zero-delta ScrollUpdate duplicates silently.
        # Qt delivers every trackpad packet twice on macOS; the second copy
        # always has pixel=(0,0) angle=(0,0) and carries no position data.
        if px.x() == 0 and px.y() == 0 and phase == Qt.ScrollPhase.ScrollUpdate:
            return True

        # RULE 2: consume the second consecutive ScrollEnd silently.
        # macOS emits ScrollEnd twice per gesture.
        if phase == Qt.ScrollPhase.ScrollEnd:
            if self._last_wheel_phase == Qt.ScrollPhase.ScrollEnd:
                return True

        # Pass the original event through unmodified so Chromium receives
        # the native NSEvent backing it (synthetic re-sends have no NSEvent
        # and are ignored by the Chromium renderer).
        self._last_wheel_phase = phase
        return False

    def contextMenuEvent(self, event):
        # Suppress Qt's native context menu so the JS contextmenu
        # handler in cat_assist can show the custom overlay menu instead.
        event.accept()

    def keyPressEvent(self, event):
        # Explicitly route Cmd+C/X/V/A/Z/Shift+Z to the page actions so
        # the shortcuts work reliably regardless of Qt focus routing.
        # On macOS Qt maps the Cmd key to ControlModifier.
        key  = event.key()
        mods = event.modifiers()
        ctrl = Qt.KeyboardModifier.ControlModifier
        wa   = QWebEnginePage.WebAction
        if mods == ctrl:
            _map = {
                Qt.Key.Key_C: wa.Copy,
                Qt.Key.Key_X: wa.Cut,
                Qt.Key.Key_V: wa.Paste,
                Qt.Key.Key_A: wa.SelectAll,
                Qt.Key.Key_Z: wa.Undo,
            }
            if key in _map:
                self.triggerPageAction(_map[key])
                # Flash the status bar so the user sees the action fired.
                _labels = {
                    Qt.Key.Key_C: 'Copied',
                    Qt.Key.Key_X: 'Cut',
                    Qt.Key.Key_V: 'Pasted',
                    Qt.Key.Key_A: 'Selected all',
                    Qt.Key.Key_Z: 'Undo',
                }
                mw = getattr(self, '_main_window', None)
                if mw is not None:
                    mw._flash_status(_labels[key], 900)
                event.accept()
                return
        elif mods == (ctrl | Qt.KeyboardModifier.ShiftModifier):
            if key == Qt.Key.Key_Z:
                self.triggerPageAction(wa.Redo)
                mw = getattr(self, '_main_window', None)
                if mw is not None:
                    mw._flash_status('Redo', 900)
                event.accept()
                return
        super().keyPressEvent(event)


# ---------------------------------------------------------------------------
# Speculation Rules: prefetch on hover for same-origin links
# ---------------------------------------------------------------------------
# Chromium 121+ / QtWebEngine 6.7+ supports the Speculation Rules API.
# Injected at DocumentReady so the link list is available.
# eagerness:'moderate' fires a network prefetch when the user hovers a link
# for ~200ms -- the page is already downloading before the click lands.
# Capped at 12 URLs per page to avoid wasting bandwidth.
