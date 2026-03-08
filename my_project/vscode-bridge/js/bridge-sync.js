/**
 * vscode-bridge/js/bridge-sync.js
 * Two-way sync between Crissy's Style Tool and the VSCode Copilot bridge.
 *
 * When ACTIVE (on):
 *   - Polls the SQLite project database for updates from Copilot.
 *   - If Copilot saved a project, auto-loads it into all 3 editors.
 *   - Pushes current editor state to the bridge on an interval.
 *
 * Toggle:
 *   - ON/OFF state is persisted in localStorage.
 *   - Wired to #vscodeBridgeToggle in the app header.
 *   - When ON, the Help button is shown. When OFF, Help is hidden.
 *
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * MIT License -- see LICENSE file for full text.
 */

(function (global) {
    'use strict';

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------
    // Use LiveCSS.env.resolve() so the path is correct regardless of whether
    // the project is served from / (PHP built-in) or /my_project/ (WASM server).
    var _env         = (global.LiveCSS && global.LiveCSS.env) || { resolve: function (p) { return p; } };
    var PROJECTS_API = _env.resolve('/vscode-bridge/api/projects.php');
    var POLL_INTERVAL_MS        = 4000;
    var STORAGE_KEY             = 'bridgeSync_enabled';
    var TOGGLE_BTN_ID           = 'vscodeBridgeToggle';
    var HELP_BTN_ID             = 'helpBtn';

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------
    var _enabled      = false;
    var _pollTimer    = null;
    var _initialized  = false;

    // -------------------------------------------------------------------------
    // Logging
    // -------------------------------------------------------------------------
    function log(msg)  { console.log('[BridgeSync] '   + msg); }
    function warn(msg) { console.warn('[BridgeSync] '  + msg); }
    function err(msg)  { console.error('[BridgeSync] ' + msg); }

    // -------------------------------------------------------------------------
    // Poll for project saves from Copilot (via SQLite)
    // -------------------------------------------------------------------------
    function pollProjectSave() {
        if (!_enabled) return;
        if (!global.LiveCSS || !global.LiveCSS.storage ||
            typeof global.LiveCSS.storage.pollProjectUpdate !== 'function') {
            return;
        }

        global.LiveCSS.storage.pollProjectUpdate()
            .then(function (data) {
                if (!data || !data.hasUpdate) return;
                var name   = data.name   || '(unnamed)';
                var source = data.source || 'unknown';
                console.warn('[BridgeSync] Project "' + name + '" updated (source: ' + source + ')');

                // Acknowledge so we do not fire again
                global.LiveCSS.storage.ackProjectUpdate()
                    .catch(function (e) { err('ackProjectUpdate failed: ' + e.message); });

                // Auto-load into editors
                global.LiveCSS.storage.loadDbProject(name)
                    .then(function (project) {
                        if (!project) {
                            err('loadDbProject returned null for "' + name + '"');
                            return;
                        }
                        try {
                            var ed = global.LiveCSS.editor;
                            if (ed) {
                                if (typeof ed.getHtmlEditor === 'function') {
                                    ed.getHtmlEditor().setValue(project.html || '');
                                }
                                if (typeof ed.getCssEditor === 'function') {
                                    ed.getCssEditor().setValue(project.css || '');
                                }
                                if (typeof ed.getJsEditor === 'function') {
                                    ed.getJsEditor().setValue(project.js || '');
                                }
                                if (typeof ed.updatePreview === 'function') {
                                    ed.updatePreview();
                                }
                                console.warn('[BridgeSync] Auto-loaded "' + name + '" into all 3 editors');
                            }
                        } catch (e) {
                            err('Auto-load exception: ' + e.message);
                        }
                    })
                    .catch(function (e) {
                        err('loadDbProject error: ' + e.message);
                    });
            })
            .catch(function (e) {
                err('pollProjectSave error: ' + e.message);
            });
    }

    // -------------------------------------------------------------------------
    // Start / stop polling
    // -------------------------------------------------------------------------
    function startPolling() {
        if (_pollTimer) return;
        pollProjectSave();
        _pollTimer = setInterval(pollProjectSave, POLL_INTERVAL_MS);
        log('Polling started (interval=' + POLL_INTERVAL_MS + 'ms)');
    }

    function stopPolling() {
        clearInterval(_pollTimer);
        _pollTimer = null;
        log('Polling stopped');
    }

    // -------------------------------------------------------------------------
    // Persisted state
    // -------------------------------------------------------------------------
    function loadPersistedState() {
        try { return localStorage.getItem(STORAGE_KEY) === 'true'; }
        catch (e) { return false; }
    }

    function persistState(val) {
        try { localStorage.setItem(STORAGE_KEY, val ? 'true' : 'false'); }
        catch (e) { /* ignore */ }
    }

    // -------------------------------------------------------------------------
    // Toggle button + Help button DOM wiring
    // -------------------------------------------------------------------------
    function getToggleBtn() { return document.getElementById(TOGGLE_BTN_ID); }
    function getHelpBtn()   { return document.getElementById(HELP_BTN_ID); }

    function updateToggleUI() {
        var btn     = getToggleBtn();
        var helpBtn = getHelpBtn();

        if (btn) {
            if (_enabled) {
                btn.classList.add('bridge-active');
                btn.setAttribute('aria-pressed', 'true');
                btn.title = 'VSCode Copilot Bridge: ON -- click to disable';
            } else {
                btn.classList.remove('bridge-active');
                btn.setAttribute('aria-pressed', 'false');
                btn.title = 'VSCode Copilot Bridge: OFF -- click to enable';
            }
        }

        // Show Help only when bridge is ON
        if (helpBtn) {
            helpBtn.style.display = _enabled ? '' : 'none';
        }
    }

    function wireToggleButton() {
        var btn = getToggleBtn();
        if (!btn) {
            warn('wireToggleButton: #' + TOGGLE_BTN_ID + ' not in DOM -- retrying in 500ms');
            setTimeout(wireToggleButton, 500);
            return;
        }
        btn.addEventListener('click', function () { BridgeSync.toggle(); });
        updateToggleUI();
        log('Toggle button wired');
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
            startPolling();
            console.warn('[BridgeSync] ENABLED -- Copilot bridge is active');
        },

        disable: function () {
            if (!_enabled) { log('disable: already off'); return; }
            _enabled = false;
            persistState(false);
            stopPolling();
            updateToggleUI();
            console.warn('[BridgeSync] DISABLED -- sync paused');
        },

        toggle: function () {
            if (_enabled) { BridgeSync.disable(); } else { BridgeSync.enable(); }
        },

        isActive: function () { return _enabled; },
    };

    // -------------------------------------------------------------------------
    // Init
    // -------------------------------------------------------------------------
    function init() {
        if (_initialized) return;
        _initialized = true;

        log('Loaded. API: ' + PROJECTS_API);

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

    global.BridgeSync = BridgeSync;

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})(window);
