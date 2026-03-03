"""
_ViewportMixin: viewport size picker and resize helpers.
"""

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QLabel, QComboBox, QPushButton,
    QInputDialog, QMessageBox,
)


class _ViewportMixin:
    """Methods for the viewport size picker in the status bar."""

    _VP_PRESETS = [
        ('320 x 568   iPhone SE 1st',        320,  568),
        ('375 x 667   iPhone SE 3rd',        375,  667),
        ('390 x 844   iPhone 14',            390,  844),
        ('414 x 896   iPhone XR',            414,  896),
        ('428 x 926   iPhone 14 Plus',       428,  926),
        ('768 x 1024  iPad portrait',        768, 1024),
        ('1024 x 768  iPad landscape',      1024,  768),
        ('1280 x 800  Small laptop',        1280,  800),
        ('1366 x 768  Common laptop',       1366,  768),
        ('1440 x 900  MacBook Pro 14"',     1440,  900),
        ('1536 x 864  Surface Pro',         1536,  864),
        ('1920 x 1080 Full HD',             1920, 1080),
        ('2560 x 1440 QHD',                 2560, 1440),
    ]

    _VP_WIDGET_STYLE = (
        'QWidget#vpBar { background: transparent; }'
        'QLabel#vpLabel { color: #6366f1; font-family: "JetBrains Mono", monospace;'
        ' font-size: 10px; padding: 0 3px 0 6px; }'
        'QLabel#vpSize { color: #a5b4fc; font-family: "JetBrains Mono", monospace;'
        ' font-size: 10px; padding: 0 2px; min-width: 80px; }'
        'QComboBox {'
        '  background: #1e1e35; color: #c4c4de;'
        '  font-family: "JetBrains Mono", monospace; font-size: 10px;'
        '  border: 1px solid rgba(99,102,241,0.4); border-radius: 3px;'
        '  padding: 1px 18px 1px 5px; min-width: 90px;'
        '}'
        'QComboBox:hover { border-color: #6366f1; }'
        'QComboBox::drop-down { border: none; width: 16px; }'
        'QComboBox QAbstractItemView {'
        '  background: #16162b; color: #c4c4de;'
        '  selection-background-color: #312e81;'
        '  font-family: "JetBrains Mono", monospace; font-size: 10px;'
        '  border: 1px solid rgba(99,102,241,0.5);'
        '}'
        'QPushButton#vpCopy {'
        '  background: #1e1e35; color: #6366f1;'
        '  font-family: "JetBrains Mono", monospace; font-size: 10px;'
        '  border: 1px solid rgba(99,102,241,0.4); border-radius: 3px;'
        '  padding: 1px 7px;'
        '}'
        'QPushButton#vpCopy:hover { background: #312e81; color: #fff; }'
        'QPushButton#vpCopy:pressed { background: #4338ca; }'
    )

    def _build_viewport_widget(self) -> QWidget:
        container = QWidget()
        container.setObjectName('vpBar')
        row = QHBoxLayout(container)
        row.setContentsMargins(0, 0, 4, 0)
        row.setSpacing(3)

        lbl = QLabel('Viewport:')
        lbl.setObjectName('vpLabel')
        row.addWidget(lbl)

        combo = QComboBox()
        combo.setObjectName('vpCombo')
        combo.addItem('-- presets --')
        for preset_label, w, h in self._VP_PRESETS:
            combo.addItem(preset_label, (w, h))
        combo.addItem('Custom...', 'custom')
        combo.setToolTip('Resize window to a preset viewport for media query testing')
        combo.activated.connect(self._on_viewport_preset_activated)
        self._vp_combo = combo
        row.addWidget(combo)

        size_lbl = QLabel('---- x ----')
        size_lbl.setObjectName('vpSize')
        size_lbl.setToolTip('Current content viewport size')
        self._vp_size_label = size_lbl
        row.addWidget(size_lbl)

        copy_btn = QPushButton('Copy')
        copy_btn.setObjectName('vpCopy')
        copy_btn.setToolTip('Copy size as CSS @media query (max-width)')
        copy_btn.clicked.connect(self._copy_viewport_size)
        row.addWidget(copy_btn)

        container.setStyleSheet(self._VP_WIDGET_STYLE)
        return container

    def _content_viewport_size(self) -> tuple:
        """Return the current content area size (tabs widget, excludes toolbar/statusbar)."""
        cw = self.tabs.width()
        ch = self.tabs.height()
        return cw, ch

    def _update_viewport_display(self):
        """Refresh the size label with the current content viewport dimensions."""
        if not hasattr(self, '_vp_size_label'):
            return
        cw, ch = self._content_viewport_size()
        self._vp_size_label.setText(f'{cw} x {ch}')

    def _on_viewport_preset_activated(self, index: int):
        if index <= 0:
            return
        data = self._vp_combo.itemData(index)
        if data == 'custom':
            self._set_custom_viewport_size()
            self._vp_combo.setCurrentIndex(0)
            return
        if isinstance(data, tuple):
            target_w, target_h = data
            self._resize_to_viewport(target_w, target_h)
        self._vp_combo.setCurrentIndex(0)

    def _resize_to_viewport(self, vp_w: int, vp_h: int):
        """Resize the main window so the content area matches vp_w x vp_h."""
        from PyQt6.QtWidgets import QApplication as _QApp
        toolbar_h   = self.navbar.height() if hasattr(self, 'navbar') else 0
        statusbar_h = self._status_bar.height()
        frame_extra_h = toolbar_h + statusbar_h
        win_w = vp_w
        win_h = vp_h + frame_extra_h

        self.resize(win_w, win_h)

        screen = _QApp.screenAt(self.pos()) or _QApp.primaryScreen()
        if screen is None:
            return
        avail = screen.availableGeometry()

        cur_x = self.x()
        cur_y = self.y()

        if cur_x + win_w > avail.right() + 1:
            cur_x = avail.right() + 1 - win_w
        if cur_y + win_h > avail.bottom() + 1:
            cur_y = avail.bottom() + 1 - win_h
        cur_x = max(cur_x, avail.left())
        cur_y = max(cur_y, avail.top())

        self.move(cur_x, cur_y)

    def _set_custom_viewport_size(self):
        """Prompt the user for a custom WxH and apply it."""
        text, ok = QInputDialog.getText(
            self, 'Custom Viewport Size',
            'Enter size as W x H  (e.g. 1280 x 720):',
        )
        if not ok or not text.strip():
            return
        text = text.strip().lower().replace('x', ' ').replace(',', ' ')
        parts = [p for p in text.split() if p.isdigit()]
        if len(parts) < 2:
            QMessageBox.warning(self, 'Viewport', 'Enter two numbers separated by x  e.g. 1280 x 720')
            return
        w, h = int(parts[0]), int(parts[1])
        if w < 100 or h < 100:
            QMessageBox.warning(self, 'Viewport', 'Width and height must both be >= 100')
            return
        self._resize_to_viewport(w, h)

    def _copy_viewport_size(self):
        """Copy the current content viewport width as a CSS media query string."""
        from PyQt6.QtWidgets import QApplication as _QApp
        cw, ch = self._content_viewport_size()
        text = f'@media (max-width: {cw}px)'
        _QApp.clipboard().setText(text)
        self._vp_size_label.setText('Copied!')
        QTimer.singleShot(900, self._update_viewport_display)
