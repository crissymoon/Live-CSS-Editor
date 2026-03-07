"""
qt_virt.py -- Qt Chromium overlay for virt-pages.

Provides a borderless, stays-on-top QWebEngineView window that is spawned by
cf_bridge over specific URLs (e.g. Stripe billing pages) that WKWebView cannot
render correctly.  The overlay is positioned to exactly cover the imgui
browser's content area, giving a seamless embedded feel.

Managed via the HTTP API on port 9925:
    POST /virt-show  {"url":"...", "x":N, "y":N, "w":N, "h":N}
    POST /virt-hide  {}
    POST /virt-move  {"x":N, "y":N, "w":N, "h":N}

Coordinates use Qt screen convention: origin at top-left of primary screen,
y increases downward (same as what C++ webview_get_content_screen_rect returns).
"""
from __future__ import annotations

import logging

from PyQt6.QtCore import (
    QEasingCurve, QPropertyAnimation, Qt, QUrl, pyqtProperty, pyqtSlot,
)
from PyQt6.QtWebEngineCore import QWebEngineProfile
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWidgets import QMainWindow, QWidget

log = logging.getLogger(__name__)


class VirtOverlay(QMainWindow):
    """
    Borderless, always-on-top Chromium window that covers the imgui browser
    content area for specific pages.

    Parameters
    ----------
    profile :
        QWebEngineProfile to use for the inner view.  Pass the same profile
        that cf_bridge uses so that saved cf_clearance / session cookies are
        available inside the overlay.
    """

    def __init__(self, profile: QWebEngineProfile | None = None) -> None:
        super().__init__()

        # Frameless + stays on top.  No title bar -- looks embedded.
        self.setWindowFlags(
            Qt.WindowType.FramelessWindowHint |
            Qt.WindowType.WindowStaysOnTopHint
        )
        self.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, False)

        # Internal opacity tracker (QPropertyAnimation needs a proper pyqtProperty)
        self._opacity: float = 0.0

        if profile is not None:
            from PyQt6.QtWebEngineCore import QWebEnginePage
            self._view = QWebEngineView(self)
            page = QWebEnginePage(profile, self._view)
            self._view.setPage(page)
        else:
            self._view = QWebEngineView(self)

        self.setCentralWidget(self._view)
        self.setWindowOpacity(0.0)

        # Fade animation --------------------------------------------------
        self._anim = QPropertyAnimation(self, b"opacity_prop")
        self._anim.setDuration(220)
        self._anim.setEasingCurve(QEasingCurve.Type.InOutQuad)

    # ── Qt property wrapper so QPropertyAnimation can drive windowOpacity ──

    @pyqtProperty(float)
    def opacity_prop(self) -> float:  # type: ignore[override]
        return self._opacity

    @opacity_prop.setter  # type: ignore[no-redef]
    def opacity_prop(self, value: float) -> None:
        self._opacity = value
        self.setWindowOpacity(value)

    # ── Public API ────────────────────────────────────────────────────────

    @pyqtSlot(str, int, int, int, int)
    def show_at(self, url: str, x: int, y: int, w: int, h: int) -> None:
        """Show the overlay at (x, y, w, h) navigated to url."""
        # Disconnect any pending hide-done signal
        try:
            self._anim.finished.disconnect()
        except Exception:
            pass

        self.setGeometry(x, y, w, h)

        current_url = self._view.url().toString()
        if current_url != url:
            self._view.setUrl(QUrl(url))

        if not self.isVisible():
            self.setWindowOpacity(0.0)
            self._opacity = 0.0
            self.show()

        self._anim.stop()
        self._anim.setStartValue(self._opacity)
        self._anim.setEndValue(1.0)
        self._anim.start()
        self.raise_()

        log.info("[qt_virt] show_at %s @ %dx%d+%d+%d", url, w, h, x, y)

    @pyqtSlot()
    def hide_overlay(self) -> None:
        """Fade out and hide the overlay."""
        if not self.isVisible():
            return

        try:
            self._anim.finished.disconnect()
        except Exception:
            pass

        self._anim.stop()
        self._anim.setStartValue(self._opacity)
        self._anim.setEndValue(0.0)
        self._anim.finished.connect(self._on_hide_done)
        self._anim.start()
        log.info("[qt_virt] hide_overlay")

    def _on_hide_done(self) -> None:
        try:
            self._anim.finished.disconnect()
        except Exception:
            pass
        if self._opacity < 0.05:
            self.hide()

    @pyqtSlot(int, int, int, int)
    def move_overlay(self, x: int, y: int, w: int, h: int) -> None:
        """Reposition/resize the overlay (called when imgui window moves)."""
        if self.isVisible():
            self.setGeometry(x, y, w, h)
