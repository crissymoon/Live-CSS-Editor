"""
_core.py: MainWindow class -- tabs, toolbar, navigation, loading overlay.
All domain-specific methods live in the mixin modules.
"""

import os
import json
import queue
import subprocess

from PyQt6.QtCore import (
    Qt, QTimer, QUrl, QPropertyAnimation, QEasingCurve, QPoint, pyqtSignal,
)
from PyQt6.QtWidgets import (
    QMainWindow, QTabWidget, QToolBar,
    QToolButton, QMenu, QLabel, QPushButton,
    QWidget, QMessageBox, QInputDialog,
    QGraphicsOpacityEffect,
)
from PyQt6.QtGui import QAction, QPixmap

# Both browser engines.
from ..wkwebview_widget import WKBrowserTab, WKPersistentProfile
from ..browser_profile import BrowserTab as QTBrowserTab, PersistentProfile as QTPersistentProfile
from ..tools_manager import ToolsManager
from ..console_panel import ConsolePanel
from ..command_server import CMD_QUEUE

from ._widgets import BrowserTabBar, HistoryLineEdit
from ._mixin_apps import _AppsMixin
from ._mixin_network import _NetworkMixin
from ._mixin_viewport import _ViewportMixin
from ._mixin_tools import _ToolsMixin
from ._mixin_passwords import _PasswordsMixin
from ._mixin_theme import _ThemeMixin


class MainWindow(
    _AppsMixin,
    _NetworkMixin,
    _ViewportMixin,
    _ToolsMixin,
    _PasswordsMixin,
    _ThemeMixin,
    QMainWindow,
):
    _net_update_ready = pyqtSignal(int, int)

    # Hosts that require WKWebView for H.264/DRM video or OAuth.
    _WK_HOSTS = {
        'linkedin.com', 'www.linkedin.com', 'lnkd.in',
        'm.linkedin.com', 'media.licdn.com',
        'bitmovin.com', 'www.bitmovin.com', 'cdn.bitmovin.com',
        'player.bitmovin.com',
    }
    # Path fragments that suggest DRM-protected content.
    _WK_PATH_HINTS = ('drm', 'widevine', 'fairplay')

    def __init__(self, frameless=False, initial_geometry=None):
        super().__init__()
        self.setWindowTitle("Crissy's Style Tool")
        self.setGeometry(100, 100, 1400, 900)

        if frameless:
            self.setWindowFlags(
                Qt.WindowType.FramelessWindowHint |
                Qt.WindowType.WindowStaysOnTopHint
            )
        if initial_geometry:
            self.setGeometry(*initial_geometry)

        # WKWebView profile (config dict only).
        self.wk_profile = WKPersistentProfile.setup_profile()
        # QtWebEngine profile (full persistent QWebEngineProfile).
        self.qt_profile = QTPersistentProfile.setup_profile()
        # Legacy attribute for anything that still reads it.
        self.profile = self.wk_profile
        self.tools_manager = ToolsManager()
        # Global engine override for testing.  None = auto-detect per URL.
        # 'wk' = always use WKWebView.  'qt' = always use QtWebEngine.
        self._force_engine: str | None = None

        # Start the apps PHP server in the background.
        from ..apps_manager import get_manager as _get_apps_manager
        _get_apps_manager().start_php_server()

        # ── Tabs ─────────────────────────────────────────────────
        self.tabs = QTabWidget()
        self.tabs.setTabBar(BrowserTabBar())
        self.setCentralWidget(self.tabs)
        self.tabs.setTabsClosable(False)
        self.tabs.tabBar().tabCloseRequested.connect(self.close_current_tab)
        self.tabs.currentChanged.connect(self.current_tab_changed)

        # ── Toolbar ───────────────────────────────────────────────
        self.navbar = QToolBar('Navigation')
        self.addToolBar(self.navbar)

        for text, slot in [
            ('←',  self.navigate_back),
            ('→',  self.navigate_forward),
            ('↻',  self.reload_page),
        ]:
            a = QAction(text, self)
            a.triggered.connect(slot)
            self.navbar.addAction(a)

        new_tab_act = QAction('New', self)
        new_tab_act.triggered.connect(lambda: self.add_new_tab(None, 'New Tab'))
        self.navbar.addAction(new_tab_act)
        self.navbar.addSeparator()

        self.url_bar = HistoryLineEdit()
        self.url_bar.setPlaceholderText('>> ENTER COORDINATES OR SEARCH QUERY...')
        self.url_bar.returnPressed.connect(self.navigate_to_url)
        self.navbar.addWidget(self.url_bar)

        # Tools menu button
        tools_btn = QToolButton()
        tools_btn.setText('Tools')
        tools_btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        self.tools_menu = QMenu(tools_btn)
        tools_btn.setMenu(self.tools_menu)
        self.refresh_tools_menu()
        self.navbar.addWidget(tools_btn)

        fill_act = QAction('Fill Dev', self)
        fill_act.triggered.connect(self.autofill_credentials)
        self.navbar.addAction(fill_act)

        self.navbar.addSeparator()

        # Apps dropdown button
        apps_btn = QToolButton()
        apps_btn.setText('Apps')
        apps_btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        self._apps_menu = QMenu(apps_btn)
        self._apps_menu.aboutToShow.connect(self._rebuild_apps_menu)
        apps_btn.setMenu(self._apps_menu)
        self.navbar.addWidget(apps_btn)

        debug_btn = QToolButton()
        debug_btn.setText('Debug')
        debug_btn.setCheckable(True)
        debug_btn.clicked.connect(self._toggle_devtools)
        self.navbar.addWidget(debug_btn)
        self._debug_btn = debug_btn
        self._devtools_window = None

        # Engine / open-in-browser dropdown button.
        eng_btn = QToolButton()
        eng_btn.setText('QT')
        eng_btn.setPopupMode(QToolButton.ToolButtonPopupMode.InstantPopup)
        self._engine_menu = QMenu(eng_btn)
        self._engine_menu.aboutToShow.connect(self._rebuild_engine_menu)
        eng_btn.setMenu(self._engine_menu)
        self.navbar.addWidget(eng_btn)
        self._engine_btn = eng_btn

        self._apply_dark_neu_theme()

        # ── Loading state ────────────────────────────────────────
        self._loading_tabs: set = set()

        # ── Status bar ───────────────────────────────────────────
        self._status_bar = self.statusBar()
        self._status_bar.setStyleSheet(
            'QStatusBar { background:#16162b; color:#6366f1;'
            'font-family:"JetBrains Mono",monospace; font-size:11px;'
            'border-top:1px solid rgba(255,255,255,0.05); padding:2px 8px; }'
            'QStatusBar::item { border: none; }'
        )
        self._status_bar.showMessage("Crissy's Browser ready")

        # ── Network throughput permanent widget ───────────────────
        self._net_label = QLabel('  TX: -- KB/s   RX: -- KB/s  ')
        self._net_label.setStyleSheet(
            'QLabel { color:#4ade80; font-family:"JetBrains Mono",monospace;'
            'font-size:11px; padding: 0 8px; }'
        )
        self._status_bar.addPermanentWidget(self._net_label)

        # ── Color picker (eyedropper) button ─────────────────────
        self._cp_btn = QToolButton()
        self._cp_btn.setText('CP')
        self._cp_btn.setToolTip('Color Picker: click, then click any pixel to grab its hex color')
        self._cp_btn.setStyleSheet(
            'QToolButton { color:#c084fc; background:transparent; border:none;'
            'font-family:"JetBrains Mono",monospace; font-size:11px; padding:0 6px; }'
            'QToolButton:hover { color:#e9d5ff; }'
        )
        self._cp_btn.clicked.connect(self._start_color_pick)
        self._status_bar.addPermanentWidget(self._cp_btn)

        # ── Viewport size picker ──────────────────────────────────
        self._viewport_widget = self._build_viewport_widget()
        self._status_bar.addPermanentWidget(self._viewport_widget)

        # ── Network counters init ─────────────────────────────────
        self._net_prev_tx: int = 0
        self._net_prev_rx: int = 0
        self._net_iface_cache: str = ''
        self._net_iface_ts: float = 0.0
        self._net_pending: bool = False
        self._net_read_counters()

        self._net_update_ready.connect(self._apply_net_display)

        self._net_timer = QTimer(self)
        self._net_timer.setInterval(1000)
        self._net_timer.timeout.connect(self._kick_net_update)
        self._net_timer.start()

        # ── Loading cat overlay ───────────────────────────────────
        self._loading_label = QLabel(
            None,
            Qt.WindowType.FramelessWindowHint
            | Qt.WindowType.WindowStaysOnTopHint
            | Qt.WindowType.Tool,
        )
        self._loading_label.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self._loading_label.setAttribute(Qt.WidgetAttribute.WA_ShowWithoutActivating)
        self._loading_label.setAttribute(Qt.WidgetAttribute.WA_TransparentForMouseEvents)
        self._loading_label.setStyleSheet('background:transparent;')
        pix = QPixmap('/Users/mac/Documents/mind-mapping/xcalibur-full-cat.png')
        if not pix.isNull():
            pix = pix.scaled(
                110, 110,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            self._loading_label.setPixmap(pix)
            self._loading_label.setFixedSize(pix.width(), pix.height())
        else:
            self._loading_label.setText('[loading]')
        self._loading_label.hide()

        self._load_eff = QGraphicsOpacityEffect(self._loading_label)
        self._load_eff.setOpacity(1.0)
        self._loading_label.setGraphicsEffect(self._load_eff)

        self._load_anim = QPropertyAnimation(self._load_eff, b'opacity', self)
        self._load_anim.setDuration(1100)
        self._load_anim.setKeyValueAt(0.0, 1.0)
        self._load_anim.setKeyValueAt(0.5, 0.18)
        self._load_anim.setKeyValueAt(1.0, 1.0)
        self._load_anim.setEasingCurve(QEasingCurve.Type.InOutSine)
        self._load_anim.setLoopCount(-1)

        # ── Console dock (bottom) ─────────────────────────────────
        self.console_panel = ConsolePanel(self)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, self.console_panel)
        self.console_panel.hide()
        self.console_panel.wire_tab_change()

        self.add_new_tab(None)

        # Poll CMD_QUEUE every 50 ms on the Qt main thread.
        self._cmd_timer = QTimer(self)
        self._cmd_timer.timeout.connect(self._process_commands)
        self._cmd_timer.start(50)

    # ── Engine auto-detection ─────────────────────────────────────

    @staticmethod
    def _engine_for_url(url):
        """Return 'wk' if the URL needs WebKit, 'qt' otherwise."""
        if url is None:
            return 'qt'
        url_str = url.toString() if hasattr(url, 'toString') else str(url)
        if not url_str or url_str.startswith('about:'):
            return 'qt'
        from urllib.parse import urlparse
        try:
            parsed = urlparse(url_str)
            host = parsed.netloc.lower().lstrip('www.')
            for wk in MainWindow._WK_HOSTS:
                if host == wk or host.endswith('.' + wk):
                    return 'wk'
            path_lower = (parsed.path or '').lower()
            for hint in MainWindow._WK_PATH_HINTS:
                if hint in path_lower:
                    return 'wk'
        except Exception:
            pass
        return 'qt'

    def _tab_engine(self, tab=None):
        """Return 'wk' or 'qt' for the given tab (defaults to current)."""
        if tab is None:
            tab = self.tabs.currentWidget()
        return 'wk' if isinstance(tab, WKBrowserTab) else 'qt'

    def _switch_engine(self):
        """Reopen the current tab in the opposite engine."""
        browser = self.tabs.currentWidget()
        if not browser:
            return
        url = browser.url()
        url_str = url.toString() if url else ''
        if not url_str or url_str.startswith('about:'):
            url_str = None
        current_engine = self._tab_engine(browser)
        new_engine = 'qt' if current_engine == 'wk' else 'wk'
        idx   = self.tabs.currentIndex()
        label = self.tabs.tabText(idx) or 'New Tab'

        # Build the replacement tab FIRST, then remove the old one.  This
        # keeps QTabWidget count above zero the entire time, which prevents
        # a macOS Qt bug where removing a createWindowContainer widget (used
        # by WKBrowserTab to embed the native NSView) while it is the last
        # tab corrupts the main window's native view hierarchy.
        #
        # Suppress repaints during the add/move/remove sequence so the UI
        # does not flash or snap to the intermediate states (the new empty
        # tab and the tab-bar slide).  Re-enable updates before the single
        # final setCurrentIndex so the transition is one clean repaint.
        self.tabs.setUpdatesEnabled(False)
        self.add_new_tab(QUrl(url_str) if url_str else None, label, engine=new_engine,
                         make_current=False)
        new_idx = self.tabs.count() - 1  # add_new_tab appended without switching
        # Slide the new tab into the slot the old tab occupied so tab order
        # is preserved (new_idx > idx because add_new_tab appends at end).
        self.tabs.tabBar().moveTab(new_idx, idx)
        # After moveTab the old tab shifted right to idx + 1; remove it.
        self.tabs.removeTab(idx + 1)
        self._loading_tabs.discard(id(browser))
        if hasattr(browser, 'cleanup'):
            browser.cleanup()
        browser.deleteLater()
        self.tabs.setUpdatesEnabled(True)
        # One clean repaint: make the new tab current now that everything is settled.
        self.tabs.setCurrentIndex(idx)

    def _update_engine_btn(self, tab=None):
        """Refresh the engine button label and window title."""
        engine = self._tab_engine(tab)
        self._engine_btn.setText('WK' if engine == 'wk' else 'QT')
        suffix = '(WKWebView)' if engine == 'wk' else '(QtWebEngine)'
        self.setWindowTitle(f"Crissy's Style Tool {suffix}")

    def _rebuild_engine_menu(self):
        """Rebuild the engine/open-in dropdown each time it opens."""
        menu = self._engine_menu
        menu.clear()

        current = self._tab_engine(self.tabs.currentWidget())

        qt_act = QAction('QtWebEngine (this tab)', self)
        qt_act.setCheckable(True)
        qt_act.setChecked(current == 'qt')
        qt_act.triggered.connect(lambda: self._switch_to_engine('qt'))
        menu.addAction(qt_act)

        wk_act = QAction('WKWebView (this tab)', self)
        wk_act.setCheckable(True)
        wk_act.setChecked(current == 'wk')
        wk_act.triggered.connect(lambda: self._switch_to_engine('wk'))
        menu.addAction(wk_act)

        menu.addSeparator()

        force_wk_act = QAction('Force WKWebView (all tabs)', self)
        force_wk_act.setCheckable(True)
        force_wk_act.setChecked(self._force_engine == 'wk')
        force_wk_act.triggered.connect(lambda: self._set_force_engine('wk'))
        menu.addAction(force_wk_act)

        force_qt_act = QAction('Force QtWebEngine (all tabs)', self)
        force_qt_act.setCheckable(True)
        force_qt_act.setChecked(self._force_engine == 'qt')
        force_qt_act.triggered.connect(lambda: self._set_force_engine('qt'))
        menu.addAction(force_qt_act)

        force_auto_act = QAction('Auto-detect engine', self)
        force_auto_act.setCheckable(True)
        force_auto_act.setChecked(self._force_engine is None)
        force_auto_act.triggered.connect(lambda: self._set_force_engine(None))
        menu.addAction(force_auto_act)

        menu.addSeparator()

        _BROWSERS = [
            ('Brave',         'Brave Browser'),
            ('Edge',          'Microsoft Edge'),
            ('Google Chrome', 'Google Chrome'),
            ('Firefox',       'Firefox'),
            ('Opera',         'Opera'),
            ('Safari',        'Safari'),
        ]
        for label, app_name in _BROWSERS:
            act = QAction(f'Open in {label}', self)
            act.triggered.connect(
                lambda _=False, a=app_name: self._open_in_external_browser(a)
            )
            menu.addAction(act)

    def _set_force_engine(self, engine: 'str | None'):
        """Set or clear the global engine override for all new tabs."""
        if self._force_engine == engine:
            # Toggling the same option off clears the force.
            self._force_engine = None
        else:
            self._force_engine = engine
        label = {'wk': 'WK (forced)', 'qt': 'QT (forced)'}.get(self._force_engine, 'QT')
        self._engine_btn.setText(label)

    def _switch_to_engine(self, target_engine: str):
        browser = self.tabs.currentWidget()
        if not browser:
            return
        if self._tab_engine(browser) == target_engine:
            return
        self._switch_engine()

    def _open_in_external_browser(self, app_name: str):
        import subprocess as _sp
        browser = self.tabs.currentWidget()
        url_str = ''
        if browser:
            url = browser.url()
            if url:
                url_str = url.toString()
        if not url_str or url_str.startswith(('about:', 'chrome:')):
            url_str = 'about:blank'
        try:
            _sp.Popen(['open', '-a', app_name, url_str])
        except Exception as exc:
            QMessageBox.warning(
                self, 'Open in Browser',
                f'Could not open {app_name}:\n{exc}\n\n'
                f'Make sure {app_name} is installed.',
            )

    def _auto_switch_if_needed(self, qurl, browser):
        """If a QT tab navigates to a WK-required URL, swap the engine."""
        # Respect a manual force override -- no auto switching.
        if self._force_engine is not None:
            return
        if self._engine_for_url(qurl) != 'wk':
            return
        idx = -1
        for i in range(self.tabs.count()):
            if self.tabs.widget(i) is browser:
                idx = i
                break
        if idx == -1:
            return
        label = self.tabs.tabText(idx) or 'New Tab'
        url_str = qurl.toString()
        try:
            browser.urlChanged.disconnect()
        except Exception:
            pass
        # Add the WK tab first so count never drops to zero (avoids the
        # macOS Qt native-view corruption that can blank the main window).
        # Suppress repaints during the swap for the same reason as
        # _switch_engine – prevents the pop/snap of intermediate states.
        self.tabs.setUpdatesEnabled(False)
        self.add_new_tab(QUrl(url_str), label, engine='wk', make_current=False)
        new_idx = self.tabs.count() - 1
        self.tabs.tabBar().moveTab(new_idx, idx)
        self.tabs.removeTab(idx + 1)
        self._loading_tabs.discard(id(browser))
        if hasattr(browser, 'cleanup'):
            browser.cleanup()
        browser.deleteLater()
        self.tabs.setUpdatesEnabled(True)
        self.tabs.setCurrentIndex(idx)

    # ── DevTools ──────────────────────────────────────────────────

    def _toggle_devtools(self, checked: bool):
        if checked:
            self.console_panel.show()
        else:
            self.console_panel.hide()

    def _on_devtools_closed(self):
        self._devtools_window = None
        self._debug_btn.setChecked(False)

    # ── Command queue ─────────────────────────────────────────────

    def _process_commands(self):
        while not CMD_QUEUE.empty():
            try:
                cmd = CMD_QUEUE.get_nowait()
            except queue.Empty:
                break
            action = cmd[0]
            if action == 'navigate':
                self.navigate_to_url_str(cmd[1])
            elif action == 'geometry':
                self.setGeometry(cmd[1], cmd[2], cmd[3], cmd[4])
            elif action == 'show':
                self.show(); self.raise_(); self.activateWindow()
            elif action == 'hide':
                self.hide()
            elif action == 'back':
                self.navigate_back()
            elif action == 'forward':
                self.navigate_forward()
            elif action == 'reload':
                self.reload_page()
            elif action == 'close':
                self.close()
            elif action == 'open_tab':
                self.add_new_tab(QUrl(cmd[1]), cmd[2])
            elif action == 'show_warning':
                QMessageBox.warning(self, cmd[1], cmd[2])

    # ── Navigation ────────────────────────────────────────────────

    def navigate_to_url_str(self, url: str):
        if not url.startswith(('http://', 'https://')):
            url = 'https://' + url
        qurl = QUrl(url)
        self._navigate_smart(qurl)
        self.show(); self.raise_(); self.activateWindow()

    def navigate_to_url(self):
        url = self.url_bar.text()
        if not url:
            return
        if not url.startswith(('http://', 'https://')):
            if '.' in url and ' ' not in url:
                url = 'https://' + url
            else:
                url = 'https://search.brave.com/search?q=' + url.replace(' ', '+')
        self._navigate_smart(QUrl(url))

    def _navigate_smart(self, qurl):
        """Load qurl in the current tab, switching engine if needed."""
        browser = self.tabs.currentWidget()
        if not browser:
            self.add_new_tab(qurl)
            return
        # When a force override is active, never switch engines automatically.
        if self._force_engine is not None:
            browser.setUrl(qurl)
            return
        needed  = self._engine_for_url(qurl)
        current = self._tab_engine(browser)
        if needed == current:
            browser.setUrl(qurl)
        else:
            idx   = self.tabs.currentIndex()
            label = self.tabs.tabText(idx) or 'New Tab'
            self.tabs.removeTab(idx)
            if hasattr(browser, 'cleanup'):
                browser.cleanup()
            browser.deleteLater()
            self.add_new_tab(qurl, label, engine=needed)

    def navigate_back(self):
        w = self.tabs.currentWidget()
        if w: w.back()

    def navigate_forward(self):
        w = self.tabs.currentWidget()
        if w: w.forward()

    def reload_page(self):
        w = self.tabs.currentWidget()
        if w: w.reload()

    # ── Tabs ──────────────────────────────────────────────────────

    def add_new_tab(self, qurl=None, label='New Tab', engine=None, make_current=True):
        if engine is None:
            engine = self._force_engine if self._force_engine is not None else self._engine_for_url(qurl)

        if engine == 'wk':
            browser = WKBrowserTab(self.wk_profile)
            browser._console_panel = self.console_panel
            browser._main_window   = self
        else:
            browser = QTBrowserTab(self.qt_profile)
            browser._main_window   = self
            browser._console_panel = self.console_panel

        if qurl:
            browser.setUrl(qurl)
        else:
            browser.setUrl(QUrl('http://localhost:8080/pb_admin/login.php'))

        i = self.tabs.addTab(browser, label)
        if make_current:
            self.tabs.setCurrentIndex(i)

        browser.urlChanged.connect(lambda u, b=browser: self.update_urlbar(u, b))
        if engine != 'wk':
            browser.urlChanged.connect(
                lambda u, b=browser: self._auto_switch_if_needed(u, b)
            )
        browser.loadStarted.connect(lambda b=browser: self._on_load_started(b))
        browser.loadProgress.connect(
            lambda pct, b=browser: self._on_load_progress(pct, b)
        )
        browser.loadFinished.connect(lambda ok, b=browser: self._on_load_finished(b))
        browser.titleChanged.connect(
            lambda t, b=browser: self.update_tab_title(b, t)
        )
        self._update_engine_btn(browser)

    def closeEvent(self, event):
        self._cmd_timer.stop()
        self._load_anim.stop()
        self._loading_label.close()

        if self._devtools_window:
            self._devtools_window.close()
            self._devtools_window = None

        while self.tabs.count():
            view = self.tabs.widget(0)
            self.tabs.removeTab(0)
            if view is not None:
                self._loading_tabs.discard(id(view))
                if hasattr(view, 'cleanup'):
                    view.cleanup()
                view.deleteLater()

        super().closeEvent(event)

    def close_current_tab(self, i: int):
        if self.tabs.count() >= 2:
            w = self.tabs.widget(i)
            self.tabs.removeTab(i)
            if w is not None:
                self._loading_tabs.discard(id(w))
                if hasattr(w, 'cleanup'):
                    w.cleanup()
                w.deleteLater()

    def current_tab_changed(self, i: int):
        if i == -1:
            return
        w = self.tabs.currentWidget()
        self.update_urlbar(w.url(), w)
        self._update_engine_btn(w)
        if id(w) in self._loading_tabs:
            self._show_loading_overlay()
        else:
            self._hide_loading_overlay()
            self._status_bar.showMessage(w.url().toString())
        if isinstance(w, WKBrowserTab):
            QTimer.singleShot(0, w.take_focus)
        else:
            w.setFocus()

    def update_urlbar(self, qurl, browser):
        if browser != self.tabs.currentWidget():
            return
        url = qurl.toString()
        self.url_bar.setText(url)
        self.url_bar.setCursorPosition(0)
        if id(browser) not in self._loading_tabs:
            self.url_bar.add_url(url)
            self._status_bar.showMessage(url)

    def update_title(self, browser):
        pass

    def update_tab_title(self, browser, title: str):
        for i in range(self.tabs.count()):
            if self.tabs.widget(i) == browser:
                display = title[:20] + '...' if len(title) > 20 else title
                self.tabs.setTabText(i, display)
                break

    # ── Loading overlay ───────────────────────────────────────────

    def _on_load_started(self, browser):
        import time as _time
        self._loading_tabs.add(id(browser))
        browser._load_start_ts = _time.monotonic()
        if browser == self.tabs.currentWidget():
            self._status_bar.showMessage('Loading...')
            self._show_loading_overlay()

    def _flash_status(self, msg: str, ms: int = 1200):
        self._status_bar.showMessage(msg, ms)


    def _on_load_progress(self, pct: int, browser):
        if browser == self.tabs.currentWidget():
            url = browser.url().toString()
            self._status_bar.showMessage(f'{pct}%  {url}')

    def _on_load_finished(self, browser):
        import time as _time
        self._loading_tabs.discard(id(browser))
        elapsed_ms = None
        t0 = getattr(browser, '_load_start_ts', None)
        if t0 is not None:
            elapsed_ms = int((_time.monotonic() - t0) * 1000)
        url = browser.url().toString()
        if elapsed_ms is not None:
            engine = 'WK' if isinstance(browser, WKBrowserTab) else 'Qt'
            print(f'[perf] {engine} load {elapsed_ms}ms  {url}', flush=True)
        if browser == self.tabs.currentWidget():
            msg = f'{url}  ({elapsed_ms}ms)' if elapsed_ms is not None else url
            self._status_bar.showMessage(msg)
            self._hide_loading_overlay()

    def _show_loading_overlay(self):
        self._loading_label.show()
        self._loading_label.raise_()
        self._position_loading_label()
        if self._load_anim.state() != QPropertyAnimation.State.Running:
            self._load_anim.start()

    def _hide_loading_overlay(self):
        self._load_anim.stop()
        self._loading_label.hide()

    def _position_loading_label(self):
        sb_h = self._status_bar.height() if self._status_bar.isVisible() else 0
        lw   = self._loading_label.width()
        lh   = self._loading_label.height()
        local_x  = self.width() - lw - 18
        local_y  = self.height() - lh - sb_h - 18
        global_pt = self.mapToGlobal(QPoint(local_x, local_y))
        self._loading_label.move(global_pt)

    # ── Window events ─────────────────────────────────────────────

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if hasattr(self, '_loading_label') and self._loading_label.isVisible():
            self._position_loading_label()
        self._update_viewport_display()
        self._update_statusbar_visibility()

    def showEvent(self, event):
        super().showEvent(event)
        QTimer.singleShot(0, self._update_viewport_display)
        QTimer.singleShot(0, self._update_statusbar_visibility)

    def _update_statusbar_visibility(self):
        narrow = self.width() < 515
        if hasattr(self, '_net_label'):
            self._net_label.setVisible(not narrow)
        if hasattr(self, '_viewport_widget'):
            if narrow:
                for child in self._viewport_widget.children():
                    if hasattr(child, 'setVisible'):
                        visible = getattr(child, 'objectName', lambda: '')() == 'vpSize'
                        child.setVisible(visible)
            else:
                for child in self._viewport_widget.children():
                    if hasattr(child, 'setVisible'):
                        child.setVisible(True)

    def moveEvent(self, event):
        super().moveEvent(event)
        if hasattr(self, '_loading_label') and self._loading_label.isVisible():
            self._position_loading_label()

    # ── Bookmark helper ───────────────────────────────────────────

    def navigate_to_bookmark(self, url: str):
        browser = self.tabs.currentWidget()
        if browser and url:
            browser.setUrl(QUrl(url))
