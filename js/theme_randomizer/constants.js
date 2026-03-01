/**
 * theme_randomizer/constants.js
 *
 * Endpoint paths and localStorage keys shared across all themeRand modules.
 * Must be loaded first -- initialises the LiveCSS._trInternal namespace.
 */

'use strict';

(function (LiveCSS) {

    // Bootstrap the shared internal namespace if it does not yet exist.
    if (!LiveCSS._trInternal) {
        LiveCSS._trInternal = { constants: {}, state: {}, dom: {}, fn: {} };
    }

    var C = LiveCSS._trInternal.constants;

    // -----------------------------------------------------------------------
    // Endpoints
    // -----------------------------------------------------------------------

    C.RANDOMIZER_DATA = 'ai/theme-randomizer.json';
    C.SHEETS_API      = 'data/stylesheets.php?json';
    C.CONTEXT_API     = 'ai/context-time.php';
    C.NUDGE_API       = 'style-sheets/theme-randomizer.php';

    // -----------------------------------------------------------------------
    // localStorage keys
    // -----------------------------------------------------------------------

    C.SK_PALETTE = 'lc-rand-palette';
    C.SK_SHEET   = 'lc-rand-sheet';
    C.SK_OPEN    = 'lc-rand-open';
    C.SK_NUDGE   = 'lc-rand-nudge';

    console.log('[themeRand:constants] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
