/**
 * theme_randomizer/utils.js
 *
 * Lightweight DOM-creation helper used across modules.
 * Requires: constants.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:utils] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    /**
     * Create an element with an optional class string.
     * @param {string} tag
     * @param {string} [cls]
     * @returns {HTMLElement}
     */
    _i.fn.mk = function mk(tag, cls) {
        try {
            var el = document.createElement(tag);
            if (cls) { el.className = cls; }
            return el;
        } catch (e) {
            console.error('[themeRand:utils] mk() failed for tag "' + tag + '":', e);
            return document.createElement('span'); // safe fallback
        }
    };

    console.log('[themeRand:utils] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
