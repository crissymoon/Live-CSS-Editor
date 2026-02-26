/**
 * native-bridge.js — Dev/test bridge overlay for Live CSS Editor
 *
 * Provides a floating toolbar with:
 *   Browse  — opens a native file dialog (Tauri) or file input (browser)
 *             and loads the selected file into the HTML or CSS editor
 *   Refresh — hard-reloads the page / webview
 *   Debug   — opens DevTools (Tauri) or toggles an inline console panel (browser)
 *
 * Works in both browser (php -S) and Tauri native contexts.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.nativeBridge = (function () {
    'use strict';

    // -----------------------------------------------------------------------
    // Environment detection
    // -----------------------------------------------------------------------
    var isTauri = !!(window.__TAURI__ || window.__TAURI_INTERNALS__);

    function tauriInvoke(cmd, args) {
        if (window.__TAURI__ && window.__TAURI__.core) {
            return window.__TAURI__.core.invoke(cmd, args || {});
        }
        if (window.__TAURI_INTERNALS__) {
            return window.__TAURI_INTERNALS__.invoke(cmd, args || {});
        }
        return Promise.reject(new Error('Not running inside Tauri'));
    }

    // -----------------------------------------------------------------------
    // Build the toolbar DOM
    // -----------------------------------------------------------------------
    function buildBar() {
        var bar = document.createElement('div');
        bar.id = 'native-bridge-bar';
        bar.innerHTML = [
            '<span class="nb-label">BRIDGE</span>',
            '<span class="nb-sep"></span>',
            '<button class="nb-btn nb-btn-browse"  id="nb-browse"  title="Browse and load a file into the editor">Browse</button>',
            '<button class="nb-btn nb-btn-refresh" id="nb-refresh" title="Reload the app">Refresh</button>',
            '<button class="nb-btn nb-btn-debug"   id="nb-debug"  title="Open debug tools">Debug</button>',
            '<span class="nb-sep"></span>',
            '<button class="nb-toggle" id="nb-toggle" title="Collapse toolbar">&#8722;</button>'
        ].join('');
        document.body.appendChild(bar);

        document.getElementById('nb-browse').addEventListener('click', browseFile);
        document.getElementById('nb-refresh').addEventListener('click', refreshApp);
        document.getElementById('nb-debug').addEventListener('click', toggleDebug);

        var collapsed = false;
        document.getElementById('nb-toggle').addEventListener('click', function () {
            collapsed = !collapsed;
            bar.classList.toggle('nb-collapsed', collapsed);
            this.innerHTML = collapsed ? '&#43;' : '&#8722;';
        });
    }

    // -----------------------------------------------------------------------
    // Browse — open file, load content into editor
    // -----------------------------------------------------------------------
    function browseFile() {
        if (isTauri) {
            tauriInvoke('pick_and_read_file')
                .then(function (result) {
                    if (result) {
                        handleFileContent(result.path, result.content);
                    }
                })
                .catch(function (err) {
                    showNotice('Browse error: ' + err, true);
                    console.error('[bridge] browse error', err);
                });
        } else {
            // Browser fallback — standard file input
            var input = document.createElement('input');
            input.type = 'file';
            input.accept = '.html,.htm,.css,.php,.txt';
            input.style.display = 'none';
            input.addEventListener('change', function () {
                var file = input.files && input.files[0];
                if (!file) return;
                var reader = new FileReader();
                reader.onload = function (e) {
                    handleFileContent(file.name, e.target.result);
                };
                reader.readAsText(file);
                document.body.removeChild(input);
            });
            document.body.appendChild(input);
            input.click();
        }
    }

    // -----------------------------------------------------------------------
    // Refresh
    // -----------------------------------------------------------------------
    function refreshApp() {
        window.location.reload();
    }

    // -----------------------------------------------------------------------
    // Debug — DevTools in Tauri, inline console panel in browser
    // -----------------------------------------------------------------------
    var debugPanelOpen = false;
    var consoleHooked  = false;

    function toggleDebug() {
        var btn = document.getElementById('nb-debug');

        if (isTauri) {
            tauriInvoke('open_devtools')
                .catch(function (err) {
                    console.warn('[bridge] devtools not available:', err);
                });
            return;
        }

        // Browser: inline console panel
        debugPanelOpen = !debugPanelOpen;
        if (btn) { btn.classList.toggle('nb-active', debugPanelOpen); }

        if (!debugPanelOpen) {
            var panel = document.getElementById('nb-debug-panel');
            if (panel) { panel.remove(); }
            return;
        }

        buildDebugPanel();
    }

    function buildDebugPanel() {
        if (document.getElementById('nb-debug-panel')) { return; }

        var panel = document.createElement('div');
        panel.id = 'nb-debug-panel';
        panel.innerHTML = [
            '<div class="nb-dp-header">',
            '  <strong>Console</strong>',
            '  <button id="nb-dp-clear">Clear</button>',
            '  <button id="nb-dp-copy">Copy</button>',
            '  <button id="nb-dp-close">Close</button>',
            '</div>',
            '<div id="nb-dp-log"></div>'
        ].join('');
        document.body.appendChild(panel);

        document.getElementById('nb-dp-close').addEventListener('click', function () {
            debugPanelOpen = false;
            var btn = document.getElementById('nb-debug');
            if (btn) { btn.classList.remove('nb-active'); }
            panel.remove();
        });

        document.getElementById('nb-dp-clear').addEventListener('click', function () {
            var log = document.getElementById('nb-dp-log');
            if (log) { log.innerHTML = ''; }
        });

        document.getElementById('nb-dp-copy').addEventListener('click', function () {
            var log = document.getElementById('nb-dp-log');
            if (!log) { return; }
            navigator.clipboard && navigator.clipboard.writeText(log.innerText);
        });

        if (!consoleHooked) { hookConsole(); }

        logLine('info', ['Console connected — ' + new Date().toLocaleTimeString()]);
    }

    // -----------------------------------------------------------------------
    // Console interception
    // -----------------------------------------------------------------------
    var origConsole = {};

    function hookConsole() {
        if (consoleHooked) { return; }
        consoleHooked = true;
        ['log', 'warn', 'error', 'info'].forEach(function (method) {
            origConsole[method] = console[method].bind(console);
            console[method] = function () {
                origConsole[method].apply(console, arguments);
                logLine(method, Array.prototype.slice.call(arguments));
            };
        });
        window.addEventListener('error', function (e) {
            logLine('error', ['Uncaught: ' + e.message + (e.filename ? ' @ ' + e.filename + ':' + e.lineno : '')]);
        });
        window.addEventListener('unhandledrejection', function (e) {
            logLine('error', ['Unhandled promise rejection: ' + (e.reason && (e.reason.message || e.reason))]);
        });
    }

    function logLine(level, args) {
        var log = document.getElementById('nb-dp-log');
        if (!log) { return; }
        var line = document.createElement('div');
        line.className = 'nb-dp-line nb-dp-' + level;
        var badge = '<span class="nb-dp-badge">[' + level.toUpperCase().slice(0, 3) + ']</span>';
        var text = args.map(function (a) {
            try { return typeof a === 'object' ? JSON.stringify(a, null, 2) : String(a); }
            catch (_) { return String(a); }
        }).join(' ');
        line.innerHTML = badge + escapeHtml(text);
        log.appendChild(line);
        log.scrollTop = log.scrollHeight;

        // Keep log from growing unbounded
        var lines = log.querySelectorAll('.nb-dp-line');
        if (lines.length > 500) { lines[0].remove(); }
    }

    function escapeHtml(str) {
        return str
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;');
    }

    // -----------------------------------------------------------------------
    // File content handler — routes into the correct editor
    // -----------------------------------------------------------------------
    function handleFileContent(filePath, content) {
        var name = filePath.replace(/\\/g, '/').split('/').pop();
        var ext  = (name.split('.').pop() || '').toLowerCase();

        var ed = window.LiveCSS && window.LiveCSS.editor;
        if (ed) {
            if (ext === 'css') {
                ed.setCssValue(content);
                showNotice('CSS loaded: ' + name);
            } else if (ext === 'html' || ext === 'htm' || ext === 'php') {
                ed.setHtmlValue(content);
                showNotice('HTML loaded: ' + name);
            } else {
                // Unknown extension — try heuristic: starts with <? or < then load as HTML
                if (/^\s*(<\?|<!|<[a-zA-Z])/.test(content)) {
                    ed.setHtmlValue(content);
                    showNotice('HTML loaded: ' + name);
                } else {
                    ed.setCssValue(content);
                    showNotice('CSS loaded: ' + name);
                }
            }
        } else {
            showNotice('Editor not ready yet', true);
        }
    }

    // -----------------------------------------------------------------------
    // Transient status notice in the toolbar
    // -----------------------------------------------------------------------
    var noticeTimer = null;

    function showNotice(msg, isError) {
        var bar = document.getElementById('native-bridge-bar');
        if (!bar) { return; }

        // Remove existing notice elements
        var prev = bar.querySelector('.nb-notice');
        if (prev) { prev.remove(); }

        var notice = document.createElement('span');
        notice.className = 'nb-notice';
        notice.style.color = isError ? '#e05555' : '#80e080';
        notice.textContent = msg;
        bar.appendChild(notice);

        clearTimeout(noticeTimer);
        noticeTimer = setTimeout(function () {
            if (notice.parentNode) { notice.remove(); }
        }, 3500);
    }

    // -----------------------------------------------------------------------
    // Public init
    // -----------------------------------------------------------------------
    function init() {
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', buildBar);
        } else {
            buildBar();
        }
    }

    return {
        init:        init,
        isTauri:     isTauri,
        browseFile:  browseFile,
        refreshApp:  refreshApp,
        toggleDebug: toggleDebug,
        showNotice:  showNotice
    };

}());

// Auto-initialize
window.LiveCSS.nativeBridge.init();
