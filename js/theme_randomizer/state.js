/**
 * theme_randomizer/state.js
 *
 * Shared mutable state for all themeRand modules.
 * Requires: constants.js (loads first / initialises _trInternal).
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:state] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    var S = _i.state;

    S.data            = null;    // parsed theme-randomizer.json
    S.sheets          = [];      // array from stylesheets.php
    S.context         = null;    // object from context-time.php or inferContext()
    S.activeSheet     = '';      // currently applied stylesheet filename
    S.activePalette   = '';      // currently applied palette id
    S.panelOpen       = false;
    S.activeNudge     = null;    // { vars: {}, duration_vars: {} } from last nudge
    S.activeNudgeSeed = null;    // integer seed used by last nudge
    S.styleLink       = null;    // <link> element for the active stylesheet

    console.log('[themeRand:state] initialised');

}(window.LiveCSS = window.LiveCSS || {}));
