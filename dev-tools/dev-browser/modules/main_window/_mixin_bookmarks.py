"""
_BookmarksMixin: bookmark add/edit/delete/manage methods.
"""

from PyQt6.QtCore import Qt, QUrl
from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
    QTableWidget, QTableWidgetItem, QAbstractItemView,
    QMessageBox, QInputDialog, QWidget, QLineEdit,
)
from PyQt6.QtGui import QColor, QAction


class _BookmarksMixin:
    """Methods for bookmark management."""

    def add_bookmark(self):
        browser = self.tabs.currentWidget()
        if not browser:
            return
        url = browser.url().toString()
        if not url or url == 'about:blank':
            return
        title, ok = QInputDialog.getText(self, 'Add Bookmark', 'Enter bookmark title:')
        if ok and title:
            if self.bookmark_manager.save_bookmark(title, url):
                self.refresh_bookmark_menu()

    def navigate_to_bookmark(self, url: str):
        browser = self.tabs.currentWidget()
        if browser and url:
            browser.setUrl(QUrl(url))

    def refresh_bookmark_menu(self):
        self.bookmark_menu.clear()
        for title, url in self.bookmark_manager.get_bookmarks().items():
            a = QAction(title, self)
            a.triggered.connect(lambda checked, u=url: self.navigate_to_bookmark(u))
            self.bookmark_menu.addAction(a)
        if self.bookmark_manager.get_bookmarks():
            self.bookmark_menu.addSeparator()
        manage = QAction('[MANAGE BOOKMARKS]', self)
        manage.triggered.connect(self.manage_bookmarks)
        self.bookmark_menu.addAction(manage)

    def manage_bookmarks(self):
        self._bm_dialog = QDialog(self)
        self._bm_dialog.setWindowTitle('Bookmark Manager')
        self._bm_dialog.setModal(True)
        self._bm_dialog.resize(720, 460)
        layout = QVBoxLayout()

        self._bm_table = QTableWidget()
        self._bm_table.setColumnCount(3)
        self._bm_table.setHorizontalHeaderLabels(['TITLE', 'URL', 'ACTIONS'])
        self._bm_table.horizontalHeader().setStretchLastSection(False)
        self._bm_table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self._bm_table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self._bm_table.verticalHeader().setVisible(False)
        self._bm_table.setAlternatingRowColors(False)
        self._bm_table.verticalHeader().setDefaultSectionSize(38)
        self._refresh_bookmark_table()
        layout.addWidget(self._bm_table)

        btn_row = QHBoxLayout()
        close_btn = QPushButton('Close')
        close_btn.clicked.connect(self._bm_dialog.close)
        btn_row.addStretch()
        btn_row.addWidget(close_btn)
        layout.addLayout(btn_row)

        self._bm_dialog.setLayout(layout)
        self._bm_dialog.setStyleSheet(self._dialog_style())
        self._bm_dialog.exec()

    def _refresh_bookmark_table(self):
        bookmarks = self.bookmark_manager.get_bookmarks()
        self._bm_table.setRowCount(len(bookmarks))
        for row, (title, url) in enumerate(bookmarks.items()):
            t_item = QTableWidgetItem(title)
            t_item.setForeground(QColor('#a5b4fc'))
            self._bm_table.setItem(row, 0, t_item)

            u_item = QTableWidgetItem(url)
            u_item.setForeground(QColor('#e8e8f0'))
            self._bm_table.setItem(row, 1, u_item)

            btn_widget = QWidget()
            btn_layout = QHBoxLayout()
            btn_layout.setContentsMargins(4, 3, 4, 3)
            btn_layout.setSpacing(6)

            edit_btn = QPushButton('EDIT')
            edit_btn.setMinimumSize(60, 28)
            edit_btn.clicked.connect(lambda _, t=title, u=url: self._edit_bookmark_row(t, u))
            btn_layout.addWidget(edit_btn)

            del_btn = QPushButton('DEL')
            del_btn.setMinimumSize(50, 28)
            del_btn.clicked.connect(lambda _, t=title: self._delete_bookmark_row(t))
            btn_layout.addWidget(del_btn)

            btn_widget.setLayout(btn_layout)
            self._bm_table.setCellWidget(row, 2, btn_widget)

        self._bm_table.setColumnWidth(0, 190)
        self._bm_table.setColumnWidth(1, 370)
        self._bm_table.setColumnWidth(2, 140)

    def _edit_bookmark_row(self, old_title: str, old_url: str):
        dialog = QDialog(self._bm_dialog)
        dialog.setWindowTitle('Edit Bookmark')
        dialog.setModal(True)
        dialog.resize(500, 160)
        layout = QVBoxLayout()

        title_row = QHBoxLayout()
        title_row.addWidget(QLabel('Title:'))
        title_edit = QLineEdit(old_title)
        title_row.addWidget(title_edit, 1)
        layout.addLayout(title_row)

        url_row = QHBoxLayout()
        url_row.addWidget(QLabel('URL:  '))
        url_edit = QLineEdit(old_url)
        url_row.addWidget(url_edit, 1)
        layout.addLayout(url_row)

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
            new_title = title_edit.text().strip()
            new_url   = url_edit.text().strip()
            if new_title and new_url:
                self.bookmark_manager.edit_bookmark(old_title, new_title, new_url)
                self._refresh_bookmark_table()
                self.refresh_bookmark_menu()

    def _delete_bookmark_row(self, title: str):
        if self.bookmark_manager.delete_bookmark(title):
            self._refresh_bookmark_table()
            self.refresh_bookmark_menu()
