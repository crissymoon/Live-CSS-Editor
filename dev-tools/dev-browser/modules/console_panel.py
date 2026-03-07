"""
console_panel.py -- A Qt-based debug panel with five tabs.

  JS Console  -- console.log/warn/error/info from the active web page
  Elements    -- DOM inspector with element picker, computed styles, box model
  Network     -- network request viewer using Performance API
  System Log  -- Python stderr/stdout captured at runtime
  AI Log      -- structured log of every AI API request and response
"""

import html as html_mod
import json
import logging
import queue
import sys

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtWidgets import (
    QDockWidget, QWidget, QVBoxLayout, QHBoxLayout,
    QTextEdit, QLineEdit, QPushButton, QLabel,
    QComboBox, QFrame, QTabWidget, QCheckBox,
)
from PyQt6.QtGui import QFont


# ---------------------------------------------------------------------------
# Thread-safe queues -- written from any thread, drained by QTimer on main thread
# ---------------------------------------------------------------------------

# AI log: cat_assist._do_ai_call puts dicts here; ConsolePanel drains them.
ai_log_queue: queue.SimpleQueue = queue.SimpleQueue()

# System log: stderr redirector puts strings here.
_sys_log_queue: queue.SimpleQueue = queue.SimpleQueue()


class _StderrCapture:
    """Tee: write to original stderr AND push lines to _sys_log_queue."""
    def __init__(self, original):
        self._orig = original

    def write(self, text):
        self._orig.write(text)
        if text and text != '\n':
            _sys_log_queue.put(text)

    def flush(self):
        self._orig.flush()

    def fileno(self):
        return self._orig.fileno()


# Install stderr capture once at module import.
if not isinstance(sys.stderr, _StderrCapture):
    sys.stderr = _StderrCapture(sys.stderr)


# ---------------------------------------------------------------------------
# Log-level colors
# ---------------------------------------------------------------------------

_LEVEL_COLORS = {
    'log':   '#c8ccd4',
    'info':  '#7dd3fc',
    'warn':  '#fbbf24',
    'error': '#f87171',
    'debug': '#a78bfa',
}

_LEVEL_PREFIX = {
    'log':   '',
    'info':  '[info] ',
    'warn':  '[warn] ',
    'error': '[error] ',
    'debug': '[debug] ',
}

_DOCK_STYLE = (
    'QDockWidget { color: #8888a0; font-size: 11px; }'
    'QDockWidget::title { background: #1a1a2e; padding: 4px 8px; text-align: left; }'
)

_OUTPUT_STYLE = (
    'QTextEdit { background: #0d0d1a; color: #c8ccd4; border: none;'
    'selection-background-color: #2d2d4a; padding: 4px 8px; }'
    'QScrollBar:vertical { background: #0d0d1a; width: 8px; }'
    'QScrollBar::handle:vertical { background: #2d2d4a; border-radius: 4px; }'
    'QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }'
)

_TAB_STYLE = (
    'QTabWidget::pane { border: none; background: #0d0d1a; }'
    'QTabBar::tab { background: #1a1a2e; color: #8888a0; padding: 4px 14px;'
    '  font-size: 11px; border: 1px solid #2d2d4a; border-bottom: none;'
    '  font-family: "JetBrains Mono", monospace; }'
    'QTabBar::tab:selected { background: #0d0d1a; color: #c8ccd4;'
    '  border-bottom: 2px solid #6366f1; }'
    'QTabBar::tab:hover:!selected { background: #16162b; }'
)

_HEADER_STYLE = (
    'QFrame { background: #1a1a2e; border-bottom: 1px solid #2d2d4a; }'
)

_BTN_STYLE = (
    'QPushButton { background: transparent; color: #8888a0;'
    'border: 1px solid #2d2d4a; border-radius: 3px; font-size: 10px; }'
    'QPushButton:hover { color: #fff; background: #6366f1; }'
    'QPushButton:checked { color: #fff; background: #6366f1; border-color: #818cf8; }'
)

_BTN_DANGER_STYLE = (
    'QPushButton { background: transparent; color: #8888a0;'
    'border: 1px solid #2d2d4a; border-radius: 3px; font-size: 10px; }'
    'QPushButton:hover { color: #fff; background: #ef4444; }'
)

_INPUT_STYLE = (
    'QLineEdit { background: transparent; color: #c8ccd4; border: none;'
    'font-family: "JetBrains Mono", monospace; font-size: 11px; }'
)


# ---------------------------------------------------------------------------
# JavaScript templates for Elements inspector
# ---------------------------------------------------------------------------

_INSPECT_ELEMENT_JS = r"""
(function(){
    var sel = _XCM_SEL_;
    var el;
    try { el = document.querySelector(sel); } catch(e) { return JSON.stringify({error:'Invalid selector: '+sel}); }
    if (!el) return JSON.stringify({error:'No element matches: '+sel});
    var cs = getComputedStyle(el);
    var rect = el.getBoundingClientRect();
    var attrs = [];
    for (var i=0; i<el.attributes.length && i<30; i++) {
        attrs.push({name: el.attributes[i].name, value: el.attributes[i].value});
    }
    return JSON.stringify({
        tag: el.tagName.toLowerCase(),
        id: el.id || '',
        classes: Array.from(el.classList),
        attrs: attrs,
        text: (el.textContent||'').substring(0,200).trim(),
        childCount: el.children.length,
        rect: {
            top: Math.round(rect.top),
            left: Math.round(rect.left),
            width: Math.round(rect.width),
            height: Math.round(rect.height)
        },
        styles: {
            color: cs.color,
            backgroundColor: cs.backgroundColor,
            fontSize: cs.fontSize,
            fontFamily: cs.fontFamily,
            fontWeight: cs.fontWeight,
            lineHeight: cs.lineHeight,
            textAlign: cs.textAlign,
            margin: cs.margin,
            padding: cs.padding,
            border: cs.border,
            display: cs.display,
            position: cs.position,
            zIndex: cs.zIndex,
            overflow: cs.overflow,
            boxSizing: cs.boxSizing,
            width: cs.width,
            height: cs.height,
            opacity: cs.opacity,
            visibility: cs.visibility,
            float: cs.cssFloat,
            flexDirection: cs.flexDirection,
            justifyContent: cs.justifyContent,
            alignItems: cs.alignItems,
            gridTemplateColumns: cs.gridTemplateColumns
        }
    });
})();
"""

_HIGHLIGHT_ELEMENT_JS = r"""
(function(){
    var sel = _XCM_SEL_;
    var old = document.getElementById('_xcm_hl');
    if (old) old.remove();
    var el;
    try { el = document.querySelector(sel); } catch(e) { return; }
    if (!el) return;
    var rect = el.getBoundingClientRect();
    var hl = document.createElement('div');
    hl.id = '_xcm_hl';
    hl.style.cssText = 'position:fixed;pointer-events:none;z-index:2147483647;'
        +'border:2px solid #6366f1;background:rgba(99,102,241,0.12);'
        +'top:'+rect.top+'px;left:'+rect.left+'px;'
        +'width:'+rect.width+'px;height:'+rect.height+'px;'
        +'transition:all 0.15s;';
    document.documentElement.appendChild(hl);
    setTimeout(function(){ var e=document.getElementById('_xcm_hl'); if(e) e.remove(); }, 3000);
})();
"""

_PICKER_ACTIVATE_JS = r"""
(function(){
    if (window._xcm_picker_active) {
        window._xcm_picker_active = false;
        if (window._xcm_picker_ov) { window._xcm_picker_ov.remove(); window._xcm_picker_ov=null; }
        document.removeEventListener('mouseover', window._xcm_picker_hover, true);
        document.removeEventListener('click', window._xcm_picker_click, true);
        return 'deactivated';
    }
    window._xcm_picker_active = true;
    window._xcm_picked_selector = '';
    var ov = document.createElement('div');
    ov.id = '_xcm_picker_ov';
    ov.style.cssText = 'position:fixed;pointer-events:none;z-index:2147483647;'
        +'border:2px solid #f59e0b;background:rgba(245,158,11,0.15);'
        +'transition:all 0.08s;display:none;';
    document.documentElement.appendChild(ov);
    window._xcm_picker_ov = ov;

    window._xcm_picker_hover = function(e) {
        if (!window._xcm_picker_active) return;
        var r = e.target.getBoundingClientRect();
        ov.style.top=r.top+'px'; ov.style.left=r.left+'px';
        ov.style.width=r.width+'px'; ov.style.height=r.height+'px';
        ov.style.display='block';
    };
    window._xcm_picker_click = function(e) {
        if (!window._xcm_picker_active) return;
        e.preventDefault(); e.stopPropagation();
        window._xcm_picker_active = false;
        ov.remove(); window._xcm_picker_ov = null;
        document.removeEventListener('mouseover', window._xcm_picker_hover, true);
        document.removeEventListener('click', window._xcm_picker_click, true);
        var el = e.target;
        window._xcm_picked_selector = _buildSel(el);
    };
    function _buildSel(el) {
        if (el.id) return '#'+el.id;
        var parts = [];
        while (el && el.nodeType === 1) {
            var part = el.tagName.toLowerCase();
            if (el.id) { parts.unshift('#'+el.id); break; }
            if (el.className && typeof el.className === 'string') {
                var cls = el.className.trim().split(/\s+/).filter(function(c){return c.length>0;}).slice(0,2).join('.');
                if (cls) part += '.'+cls;
            }
            parts.unshift(part);
            el = el.parentElement;
        }
        return parts.join(' > ');
    }
    document.addEventListener('mouseover', window._xcm_picker_hover, true);
    document.addEventListener('click', window._xcm_picker_click, true);
    return 'activated';
})();
"""

_PICKER_CHECK_JS = "window._xcm_picked_selector || '';"

_DOM_TREE_JS = r"""
(function(){
    var root = _XCM_ROOT_;
    var maxD = _XCM_DEPTH_;
    function walk(el, depth) {
        if (depth > maxD) return [];
        var tag = el.tagName ? el.tagName.toLowerCase() : '';
        if (!tag) return [];
        var id = el.id ? '#'+el.id : '';
        var cls = (el.className && typeof el.className === 'string')
            ? '.'+el.className.trim().split(/\s+/).slice(0,3).join('.') : '';
        var indent = '';
        for (var i=0;i<depth;i++) indent += '  ';
        var result = [indent + tag + id + cls];
        var ch = el.children;
        for (var i=0; i<ch.length && i<80; i++) {
            result = result.concat(walk(ch[i], depth+1));
        }
        if (ch.length > 80) result.push(indent+'  ... ('+(ch.length-80)+' more children)');
        return result;
    }
    return walk(root, 0).join('\n');
})();
"""

# ---------------------------------------------------------------------------
# JavaScript templates for Network tab
# ---------------------------------------------------------------------------

_NETWORK_ENTRIES_JS = r"""
(function(){
    var entries = performance.getEntriesByType('resource');
    var nav = performance.getEntriesByType('navigation');
    var result = {resources: [], navigation: null, timing: {}};
    if (nav.length > 0) {
        var n = nav[0];
        result.navigation = {
            type: n.type,
            duration: Math.round(n.duration),
            domContentLoaded: Math.round(n.domContentLoadedEventEnd - n.startTime),
            domComplete: Math.round(n.domComplete - n.startTime),
            loadEvent: Math.round(n.loadEventEnd - n.startTime),
            transferSize: n.transferSize || 0,
            domNodes: document.getElementsByTagName('*').length
        };
    }
    for (var i=0; i<entries.length; i++) {
        var e = entries[i];
        var url = e.name;
        var parts = url.split('/');
        var short = (parts[parts.length-1] || url).split('?')[0];
        if (short.length > 60) short = short.substring(0,57) + '...';
        result.resources.push({
            name: short,
            fullUrl: url,
            type: e.initiatorType || 'other',
            duration: Math.round(e.duration),
            size: e.transferSize || 0,
            start: Math.round(e.startTime),
            status: e.responseStatus || 0
        });
    }
    result.timing.domNodes = document.getElementsByTagName('*').length;
    result.timing.jsHeap = (performance.memory && performance.memory.usedJSHeapSize)
        ? Math.round(performance.memory.usedJSHeapSize / 1048576) : null;
    return JSON.stringify(result);
})();
"""

_NETWORK_CLEAR_JS = "performance.clearResourceTimings();"

_PAGE_METRICS_JS = r"""
(function(){
    var m = {};
    m.url = location.href;
    m.title = document.title;
    m.domNodes = document.getElementsByTagName('*').length;
    m.scripts = document.scripts.length;
    m.stylesheets = document.styleSheets.length;
    m.images = document.images.length;
    m.links = document.links.length;
    m.forms = document.forms.length;
    m.iframes = document.querySelectorAll('iframe').length;
    m.cookies = document.cookie ? document.cookie.split(';').length : 0;
    var perf = performance.getEntriesByType('navigation');
    if (perf.length) {
        var p = perf[0];
        m.loadTime = Math.round(p.loadEventEnd - p.startTime);
        m.domReady = Math.round(p.domContentLoadedEventEnd - p.startTime);
        m.ttfb = Math.round(p.responseStart - p.requestStart);
    }
    m.resources = performance.getEntriesByType('resource').length;
    if (performance.memory) {
        m.jsHeapMB = Math.round(performance.memory.usedJSHeapSize / 1048576);
    }
    return JSON.stringify(m);
})();
"""


# ---------------------------------------------------------------------------
# Helper: build a read-only output QTextEdit
# ---------------------------------------------------------------------------

def _make_output() -> QTextEdit:
    w = QTextEdit()
    w.setReadOnly(True)
    w.setFont(QFont('JetBrains Mono', 11))
    w.setStyleSheet(_OUTPUT_STYLE)
    return w


def _scroll_bottom(edit: QTextEdit):
    sb = edit.verticalScrollBar()
    sb.setValue(sb.maximum())


def _append_html(edit: QTextEdit, html: str):
    # QTextEdit.append() wraps content in a <p> block that drops inline
    # <span> styles in PyQt6.  Using QTextCursor.insertHtml() at the end
    # of the document preserves all inline style attributes reliably.
    cursor = edit.textCursor()
    cursor.movePosition(cursor.MoveOperation.End)
    if not edit.document().isEmpty():
        cursor.insertBlock()
    cursor.insertHtml(html)
    _scroll_bottom(edit)


# ---------------------------------------------------------------------------
# ConsolePanel
# ---------------------------------------------------------------------------

class ConsolePanel(QDockWidget):
    """Dockable panel with five tabs: JS Console, Elements, Network,
    System Log, AI Log.

    Public API (backwards compatible):
        panel.append_message(level, text, src=None, url=None, line=None, col=None)
        panel.clear()
    """

    def __init__(self, main_window):
        super().__init__('Console', main_window)
        self.main_window = main_window
        self.setObjectName('ConsolePanel')
        self.setAllowedAreas(
            Qt.DockWidgetArea.BottomDockWidgetArea
            | Qt.DockWidgetArea.TopDockWidgetArea
        )
        self.setStyleSheet(_DOCK_STYLE)

        # Root container
        root = QWidget()
        root_layout = QVBoxLayout(root)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.setSpacing(0)

        # ------------------------------------------------------------------
        # Tab widget
        # ------------------------------------------------------------------
        self._tabs = QTabWidget()
        self._tabs.setStyleSheet(_TAB_STYLE)
        root_layout.addWidget(self._tabs)

        # Tab 0: JS Console
        self._tabs.addTab(self._build_js_tab(), 'JS Console')

        # Tab 1: Elements
        self._tabs.addTab(self._build_elements_tab(), 'Elements')

        # Tab 2: Network
        self._tabs.addTab(self._build_network_tab(), 'Network')

        # Tab 3: System Log
        self._sys_output = _make_output()
        sys_widget = QWidget()
        sys_layout = QVBoxLayout(sys_widget)
        sys_layout.setContentsMargins(0, 0, 0, 0)
        sys_layout.setSpacing(0)
        sys_layout.addWidget(self._sys_header())
        sys_layout.addWidget(self._sys_output, 1)
        self._tabs.addTab(sys_widget, 'System Log')

        # Tab 4: AI Log
        self._ai_output = _make_output()
        ai_widget = QWidget()
        ai_layout = QVBoxLayout(ai_widget)
        ai_layout.setContentsMargins(0, 0, 0, 0)
        ai_layout.setSpacing(0)
        ai_layout.addWidget(self._ai_header())
        ai_layout.addWidget(self._ai_output, 1)
        self._tabs.addTab(ai_widget, 'AI Log')

        self.setWidget(root)

        # JS console message store
        self._messages = []
        self._max_messages = 2000

        # Element picker polling state
        self._picker_active = False
        self._picker_timer = None

        # Poll queues every 200 ms
        self._poll_timer = QTimer(self)
        self._poll_timer.timeout.connect(self._drain_queues)
        self._poll_timer.start(200)

    # ==================================================================
    #  JS Console tab
    # ==================================================================

    def _build_js_tab(self) -> QWidget:
        w = QWidget()
        layout = QVBoxLayout(w)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Header bar
        header = QFrame()
        header.setFixedHeight(30)
        header.setStyleSheet(_HEADER_STYLE)
        h_layout = QHBoxLayout(header)
        h_layout.setContentsMargins(8, 2, 8, 2)

        title_lbl = QLabel('Console')
        title_lbl.setStyleSheet(
            'color: #8888a0; font-size: 11px; font-weight: bold;'
            'font-family: "JetBrains Mono", monospace;'
        )
        h_layout.addWidget(title_lbl)

        self._filter = QComboBox()
        self._filter.addItems(['All', 'Errors', 'Warnings', 'Info', 'Log'])
        self._filter.setFixedWidth(90)
        self._filter.setStyleSheet(
            'QComboBox { background: #16162b; color: #8888a0; border: 1px solid #2d2d4a;'
            'border-radius: 3px; padding: 2px 6px; font-size: 10px; }'
            'QComboBox::drop-down { border: none; }'
            'QComboBox QAbstractItemView { background: #16162b; color: #c8ccd4;'
            'selection-background-color: #2d2d4a; }'
        )
        self._filter.currentTextChanged.connect(self._apply_filter)
        h_layout.addWidget(self._filter)
        h_layout.addStretch()

        clear_btn = QPushButton('Clear')
        clear_btn.setFixedSize(50, 22)
        clear_btn.setStyleSheet(_BTN_DANGER_STYLE)
        clear_btn.clicked.connect(self.clear)
        h_layout.addWidget(clear_btn)

        layout.addWidget(header)

        self._output = _make_output()
        layout.addWidget(self._output, 1)

        # Input bar
        input_frame = QFrame()
        input_frame.setFixedHeight(30)
        input_frame.setStyleSheet(
            'QFrame { background: #1a1a2e; border-top: 1px solid #2d2d4a; }'
        )
        i_layout = QHBoxLayout(input_frame)
        i_layout.setContentsMargins(8, 2, 8, 2)

        prompt_lbl = QLabel('>')
        prompt_lbl.setStyleSheet(
            'color: #6366f1; font-size: 13px; font-weight: bold;'
            'font-family: "JetBrains Mono", monospace;'
        )
        i_layout.addWidget(prompt_lbl)

        self._input = QLineEdit()
        self._input.setPlaceholderText('Evaluate JavaScript...')
        self._input.setStyleSheet(_INPUT_STYLE)
        self._input.returnPressed.connect(self._eval_js)
        i_layout.addWidget(self._input, 1)

        layout.addWidget(input_frame)
        return w

    # ==================================================================
    #  Elements tab
    # ==================================================================

    def _build_elements_tab(self) -> QWidget:
        w = QWidget()
        layout = QVBoxLayout(w)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Header
        header = QFrame()
        header.setFixedHeight(30)
        header.setStyleSheet(_HEADER_STYLE)
        h_layout = QHBoxLayout(header)
        h_layout.setContentsMargins(8, 2, 8, 2)

        self._el_input = QLineEdit()
        self._el_input.setPlaceholderText('CSS selector (e.g. #main, .header, div.card)')
        self._el_input.setStyleSheet(_INPUT_STYLE)
        self._el_input.returnPressed.connect(self._inspect_element)
        h_layout.addWidget(self._el_input, 1)

        inspect_btn = QPushButton('Inspect')
        inspect_btn.setFixedSize(56, 22)
        inspect_btn.setStyleSheet(_BTN_STYLE)
        inspect_btn.clicked.connect(self._inspect_element)
        h_layout.addWidget(inspect_btn)

        self._pick_btn = QPushButton('Pick')
        self._pick_btn.setFixedSize(42, 22)
        self._pick_btn.setCheckable(True)
        self._pick_btn.setStyleSheet(_BTN_STYLE)
        self._pick_btn.clicked.connect(self._toggle_picker)
        h_layout.addWidget(self._pick_btn)

        dom_btn = QPushButton('DOM Tree')
        dom_btn.setFixedSize(66, 22)
        dom_btn.setStyleSheet(_BTN_STYLE)
        dom_btn.clicked.connect(self._show_dom_tree)
        h_layout.addWidget(dom_btn)

        metrics_btn = QPushButton('Metrics')
        metrics_btn.setFixedSize(56, 22)
        metrics_btn.setStyleSheet(_BTN_STYLE)
        metrics_btn.clicked.connect(self._show_page_metrics)
        h_layout.addWidget(metrics_btn)

        clear_btn = QPushButton('Clear')
        clear_btn.setFixedSize(50, 22)
        clear_btn.setStyleSheet(_BTN_DANGER_STYLE)
        clear_btn.clicked.connect(lambda: self._el_output.clear())
        h_layout.addWidget(clear_btn)

        layout.addWidget(header)

        self._el_output = _make_output()
        layout.addWidget(self._el_output, 1)
        return w

    # ==================================================================
    #  Network tab
    # ==================================================================

    def _build_network_tab(self) -> QWidget:
        w = QWidget()
        layout = QVBoxLayout(w)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        # Header
        header = QFrame()
        header.setFixedHeight(30)
        header.setStyleSheet(_HEADER_STYLE)
        h_layout = QHBoxLayout(header)
        h_layout.setContentsMargins(8, 2, 8, 2)

        title_lbl = QLabel('Network')
        title_lbl.setStyleSheet(
            'color: #8888a0; font-size: 11px; font-weight: bold;'
            'font-family: "JetBrains Mono", monospace;'
        )
        h_layout.addWidget(title_lbl)

        self._net_type_filter = QComboBox()
        self._net_type_filter.addItems([
            'All', 'script', 'css', 'img', 'xmlhttprequest',
            'fetch', 'font', 'link', 'other',
        ])
        self._net_type_filter.setFixedWidth(110)
        self._net_type_filter.setStyleSheet(
            'QComboBox { background: #16162b; color: #8888a0; border: 1px solid #2d2d4a;'
            'border-radius: 3px; padding: 2px 6px; font-size: 10px; }'
            'QComboBox::drop-down { border: none; }'
            'QComboBox QAbstractItemView { background: #16162b; color: #c8ccd4;'
            'selection-background-color: #2d2d4a; }'
        )
        h_layout.addWidget(self._net_type_filter)

        h_layout.addStretch()

        capture_btn = QPushButton('Capture')
        capture_btn.setFixedSize(60, 22)
        capture_btn.setStyleSheet(_BTN_STYLE)
        capture_btn.clicked.connect(self._capture_network)
        h_layout.addWidget(capture_btn)

        self._net_auto = QCheckBox('Auto')
        self._net_auto.setStyleSheet(
            'QCheckBox { color: #8888a0; font-size: 10px;'
            'font-family: "JetBrains Mono", monospace; }'
        )
        self._net_auto.setToolTip('Auto-capture every 3 seconds')
        self._net_auto.toggled.connect(self._toggle_auto_net)
        h_layout.addWidget(self._net_auto)

        reset_btn = QPushButton('Reset')
        reset_btn.setFixedSize(46, 22)
        reset_btn.setStyleSheet(_BTN_STYLE)
        reset_btn.clicked.connect(self._reset_network)
        h_layout.addWidget(reset_btn)

        clear_btn = QPushButton('Clear')
        clear_btn.setFixedSize(50, 22)
        clear_btn.setStyleSheet(_BTN_DANGER_STYLE)
        clear_btn.clicked.connect(lambda: self._net_output.clear())
        h_layout.addWidget(clear_btn)

        layout.addWidget(header)

        self._net_output = _make_output()
        layout.addWidget(self._net_output, 1)

        # Auto-capture timer
        self._net_timer = QTimer(self)
        self._net_timer.timeout.connect(self._capture_network)

        return w

    # ------------------------------------------------------------------
    # Build System Log header
    # ------------------------------------------------------------------

    def _sys_header(self) -> QFrame:
        hdr = QFrame()
        hdr.setFixedHeight(30)
        hdr.setStyleSheet(_HEADER_STYLE)
        hl = QHBoxLayout(hdr)
        hl.setContentsMargins(8, 2, 8, 2)
        lbl = QLabel('Python stderr / stdout')
        lbl.setStyleSheet(
            'color: #8888a0; font-size: 11px; font-weight: bold;'
            'font-family: "JetBrains Mono", monospace;'
        )
        hl.addWidget(lbl)
        hl.addStretch()
        btn = QPushButton('Clear')
        btn.setFixedSize(50, 22)
        btn.setStyleSheet(_BTN_DANGER_STYLE)
        btn.clicked.connect(self._sys_output.clear)
        hl.addWidget(btn)
        return hdr

    # ------------------------------------------------------------------
    # Build AI Log header
    # ------------------------------------------------------------------

    def _ai_header(self) -> QFrame:
        hdr = QFrame()
        hdr.setFixedHeight(30)
        hdr.setStyleSheet(_HEADER_STYLE)
        hl = QHBoxLayout(hdr)
        hl.setContentsMargins(8, 2, 8, 2)
        lbl = QLabel('AI API Requests / Responses')
        lbl.setStyleSheet(
            'color: #8888a0; font-size: 11px; font-weight: bold;'
            'font-family: "JetBrains Mono", monospace;'
        )
        hl.addWidget(lbl)
        hl.addStretch()
        btn = QPushButton('Clear')
        btn.setFixedSize(50, 22)
        btn.setStyleSheet(_BTN_DANGER_STYLE)
        btn.clicked.connect(self._ai_output.clear)
        hl.addWidget(btn)
        return hdr

    # ==================================================================
    #  Helper: get active browser tab
    # ==================================================================

    def _active_tab(self):
        """Return the current browser tab widget, or None."""
        try:
            return self.main_window.tabs.currentWidget()
        except Exception:
            return None

    def _run_page_js(self, js, callback):
        """Run JS in the active tab and deliver the result to callback."""
        browser = self._active_tab()
        if not browser:
            callback(None)
            return
        if hasattr(browser, 'page'):
            browser.page().runJavaScript(js, callback)
        else:
            callback(None)

    # ==================================================================
    #  Elements: inspect element
    # ==================================================================

    def _inspect_element(self):
        selector = self._el_input.text().strip()
        if not selector:
            self._el_write('warn', 'Enter a CSS selector to inspect.')
            return
        escaped = json.dumps(selector)
        js = _INSPECT_ELEMENT_JS.replace('_XCM_SEL_', escaped)
        highlight_js = _HIGHLIGHT_ELEMENT_JS.replace('_XCM_SEL_', escaped)

        def _on_result(result):
            if result is None:
                self._el_write('error', '(no result -- tab may not support JS eval)')
                return
            try:
                data = json.loads(result)
            except (json.JSONDecodeError, TypeError):
                self._el_write('log', str(result))
                return
            if 'error' in data:
                self._el_write('error', data['error'])
                return
            self._display_element_info(data)

        self._run_page_js(js, _on_result)
        # Also highlight the element on the page
        self._run_page_js(highlight_js, lambda _: None)

    def _display_element_info(self, data):
        tag = data.get('tag', '')
        eid = data.get('id', '')
        classes = data.get('classes', [])
        attrs = data.get('attrs', [])
        text = data.get('text', '')
        rect = data.get('rect', {})
        styles = data.get('styles', {})
        child_count = data.get('childCount', 0)

        # Build selector display
        sel_display = tag
        if eid:
            sel_display += '#' + eid
        if classes:
            sel_display += '.' + '.'.join(classes)

        self._el_write_html(
            f'<span style="color:#6366f1; font-size:12px; font-weight:bold;">'
            f'&lt;{_esc(sel_display)}&gt;</span>'
            f'<span style="color:#4b5563; font-size:10px;">  '
            f'{child_count} children</span>'
        )

        # Attributes
        if attrs:
            attr_parts = []
            for a in attrs[:15]:
                name = _esc(a.get('name', ''))
                val = _esc(a.get('value', ''))
                if len(val) > 60:
                    val = val[:57] + '...'
                attr_parts.append(
                    f'<span style="color:#fbbf24;">{name}</span>'
                    f'=<span style="color:#4ade80;">"{val}"</span>'
                )
            self._el_write_html(
                '<span style="color:#8888a0; font-size:10px;">  '
                + '  '.join(attr_parts) + '</span>'
            )

        # Box model
        if rect:
            self._el_write_html(
                f'<span style="color:#7dd3fc; font-size:11px;">'
                f'  Box: {rect.get("width", 0)}x{rect.get("height", 0)} '
                f'at ({rect.get("left", 0)}, {rect.get("top", 0)})</span>'
            )

        # Text preview
        if text:
            t = _esc(text[:150])
            self._el_write_html(
                f'<span style="color:#8888a0; font-size:10px;">'
                f'  Text: "{t}"</span>'
            )

        # Computed styles
        self._el_write_html(
            '<span style="color:#c8ccd4; font-size:11px; font-weight:bold;">'
            '  Computed Styles:</span>'
        )
        for prop, val in styles.items():
            if val and val != 'none' and val != 'normal' and val != 'auto' and val != '0px':
                pname = _camel_to_css(prop)
                self._el_write_html(
                    f'<span style="color:#a78bfa; font-size:10px;">    '
                    f'{_esc(pname)}: </span>'
                    f'<span style="color:#c8ccd4; font-size:10px;">'
                    f'{_esc(str(val))}</span>'
                )

        # Separator
        self._el_write_html(
            '<span style="color:#2d2d4a;">  '
            '------------------------------------------------</span>'
        )

    # ==================================================================
    #  Elements: pick element from page
    # ==================================================================

    def _toggle_picker(self):
        browser = self._active_tab()
        if not browser:
            self._el_write('error', '(no active tab)')
            self._pick_btn.setChecked(False)
            return

        def _on_result(result):
            if result == 'activated':
                self._picker_active = True
                self._el_write('info', 'Element picker active -- click any element on the page.')
                # Start polling for picked selector
                self._picker_timer = QTimer(self)
                self._picker_timer.timeout.connect(self._poll_picker)
                self._picker_timer.start(300)
            else:
                self._picker_active = False
                self._pick_btn.setChecked(False)
                if self._picker_timer:
                    self._picker_timer.stop()
                    self._picker_timer = None

        self._run_page_js(_PICKER_ACTIVATE_JS, _on_result)

    def _poll_picker(self):
        if not self._picker_active:
            if self._picker_timer:
                self._picker_timer.stop()
                self._picker_timer = None
            return

        def _on_result(result):
            if result and isinstance(result, str) and result.strip():
                self._picker_active = False
                self._pick_btn.setChecked(False)
                if self._picker_timer:
                    self._picker_timer.stop()
                    self._picker_timer = None
                self._el_input.setText(result)
                self._inspect_element()

        self._run_page_js(_PICKER_CHECK_JS, _on_result)

    # ==================================================================
    #  Elements: DOM tree
    # ==================================================================

    def _show_dom_tree(self):
        selector = self._el_input.text().strip()
        if selector:
            root_expr = 'document.querySelector(%s) || document.body' % json.dumps(selector)
        else:
            root_expr = 'document.body'
        js = _DOM_TREE_JS.replace('_XCM_ROOT_', root_expr).replace('_XCM_DEPTH_', '6')

        def _on_result(result):
            if result is None:
                self._el_write('error', '(no result)')
                return
            text = str(result)
            self._el_write_html(
                '<span style="color:#6366f1; font-size:11px; font-weight:bold;">'
                'DOM Tree (max depth 6):</span>'
            )
            for line in text.split('\n'):
                esc = _esc(line)
                # Color tags vs indentation
                stripped = line.lstrip()
                indent = len(line) - len(stripped)
                ind_html = '&nbsp;' * indent
                # Highlight IDs and classes
                parts = stripped.split('#')
                if len(parts) > 1 and '.' in parts[-1]:
                    tag_part = parts[0]
                    rest = '#'.join(parts[1:])
                    cls_parts = rest.split('.')
                    colored = (
                        f'<span style="color:#c8ccd4;">{_esc(tag_part)}</span>'
                        f'<span style="color:#fbbf24;">#{_esc(cls_parts[0])}</span>'
                    )
                    for cp in cls_parts[1:]:
                        colored += f'<span style="color:#4ade80;">.{_esc(cp)}</span>'
                elif '#' in stripped:
                    tag_part, id_part = stripped.split('#', 1)
                    colored = (
                        f'<span style="color:#c8ccd4;">{_esc(tag_part)}</span>'
                        f'<span style="color:#fbbf24;">#{_esc(id_part)}</span>'
                    )
                elif '.' in stripped and not stripped.startswith('.'):
                    parts = stripped.split('.')
                    colored = f'<span style="color:#c8ccd4;">{_esc(parts[0])}</span>'
                    for p in parts[1:]:
                        colored += f'<span style="color:#4ade80;">.{_esc(p)}</span>'
                else:
                    colored = f'<span style="color:#c8ccd4;">{esc}</span>'

                self._el_write_html(
                    f'<span style="font-size:10px;">{ind_html}{colored}</span>'
                )

        self._run_page_js(js, _on_result)

    # ==================================================================
    #  Elements: page metrics
    # ==================================================================

    def _show_page_metrics(self):
        def _on_result(result):
            if result is None:
                self._el_write('error', '(no result)')
                return
            try:
                data = json.loads(result)
            except (json.JSONDecodeError, TypeError):
                self._el_write('log', str(result))
                return

            self._el_write_html(
                '<span style="color:#6366f1; font-size:12px; font-weight:bold;">'
                'Page Metrics</span>'
            )
            _metrics = [
                ('URL', data.get('url', '')),
                ('Title', data.get('title', '')),
                ('DOM Nodes', data.get('domNodes')),
                ('Scripts', data.get('scripts')),
                ('Stylesheets', data.get('stylesheets')),
                ('Images', data.get('images')),
                ('Links', data.get('links')),
                ('Forms', data.get('forms')),
                ('Iframes', data.get('iframes')),
                ('Cookies', data.get('cookies')),
                ('Resources', data.get('resources')),
                ('Load Time', '%s ms' % data.get('loadTime', '--')),
                ('DOM Ready', '%s ms' % data.get('domReady', '--')),
                ('TTFB', '%s ms' % data.get('ttfb', '--')),
            ]
            if data.get('jsHeapMB') is not None:
                _metrics.append(('JS Heap', '%s MB' % data['jsHeapMB']))

            for label, value in _metrics:
                self._el_write_html(
                    f'<span style="color:#7dd3fc; font-size:11px;">  '
                    f'{_esc(label)}:</span> '
                    f'<span style="color:#c8ccd4; font-size:11px;">'
                    f'{_esc(str(value))}</span>'
                )
            self._el_write_html(
                '<span style="color:#2d2d4a;">  '
                '------------------------------------------------</span>'
            )

        self._run_page_js(_PAGE_METRICS_JS, _on_result)

    # ==================================================================
    #  Elements helpers
    # ==================================================================

    def _el_write(self, level, text):
        color = _LEVEL_COLORS.get(level, '#c8ccd4')
        prefix = _LEVEL_PREFIX.get(level, '')
        escaped = _esc(text)
        _append_html(self._el_output,
            f'<span style="color:{color}; font-family: JetBrains Mono, monospace;'
            f'font-size: 11px;">{prefix}{escaped}</span>'
        )

    def _el_write_html(self, html_str):
        _append_html(self._el_output,
            f'<span style="font-family: JetBrains Mono, monospace;">'
            f'{html_str}</span>'
        )

    # ==================================================================
    #  Network: capture
    # ==================================================================

    def _capture_network(self):
        def _on_result(result):
            if result is None:
                self._net_write('error', '(no result -- tab may not support JS eval)')
                return
            try:
                data = json.loads(result)
            except (json.JSONDecodeError, TypeError):
                self._net_write('log', str(result))
                return
            self._display_network(data)

        self._run_page_js(_NETWORK_ENTRIES_JS, _on_result)

    def _display_network(self, data):
        nav = data.get('navigation')
        resources = data.get('resources', [])
        timing = data.get('timing', {})
        type_filter = self._net_type_filter.currentText()

        # Navigation summary
        if nav:
            self._net_write_html(
                f'<span style="color:#6366f1; font-size:11px; font-weight:bold;">'
                f'Navigation ({_esc(nav.get("type",""))})  '
                f'</span>'
                f'<span style="color:#c8ccd4; font-size:10px;">'
                f'DOM Ready: {nav.get("domContentLoaded", "--")}ms  '
                f'Load: {nav.get("loadEvent", "--")}ms  '
                f'DOM Nodes: {nav.get("domNodes", "--")}  '
                f'Transfer: {_fmt_size(nav.get("transferSize", 0))}'
                f'</span>'
            )

        # Resource table header
        total = len(resources)
        if type_filter != 'All':
            resources = [r for r in resources if r.get('type', '') == type_filter]
        filtered = len(resources)

        total_size = sum(r.get('size', 0) for r in resources)
        total_dur = max((r.get('start', 0) + r.get('duration', 0) for r in resources), default=0)

        self._net_write_html(
            f'<span style="color:#8888a0; font-size:10px;">'
            f'{filtered} resources'
            + (f' (of {total})' if type_filter != 'All' else '')
            + f'  |  Total: {_fmt_size(total_size)}  '
            f'|  Waterfall: {total_dur}ms</span>'
        )

        self._net_write_html(
            '<span style="color:#4b5563; font-size:10px;">'
            '  %-40s %-12s %8s %8s %6s</span>' % (
                'Resource', 'Type', 'Size', 'Time', 'Status'
            )
        )

        # Sort by start time
        resources.sort(key=lambda r: r.get('start', 0))

        for r in resources:
            name = r.get('name', '?')
            rtype = r.get('type', '')
            size = r.get('size', 0)
            dur = r.get('duration', 0)
            status = r.get('status', 0)

            # Color code by type
            type_colors = {
                'script': '#fbbf24',
                'css': '#a78bfa',
                'img': '#4ade80',
                'xmlhttprequest': '#7dd3fc',
                'fetch': '#7dd3fc',
                'font': '#f472b6',
                'link': '#818cf8',
            }
            color = type_colors.get(rtype, '#8888a0')

            # Status color
            if status >= 400:
                status_color = '#f87171'
            elif status >= 300:
                status_color = '#fbbf24'
            elif status > 0:
                status_color = '#4ade80'
            else:
                status_color = '#4b5563'

            status_str = str(status) if status > 0 else '--'
            name_esc = _esc(name[:42])
            size_str = _fmt_size(size)
            dur_str = '%dms' % dur

            self._net_write_html(
                f'<span style="color:{color}; font-size:10px;">  '
                f'{name_esc:<42}</span>'
                f'<span style="color:#8888a0; font-size:10px;"> '
                f'{_esc(rtype):<12}</span>'
                f'<span style="color:#c8ccd4; font-size:10px;"> '
                f'{size_str:>8}</span>'
                f'<span style="color:#c8ccd4; font-size:10px;"> '
                f'{dur_str:>8}</span>'
                f'<span style="color:{status_color}; font-size:10px;"> '
                f'{status_str:>6}</span>'
            )

        self._net_write_html(
            '<span style="color:#2d2d4a;">  '
            '------------------------------------------------</span>'
        )

    def _toggle_auto_net(self, checked):
        if checked:
            self._net_timer.start(3000)
            self._capture_network()
        else:
            self._net_timer.stop()

    def _reset_network(self):
        """Clear the browser's resource timing buffer."""
        self._run_page_js(_NETWORK_CLEAR_JS, lambda _: None)
        self._net_output.clear()
        self._net_write('info', 'Resource timings cleared.')

    def _net_write(self, level, text):
        color = _LEVEL_COLORS.get(level, '#c8ccd4')
        prefix = _LEVEL_PREFIX.get(level, '')
        escaped = _esc(text)
        _append_html(self._net_output,
            f'<span style="color:{color}; font-family: JetBrains Mono, monospace;'
            f'font-size: 11px;">{prefix}{escaped}</span>'
        )

    def _net_write_html(self, html_str):
        _append_html(self._net_output,
            f'<span style="font-family: JetBrains Mono, monospace;">'
            f'{html_str}</span>'
        )

    # ==================================================================
    #  Queue drain (System Log / AI Log)
    # ==================================================================

    def _drain_queues(self):
        # System log
        changed_sys = False
        while True:
            try:
                text = _sys_log_queue.get_nowait()
            except queue.Empty:
                break
            self._write_sys(text)
            changed_sys = True

        # AI log
        changed_ai = False
        while True:
            try:
                rec = ai_log_queue.get_nowait()
            except queue.Empty:
                break
            self._write_ai(rec)
            changed_ai = True

        # Flash tab labels when there is new content and the tab is not active
        if changed_sys and self._tabs.currentIndex() != 3:
            self._tabs.setTabText(3, 'System Log *')
        if changed_ai and self._tabs.currentIndex() != 4:
            self._tabs.setTabText(4, 'AI Log *')

    def _on_tab_changed(self, idx):
        if idx == 0:
            self._tabs.setTabText(0, 'JS Console')
        elif idx == 3:
            self._tabs.setTabText(3, 'System Log')
        elif idx == 4:
            self._tabs.setTabText(4, 'AI Log')

    def _write_sys(self, text: str):
        text = text.rstrip('\n')
        if not text:
            return
        escaped = _esc(text)
        if 'error' in text.lower() or 'traceback' in text.lower() or 'exception' in text.lower():
            color = '#f87171'
        elif text.startswith('[cat_assist]'):
            color = '#fbbf24'
        else:
            color = '#c8ccd4'
        html = (
            f'<span style="color:{color}; font-family: JetBrains Mono, monospace;'
            f'font-size: 11px;">{escaped}</span>'
        )
        _append_html(self._sys_output, html)

    def _write_ai(self, rec: dict):
        """rec keys: kind, action, model, status, body, elapsed_ms, error"""
        kind = rec.get('kind', '')
        if kind == 'request':
            action  = rec.get('action', '')
            model   = rec.get('model', '')
            preview = rec.get('preview', '')
            html = (
                f'<span style="color:#6366f1; font-family: JetBrains Mono, monospace; font-size:11px;">'
                f'--&gt; REQUEST  action={action}  model={model}</span>'
            )
            _append_html(self._ai_output, html)
            if preview:
                esc = _esc(preview)
                _append_html(self._ai_output,
                    f'<span style="color:#8888a0; font-size:10px; font-family: JetBrains Mono, monospace;">'
                    f'    {esc}</span>')
        elif kind == 'response':
            status  = rec.get('status', '')
            elapsed = rec.get('elapsed_ms', 0)
            reply   = rec.get('reply', '')
            color   = '#4ade80' if status == 'ok' else '#f87171'
            html = (
                f'<span style="color:{color}; font-family: JetBrains Mono, monospace; font-size:11px;">'
                f'&lt;-- RESPONSE status={status}  {elapsed:.0f}ms</span>'
            )
            _append_html(self._ai_output, html)
            if reply:
                esc = _esc(reply[:400])
                _append_html(self._ai_output,
                    f'<span style="color:#c8ccd4; font-size:10px; font-family: JetBrains Mono, monospace;">'
                    f'    {esc}</span>')
        elif kind == 'error':
            msg = rec.get('error', '')
            esc = _esc(msg)
            _append_html(self._ai_output,
                f'<span style="color:#f87171; font-family: JetBrains Mono, monospace; font-size:11px;">'
                f'!!! ERROR  {esc}</span>')

    # ==================================================================
    #  JS Console public API (backwards compatible)
    # ==================================================================

    def append_message(self, level, text, src=None, url=None, line=None, col=None):
        """Add a JS console message."""
        if len(self._messages) >= self._max_messages:
            self._messages = self._messages[500:]

        self._messages.append((level, text, src, url, line, col))
        if not self._passes_filter(level):
            return
        self._write_line(level, text, src, line, col)

        # Flash JS tab label if not active
        if self._tabs.currentIndex() != 0:
            self._tabs.setTabText(0, 'JS Console *')

    def _write_line(self, level, text, src=None, line=None, col=None):
        color  = _LEVEL_COLORS.get(level, '#c8ccd4')
        prefix = _LEVEL_PREFIX.get(level, '')
        escaped = _esc(text)
        src_html = ''
        if src:
            loc = src
            if line:
                loc += f':{line}'
            loc_esc = _esc(loc)
            src_html = (
                f'<span style="color:#4b5563; font-size:10px; float:right;'
                f'padding-left:12px;"> {loc_esc}</span>'
            )
        html = (
            f'<span style="color:{color}; font-family: JetBrains Mono, monospace;'
            f'font-size: 11px;">{src_html}{prefix}{escaped}</span>'
        )
        _append_html(self._output, html)

    def _passes_filter(self, level):
        f = self._filter.currentText()
        if f == 'All':       return True
        if f == 'Errors':    return level == 'error'
        if f == 'Warnings':  return level == 'warn'
        if f == 'Info':      return level == 'info'
        if f == 'Log':       return level == 'log'
        return True

    def _apply_filter(self, text):
        self._output.clear()
        for record in self._messages:
            level, msg, src, url, line, col = record
            if self._passes_filter(level):
                self._write_line(level, msg, src, line, col)

    def clear(self):
        self._messages.clear()
        self._output.clear()

    def _eval_js(self):
        js = self._input.text().strip()
        if not js:
            return
        self._input.clear()
        self._write_line('debug', '> ' + js)

        browser = self._active_tab()
        if not browser:
            self._write_line('error', '(no active tab)')
            return

        # Use page().runJavaScript for both WK and Qt engines.
        # The WK _PageShim supports callbacks, so results are captured.
        if hasattr(browser, 'page'):
            def _on_result(result):
                if result is not None:
                    text = result if isinstance(result, str) else repr(result)
                    self._write_line('log', text)
            browser.page().runJavaScript(js, _on_result)
        else:
            self._write_line('error', '(cannot eval JS in this tab type)')

    # ------------------------------------------------------------------
    # Wire tab-change signal after __init__ (called by MainWindow)
    # ------------------------------------------------------------------

    def wire_tab_change(self):
        self._tabs.currentChanged.connect(self._on_tab_changed)


# ---------------------------------------------------------------------------
# Module-level helpers
# ---------------------------------------------------------------------------

def _esc(text: str) -> str:
    """HTML-escape a string for safe embedding."""
    return html_mod.escape(str(text), quote=False)


def _camel_to_css(name: str) -> str:
    """Convert camelCase to kebab-case (e.g. fontSize -> font-size)."""
    import re
    return re.sub(r'([A-Z])', r'-\1', name).lower()


def _fmt_size(nbytes) -> str:
    """Format a byte count as a human-readable string."""
    if nbytes is None or nbytes == 0:
        return '--'
    if nbytes < 1024:
        return '%d B' % nbytes
    if nbytes < 1048576:
        return '%.1f KB' % (nbytes / 1024)
    return '%.1f MB' % (nbytes / 1048576)
