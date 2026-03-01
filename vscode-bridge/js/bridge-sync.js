/**
 * vscode-bridge/js/bridge-sync.js
 * Two-way sync between Crissy's Style Tool and the VSCode Copilot MCP bridge.
 *
 * What this does when ACTIVE (on):
 *   - Every PUSH_INTERVAL_MS: reads the current editor state and POSTs it to
 *     bridge.php so Copilot (via read_active_session) can see what you are working on.
 *
 *   - Every POLL_INTERVAL_MS: asks bridge.php if Copilot has written any stylesheet
 *     changes. If yes, fetches the new file content from disk, loads it directly into
 *     the CodeMirror editor via LiveCSS.editor.setCssValue(), and also reloads the
 *     <link> tag so the live preview updates - all without a page reload.
 *
 * Toggle:
 *   - ON/OFF state is persisted in localStorage so it survives page reloads.
 *   - Wired to #vscodeBridgeToggle in the app header (added by index.php).
 *   - Public API: BridgeSync.enable() / .disable() / .toggle() / .isActive()
 *
 * All branches emit console.log / console.warn / console.error so every state
 * change is visible in DevTools without opening the source.
 */

(function (global) {
    'use strict';

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------
    var BRIDGE_URL       = '/vscode-bridge/api/bridge.php';
    var SHEETS_BASE      = '/style-sheets/';
    var PUSH_INTERVAL_MS = 5000;
    var POLL_INTERVAL_MS = 3000;
    var REFRESH_INTERVAL_MS = 2000;
    var STORAGE_KEY      = 'bridgeSync_enabled';
    var TOGGLE_BTN_ID    = 'vscodeBridgeToggle';

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    var _enabled     = false;
    var _pushTimer   = null;
    var _pollTimer   = null;
    var _refreshTimer = null;
    var _pushBounce  = null;
    var _initialized = false;

    // -------------------------------------------------------------------------
    // Logging - always emit to console so nothing is invisible
    // -------------------------------------------------------------------------
    function log(msg)  { console.log('[BridgeSync] '   + msg); }
    function warn(msg) { console.warn('[BridgeSync] '  + msg); }
    function err(msg)  { console.error('[BridgeSync] ' + msg); }

    // -------------------------------------------------------------------------
    // Read current state from the app
    // -------------------------------------------------------------------------
    function getEditorCSS() {
        try {
            // Primary: LiveCSS.editor.getCssEditor() -> CodeMirror instance
            if (global.LiveCSS && global.LiveCSS.editor &&
                typeof global.LiveCSS.editor.getCssEditor === 'function') {
                var cm = global.LiveCSS.editor.getCssEditor();
                if (cm && typeof cm.getValue === 'function') return cm.getValue();
            }
            // Fallback: bare CodeMirror global
            if (global.editor && typeof global.editor.getValue === 'function') {
                return global.editor.getValue();
            }
            warn('getEditorCSS: could not find CodeMirror instance');
            return '';
        } catch (e) {
            err('getEditorCSS exception: ' + e.message);
            return '';
        }
    }

    function getActiveSheet() {
        try {
            if (global.LiveCSS && global.LiveCSS.activeSheet) return global.LiveCSS.activeSheet;
            var link = document.querySelector('link[href*="style-sheets"]');
            if (link) {
                var m = (link.getAttribute('href') || '').match(/([^/]+\.css)/);
                if (m) return m[1];
            }
            var sel = document.querySelector('#theme-select, #stylesheet-select, select[data-theme]');
            if (sel && sel.value) return sel.value;
            return '';
        } catch (e) {
            err('getActiveSheet exception: ' + e.message);
            return '';
        }
    }

    function getHTMLPreview() {
        try {
            var frame = document.getElementById('previewFrame');
            if (frame && frame.contentDocument && frame.contentDocument.body) {
                return frame.contentDocument.body.innerHTML || '';
            }
        } catch (e) { /* cross-origin - silently skip */ }
        return '';
    }

    function getSavedProjects() {
        try {
            var raw = localStorage.getItem('liveCssEditor_projects');
            return raw ? JSON.parse(raw) : {};
        } catch (e) {
            err('getSavedProjects exception: ' + e.message);
            return {};
        }
    }

    // -------------------------------------------------------------------------
    // Apply incoming CSS from Copilot directly into the editor and preview
    // -------------------------------------------------------------------------
    function applyIncomingCSS(filename, cssText) {
        var editorUpdated = false;

        // 1. Push into the CodeMirror editor - this is what the user sees and edits
        try {
            if (global.LiveCSS && global.LiveCSS.editor &&
                typeof global.LiveCSS.editor.setCssValue === 'function') {
                global.LiveCSS.editor.setCssValue(cssText);
                log('applyIncomingCSS: wrote ' + cssText.length + ' chars into CodeMirror for "' + filename + '"');
                editorUpdated = true;
            } else {
                warn('applyIncomingCSS: LiveCSS.editor.setCssValue not ready - editor may still be initializing');
            }
        } catch (e) {
            err('applyIncomingCSS: setCssValue threw: ' + e.message);
        }

        // 2. Bump the <link> tag so the live preview iframe refreshes the file from disk
        try {
            var links    = document.querySelectorAll('link[rel="stylesheet"]');
            var reloaded = false;
            links.forEach(function (link) {
                var href = link.getAttribute('href') || '';
                if (href.indexOf(filename) !== -1) {
                    var base = href.split('?')[0];
                    link.setAttribute('href', base + '?v=' + Date.now());
                    log('applyIncomingCSS: bumped <link> for ' + base);
                    reloaded = true;
                }
            });
            if (!reloaded) {
                warn('applyIncomingCSS: no <link> matched "' + filename + '" - preview link not updated');
            }
        } catch (e) {
            err('applyIncomingCSS: <link> bump threw: ' + e.message);
        }

        // 3. Broadcast event so outline, color tools, etc. can react
        try {
            document.dispatchEvent(new CustomEvent('bridgeStylesheetUpdated', {
                detail: { file: filename, by: 'vscode-copilot', css: cssText }
            }));
        } catch (e) {
            err('applyIncomingCSS: CustomEvent dispatch threw: ' + e.message);
        }

        if (!editorUpdated) {
            warn('applyIncomingCSS: editor was not updated (not ready?). Changes are on disk - reload the page if the editor looks stale.');
        }
    }

    // -------------------------------------------------------------------------
    // Fetch the updated file from disk then apply it
    // -------------------------------------------------------------------------
    function fetchAndApplySheet(filename, timestamp) {
        var url = SHEETS_BASE + encodeURIComponent(filename) + '?v=' + (timestamp || Date.now());
        console.warn('[BridgeSync] Copilot updated "' + filename + '" - fetching ' + url);

        fetch(url, { cache: 'no-store' })
            .then(function (res) {
                if (!res.ok) {
                    err('fetchAndApplySheet: HTTP ' + res.status + ' fetching ' + url);
                    return null;
                }
                return res.text();
            })
            .then(function (cssText) {
                if (cssText === null) {
                    err('fetchAndApplySheet: null body for "' + filename + '"');
                    return;
                }
                applyIncomingCSS(filename, cssText);
            })
            .catch(function (e) {
                err('fetchAndApplySheet network error for "' + filename + '": ' + e.message);
            });
    }

    // -------------------------------------------------------------------------
    // Push current session state to the bridge
    // -------------------------------------------------------------------------
    function pushSession() {
        if (!_enabled) return;

        var payload = {
            css:         getEditorCSS(),
            html:        getHTMLPreview(),
            activeSheet: getActiveSheet(),
            projects:    getSavedProjects(),
        };

        fetch(BRIDGE_URL + '?action=push_session', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(payload),
        })
        .then(function (res) {
            if (!res.ok) {
                return res.text().then(function (t) {
                    err('push_session HTTP ' + res.status + ': ' + t);
                });
            }
            return res.json().then(function (data) {
                if (data && data.syncedAt) {
                    log('Session pushed ' + data.syncedAt + ' | ' + (payload.activeSheet || 'no sheet') + ' | ' + payload.css.length + ' chars');
                } else if (data && !data.success) {
                    warn('push_session: ' + (data.error || JSON.stringify(data)));
                }
            });
        })
        .catch(function (e) {
            err('push_session network error: ' + e.message);
        });
    }

    // -------------------------------------------------------------------------
    // Poll bridge for changes made by Copilot
    // -------------------------------------------------------------------------
    function pollChanges() {
        if (!_enabled) return;

        fetch(BRIDGE_URL + '?action=poll_changes', { method: 'GET', cache: 'no-store' })
        .then(function (res) {
            if (!res.ok) {
                return res.text().then(function (t) {
                    err('poll_changes HTTP ' + res.status + ': ' + t);
                });
            }
            return res.json().then(function (data) {
                if (!data) { err('poll_changes: empty response'); return; }
                if (!data.success) { warn('poll_changes: ' + (data.error || 'unknown')); return; }
                if (data.hasChanges && data.file) {
                    // Acknowledge first so a second fast poll does not re-fire
                    acknowledgeChange();
                    fetchAndApplySheet(data.file, data.updatedAt);
                }
            });
        })
        .catch(function (e) {
            err('poll_changes network error: ' + e.message);
        });
    }

    // -------------------------------------------------------------------------
    // Poll bridge for a refresh signal sent by Copilot / MCP
    // -------------------------------------------------------------------------
    function pollRefresh() {
        if (!_enabled) return;

        fetch(BRIDGE_URL + '?action=poll_refresh', { method: 'GET', cache: 'no-store' })
        .then(function (res) {
            if (!res.ok) {
                return res.text().then(function (t) {
                    err('poll_refresh HTTP ' + res.status + ': ' + t);
                });
            }
            return res.json().then(function (data) {
                if (!data) { err('poll_refresh: empty response'); return; }
                if (!data.success) { warn('poll_refresh: ' + (data.error || 'unknown')); return; }
                if (data.shouldRefresh) {
                    console.warn('[BridgeSync] Refresh requested by Copilot (at ' + (data.requestedAt || '?') + ') - reloading page');
                    try {
                        if (window.LiveCSS && window.LiveCSS.nativeBridge &&
                            typeof window.LiveCSS.nativeBridge.refreshApp === 'function') {
                            window.LiveCSS.nativeBridge.refreshApp();
                        } else {
                            window.location.reload();
                        }
                    } catch (e) {
                        err('pollRefresh: reload threw: ' + e.message + ' - falling back to location.reload()');
                        window.location.reload();
                    }
                }
            });
        })
        .catch(function (e) {
            err('poll_refresh network error: ' + e.message);
        });
    }

    // -------------------------------------------------------------------------
    // Acknowledge so the next poll does not re-trigger the same change
    // -------------------------------------------------------------------------
    function acknowledgeChange() {
        fetch(BRIDGE_URL + '?action=ack_changes', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({ acked: true }),
        })
        .then(function (res) { return res.json(); })
        .then(function (data) {
            if (data && data.success) {
                log('Change acknowledged');
            } else {
                warn('ack_changes unexpected response: ' + JSON.stringify(data));
            }
        })
        .catch(function (e) {
            err('acknowledgeChange network error: ' + e.message);
        });
    }

    // -------------------------------------------------------------------------
    // Toggle button DOM wiring
    // -------------------------------------------------------------------------
    function getToggleBtn() {
        return document.getElementById(TOGGLE_BTN_ID);
    }

    function updateToggleUI() {
        var btn = getToggleBtn();
        if (!btn) {
            warn('updateToggleUI: #' + TOGGLE_BTN_ID + ' not found in DOM');
            return;
        }
        if (_enabled) {
            btn.classList.add('bridge-active');
            btn.setAttribute('aria-pressed', 'true');
            btn.title = 'VSCode Copilot Bridge: ON - click to disable';
        } else {
            btn.classList.remove('bridge-active');
            btn.setAttribute('aria-pressed', 'false');
            btn.title = 'VSCode Copilot Bridge: OFF - click to enable';
        }
    }

    function wireToggleButton() {
        var btn = getToggleBtn();
        if (!btn) {
            warn('wireToggleButton: #' + TOGGLE_BTN_ID + ' not in DOM yet - retrying in 500ms');
            setTimeout(wireToggleButton, 500);
            return;
        }
        btn.addEventListener('click', function () { BridgeSync.toggle(); });
        updateToggleUI();
        log('Toggle button wired to #' + TOGGLE_BTN_ID);
    }

    // -------------------------------------------------------------------------
    // Debounced push on editor keyup
    // -------------------------------------------------------------------------
    function onEditorInput() {
        if (!_enabled) return;
        clearTimeout(_pushBounce);
        _pushBounce = setTimeout(pushSession, 1500);
    }

    // -------------------------------------------------------------------------
    // Start / stop sync intervals
    // -------------------------------------------------------------------------
    function startIntervals() {
        if (_pushTimer || _pollTimer) { warn('startIntervals: already running'); return; }
        pushSession(); // immediate push so Copilot has current state right away
        _pushTimer    = setInterval(pushSession,   PUSH_INTERVAL_MS);
        _pollTimer    = setInterval(pollChanges,   POLL_INTERVAL_MS);
        _refreshTimer = setInterval(pollRefresh,   REFRESH_INTERVAL_MS);
        log('Intervals started (push=' + PUSH_INTERVAL_MS + 'ms, poll=' + POLL_INTERVAL_MS + 'ms, refresh=' + REFRESH_INTERVAL_MS + 'ms)');
    }

    function stopIntervals() {
        clearInterval(_pushTimer);
        clearInterval(_pollTimer);
        clearInterval(_refreshTimer);
        clearTimeout(_pushBounce);
        _pushTimer = _pollTimer = _refreshTimer = _pushBounce = null;
        log('Intervals stopped');
    }

    // -------------------------------------------------------------------------
    // Persisted state helpers
    // -------------------------------------------------------------------------
    function loadPersistedState() {
        try { return localStorage.getItem(STORAGE_KEY) === 'true'; }
        catch (e) { err('loadPersistedState: ' + e.message); return false; }
    }

    function persistState(val) {
        try { localStorage.setItem(STORAGE_KEY, val ? 'true' : 'false'); }
        catch (e) { err('persistState: ' + e.message); }
    }

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------
    var BridgeSync = {

        enable: function () {
            if (_enabled) { log('enable: already on'); return; }
            _enabled = true;
            persistState(true);
            updateToggleUI();
            startIntervals();
            console.warn('[BridgeSync] ENABLED - Copilot can now read and edit your stylesheets from VSCode');
        },

        disable: function () {
            if (!_enabled) { log('disable: already off'); return; }
            _enabled = false;
            persistState(false);
            stopIntervals();
            updateToggleUI();
            console.warn('[BridgeSync] DISABLED - sync paused');
        },

        toggle: function () {
            if (_enabled) { BridgeSync.disable(); } else { BridgeSync.enable(); }
        },

        isActive: function () { return _enabled; },

        // Force a one-off push (useful after saving a project)
        push: function () {
            if (!_enabled) { warn('push: bridge is off - call BridgeSync.enable() first'); return; }
            pushSession();
        },

        // Manually trigger a change poll
        poll: function () { pollChanges(); },

        configure: function (opts) {
            if (!opts) return;
            if (opts.pushInterval) PUSH_INTERVAL_MS = opts.pushInterval;
            if (opts.pollInterval) POLL_INTERVAL_MS = opts.pollInterval;
            if (opts.bridgeUrl)    BRIDGE_URL        = opts.bridgeUrl;
            if (opts.sheetsBase)   SHEETS_BASE       = opts.sheetsBase;
            log('configure: ' + JSON.stringify(opts));
        },
    };

    // -------------------------------------------------------------------------
    // Init
    // -------------------------------------------------------------------------
    function init() {
        if (_initialized) { warn('init: already ran'); return; }
        _initialized = true;

        log('Loaded. Endpoint: ' + BRIDGE_URL + ' | Sheets base: ' + SHEETS_BASE);

        document.addEventListener('keyup', onEditorInput);
        wireToggleButton();

        // Restore persisted on/off state
        if (loadPersistedState()) {
            log('Restoring ON state from localStorage');
            BridgeSync.enable();
        } else {
            log('Bridge is OFF. Click "VSCode Bridge" in the header to enable.');
            updateToggleUI();
        }
    }

    // Expose globally before DOMContentLoaded so configure() can be called early
    global.BridgeSync = BridgeSync;

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})(window);
