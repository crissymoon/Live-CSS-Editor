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
// TOOLBAR STATE
// ================================================================

var _state = {
  url: '', loading: false, progress: 0,
  canBack: false, canFwd: false,
  https: false, http: false,
  devtOpen: false, jsOn: true, isBm: false,
  tabs: [], activeTab: 0
};
var _pendingUrl  = null;
var _navStartUrl = null; // s.url recorded when navigation begins

function xcmSetState(s) {
  var prev = _state;
  _state = s;

  // Tab bar
  if (s.tabs) {
    // Clear per-navigation state when the active tab changes
    if (s.activeTab !== prev.activeTab) {
      _pendingUrl  = null;
      _navStartUrl = null;
    }
    updateTabBar(s.tabs, s.activeTab || 0);
  }

  // URL bar
  var urlEl   = document.getElementById('url');
  var focused = document.activeElement === urlEl;
  if (!focused) {
    var curUrl = s.url || '';
    if (_pendingUrl) {
      if (s.loading) {
        // Navigation just started -- record the URL we are leaving
        if (!_navStartUrl) _navStartUrl = curUrl;
      } else {
        // Navigation stopped.  If the URL actually changed, it succeeded --
        // clear _pendingUrl and show the real URL.  If it did NOT change,
        // navigation failed (no commit); keep _pendingUrl so the user can
        // see and correct what they typed instead of the bar snapping back
        // to the previous page's URL (e.g. the login page).
        if (curUrl !== _navStartUrl) _pendingUrl = null;
        _navStartUrl = null;
      }
    }
    urlEl.value = displayUrl(_pendingUrl || curUrl);
  }

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
  document.getElementById('sec').className = s.https ? 'sec-https' : (s.http ? 'sec-http' : 'sec-other');

  // drawer labels
  document.getElementById('di-reload-lbl').textContent = s.loading ? 'Stop' : 'Reload';
  document.getElementById('di-devt-lbl').textContent   = s.devtOpen ? 'Close Developer Tools' : 'Developer Tools';
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
function onUrlFocus() {
  var el = document.getElementById('url');
  // Restore the full URL (with scheme) so the user can edit or copy it
  // correctly.  The display had the scheme stripped while the bar was idle.
  el.value = _pendingUrl || _state.url || '';
  el.select();
  xcm('urlfocus');
}
function onUrlBlur()  { xcm('urlblur');  }
function toggleInfo() {
  document.getElementById('info-slide').classList.toggle('open');
}

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
  document.getElementById('drawer').classList.add('open');
  document.getElementById('more').classList.add('active');
  _moreShowUp(true);
  xcm('dropdownOpen', DRAWER_H);
}
function closeDrawer() {
  if (!DRAWER_OPEN) return;
  DRAWER_OPEN = false;
  document.getElementById('drawer').classList.remove('open');
  document.getElementById('more').classList.remove('active');
  _moreShowUp(false);
  bmDrawerBack();
  xcm('dropdownClose');
}
function closeDrawerSilent() {
  DRAWER_OPEN = false;
  document.getElementById('drawer').classList.remove('open');
  document.getElementById('more').classList.remove('active');
  _moreShowUp(false);
  bmDrawerBack();
}
function toggleDrawer() {
  if (DRAWER_OPEN) closeDrawer(); else openDrawer();
}
function drawerAction(action) {
  closeDrawer();
  setTimeout(function() { xcm(action); }, 40);
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
