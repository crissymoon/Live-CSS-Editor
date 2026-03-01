/**
 * theme_randomizer/sheets.js
 *
 * Swap the active application stylesheet via a managed <link> element.
 * Requires: constants.js, state.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:sheets] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    /**
     * Swap the application stylesheet.
     * Inserts a new <link> into <head> and removes the previous one.
     * Passing an empty path clears the active stylesheet.
     * @param {string} path      - href value for the new <link>
     * @param {string} [filename] - display name stored in state.activeSheet
     */
    _i.fn.applySheet = function applySheet(path, filename) {
        var S   = _i.state;
        var dom = _i.dom;
        try {
            if (S.styleLink) {
                if (S.styleLink.parentNode) {
                    S.styleLink.parentNode.removeChild(S.styleLink);
                }
                S.styleLink = null;
            }

            if (!path) {
                S.activeSheet = '';
                if (dom.sheetSelect) { dom.sheetSelect.value = ''; }
                return;
            }

            var link   = document.createElement('link');
            link.rel   = 'stylesheet';
            link.href  = path;
            document.head.appendChild(link);

            S.styleLink  = link;
            S.activeSheet = filename || path;

            if (dom.sheetSelect) { dom.sheetSelect.value = S.activeSheet; }
        } catch (e) {
            console.error('[themeRand:sheets] applySheet() failed for path "' + path + '":', e);
        }
    };

    console.log('[themeRand:sheets] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
