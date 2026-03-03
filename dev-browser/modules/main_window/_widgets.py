"""
Reusable widget classes used by MainWindow.
  - BrowserTabBar  : QTabBar with a visible close button on every tab.
  - HistoryLineEdit: URL bar with select-all-on-click, history dropdown, persistence.
"""

import os
import json

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtWidgets import (
    QTabBar, QToolButton, QLineEdit, QFrame, QVBoxLayout,
    QPushButton, QListWidget, QMessageBox,
)


class BrowserTabBar(QTabBar):
    """QTabBar that places a visible x close button on every tab."""

    _BTN_STYLE = (
        'QToolButton {'
        '  color: #8888a0; background: transparent; border: none;'
        '  font-size: 14px; font-weight: bold; padding: 0; margin: 0 2px;'
        '}'
        'QToolButton:hover { color: #ffffff; background: #ef4444; border-radius: 3px; }'
    )

    def tabInserted(self, index: int):
        super().tabInserted(index)
        btn = QToolButton()
        btn.setText('\u00d7')   # x
        btn.setFixedSize(17, 17)
        btn.setStyleSheet(self._BTN_STYLE)
        btn.setToolTip('Close tab')
        btn.clicked.connect(lambda: self._close_clicked(btn))
        self.setTabButton(index, QTabBar.ButtonPosition.RightSide, btn)

    def _close_clicked(self, btn: QToolButton):
        for i in range(self.count()):
            if self.tabButton(i, QTabBar.ButtonPosition.RightSide) is btn:
                self.tabCloseRequested.emit(i)
                return


class HistoryLineEdit(QLineEdit):
    """URL bar with select-all-on-click, typed-history dropdown, and persistence."""

    MAX_HISTORY  = 200
    SHOW_RESULTS = 4
    _HISTORY_FILE = os.path.join(
        os.path.dirname(__file__), '..', 'url_history.json'
    )

    _POPUP_STYLE = (
        'QFrame { background:#16162b; border:1px solid rgba(99,102,241,0.55);'
        ' border-radius:0 0 6px 6px; }'
        'QListWidget { background:transparent; border:none; color:#e8e8f0;'
        ' font-family:"SF Mono",monospace; font-size:13px; outline:none; }'
        'QListWidget::item { padding:7px 10px; border:none; }'
        'QListWidget::item:selected, QListWidget::item:hover'
        ' { background:#1f1f35; color:#ffffff; }'
        'QPushButton { background:transparent; color:#6366f1; border:none;'
        ' border-top:1px solid rgba(255,255,255,0.07);'
        ' padding:7px 10px; font-size:12px; text-align:left; }'
        'QPushButton:hover { background:#1f1f35; color:#ef4444; }'
    )

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._click_selects = False
        self._history: list  = []
        self._popup          = None
        self._popup_list     = None
        self._load_history()
        self.textEdited.connect(self._on_text_edited)

    # -- Select-all on first click -----------------------------------------

    def focusInEvent(self, event):
        super().focusInEvent(event)
        self._click_selects = True

    def mousePressEvent(self, event):
        super().mousePressEvent(event)
        if self._click_selects:
            self.selectAll()
            self._click_selects = False

    # -- History persistence -----------------------------------------------

    def _load_history(self):
        try:
            path = os.path.normpath(self._HISTORY_FILE)
            if os.path.exists(path):
                with open(path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    if isinstance(data, list):
                        self._history = data[:self.MAX_HISTORY]
        except Exception:
            self._history = []

    def _save_history(self):
        try:
            path = os.path.normpath(self._HISTORY_FILE)
            with open(path, 'w', encoding='utf-8') as f:
                json.dump(self._history, f, indent=2)
        except Exception:
            pass

    def add_url(self, url: str):
        if not url or url.startswith('data:') or url in ('about:blank', ''):
            return
        if url in self._history:
            self._history.remove(url)
        self._history.insert(0, url)
        if len(self._history) > self.MAX_HISTORY:
            self._history = self._history[:self.MAX_HISTORY]
        self._save_history()

    def _clear_history(self):
        self._hide_popup()
        reply = QMessageBox.question(
            self.window(), 'Clear History',
            'Are you sure you want to clear all browsing history?',
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if reply == QMessageBox.StandardButton.Yes:
            self._history.clear()
            self._save_history()

    # -- Dropdown popup ----------------------------------------------------

    def _on_text_edited(self, text: str):
        if not text:
            self._hide_popup()
            return
        tl = text.lower()
        matches = [u for u in self._history if tl in u.lower()][:self.SHOW_RESULTS]
        if not matches:
            self._hide_popup()
            return
        self._show_popup(matches)

    def _ensure_popup(self):
        if self._popup is not None:
            return
        win = self.window()
        frame = QFrame(win)
        frame.setFrameShape(QFrame.Shape.StyledPanel)
        frame.setStyleSheet(self._POPUP_STYLE)

        layout = QVBoxLayout(frame)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        lw = QListWidget()
        lw.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        lw.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        lw.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        lw.itemClicked.connect(self._on_item_clicked)
        layout.addWidget(lw)

        clear_btn = QPushButton('Clear history')
        clear_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        clear_btn.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        clear_btn.clicked.connect(self._clear_history)
        layout.addWidget(clear_btn)

        frame.hide()
        self._popup      = frame
        self._popup_list = lw

    def _show_popup(self, matches: list):
        self._ensure_popup()
        self._popup_list.clear()
        for url in matches:
            self._popup_list.addItem(url)

        item_h = 35
        self._popup_list.setFixedHeight(len(matches) * item_h)

        pos = self.mapTo(self.window(), self.rect().bottomLeft())
        self._popup.move(pos)
        self._popup.setFixedWidth(self.width())
        self._popup.adjustSize()
        self._popup.show()
        self._popup.raise_()

    def _hide_popup(self):
        if self._popup:
            self._popup.hide()

    def _on_item_clicked(self, item):
        self.setText(item.text())
        self._hide_popup()
        self.returnPressed.emit()

    # -- Keyboard navigation in popup -------------------------------------

    def keyPressEvent(self, event):
        if self._popup and self._popup.isVisible():
            key = event.key()
            if key == Qt.Key.Key_Down:
                self._popup_list.setCurrentRow(
                    min(self._popup_list.currentRow() + 1,
                        self._popup_list.count() - 1))
                return
            elif key == Qt.Key.Key_Up:
                self._popup_list.setCurrentRow(
                    max(self._popup_list.currentRow() - 1, 0))
                return
            elif key in (Qt.Key.Key_Return, Qt.Key.Key_Enter):
                item = self._popup_list.currentItem()
                if item:
                    self._on_item_clicked(item)
                    return
            elif key == Qt.Key.Key_Escape:
                self._hide_popup()
                return
        super().keyPressEvent(event)

    def focusOutEvent(self, event):
        super().focusOutEvent(event)
        QTimer.singleShot(150, self._hide_popup)
