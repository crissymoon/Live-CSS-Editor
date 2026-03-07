"""
CommanderDialog: a simple launcher for macOS .command / shell scripts.
"""

import os
import json
import subprocess

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QLabel,
    QPushButton, QListWidget, QListWidgetItem,
    QMessageBox, QInputDialog, QFileDialog,
)


_COMMANDER_STORE = os.path.expanduser('~/.xcaliburmoon_commander.json')
_COMMANDER_DEFAULTS = [
    {
        'label': "Crissy's Design Tool - Pro",
        'path':  '/Users/mac/Documents/live-css/Live CSS Editor.command',
    },
    {
        'label': 'XCMDev',
        'path':  '/Users/mac/Documents/mind-mapping/XCMDev.command',
    },
]


def _commander_load() -> list:
    """Load saved commander entries, seeding defaults on first run."""
    if os.path.exists(_COMMANDER_STORE):
        try:
            with open(_COMMANDER_STORE, 'r') as f:
                data = json.load(f)
            if isinstance(data, list):
                return data
        except Exception:
            pass
    _commander_save(_COMMANDER_DEFAULTS)
    return list(_COMMANDER_DEFAULTS)


def _commander_save(entries: list):
    try:
        with open(_COMMANDER_STORE, 'w') as f:
            json.dump(entries, f, indent=2)
    except Exception as exc:
        print(f'[commander] save error: {exc}')


class CommanderDialog(QDialog):
    """A simple launcher for macOS .command / shell scripts."""

    _STYLE = """
        QDialog { background: #1a1a2e; }
        QLabel#title {
            color: #c8aaff; font-size: 15px; font-weight: bold; padding: 6px 0;
        }
        QListWidget {
            background: #12122a; color: #ddd; border: 1px solid #333;
            font-family: monospace; font-size: 12px;
        }
        QListWidget::item:selected { background: #2a2a5a; color: #fff; }
        QPushButton {
            background: #252545; color: #ccc; border: 1px solid #444;
            padding: 5px 14px; border-radius: 4px;
        }
        QPushButton:hover { background: #353565; color: #fff; }
        QPushButton#run_btn { background: #1e4a1e; color: #8eff8e; border-color: #2a7a2a; }
        QPushButton#run_btn:hover { background: #256025; }
        QPushButton#remove_btn { background: #4a1e1e; color: #ff9e9e; border-color: #7a2a2a; }
        QPushButton#remove_btn:hover { background: #602525; }
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle('Commander')
        self.setMinimumSize(620, 360)
        self.setStyleSheet(self._STYLE)

        self._entries = _commander_load()

        layout = QVBoxLayout(self)
        layout.setContentsMargins(14, 12, 14, 12)
        layout.setSpacing(8)

        title = QLabel('Commander')
        title.setObjectName('title')
        layout.addWidget(title)

        self._list = QListWidget()
        self._list.setAlternatingRowColors(True)
        self._list.itemDoubleClicked.connect(self._run_selected)
        layout.addWidget(self._list)

        btn_row = QHBoxLayout()
        btn_row.setSpacing(6)

        run_btn = QPushButton('Run')
        run_btn.setObjectName('run_btn')
        run_btn.clicked.connect(self._run_selected)
        btn_row.addWidget(run_btn)

        add_btn = QPushButton('Add Script...')
        add_btn.clicked.connect(self._add_entry)
        btn_row.addWidget(add_btn)

        remove_btn = QPushButton('Remove')
        remove_btn.setObjectName('remove_btn')
        remove_btn.clicked.connect(self._remove_selected)
        btn_row.addWidget(remove_btn)

        btn_row.addStretch()

        close_btn = QPushButton('Close')
        close_btn.clicked.connect(self.accept)
        btn_row.addWidget(close_btn)

        layout.addLayout(btn_row)
        self._refresh_list()

    def _refresh_list(self):
        self._list.clear()
        for e in self._entries:
            label = e.get('label', '')
            path  = e.get('path', '')
            item  = QListWidgetItem(f'{label}   --   {path}')
            item.setData(Qt.ItemDataRole.UserRole, e)
            self._list.addItem(item)

    def _run_selected(self, *_):
        item = self._list.currentItem()
        if not item:
            QMessageBox.information(self, 'Commander', 'Select a script first.')
            return
        entry = item.data(Qt.ItemDataRole.UserRole)
        path  = entry.get('path', '')
        if not os.path.exists(path):
            QMessageBox.warning(self, 'Commander', f'File not found:\n{path}')
            return
        try:
            os.chmod(path, 0o755)
        except Exception:
            pass
        subprocess.Popen(['open', path])

    def _add_entry(self):
        path, _ = QFileDialog.getOpenFileName(
            self, 'Select Script',
            os.path.expanduser('~'),
            'Scripts (*.command *.sh *.zsh *.bash);;All Files (*)',
        )
        if not path:
            return
        label, ok = QInputDialog.getText(
            self, 'Commander', 'Label for this script:',
            text=os.path.splitext(os.path.basename(path))[0],
        )
        if not ok or not label.strip():
            return
        self._entries.append({'label': label.strip(), 'path': path})
        _commander_save(self._entries)
        self._refresh_list()
        self._list.setCurrentRow(len(self._entries) - 1)

    def _remove_selected(self):
        row = self._list.currentRow()
        if row < 0:
            return
        entry = self._entries[row]
        ans = QMessageBox.question(
            self, 'Commander',
            f'Remove "{entry["label"]}"?',
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if ans == QMessageBox.StandardButton.Yes:
            self._entries.pop(row)
            _commander_save(self._entries)
            self._refresh_list()
