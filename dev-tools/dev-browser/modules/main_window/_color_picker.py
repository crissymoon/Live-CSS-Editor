"""
_ColorPickOverlay: fullscreen eyedropper overlay.
Grabs a screenshot via macOS screencapture, shows a magnified loupe,
and emits color_picked(hex_str) when the user clicks a pixel.
"""

import subprocess

from PyQt6.QtCore import Qt, QRectF, QRect
from PyQt6.QtWidgets import QLabel
from PyQt6.QtGui import (
    QColor, QPixmap, QCursor, QPainter, QFont, QPen, QPainterPath,
)
from PyQt6.QtCore import pyqtSignal


class _ColorPickOverlay(QLabel):
    """Fullscreen overlay that shows a magnified loupe around the cursor.
    Click any pixel to emit *color_picked* with its hex string.
    Uses macOS screencapture for a reliable grab, then sets itself as
    a fixed-size QLabel with the screenshot as background."""

    color_picked = pyqtSignal(str)

    _LOUPE_R = 80
    _ZOOM = 4

    def __init__(self, parent=None, pre_shot: QPixmap = None):
        super().__init__(None)
        self.setWindowFlags(
            Qt.WindowType.FramelessWindowHint
            | Qt.WindowType.WindowStaysOnTopHint
            | Qt.WindowType.Window
        )
        self.setCursor(Qt.CursorShape.CrossCursor)
        self.setMouseTracking(True)
        self._caller = parent
        self._tmp = None

        from PyQt6.QtWidgets import QApplication as _QApp
        screen = _QApp.primaryScreen()
        geo = screen.geometry() if screen else None

        if pre_shot and not pre_shot.isNull():
            # Use the screenshot grabbed before this window was created so
            # that the browser content is visible rather than the desktop.
            self._shot = pre_shot
        else:
            # Fallback: screencapture subprocess (may show desktop on macOS
            # if the browser lost focus during window construction).
            import tempfile
            self._tmp = tempfile.NamedTemporaryFile(suffix='.png', delete=False)
            self._tmp.close()
            subprocess.call(
                ['screencapture', '-x', '-C', self._tmp.name],
                timeout=5,
            )
            self._shot = QPixmap(self._tmp.name)
            if self._shot.isNull() and screen:
                self._shot = screen.grabWindow(0)

        # Do NOT scale the pixmap. Qt paints it at the correct logical size
        # automatically via devicePixelRatio. Scaling to logical pixels would
        # discard half the physical pixels on Retina displays, making the image
        # appear tiny in the top-left corner.
        # Store DPR so mouse coordinates (logical) map correctly to image pixels.
        self._dpr = self._shot.devicePixelRatio() if not self._shot.isNull() else 1.0

        self._image = self._shot.toImage()

        if geo:
            self.setGeometry(geo)
        else:
            self.setFixedSize(self._shot.size())

        self._mx = 0
        self._my = 0

    # -- cleanup -----------------------------------------------------------

    def _cleanup(self):
        import os
        if self._tmp is not None:
            try:
                os.unlink(self._tmp.name)
            except OSError:
                pass
        self.deleteLater()

    def closeEvent(self, ev):
        self._cleanup()
        super().closeEvent(ev)

    # -- events ------------------------------------------------------------

    def mouseMoveEvent(self, ev):
        self._mx = int(ev.position().x())
        self._my = int(ev.position().y())
        self.update()

    def mousePressEvent(self, ev):
        x = max(0, min(int(ev.position().x() * self._dpr), self._image.width() - 1))
        y = max(0, min(int(ev.position().y() * self._dpr), self._image.height() - 1))
        c = QColor(self._image.pixel(x, y))
        self.color_picked.emit(c.name())
        self.close()

    def keyPressEvent(self, ev):
        if ev.key() == Qt.Key.Key_Escape:
            self.close()

    # -- painting ----------------------------------------------------------

    def paintEvent(self, ev):
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing)

        p.drawPixmap(0, 0, self._shot)
        p.fillRect(self.rect(), QColor(0, 0, 0, 50))

        mx, my = self._mx, self._my
        R = self._LOUPE_R
        Z = self._ZOOM
        src_half = R // Z

        # Source rect is in physical image pixels; convert logical coords by DPR.
        imx = int(mx * self._dpr)
        imy = int(my * self._dpr)
        src_rect = QRect(imx - src_half, imy - src_half, src_half * 2, src_half * 2)
        dst_rect = QRectF(mx - R, my - R, R * 2, R * 2)

        path = QPainterPath()
        path.addEllipse(dst_rect)
        p.setClipPath(path)
        p.drawPixmap(dst_rect, self._shot, QRectF(src_rect))
        p.setClipping(False)

        ring_pen = QPen(QColor('#6366f1'))
        ring_pen.setWidth(2)
        p.setPen(ring_pen)
        p.setBrush(Qt.BrushStyle.NoBrush)
        p.drawEllipse(dst_rect)

        ch_pen = QPen(QColor(255, 255, 255, 160))
        ch_pen.setWidth(1)
        p.setPen(ch_pen)
        p.drawLine(mx - 6, my, mx + 6, my)
        p.drawLine(mx, my - 6, mx, my + 6)

        px = max(0, min(int(mx * self._dpr), self._image.width() - 1))
        py = max(0, min(int(my * self._dpr), self._image.height() - 1))
        c = QColor(self._image.pixel(px, py))
        hex_str = c.name()

        lbl_x = mx - 40
        lbl_y = my + R + 10
        lbl_rect = QRectF(lbl_x, lbl_y, 80, 22)
        p.setPen(Qt.PenStyle.NoPen)
        p.setBrush(QColor(22, 22, 43, 220))
        p.drawRoundedRect(lbl_rect, 4, 4)

        p.setPen(QColor('#e8e8f0'))
        p.setFont(QFont('JetBrains Mono', 11))
        p.drawText(lbl_rect, Qt.AlignmentFlag.AlignCenter, hex_str)

        sw_rect = QRectF(lbl_x + 84, lbl_y + 2, 18, 18)
        p.setBrush(c)
        p.setPen(QPen(QColor('#ffffff'), 1))
        p.drawRoundedRect(sw_rect, 3, 3)

        p.end()
