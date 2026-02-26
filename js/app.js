/**
 * app.js — Entry point. Boots all modules in dependency order.
 *
 * Waits for cdn-loader to resolve CodeMirror before initializing.
 * Reads config from window.LiveCSSData, which is set inline by index.php.
 */
(function () {

    var data = window.LiveCSSData;

    LiveCSS.cdnLoader.load(function () {

        // 1. Editors + live preview (must be first — other modules depend on getCssEditor/getHtmlEditor)
        LiveCSS.editor.init(data.defaultHtml, data.defaultCss, data.defaultJs);

        // 2. Header property-reference dropdown
        LiveCSS.propertyLookup.init(data.propertyValues);

        // 3. Fuzzy autocomplete in the CSS editor
        LiveCSS.fuzzy.init(data.allCssProperties);

        // 4. Save modal
        LiveCSS.modalSave.init();

        // 5. Load modal
        LiveCSS.modalLoad.init();

        // 6. Resizable panel gutters
        LiveCSS.gutter.init();

        // 7. Reset content button
        document.getElementById('resetBtn').addEventListener('click', function () {
            if (confirm('Reset all editors to default code?')) {
                LiveCSS.editor.getHtmlEditor().setValue(data.defaultHtml);
                LiveCSS.editor.getCssEditor().setValue(data.defaultCss);
                LiveCSS.editor.getJsEditor().setValue(data.defaultJs || '');
                LiveCSS.editor.updatePreview();
            }
        });

        // 8. Reset layout button
        document.getElementById('resetLayoutBtn').addEventListener('click', function () {
            LiveCSS.gutter.resetLayout();
        });

    });

}());
