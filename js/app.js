/**
 * app.js — Entry point. Boots all modules in dependency order.
 *
 * Waits for cdn-loader to resolve CodeMirror before initializing.
 * Reads config from window.LiveCSSData, which is set inline by index.php.
 */
(function () {

    var data = window.LiveCSSData;

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
        LiveCSS.fuzzy.init(data.allCssProperties);
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

        // 8. Color harmony floating tool
        LiveCSS.colorHarmony.init();

        // 9. Per-panel inline search bars
        LiveCSS.editorSearch.init();

        // 10. Custom indent guides (only within leading whitespace)
        LiveCSS.indentGuide.attach(LiveCSS.editor.getJsEditor());
        LiveCSS.indentGuide.attach(LiveCSS.editor.getHtmlEditor());
        LiveCSS.indentGuide.attach(LiveCSS.editor.getCssEditor());
        LiveCSS.indentGuide.init();

        // 11. Reset content button
        document.getElementById('resetBtn').addEventListener('click', function () {
            if (confirm('Reset all editors to default code?')) {
                LiveCSS.editor.getHtmlEditor().setValue(data.defaultHtml);
                LiveCSS.editor.getCssEditor().setValue(data.defaultCss);
                LiveCSS.editor.getJsEditor().setValue(data.defaultJs || '');
                LiveCSS.editor.updatePreview();
            }
        });

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



    });

}());
