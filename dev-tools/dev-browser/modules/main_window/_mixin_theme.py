"""
_ThemeMixin: dialog stylesheet and main window dark-neu theme.
"""


class _ThemeMixin:
    """Methods for applying visual themes to dialogs and the main window."""

    def _dialog_style(self) -> str:
        return """
            QDialog {
                background-color: #1a1a2e;
                color: #e8e8f0;
                border: 1px solid rgba(99,102,241,0.4);
                border-radius: 10px;
            }
            QLabel {
                color: #e8e8f0;
                font-family: 'JetBrains Mono', monospace;
            }
            QListWidget {
                background-color: #13131f;
                border: 1px solid rgba(255,255,255,0.06);
                color: #e8e8f0;
                font-family: 'JetBrains Mono', 'SF Mono', monospace;
                font-size: 11px;
            }
            QListWidget::item             { padding: 6px 8px; border-bottom: 1px solid rgba(255,255,255,0.04); }
            QListWidget::item:selected    { background-color: #6366f1; color: #ffffff; }
            QPushButton {
                background-color: #1a1a2e; color: #8888a0;
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 6px; padding: 7px 16px;
                font-family: 'JetBrains Mono', monospace;
                font-size: 10px; font-weight: bold;
                text-transform: uppercase; letter-spacing: 1px;
            }
            QPushButton:hover   { background-color: #6366f1; color: #ffffff; border-color: #6366f1; }
            QPushButton:pressed { background-color: #4f52c9; }
            QTableWidget {
                background-color: #13131f;
                border: 1px solid rgba(255,255,255,0.06);
                gridline-color: rgba(255,255,255,0.04);
                color: #e8e8f0;
                font-family: 'JetBrains Mono', 'SF Mono', monospace;
                font-size: 11px;
            }
            QTableWidget::item          { padding: 5px; border-bottom: 1px solid rgba(255,255,255,0.04); }
            QTableWidget::item:selected { background-color: #6366f1; color: #ffffff; }
            QHeaderView::section {
                background-color: #16162b; color: #8888a0;
                border: 1px solid rgba(255,255,255,0.06);
                font-weight: bold; font-size: 10px; padding: 5px;
            }
            QLineEdit {
                background-color: #13131f;
                border: 1px solid rgba(255,255,255,0.06);
                color: #e8e8f0; padding: 6px 10px;
                font-family: 'JetBrains Mono', monospace;
            }
            QLineEdit:focus { border-color: #6366f1; }
            QComboBox {
                background-color: #13131f;
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 4px;
                color: #e8e8f0; padding: 4px 8px;
                font-family: 'JetBrains Mono', monospace;
                font-size: 10px;
            }
            QComboBox:focus { border-color: #6366f1; }
            QComboBox::drop-down {
                border: none; width: 18px;
            }
            QComboBox QAbstractItemView {
                background-color: #16162b;
                border: 1px solid rgba(255,255,255,0.08);
                color: #e8e8f0;
                selection-background-color: #6366f1;
            }
        """

    def _apply_dark_neu_theme(self):
        self.setStyleSheet("""
            QMainWindow { background-color: #1a1a2e; color: #e8e8f0; }
            QToolBar {
                background-color: #16162b; border: none;
                border-bottom: 1px solid rgba(255,255,255,0.05);
                padding: 4px 6px; spacing: 3px;
            }
            QToolButton {
                background-color: #1a1a2e; color: #8888a0;
                border: 1px solid rgba(255,255,255,0.07);
                border-radius: 6px; padding: 5px 10px; margin: 1px;
                font-family: 'JetBrains Mono','SF Mono','Consolas',monospace;
                font-size: 11px; font-weight: bold;
            }
            QToolButton:hover    { background-color: #6366f1; color: #ffffff; border-color: #6366f1; }
            QToolButton:pressed  { background-color: #4f52c9; color: #ffffff; }
            QToolButton::menu-indicator { image:none; width:0px; height:0px; }
            QToolButton::menu-arrow     { image:none; width:0px; height:0px; }
            QToolButton::menu-button    { border:none; background:transparent; width:0px; }
            QMenu {
                background-color: #16162b;
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 8px; color: #e8e8f0;
                font-family: 'JetBrains Mono','SF Mono',monospace; font-size: 11px;
            }
            QMenu::item            { padding: 6px 14px; border-bottom: 1px solid rgba(255,255,255,0.04); }
            QMenu::item:selected   { background-color: #6366f1; color: #ffffff; }
            QMenu::separator       { height: 1px; background: rgba(255,255,255,0.06); }
            QLineEdit {
                background-color: #13131f;
                border: 1px solid rgba(255,255,255,0.07);
                border-radius: 6px; padding: 6px 10px;
                font-size: 12px; color: #e8e8f0;
                font-family: 'JetBrains Mono','SF Mono',monospace;
                selection-background-color: #6366f1; selection-color: #ffffff;
            }
            QLineEdit:focus { border-color: #6366f1; }
            QTabWidget::pane {
                border: 1px solid rgba(255,255,255,0.06);
                border-radius: 0 0 8px 8px; background-color: #1a1a2e;
                margin-top: -1px;
            }
            QTabBar::tab {
                background-color: #13131f; color: #8888a0;
                border: 1px solid rgba(255,255,255,0.18);
                border-bottom: none;
                border-top-left-radius: 6px; border-top-right-radius: 6px;
                border-bottom-left-radius: 0; border-bottom-right-radius: 0;
                padding: 6px 14px; margin-right: 2px;
                font-family: 'JetBrains Mono','SF Mono',monospace;
                font-size: 10px; font-weight: bold;
            }
            QTabBar::tab:selected {
                background-color: #252540; color: #ffffff;
                border: 2px solid rgba(99,102,241,0.7);
                border-bottom: 2px solid #252540;
                border-top: 2px solid #6366f1;
            }
            QTabBar::tab:hover    { background-color: #1f1f35; color: #e8e8f0; }
            QScrollBar:vertical         { background: #13131f; width: 7px; border: none; }
            QScrollBar::handle:vertical { background: #252540; min-height: 20px; border-radius: 3px; }
            QScrollBar::handle:vertical:hover { background: #6366f1; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
            QScrollBar:horizontal         { background: #13131f; height: 7px; border: none; }
            QScrollBar::handle:horizontal { background: #252540; min-width: 20px; border-radius: 3px; }
            QScrollBar::handle:horizontal:hover { background: #6366f1; }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        """)
