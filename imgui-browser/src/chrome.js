var DEFAULT_FAVICON = 'data:image/png;base64,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';

/* chrome.js -- runtime logic for the XCM browser chrome
   Loaded by chrome.html which is hosted in a WKWebView child panel.
   ------------------------------------------------------------------ */

// ---- bridge ----
function xcm(action, data) {
  var msg = {action: action};
  if (data !== undefined) msg.data = String(data);
  try { window.webkit.messageHandlers.xcmBridge.postMessage(msg); }
  catch(e) { console.warn('xcmBridge not available', e); }
}

// ================================================================
// TAB BAR
// ================================================================

var _tabs      = [];
var _activeTab = 0;

// Tab drag state
var _drag = null;  // {idx, startX, currentX, moved}

function tabNew() { xcm('tab_new'); }

function tabSwitch(idx) {
  xcm('tab_switch', idx);
}

function tabClose(idx, evt) {
  evt.stopPropagation();
  xcm('tab_close', idx);
}

// Rebuild the tab DOM whenever the tabs array changes
var _lastTabsSig = '';

function updateTabBar(tabs, activeTab) {
  var sig = JSON.stringify(tabs) + '|' + activeTab;
  if (sig === _lastTabsSig) return;
  _lastTabsSig = sig;
  _tabs = tabs;
  _activeTab = activeTab;

  var row = document.getElementById('tab-row');
  var newtab = document.getElementById('newtab');

  // Remove existing tab chips (keep newtab)
  var chips = row.querySelectorAll('.tab');
  for (var i = 0; i < chips.length; i++) row.removeChild(chips[i]);

  // Insert tabs before newtab button
  for (var i = 0; i < tabs.length; i++) {
    var t = tabs[i];
    var chip = document.createElement('div');
    chip.className = 'tab' +
      (i === activeTab        ? ' active'  : '') +
      (t.loading              ? ' loading' : '');
    chip.setAttribute('data-idx', i);
    chip.title = t.title || 'New Tab';

    // loading dot
    var dot = document.createElement('span');
    dot.className = 'tab-dot';
    chip.appendChild(dot);

    // favicon -- shown when loaded, hidden when loading or on error
    var fav = document.createElement('img');
    fav.className    = 'tab-fav';
    fav.width        = 14;
    fav.height       = 14;
    // Use the page favicon when available, fall back to the default icon.
    fav.src = t.favicon || DEFAULT_FAVICON;
    fav.onerror = function() { this.src = DEFAULT_FAVICON; };
    chip.appendChild(fav);

    // title
    var title = document.createElement('span');
    title.className = 'tab-title';
    title.textContent = t.title || 'New Tab';
    chip.appendChild(title);

    // close button (only visible on hover/active via CSS)
    if (tabs.length > 1) {
      var cls = document.createElement('button');
      cls.className = 'tab-close';
      cls.title = 'Close tab';
      cls.innerHTML = '<svg viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>';
      (function(capturedIdx) {
        cls.onclick = function(e) { tabClose(capturedIdx, e); };
      })(i);
      chip.appendChild(cls);
    }

    // click to switch
    (function(capturedIdx) {
      chip.onclick = function() { tabSwitch(capturedIdx); };
    })(i);

    // drag to reorder
    chip.addEventListener('mousedown', onTabMouseDown);

    row.insertBefore(chip, newtab);
  }
}

// Tab drag-to-reorder
function onTabMouseDown(e) {
  if (e.button !== 0) return;
  var idx = parseInt(this.getAttribute('data-idx'), 10);
  _drag = {idx: idx, startX: e.clientX, currentX: e.clientX, moved: false};
  e.preventDefault();
}

document.addEventListener('mousemove', function(e) {
  if (!_drag) return;
  _drag.currentX = e.clientX;
  var dx = Math.abs(_drag.currentX - _drag.startX);
  if (dx > 6) _drag.moved = true;
});

document.addEventListener('mouseup', function(e) {
  if (!_drag) return;
  if (_drag.moved) {
    var row     = document.getElementById('tab-row');
    var chips   = row.querySelectorAll('.tab');
    var dropIdx = _drag.idx;
    for (var i = 0; i < chips.length; i++) {
      var r = chips[i].getBoundingClientRect();
      if (e.clientX >= r.left && e.clientX < r.right) { dropIdx = i; break; }
    }
    if (dropIdx !== _drag.idx) {
      xcm('tab_move', _drag.idx + ',' + dropIdx);
    }
  }
  _drag = null;
});

// ================================================================
// JS-DRIVEN HOVER (called by native mouse-moved monitor via
// evaluateJavaScript -- CSS :hover is unreliable when the panel is
// not the key window).
// ================================================================

// xcmMouseMove(x, y) -- called by the native mouse-moved monitor every
// time the cursor moves inside the toolbar panel. x/y are CSS pixels
// (top-left origin, matching getBoundingClientRect).
//
// Strategy: use document.elementFromPoint to find the exact element
// under the cursor, then walk up the DOM tree adding "js-hover" to
// that element and all its ancestors. Clear js-hover from everything
// else first so we never leave stale state on elements the cursor
// has moved away from. This covers tabs, toolbar buttons, traffic
// lights, the close X -- every interactive element automatically.
function xcmMouseMove(x, y) {
  // Clear all previous js-hover in one pass.
  xcmMouseLeave();

  // Find the element under the cursor (returns null if outside viewport).
  var el = document.elementFromPoint(x, y);
  if (!el) return;

  // Walk from the element up to <body> adding js-hover to each node
  // so parent-child CSS selectors like .tab.js-hover .tab-close work.
  var node = el;
  while (node && node !== document.documentElement) {
    if (node.classList) node.classList.add('js-hover');
    node = node.parentElement;
  }
}

// xcmMouseLeave() -- called when cursor leaves the panel or mouse-up.
// Removes js-hover from every element that has it.
function xcmMouseLeave() {
  var all = document.querySelectorAll('.js-hover');
  for (var i = 0; i < all.length; i++) all[i].classList.remove('js-hover');
}

// ================================================================
// TOOLBAR STATE
// ================================================================

var _state = {
  url: '', loading: false, progress: 0,
  canBack: false, canFwd: false,
  https: false, http: false,
  devtOpen: false, jsOn: true, isBm: false,
  tabs: [], activeTab: 0,
  phpPort: 9879
};
var _pendingUrl  = null;
var _navStartUrl = null; // s.url recorded when navigation begins

// Compact signature of the last state pushed to the DOM.
// xcmSetState is called every render frame from native; we skip all DOM
// writes when nothing has actually changed to prevent the constant style
// recalculations from disrupting WKWebView text-input rendering.
var _lastStateSig = '';

function xcmSetState(s) {
  var prev = _state;
  _state = s;

  // ---- Tab bar (has its own sig guard inside updateTabBar) ----
  if (s.tabs) {
    if (s.activeTab !== prev.activeTab) {
      _pendingUrl  = null;
      _navStartUrl = null;
    }
    updateTabBar(s.tabs, s.activeTab || 0);
  }

  // ---- URL bar (always evaluated: depends on focus + local _pendingUrl) ----
  var urlEl   = document.getElementById('url');
  var focused = document.activeElement === urlEl;
  if (!focused) {
    var curUrl = s.url || '';
    if (_pendingUrl) {
      if (s.loading) {
        if (!_navStartUrl) _navStartUrl = curUrl;
      } else {
        // Navigation stopped.  Clear pending only if the URL actually changed
        // (i.e. the navigation committed).  If it didn't change, keep the
        // pending URL so the user can see and fix what they typed.
        if (curUrl !== _navStartUrl) _pendingUrl = null;
        _navStartUrl = null;
      }
    }
    urlEl.value = displayUrl(_pendingUrl || curUrl);
  }

  // ---- Everything else: skip if state hasn't changed ----
  var progVal   = (s.loading && s.progress > 0 && s.progress < 1)
                  ? s.progress.toFixed(2) : '0';
  var secClass  = s.https ? 'sec-https' : (s.http ? 'sec-http' : 'sec-other');
  var sig = (s.url||'') + '|' + s.loading + '|' + progVal +
            '|' + s.canBack + '|' + s.canFwd +
            '|' + secClass +
            '|' + s.devtOpen + '|' + s.jsOn + '|' + s.isBm +
            '|' + (s.phpOk||false) + '|' + (s.nodeOk||false) +
            '|' + (s.statusTxt||'') +
            '|' + (s.vpW||0) + 'x' + (s.vpH||0);
  if (sig === _lastStateSig) return;
  _lastStateSig = sig;

  // back / fwd
  document.getElementById('btn-back').disabled = !s.canBack;
  document.getElementById('btn-fwd').disabled  = !s.canFwd;

  // reload / stop
  document.getElementById('ico-reload').style.display = s.loading ? 'none' : '';
  document.getElementById('ico-stop').style.display   = s.loading ? ''     : 'none';
  document.getElementById('btn-reload').title = s.loading ? 'Stop' : 'Reload';

  // progress bar
  var prog = document.getElementById('prog');
  if (s.loading && s.progress > 0 && s.progress < 1) {
    prog.style.display = 'block';
    prog.style.width   = (s.progress * 100).toFixed(1) + '%';
  } else {
    prog.style.display = 'none';
    prog.style.width   = '0%';
  }

  // security icon
  document.getElementById('ico-globe').style.display  = (!s.https && !s.http) ? '' : 'none';
  document.getElementById('ico-lock').style.display   = s.https ? '' : 'none';
  document.getElementById('ico-unlock').style.display = (s.http && !s.https) ? '' : 'none';
  document.getElementById('sec').className = secClass;

  // drawer labels
  document.getElementById('di-reload-lbl').textContent = s.loading ? 'Stop' : 'Reload';
  document.getElementById('di-devt-toggle-lbl').textContent = s.devtOpen ? 'Close DevTools Panel' : 'Toggle DevTools Panel';
  document.getElementById('di-js-lbl').textContent     = s.jsOn ? 'Disable JavaScript' : 'Enable JavaScript';
  bmUpdateLabel();

  document.getElementById('di-back').className = 'ditem' + (s.canBack ? '' : ' disabled');
  document.getElementById('di-fwd').className  = 'ditem' + (s.canFwd  ? '' : ' disabled');

  document.getElementById('more').style.color =
    (s.devtOpen || s.isBm || !s.jsOn) ? 'var(--accent)' : '';

  // Info slide: server status dots, status text, viewport
  if (s.phpOk !== undefined) {
    var pd = document.getElementById('info-php-dot');
    if (pd) pd.className = 'info-dot ' + (s.phpOk ? 'ok' : 'bad');
  }
  if (s.nodeOk !== undefined) {
    var nd = document.getElementById('info-js-dot');
    if (nd) nd.className = 'info-dot ' + (s.nodeOk ? 'ok' : 'bad');
  }
  if (s.statusTxt !== undefined) {
    var stEl  = document.getElementById('info-status');
    var stSep = document.getElementById('info-sep-st');
    if (stEl) {
      stEl.textContent = s.statusTxt;
      var show = !!s.statusTxt;
      stEl.style.display  = show ? '' : 'none';
      if (stSep) stSep.style.display = show ? '' : 'none';
    }
  }
  if (s.vpW !== undefined) {
    var vp = document.getElementById('info-vp');
    if (vp) vp.textContent = s.vpW + ' \u00D7 ' + s.vpH;
  }

  if (document.getElementById('di-ico-reload')) {
    document.getElementById('di-ico-reload').style.display = s.loading ? 'none' : '';
    document.getElementById('di-ico-stop').style.display   = s.loading ? ''     : 'none';
  }
}

// ================================================================
// URL INPUT
// ================================================================

// Strip the scheme (and leading www.) for display so the bar looks clean
// when the user is not actively editing.  The full URL is restored on focus.
function displayUrl(url) {
  if (!url) return '';
  return url.replace(/^https?:\/\/(www\.)?/, '');
}

function resolveUrl(raw) {
  var v = raw.trim();
  if (!v) return null;
  // Already has a scheme -- use as-is
  if (v.indexOf('://') !== -1) return v;
  // Single-word shortcuts: localhost or localhost:port
  if (/^localhost(:\d+)?(\/.*)?$/.test(v)) return 'http://' + v;
  // Looks like a hostname or IP: no spaces, has a dot, does not look like a
  // natural language phrase (no multiple words separated by spaces)
  var noSpaces = v.indexOf(' ') === -1;
  var hasDot   = v.indexOf('.') !== -1;
  // A plain IP address  (e.g. 192.168.1.1)
  var isIp = /^\d{1,3}(\.\d{1,3}){3}(:\d+)?(\/.*)?$/.test(v);
  // A hostname-like token: has a dot, no spaces, and the part after the last
  // dot is a short alphanumeric TLD (not a sentence ending with a period)
  var isTld = noSpaces && hasDot && /\.[a-zA-Z]{2,}(\/.*)?$/.test(v);
  if (isIp || isTld) return 'https://' + v;
  // Everything else is a search query
  return 'https://www.google.com/search?q=' + encodeURIComponent(v);
}

function onUrlKey(e) {
  if (e.key === 'Enter') {
    var url = resolveUrl(e.target.value);
    if (url) {
      _pendingUrl = url;
      xcm('navigate', url);
      e.target.blur();
    }
  }
  if (e.key === 'Escape') {
    _pendingUrl  = null;
    _navStartUrl = null;
    e.target.value = displayUrl(_state.url || '');
    e.target.blur();
  }
}
var _urlFocused = false;

function onUrlFocus() {
  _urlFocused = true;
  var el = document.getElementById('url');
  el.value = _pendingUrl || _state.url || '';
  el.select();
  xcm('urlfocus');
}
function onUrlBlur() {
  _urlFocused = false;
  xcm('urlblur');
}
function toggleInfo() {
  document.getElementById('info-slide').classList.toggle('open');
}

// ================================================================
// KEYBOARD SHORTCUTS (when chrome.html panel has focus)
// ================================================================
// Intercepts Cmd+C/X/V/A and routes them appropriately.
// C/X/A in text inputs are driven via execCommand/select() because WKWebView
// does not reliably relay those key-equivalents to HTML inputs.
// V when the URL bar is focused routes to paste_url so the system clipboard
// text is read via NSPasteboard (bypasses WKWebView's internal cache).
document.addEventListener('keydown', function (e) {
  if (!e.metaKey) return;
  // Let the browser handle Cmd+Z/Shift+Z (undo/redo) natively.
  if (e.key === 'z' || e.key === 'Z') return;
  var tag = (document.activeElement ? document.activeElement.tagName.toLowerCase() : '');
  var isInput = (tag === 'input' || tag === 'textarea');
  switch (e.key) {
    case 'c':
      if (isInput) {
        // WKWebView does not reliably relay Cmd+C to HTML inputs; drive it
        // explicitly so the selected text reaches the system clipboard.
        document.execCommand('copy');
        e.preventDefault();
      } else {
        xcm('copy'); e.preventDefault();
      }
      break;
    case 'x':
      if (isInput) {
        document.execCommand('cut');
        e.preventDefault();
      } else {
        xcm('cut'); e.preventDefault();
      }
      break;
    case 'v':
      if (_urlFocused) {
        // Route through paste_url so we read NSPasteboard directly (bypasses
        // WKWebView's internal clipboard cache).
        xcm('paste_url'); e.preventDefault();
      } else if (!isInput) {
        xcm('paste'); e.preventDefault();
      }
      break;
    case 'a':
      if (isInput) {
        document.activeElement.select();
        e.preventDefault();
      } else {
        xcm('select_all'); e.preventDefault();
      }
      break;
  }
});

// ================================================================
// DRAWER
// ================================================================

var DRAWER_OPEN = false;
var DRAWER_H    = 220;

function _moreShowUp(on) {
  document.getElementById('more-dots').style.display = on ? 'none' : '';
  document.getElementById('more-up').style.display   = on ? ''     : 'none';
}
function openDrawer() {
  if (DRAWER_OPEN) return;
  DRAWER_OPEN = true;
  var el = document.getElementById('drawer');
  el.classList.add('open');
  document.getElementById('more').classList.add('active');
  _moreShowUp(true);
  // Measure real content height after the drawer is visible; add 2px for the
  // top border so the native panel frame exactly contains all rows.
  var h = el.scrollHeight + 2;
  if (h < 80) h = 80;
  DRAWER_H = h;
  xcm('dropdownOpen', h);
}
function closeDrawer() {
  if (!DRAWER_OPEN) return;
  DRAWER_OPEN = false;
  document.getElementById('drawer').classList.remove('open');
  document.getElementById('more').classList.remove('active');
  _moreShowUp(false);
  bmDrawerBack();
  // Collapse the devtools submenu so it is closed next time drawer opens.
  _devtMenuOpen = false;
  var sub  = document.getElementById('devt-submenu');
  var chev = document.getElementById('devt-chev');
  if (sub)  sub.classList.remove('open');
  if (chev) chev.classList.remove('open');
  xcm('dropdownClose');
}
function closeDrawerSilent() {
  DRAWER_OPEN = false;
  document.getElementById('drawer').classList.remove('open');
  document.getElementById('more').classList.remove('active');
  _moreShowUp(false);
  bmDrawerBack();
  _devtMenuOpen = false;
  var sub  = document.getElementById('devt-submenu');
  var chev = document.getElementById('devt-chev');
  if (sub)  sub.classList.remove('open');
  if (chev) chev.classList.remove('open');
}
function toggleDrawer() {
  if (DRAWER_OPEN) closeDrawer(); else openDrawer();
}
function drawerAction(action) {
  closeDrawer();
  setTimeout(function() { xcm(action); }, 40);
}
function openApp(slug) {
  var url = 'http://127.0.0.1:' + _state.phpPort + '/' + slug + '/';
  closeDrawer();
  setTimeout(function() { xcm('open_url', url); }, 40);
}

var _devtMenuOpen = false;
function toggleDevtMenu() {
  _devtMenuOpen = !_devtMenuOpen;
  var sub  = document.getElementById('devt-submenu');
  var chev = document.getElementById('devt-chev');
  if (_devtMenuOpen) {
    sub.classList.add('open');
    chev.classList.add('open');
  } else {
    sub.classList.remove('open');
    chev.classList.remove('open');
  }
  // Recompute drawer height so the native panel resizes.
  var el = document.getElementById('drawer');
  var h  = el.scrollHeight + 2;
  if (h < 80) h = 80;
  DRAWER_H = h;
  xcm('dropdownOpen', h);
}

// ================================================================
// DEBUG CONSOLE
// ================================================================

var _consoleLogs = [];
var _consoleOpen = false;

(function() {
  var _wrap = function(level, orig) {
    return function() {
      orig.apply(console, arguments);
      if (!_consoleOpen) return;
      var parts = [];
      for (var i = 0; i < arguments.length; i++) {
        try { parts.push(typeof arguments[i] === 'object'
          ? JSON.stringify(arguments[i]) : String(arguments[i])); }
        catch(e) { parts.push(String(arguments[i])); }
      }
      consoleAppend(level, parts.join(' '));
    };
  };
  console.log   = _wrap('log',   console.log);
  console.info  = _wrap('info',  console.info);
  console.warn  = _wrap('warn',  console.warn);
  console.error = _wrap('err',   console.error);
  window.addEventListener('error', function(e) {
    if (_consoleOpen) consoleAppend('err', (e.message||'error') + (e.filename ? ' @ ' + e.filename + ':' + e.lineno : ''));
  });
})();

function consoleAppend(level, text) {
  var el = document.getElementById('console-log');
  if (!el) return;
  var empty = el.querySelector('.cl-empty');
  if (empty) el.removeChild(empty);
  var entry = document.createElement('div');
  entry.className = 'cl-entry cl-' + level;
  var badge = document.createElement('span');
  badge.className = 'cl-badge';
  badge.textContent = level.toUpperCase();
  entry.appendChild(badge);
  entry.appendChild(document.createTextNode(text));
  el.appendChild(entry);
  el.scrollTop = el.scrollHeight;
}

function consoleClear() {
  var el = document.getElementById('console-log');
  if (el) el.innerHTML = '<div class="cl-empty">Console cleared</div>';
}

function consoleRun() {
  var inp = document.getElementById('console-input');
  var code = inp ? inp.value.trim() : '';
  if (!code) return;
  consoleAppend('eval', '> ' + code);
  if (inp) inp.value = '';
  try {
    // eslint-disable-next-line no-eval
    var result = eval(code); // jshint ignore:line
    consoleAppend('res', String(result !== undefined ? result : '(undefined)'));
  } catch(e) {
    consoleAppend('err', e.toString());
  }
}

function consoleInputKey(e) {
  if (e.key === 'Enter') consoleRun();
}

function openConsole() {
  _consoleOpen = true;
  document.getElementById('drawer-grid').style.display    = 'none';
  document.getElementById('bm-add-panel').classList.remove('active');
  document.getElementById('bm-list-panel').classList.remove('active');
  document.getElementById('console-panel').classList.add('active');
  var el = document.getElementById('console-log');
  if (el && !el.children.length)
    el.innerHTML = '<div class="cl-empty">No messages yet</div>';
}

function closeConsolePanel() {
  _consoleOpen = false;
  document.getElementById('console-panel').classList.remove('active');
  document.getElementById('drawer-grid').style.display = '';
}

// ================================================================
// BOOKMARKS (localStorage)
// ================================================================

var BM_KEY = 'xcm_bookmarks';

function bmLoad() {
  try { return JSON.parse(localStorage.getItem(BM_KEY) || '[]'); }
  catch(e) { return []; }
}
function bmSave(arr) {
  localStorage.setItem(BM_KEY, JSON.stringify(arr));
}
function bmHasCurrent() {
  var url = _state.url || '';
  if (!url) return false;
  var list = bmLoad();
  for (var i = 0; i < list.length; i++) if (list[i].url === url) return true;
  return false;
}
function bmUpdateLabel() {
  var el = document.getElementById('di-bm-lbl');
  if (el) el.textContent = bmHasCurrent() ? 'Remove Bookmark' : 'Bookmark This Page';
}
function openBmAdd() {
  if (bmHasCurrent()) {
    // Remove instead of add
    var url = _state.url || '';
    var list = bmLoad().filter(function(b){ return b.url !== url; });
    bmSave(list);
    bmUpdateLabel();
    closeDrawer();
    return;
  }
  // Pre-fill fields
  var title = (_state.tabs && _state.tabs[_state.activeTab])
    ? (_state.tabs[_state.activeTab].title || '') : '';
  var url = _state.url || '';
  document.getElementById('bm-name-input').value = title;
  document.getElementById('bm-url-input').value  = url;
  document.getElementById('drawer-grid').style.display      = 'none';
  document.getElementById('bm-list-panel').classList.remove('active');
  document.getElementById('bm-add-panel').classList.add('active');
}
function openBmList() {
  bmRenderList();
  document.getElementById('drawer-grid').style.display      = 'none';
  document.getElementById('bm-add-panel').classList.remove('active');
  document.getElementById('bm-list-panel').classList.add('active');
}
function bmDrawerBack() {
  document.getElementById('drawer-grid').style.display      = '';
  document.getElementById('bm-add-panel').classList.remove('active');
  document.getElementById('bm-list-panel').classList.remove('active');
  document.getElementById('console-panel').classList.remove('active');
  _consoleOpen = false;
}
function bmSaveAction() {
  var name = document.getElementById('bm-name-input').value.trim();
  var url  = document.getElementById('bm-url-input').value.trim();
  if (!url) return;
  if (!name) name = url;
  var list = bmLoad();
  // Replace if URL already exists
  var found = false;
  for (var i = 0; i < list.length; i++) {
    if (list[i].url === url) { list[i].name = name; found = true; break; }
  }
  if (!found) list.push({name: name, url: url});
  bmSave(list);
  bmUpdateLabel();
  bmDrawerBack();
}
function bmDelete(idx, evt) {
  evt.stopPropagation();
  var list = bmLoad();
  list.splice(idx, 1);
  bmSave(list);
  bmUpdateLabel();
  bmRenderList();
}
function bmRenderList() {
  var list = bmLoad();
  var el = document.getElementById('bm-list-scroll');
  if (!el) return;
  if (list.length === 0) {
    el.innerHTML = '<div class="bm-empty">No bookmarks saved</div>';
    return;
  }
  var html = '';
  for (var i = 0; i < list.length; i++) {
    var b = list[i];
    var safeName = b.name.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;');
    var safeUrl  = b.url.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/"/g,'&quot;');
    html += '<div class="bm-row" onclick="bmNavigate('+i+')">' +
      '<div class="bm-row-text">' +
        '<div class="bm-row-name">' + safeName + '</div>' +
        '<div class="bm-row-url">' + safeUrl + '</div>' +
      '</div>' +
      '<button class="bm-del" title="Remove" onclick="bmDelete('+i+',event)">' +
        '<svg viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>' +
      '</button>' +
    '</div>';
  }
  el.innerHTML = html;
}
function bmNavigate(idx) {
  var list = bmLoad();
  if (!list[idx]) return;
  var url = list[idx].url;
  _pendingUrl = url;
  xcm('navigate', url);
  closeDrawer();
}

// ================================================================
// RIGHT-CLICK CONTEXT MENU
// ================================================================
var _ctxMenu = null;
var _ctxOpen = false;

function _ctxEl() {
  return _ctxMenu || (_ctxMenu = document.getElementById('ctx-menu'));
}

function ctxShow(x, y) {
  var el = _ctxEl();
  if (!el) return;
  // Horizontal pill: only set left so it stays on the toolbar row (top is fixed in CSS).
  // Flip left if the menu would overflow the right edge.
  var vw = window.innerWidth;
  var menuW = el.offsetWidth || 180;
  var left = (x + 4 + menuW > vw) ? Math.max(0, x - menuW - 4) : (x + 4);
  el.style.left = left + 'px';
  el.classList.add('ctx-open');
  _ctxOpen = true;
}

function ctxHide() {
  var el = _ctxEl();
  if (!el) return;
  el.classList.remove('ctx-open');
  _ctxOpen = false;
  // Remove js-hover from all ctx items
  el.querySelectorAll('.js-hover').forEach(function(e){ e.classList.remove('js-hover'); });
}

function ctxAction(cmd) {
  ctxHide();
  var isInput = document.activeElement &&
    (document.activeElement.tagName === 'INPUT' ||
     document.activeElement.tagName === 'TEXTAREA');
  if (cmd === 'cut') {
    if (isInput) { document.execCommand('cut'); }
    else         { xcm('cut'); }
  } else if (cmd === 'copy') {
    if (isInput) { document.execCommand('copy'); }
    else         { xcm('copy'); }
  } else if (cmd === 'paste') {
    if (_urlFocused) { xcm('paste_url'); }
    else if (isInput){ document.execCommand('paste'); }
    else             { xcm('paste'); }
  }
}

document.addEventListener('contextmenu', function(e) {
  e.preventDefault();
  ctxShow(e.clientX, e.clientY);
});

// Dismiss on mousedown outside the menu
document.addEventListener('mousedown', function(e) {
  if (!_ctxOpen) return;
  var el = _ctxEl();
  if (el && !el.contains(e.target)) ctxHide();
});

// Dismiss on scroll or resize
document.addEventListener('scroll', ctxHide, true);
window.addEventListener('resize', ctxHide);

