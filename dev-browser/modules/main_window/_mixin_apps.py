"""
_AppsMixin: apps menu, Commander.
"""

import os

from PyQt6.QtCore import Qt, QTimer, QUrl
from PyQt6.QtGui import QAction
from PyQt6.QtWidgets import QMessageBox


class _AppsMixin:
    """Methods for the Apps dropdown and Commander."""

    @property
    def _PHP_BIN(self) -> str:  # type: ignore[override]
        from ..apps_manager import find_php
        return find_php()

    def _rebuild_apps_menu(self):
        """Populate the Apps dropdown from the apps/ directory each time it opens."""
        try:
            self._rebuild_apps_menu_inner()
        except Exception as exc:
            import traceback
            QMessageBox.critical(self, 'Apps menu error',
                traceback.format_exc())

    def _rebuild_apps_menu_inner(self):
        from ..apps_manager import get_manager
        m    = get_manager()
        menu = self._apps_menu
        menu.clear()

        apps = m.list_apps()
        if apps:
            for app in apps:
                name  = app.get('name', app.get('slug', '?'))
                label = f"{name}  [auto]" if app.get('has_automation') else name
                act   = QAction(label, self)
                slug  = app['slug']
                act.triggered.connect(lambda _=False, s=slug: self._open_app(s))
                menu.addAction(act)
        else:
            no_apps = menu.addAction('-- no apps installed --')
            no_apps.setEnabled(False)

        menu.addSeparator()

        commander_act = QAction('Commander', self)
        commander_act.triggered.connect(self._open_commander)
        menu.addAction(commander_act)

        menu.addSeparator()

        open_folder_act = QAction('Open Apps Folder', self)
        open_folder_act.triggered.connect(self._open_apps_folder)
        menu.addAction(open_folder_act)

    def _open_app(self, slug: str):
        """Ensure PHP server is running then open the app in a new tab."""
        try:
            self._open_app_inner(slug)
        except Exception as exc:
            import traceback
            QMessageBox.critical(self, 'Open app error',
                traceback.format_exc())

    def _open_app_inner(self, slug: str):
        from ..apps_manager import get_manager
        m = get_manager()
        if not m.php_running():
            ok = m.start_php_server()
            if not ok:
                QMessageBox.warning(
                    self, 'Apps',
                    'Could not start the PHP server.\n'
                    'Make sure PHP is installed (run: php -v in Terminal).',
                )
                return
        url   = m.get_app_url(slug)
        label = slug
        apps  = {a['slug']: a.get('name', slug) for a in m.list_apps()}
        label = apps.get(slug, slug)
        self.add_new_tab(QUrl(url), label)

    def _open_commander(self):
        """Open the Commander script launcher dialog."""
        from ._commander import CommanderDialog
        dlg = CommanderDialog(self)
        dlg.exec()

    def _open_apps_folder(self):
        """Reveal the apps/ directory in Finder."""
        import subprocess as _sp
        from ..apps_manager import AppsManager
        os.makedirs(AppsManager.APPS_DIR, exist_ok=True)
        _sp.Popen(['open', AppsManager.APPS_DIR])
