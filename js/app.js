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

        // 2. Header property-reference dropdown
        LiveCSS.propertyLookup.init(data.propertyValues);

        // 3. Fuzzy autocomplete — CSS, JS, and HTML editors
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

        // 8. Color harmony floating tool
        LiveCSS.colorHarmony.init();

        // 9. Reset content button
        document.getElementById('resetBtn').addEventListener('click', function () {
            if (confirm('Reset all editors to default code?')) {
                LiveCSS.editor.getHtmlEditor().setValue(data.defaultHtml);
                LiveCSS.editor.getCssEditor().setValue(data.defaultCss);
                LiveCSS.editor.getJsEditor().setValue(data.defaultJs || '');
                LiveCSS.editor.updatePreview();
            }
        });

        // 10. Auto-save current work on every editor change (debounced 1.5 s)
        var autoSave = LiveCSS.utils.debounce(function () {
            LiveCSS.storage.saveAutosave(
                LiveCSS.editor.getHtmlEditor().getValue(),
                LiveCSS.editor.getCssEditor().getValue(),
                LiveCSS.editor.getJsEditor().getValue()
            );
        }, 1500);
        LiveCSS.editor.getHtmlEditor().on('change', autoSave);
        LiveCSS.editor.getCssEditor().on('change',  autoSave);
        LiveCSS.editor.getJsEditor().on('change',   autoSave);

        // Also save immediately before the page unloads
        window.addEventListener('beforeunload', function () {
            LiveCSS.storage.saveAutosave(
                LiveCSS.editor.getHtmlEditor().getValue(),
                LiveCSS.editor.getCssEditor().getValue(),
                LiveCSS.editor.getJsEditor().getValue()
            );
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
