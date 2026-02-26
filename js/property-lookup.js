/**
 * property-lookup.js — Header CSS property reference dropdown and insert button
 * Attached to window.LiveCSS.propertyLookup
 *
 * Call LiveCSS.propertyLookup.init(propertyValues) after editor.init().
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.propertyLookup = (function () {

    function init(propertyValues) {
        var propertySelect = document.getElementById('propertySelect');
        var insertBtn      = document.getElementById('insertPropertyBtn');
        var infoBar        = document.getElementById('propertyInfoBar');
        var infoName       = document.getElementById('propertyInfoName');
        var infoValues     = document.getElementById('propertyInfoValues');
        var infoClose      = document.getElementById('propertyInfoClose');

        // Show the info bar when a property is chosen
        propertySelect.addEventListener('change', function () {
            var prop = this.value;
            if (prop) {
                infoName.textContent   = prop + ':';
                infoValues.textContent = propertyValues[prop] || '(see MDN for values)';
                infoBar.classList.remove('hidden');
            } else {
                infoBar.classList.add('hidden');
            }
        });

        infoClose.addEventListener('click', function () {
            infoBar.classList.add('hidden');
        });

        // Insert "  property: ;" snippet at the CSS editor cursor
        insertBtn.addEventListener('click', function () {
            var prop = propertySelect.value;
            if (!prop) return;

            var cm      = LiveCSS.editor.getCssEditor();
            var snippet = '  ' + prop + ': ;';
            var cursor  = cm.getCursor();

            cm.replaceRange(snippet + '\n', cursor);
            // Place cursor just before the semicolon so the value can be typed immediately
            cm.setCursor({ line: cursor.line, ch: cursor.ch + snippet.length - 1 });
            cm.focus();
        });
    }

    return { init: init };

}());
