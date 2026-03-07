"""
_ToolsMixin: Tools dropdown and manager.

Tools are stored in dev-browser/tools.json as a list of:
  {"name": "...", "url": "http://...", "type": "tool" | "link"}

"type" is cosmetic only -- both navigate the browser to the URL.
Paths without a scheme are auto-prefixed with http://127.0.0.1:8080.
"""

from PyQt6.QtCore import Qt, QUrl, QTimer
from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
    QTableWidget, QTableWidgetItem, QAbstractItemView,
    QComboBox, QWidget, QLineEdit, QFrame,
)
from PyQt6.QtGui import QColor, QAction


class _ToolsMixin:
    """Methods for the Tools dropdown and manager."""

    # -- Navigation ────────────────────────────────────────────────

    def navigate_to_tool(self, url: str, kind: str = 'tool', name: str = ''):
        qurl  = QUrl(url)
        label = name if name else 'Tool'
        if kind == 'tool':
            # Step 1: maximize and open a blank tab so the widget is created
            # and sized before the actual page is loaded.
            self.showMaximized()
            self.add_new_tab(QUrl('about:blank'), label)
            browser = self.tabs.currentWidget()
            if not browser:
                return
            # Step 2: wait for the maximize animation to fully complete
            # (macOS animate-resize is ~200ms) before navigating so that
            # the page JS sees the correct window dimensions when it calls
            # applyDefaultLayout() / getLayoutRect().
            QTimer.singleShot(300, lambda b=browser: b.setUrl(qurl))
        else:
            self.add_new_tab(qurl, label)

    # ── Toolbar dropdown ─────────────────────────────────────────

    def refresh_tools_menu(self):
        self.tools_menu.clear()
        for tool in self.tools_manager.get_tools():
            label = tool['name']
            kind  = tool.get('type', 'tool')
            if kind == 'link':
                label = label + '  [link]'
            a = QAction(label, self)
            a.triggered.connect(
                lambda checked, u=tool['url'], k=kind, n=tool['name']: self.navigate_to_tool(u, k, n)
            )
            self.tools_menu.addAction(a)
        self.tools_menu.addSeparator()
        manage = QAction('[MANAGE TOOLS]', self)
        manage.triggered.connect(self.manage_tools)
        self.tools_menu.addAction(manage)

    # ── Manager dialog ────────────────────────────────────────────

    def manage_tools(self):
        self._tools_dialog = QDialog(self)
        self._tools_dialog.setWindowTitle('Tools Manager')
        self._tools_dialog.setModal(True)
        self._tools_dialog.resize(900, 540)
        layout = QVBoxLayout()

        self._tools_table = QTableWidget()
        self._tools_table.setColumnCount(4)
        self._tools_table.setHorizontalHeaderLabels(['NAME', 'URL / PATH', 'TYPE', 'ACTIONS'])
        self._tools_table.horizontalHeader().setStretchLastSection(False)
        self._tools_table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self._tools_table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self._tools_table.verticalHeader().setVisible(False)
        self._tools_table.setAlternatingRowColors(False)
        self._tools_table.verticalHeader().setDefaultSectionSize(46)
        self._refresh_tools_table()
        layout.addWidget(self._tools_table)

        # ── Add new tool form ──────────────────────────────────────
        sep = QFrame()
        sep.setFrameShape(QFrame.Shape.HLine)
        sep.setStyleSheet('color: #2a2a40;')
        layout.addWidget(sep)

        add_lbl = QLabel('ADD TOOL')
        add_lbl.setStyleSheet(
            'color:#6366f1;font-family:"JetBrains Mono",monospace;'
            'font-size:11px;font-weight:bold;letter-spacing:1px;padding:4px 0 2px 0;'
        )
        layout.addWidget(add_lbl)

        form_row = QHBoxLayout()

        self._new_name_edit = QLineEdit()
        self._new_name_edit.setPlaceholderText('Name')
        self._new_name_edit.setMinimumWidth(160)
        form_row.addWidget(self._new_name_edit, 2)

        self._new_url_edit = QLineEdit()
        self._new_url_edit.setPlaceholderText('URL or /path/on/server')
        form_row.addWidget(self._new_url_edit, 4)

        self._new_type_combo = QComboBox()
        self._new_type_combo.addItems(['tool', 'link'])
        self._new_type_combo.setMinimumWidth(80)
        form_row.addWidget(self._new_type_combo)

        add_btn = QPushButton('ADD')
        add_btn.setMinimumWidth(60)
        add_btn.setFixedHeight(30)
        add_btn.setStyleSheet(
            'QPushButton { background:#252540; color:#6ee7b7; border:1px solid rgba(110,231,183,0.3);'
            ' border-radius:4px; padding:4px 14px; font-family:"JetBrains Mono",monospace;'
            ' font-size:10px; font-weight:bold; }'
            ' QPushButton:hover { background:#065f46; color:#fff; border-color:#6ee7b7; }'
            ' QPushButton:pressed { background:#047857; }'
        )
        add_btn.clicked.connect(self._add_tool_row)
        form_row.addWidget(add_btn)

        layout.addLayout(form_row)

        btn_row = QHBoxLayout()
        close_btn = QPushButton('Close')
        close_btn.setFixedHeight(30)
        close_btn.setMinimumWidth(80)
        close_btn.setStyleSheet(
            'QPushButton { background:#252540; color:#c4c4d4; border:1px solid rgba(255,255,255,0.12);'
            ' border-radius:4px; padding:4px 18px; font-family:"JetBrains Mono",monospace;'
            ' font-size:10px; font-weight:bold; }'
            ' QPushButton:hover { background:#6366f1; color:#fff; border-color:#6366f1; }'
        )
        close_btn.clicked.connect(self._tools_dialog.close)
        btn_row.addStretch()
        btn_row.addWidget(close_btn)
        layout.addLayout(btn_row)

        self._tools_dialog.setLayout(layout)
        self._tools_dialog.setStyleSheet(self._dialog_style())
        self._tools_dialog.exec()

    def _refresh_tools_table(self):
        tools = self.tools_manager.get_tools()
        self._tools_table.setRowCount(len(tools))
        for idx, tool in enumerate(tools):
            n_item = QTableWidgetItem(tool['name'])
            n_item.setForeground(QColor('#a5b4fc'))
            self._tools_table.setItem(idx, 0, n_item)

            u_item = QTableWidgetItem(tool['url'])
            u_item.setForeground(QColor('#e8e8f0'))
            self._tools_table.setItem(idx, 1, u_item)

            t_item = QTableWidgetItem(tool.get('type', 'tool'))
            t_item.setForeground(QColor('#6ee7b7') if tool.get('type') == 'tool' else QColor('#fcd34d'))
            self._tools_table.setItem(idx, 2, t_item)

            btn_widget = QWidget()
            btn_layout = QHBoxLayout()
            btn_layout.setContentsMargins(4, 6, 4, 6)
            btn_layout.setSpacing(6)

            _btn_style = (
                'QPushButton { background:#252540; color:#c4c4d4; border:1px solid rgba(255,255,255,0.12);'
                ' border-radius:4px; padding:4px 10px; font-family:"JetBrains Mono",monospace;'
                ' font-size:10px; font-weight:bold; min-height:22px; }'
                ' QPushButton:hover { background:#6366f1; color:#fff; border-color:#6366f1; }'
                ' QPushButton:pressed { background:#4f52c9; }'
            )

            edit_btn = QPushButton('EDIT')
            edit_btn.setFixedHeight(28)
            edit_btn.setMinimumWidth(52)
            edit_btn.setStyleSheet(_btn_style)
            edit_btn.clicked.connect(
                lambda _, i=idx, t=tool: self._edit_tool_row(i, t)
            )
            btn_layout.addWidget(edit_btn)

            up_btn = QPushButton('UP')
            up_btn.setFixedHeight(28)
            up_btn.setMinimumWidth(42)
            up_btn.setStyleSheet(_btn_style)
            up_btn.clicked.connect(lambda _, i=idx: self._move_tool(i, 'up'))
            btn_layout.addWidget(up_btn)

            dn_btn = QPushButton('DN')
            dn_btn.setFixedHeight(28)
            dn_btn.setMinimumWidth(42)
            dn_btn.setStyleSheet(_btn_style)
            dn_btn.clicked.connect(lambda _, i=idx: self._move_tool(i, 'down'))
            btn_layout.addWidget(dn_btn)

            del_btn = QPushButton('DEL')
            del_btn.setFixedHeight(28)
            del_btn.setMinimumWidth(48)
            del_btn.setStyleSheet(
                'QPushButton { background:#2a1a1a; color:#f87171; border:1px solid rgba(248,113,113,0.25);'
                ' border-radius:4px; padding:4px 10px; font-family:"JetBrains Mono",monospace;'
                ' font-size:10px; font-weight:bold; min-height:22px; }'
                ' QPushButton:hover { background:#7f1d1d; color:#fff; border-color:#f87171; }'
                ' QPushButton:pressed { background:#991b1b; }'
            )
            del_btn.clicked.connect(lambda _, i=idx: self._delete_tool_row(i))

            btn_widget.setLayout(btn_layout)
            self._tools_table.setCellWidget(idx, 3, btn_widget)

        self._tools_table.setColumnWidth(0, 180)
        self._tools_table.setColumnWidth(1, 360)
        self._tools_table.setColumnWidth(2, 55)
        self._tools_table.setColumnWidth(3, 270)
        self._tools_table.resizeRowsToContents()

    def _add_tool_row(self):
        name = self._new_name_edit.text().strip()
        url  = self._new_url_edit.text().strip()
        kind = self._new_type_combo.currentText()
        if not name or not url:
            return
        if self.tools_manager.add_tool(name, url, kind):
            self._new_name_edit.clear()
            self._new_url_edit.clear()
            self._refresh_tools_table()
            self.refresh_tools_menu()

    def _edit_tool_row(self, index: int, tool: dict):
        dialog = QDialog(self._tools_dialog)
        dialog.setWindowTitle('Edit Tool')
        dialog.setModal(True)
        dialog.resize(520, 175)
        layout = QVBoxLayout()

        name_row = QHBoxLayout()
        name_row.addWidget(QLabel('Name:'))
        name_edit = QLineEdit(tool['name'])
        name_row.addWidget(name_edit, 1)
        layout.addLayout(name_row)

        url_row = QHBoxLayout()
        url_row.addWidget(QLabel('URL:  '))
        url_edit = QLineEdit(tool['url'])
        url_row.addWidget(url_edit, 1)
        layout.addLayout(url_row)

        type_row = QHBoxLayout()
        type_row.addWidget(QLabel('Type: '))
        type_combo = QComboBox()
        type_combo.addItems(['tool', 'link'])
        type_combo.setCurrentText(tool.get('type', 'tool'))
        type_row.addWidget(type_combo)
        type_row.addStretch()
        layout.addLayout(type_row)

        btn_row = QHBoxLayout()
        save_btn   = QPushButton('Save')
        cancel_btn = QPushButton('Cancel')
        btn_row.addStretch()
        btn_row.addWidget(save_btn)
        btn_row.addWidget(cancel_btn)
        layout.addLayout(btn_row)

        dialog.setLayout(layout)
        dialog.setStyleSheet(self._dialog_style())

        cancel_btn.clicked.connect(dialog.reject)
        save_btn.clicked.connect(dialog.accept)

        if dialog.exec() == QDialog.DialogCode.Accepted:
            self.tools_manager.edit_tool(
                index,
                name_edit.text(),
                url_edit.text(),
                type_combo.currentText(),
            )
            self._refresh_tools_table()
            self.refresh_tools_menu()

    def _move_tool(self, index: int, direction: str):
        if direction == 'up':
            self.tools_manager.move_up(index)
        else:
            self.tools_manager.move_down(index)
        self._refresh_tools_table()
        self.refresh_tools_menu()

    def _delete_tool_row(self, index: int):
        if self.tools_manager.delete_tool(index):
            self._refresh_tools_table()
            self.refresh_tools_menu()

    # ── Kept so old toolbar "Add" wiring doesn't crash ────────────
    def add_bookmark(self):
        pass

    def refresh_bookmark_menu(self):
        pass
