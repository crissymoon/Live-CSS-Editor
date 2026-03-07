/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/**
 * app.js -- Entry point. Boots all modules in dependency order.
 *
 * Waits for cdn-loader to resolve CodeMirror before initializing.
 * Reads config from window.LiveCSSData, which is set inline by index.php.
 */
(function () {

    var data = window.LiveCSSData;

    // ── Menubar click-to-open dropdowns ─────────────────────────────────────
    // Adds/removes .menu-open on .menu-item when the user clicks .menu-label.
    // Clicking outside (or pressing Escape) closes all open menus.
    (function () {
        var menubar = document.getElementById('appMenubar');
        if (!menubar) { return; }

        function closeAll() {
            var open = menubar.querySelectorAll('.menu-item.menu-open');
            for (var i = 0; i < open.length; i++) {
                open[i].classList.remove('menu-open');
            }
        }

        var labels = menubar.querySelectorAll('.menu-label');
        for (var i = 0; i < labels.length; i++) {
            (function (label) {
                label.addEventListener('click', function (e) {
                    e.stopPropagation();
                    var item   = label.parentElement;
                    var isOpen = item.classList.contains('menu-open');
                    closeAll();
                    if (!isOpen) { item.classList.add('menu-open'); }
                });
            }(labels[i]));
        }

        // Close when clicking a menu item button
        menubar.addEventListener('click', function (e) {
            if (e.target && e.target.tagName === 'BUTTON') {
                closeAll();
            }
        });

        // Close on outside click or Escape
        document.addEventListener('click', closeAll);
        document.addEventListener('keydown', function (e) {
            if (e.key === 'Escape') { closeAll(); }
        });
    }());

    LiveCSS.cdnLoader.load(function () {

        // 1. Editors — use auto-saved session if one exists, otherwise defaults
        var saved = LiveCSS.storage.loadAutosave();
        LiveCSS.editor.init(
            saved ? saved.html : data.defaultHtml,
            saved ? saved.css  : data.defaultCss,
            saved ? saved.js   : data.defaultJs
        );

        // 2. Session restore bar — show if history exists
        (function () {
            var hist = LiveCSS.storage.getHistory();
            if (!hist || hist.length === 0) { return; }

            var bar = document.getElementById('sessionRestoreBar');
            var sel = document.getElementById('sessionHistorySelect');
            if (!bar || !sel) { return; }

            function fmtDate(ts) {
                var d   = new Date(ts);
                var now = new Date();
                var time = d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
                if (d.toDateString() === now.toDateString()) { return 'Today ' + time; }
                var yesterday = new Date(now - 86400000);
                if (d.toDateString() === yesterday.toDateString()) { return 'Yesterday ' + time; }
                return d.toLocaleDateString([], { month: 'short', day: 'numeric' }) + '  ' + time;
            }

            hist.forEach(function (entry, i) {
                var opt = document.createElement('option');
                opt.value = i;
                opt.textContent = (i === 0 ? 'Last session — ' : '') + fmtDate(entry.ts);
                sel.appendChild(opt);
            });

            bar.classList.remove('hidden');

            document.getElementById('sessionRestoreBtn').addEventListener('click', function () {
                var entry = hist[parseInt(sel.value, 10)];
                if (!entry) { return; }
                LiveCSS.editor.getHtmlEditor().setValue(entry.html || '');
                LiveCSS.editor.getCssEditor().setValue(entry.css  || '');
                LiveCSS.editor.getJsEditor().setValue(entry.js    || '');
                LiveCSS.editor.updatePreview();
                bar.classList.add('hidden');
            });

            document.getElementById('sessionDismissBtn').addEventListener('click', function () {
                bar.classList.add('hidden');
            });
        }());

        // 3. Header property-reference dropdown
        LiveCSS.propertyLookup.init(data.propertyValues);

        // 4. Fuzzy autocomplete — CSS, JS, and HTML editors
        LiveCSS.fuzzy.init(data.allCssProperties, data.propertyValues);
        LiveCSS.fuzzy.initJs();
        LiveCSS.fuzzy.initHtml();

        // 4. Save modal
        LiveCSS.modalSave.init();

        // 5. Load modal
        LiveCSS.modalLoad.init();

        // 6. Resizable panel gutters
        LiveCSS.gutter.init();

        // 7. Inline color swatches in all three editors
        LiveCSS.colorSwatch.init();

        // 7b. Inline size slider diamonds (CSS editor only)
        LiveCSS.sizeSlider.init();

        // 7c. Widgets toggle button (color swatches + size sliders on/off)
        (function () {
            try {
                var btn = document.getElementById('widgetsBtn');
                if (!btn) { console.warn('[app] widgetsBtn not found in DOM'); return; }

                // Restore persisted state -- default is enabled (true)
                var uiState = LiveCSS.storage.loadUIState() || {};
                var widgetsOn = uiState.widgetsEnabled !== false;

                function applyWidgetState(on) {
                    try { LiveCSS.colorSwatch.setEnabled(on); } catch (e) { console.error('[app] colorSwatch.setEnabled failed:', e); }
                    try { LiveCSS.sizeSlider.setEnabled(on);  } catch (e) { console.error('[app] sizeSlider.setEnabled failed:', e); }
                    btn.classList.toggle('menu-btn-active', on);
                    console.log('[app] widgets ' + (on ? 'enabled' : 'disabled'));
                }

                // Apply initial state without triggering a redundant rescan on first load
                // (both modules already queued their initial scans via setTimeout).
                btn.classList.toggle('menu-btn-active', widgetsOn);
                if (!widgetsOn) {
                    // Widgets were persisted as off -- disable after the initial scan timeout
                    setTimeout(function () { applyWidgetState(false); }, 700);
                }

                btn.addEventListener('click', function () {
                    widgetsOn = !widgetsOn;
                    applyWidgetState(widgetsOn);
                    try {
                        var s = LiveCSS.storage.loadUIState() || {};
                        s.widgetsEnabled = widgetsOn;
                        LiveCSS.storage.saveUIState(s);
                    } catch (e) {
                        console.error('[app] failed to save widgetsEnabled state:', e);
                    }
                });
            } catch (e) {
                console.error('[app] widgets toggle setup failed:', e);
            }
        }());

        // 8. Color harmony floating tool
        LiveCSS.colorHarmony.init();

        // 9. Per-panel inline search bars
        LiveCSS.editorSearch.init();

        // 10. Custom indent guides (only within leading whitespace)
        LiveCSS.indentGuide.attach(LiveCSS.editor.getJsEditor());
        LiveCSS.indentGuide.attach(LiveCSS.editor.getHtmlEditor());
        LiveCSS.indentGuide.attach(LiveCSS.editor.getCssEditor());
        LiveCSS.indentGuide.init();

        // 11. Wireframe tool
        try {
            if (LiveCSS.wireframe && typeof LiveCSS.wireframe.init === 'function') {
                LiveCSS.wireframe.init();
            } else {
                console.warn('[app] LiveCSS.wireframe not ready -- wireframe.js module may have failed to load');
            }
        } catch (e) {
            console.error('[app] wireframe.init threw:', e);
        }

        // Escape closes wireframe overlay regardless of module state
        (function () {
            var wfOverlay = document.getElementById('wireframeOverlay');
            if (!wfOverlay) { return; }
            document.addEventListener('keydown', function (e) {
                if (e.key === 'Escape' && !wfOverlay.classList.contains('hidden')) {
                    wfOverlay.classList.add('hidden');
                }
            });
        }());

        document.getElementById('resetBtn').addEventListener('click', function () {
            if (confirm('Reset all editors to default code?')) {
                LiveCSS.editor.getHtmlEditor().setValue(data.defaultHtml);
                LiveCSS.editor.getCssEditor().setValue(data.defaultCss);
                LiveCSS.editor.getJsEditor().setValue(data.defaultJs || '');
                LiveCSS.editor.updatePreview();
            }
        });

        // Sync to Bridge: saves current editors to DB + writes files to projects/
        (function () {
            var syncBtn = document.getElementById('syncToBridgeBtn');
            if (!syncBtn) {
                console.warn('[app] syncToBridgeBtn not found in DOM, skipping sync setup');
                return;
            }
            syncBtn.addEventListener('click', function () {
                try {
                    var name = prompt('Project name to sync:', 'crissys-style-tool');
                    if (!name) { return; }

                    var htmlVal = LiveCSS.editor.getHtmlEditor().getValue();
                    var cssVal  = LiveCSS.editor.getCssEditor().getValue();
                    var jsVal   = LiveCSS.editor.getJsEditor().getValue();

                    console.log('[sync] Syncing project "' + name + '" to bridge (' +
                        htmlVal.length + ' html, ' + cssVal.length + ' css, ' + jsVal.length + ' js)');

                    var xhr = new XMLHttpRequest();
                    xhr.open('POST', '/vscode-bridge/api/projects.php?action=sync_to_bridge', true);
                    xhr.setRequestHeader('Content-Type', 'application/json');
                    xhr.onload = function () {
                        try {
                            var res = JSON.parse(xhr.responseText);
                            if (res.success) {
                                console.log('[sync] OK: project "' + name + '" saved + exported', res.files);
                                alert('Synced "' + name + '" to bridge.\nFiles written: ' + (res.files || []).join(', '));
                            } else {
                                console.error('[sync] Server error:', res.error || res);
                                alert('Sync failed: ' + (res.error || 'Unknown error'));
                            }
                        } catch (parseErr) {
                            console.error('[sync] Response parse error:', parseErr, xhr.responseText);
                            alert('Sync failed: could not parse response');
                        }
                    };
                    xhr.onerror = function () {
                        console.error('[sync] Network error, status:', xhr.status);
                        alert('Sync failed: network error');
                    };
                    xhr.send(JSON.stringify({ name: name, html: htmlVal, css: cssVal, js: jsVal }));
                } catch (err) {
                    console.error('[sync] Unexpected error:', err);
                    alert('Sync failed: ' + err.message);
                }
            });
        }());

        // Pull from VSCode: runs update-live-css.sh then reloads the project
        (function () {
            var pullBtn = document.getElementById('pullFromVscodeBtn');
            if (!pullBtn) {
                console.warn('[app] pullFromVscodeBtn not found in DOM, skipping pull setup');
                return;
            }
            pullBtn.addEventListener('click', function () {
                try {
                    var name = prompt('Project name to pull:', 'crissys-style-tool');
                    if (!name) { return; }

                    console.log('[pull] Running update-live-css.sh for "' + name + '"...');
                    pullBtn.disabled = true;
                    pullBtn.textContent = 'Pulling...';

                    var xhr = new XMLHttpRequest();
                    xhr.open('GET', '/vscode-bridge/api/pull-from-vscode.php?project=' + encodeURIComponent(name), true);
                    xhr.onload = function () {
                        pullBtn.disabled = false;
                        pullBtn.textContent = 'Pull from VSCode';
                        try {
                            var res = JSON.parse(xhr.responseText);
                            if (res.success) {
                                var stepLog = (res.steps || []).map(function (s) {
                                    return (s.ok ? '[ok]' : '[fail]') + ' ' + s.label + (s.detail ? ': ' + s.detail : '');
                                }).join('\n');
                                console.log('[pull] OK:\n' + stepLog);
                                // Load the updated project into the editors
                                if (typeof LiveCSS.storage.loadDbProject === 'function') {
                                    LiveCSS.storage.loadDbProject(name)
                                        .then(function (proj) {
                                            if (proj && LiveCSS.editor) {
                                                LiveCSS.editor.setHtmlValue(proj.html || '');
                                                LiveCSS.editor.setCssValue(proj.css || '');
                                                LiveCSS.editor.setJsValue(proj.js || '');
                                                LiveCSS.editor.updatePreview();
                                                console.log('[pull] Loaded "' + name + '" into editors');
                                                alert('Pulled and loaded "' + name + '" from VSCode.');
                                            } else {
                                                console.warn('[pull] Pull complete but editors not ready -- use Load to apply');
                                                alert('Pull complete. Use Load to apply the project.');
                                            }
                                        })
                                        .catch(function (e) {
                                            console.error('[pull] loadDbProject failed:', e);
                                            alert('Pull complete but could not auto-load. Use Load to apply.');
                                        });
                                } else {
                                    console.warn('[pull] loadDbProject not available');
                                    alert('Pull complete. Click Load to apply the project.');
                                }
                            } else {
                                var errDetail = (res.errors || []).join(', ') || res.error || 'Unknown error';
                                console.error('[pull] Failed:', errDetail, res.steps || []);
                                alert('Pull failed: ' + errDetail);
                            }
                        } catch (parseErr) {
                            console.error('[pull] Response parse error:', parseErr, xhr.responseText);
                            alert('Pull failed: could not parse response');
                        }
                    };
                    xhr.onerror = function () {
                        pullBtn.disabled = false;
                        pullBtn.textContent = 'Pull from VSCode';
                        console.error('[pull] Network error, status:', xhr.status);
                        alert('Pull failed: network error');
                    };
                    xhr.send();
                } catch (err) {
                    console.error('[pull] Unexpected error:', err);
                    alert('Pull failed: ' + err.message);
                }
            });
        }());

        // 11. Auto-save current work on every editor change (debounced 1.5 s)
        var autosaveEl = document.getElementById('autosaveStatus');
        var autosaveFadeTimer = null;

        function showAutosaveStatus(msg, cls) {
            if (!autosaveEl) { return; }
            clearTimeout(autosaveFadeTimer);
            autosaveEl.textContent  = msg;
            autosaveEl.className    = 'autosave-status ' + cls;
            if (cls === 'autosave-saved') {
                autosaveFadeTimer = setTimeout(function () {
                    autosaveEl.textContent = '';
                    autosaveEl.className   = 'autosave-status';
                }, 2200);
            }
        }

        var autoSave = LiveCSS.utils.debounce(function () {
            LiveCSS.storage.saveAutosave(
                LiveCSS.editor.getHtmlEditor().getValue(),
                LiveCSS.editor.getCssEditor().getValue(),
                LiveCSS.editor.getJsEditor().getValue()
            );
            showAutosaveStatus('Saved', 'autosave-saved');
        }, 1500);

        function onEditorChange() {
            showAutosaveStatus('Saving...', 'autosave-saving');
            autoSave();
        }

        LiveCSS.editor.getHtmlEditor().on('change', onEditorChange);
        LiveCSS.editor.getCssEditor().on('change',  onEditorChange);
        LiveCSS.editor.getJsEditor().on('change',   onEditorChange);

        // Also save immediately before the page unloads, and push a history snapshot
        window.addEventListener('beforeunload', function () {
            var h = LiveCSS.editor.getHtmlEditor().getValue();
            var c = LiveCSS.editor.getCssEditor().getValue();
            var j = LiveCSS.editor.getJsEditor().getValue();
            LiveCSS.storage.saveAutosave(h, c, j);
            LiveCSS.storage.pushHistory(h, c, j);

            // Final flush of all UI layout state
            var state = LiveCSS.storage.loadUIState() || {};
            state.panels = LiveCSS.gutter.getLayoutState();
            LiveCSS.storage.saveUIState(state);
        });

        // 11. Undo / redo buttons in panel headers
        var ed = LiveCSS.editor;
        document.getElementById('jsUndoBtn').addEventListener('mousedown',   function (e) { e.preventDefault(); ed.getJsEditor().undo();   });
        document.getElementById('jsRedoBtn').addEventListener('mousedown',   function (e) { e.preventDefault(); ed.getJsEditor().redo();   });
        document.getElementById('htmlUndoBtn').addEventListener('mousedown', function (e) { e.preventDefault(); ed.getHtmlEditor().undo(); });
        document.getElementById('htmlRedoBtn').addEventListener('mousedown', function (e) { e.preventDefault(); ed.getHtmlEditor().redo(); });
        document.getElementById('cssUndoBtn').addEventListener('mousedown',  function (e) { e.preventDefault(); ed.getCssEditor().undo();  });
        document.getElementById('cssRedoBtn').addEventListener('mousedown',  function (e) { e.preventDefault(); ed.getCssEditor().redo();  });

        // 12. Reset layout button
        document.getElementById('resetLayoutBtn').addEventListener('click', function () {
            LiveCSS.gutter.resetLayout();
        });

        // 13. Code Agent modal
        if (LiveCSS.agent && LiveCSS.agent.init) {
            LiveCSS.agent.init();
        }
        document.getElementById('agentBtn').addEventListener('click', function () {
            if (LiveCSS.agent) { LiveCSS.agent.open(); }
        });

    });

}());
