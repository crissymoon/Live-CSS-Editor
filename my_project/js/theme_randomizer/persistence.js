/**
 * theme_randomizer/persistence.js
 *
 * localStorage helpers for persisting user preferences.
 * Requires: constants.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:persistence] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    /**
     * Write a preference to localStorage.
     * Silently ignores quota or security errors but logs them for debugging.
     * @param {string} key
     * @param {string} value
     */
    _i.fn.savePref = function savePref(key, value) {
        try {
            localStorage.setItem(key, value);
        } catch (e) {
            console.warn('[themeRand:persistence] savePref() could not write key "' + key + '":', e);
        }
    };

    /**
     * Remove a preference from localStorage.
     * @param {string} key
     */
    _i.fn.clearPref = function clearPref(key) {
        try {
            localStorage.removeItem(key);
        } catch (e) {
            console.warn('[themeRand:persistence] clearPref() could not remove key "' + key + '":', e);
        }
    };

    console.log('[themeRand:persistence] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
