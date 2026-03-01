/**
 * theme_randomizer/randomize.js
 *
 * Pick a new random palette and apply it.
 * Requires: constants.js, state.js, context.js, palette.js, persistence.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:randomize] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    _i.fn.randomizePalette = function randomizePalette() {
        var S  = _i.state;
        var C  = _i.constants;
        var fn = _i.fn;
        if (!S.data) {
            console.warn('[themeRand:randomize] randomizePalette() called before data was loaded.');
            return;
        }
        try {
            var ctx = S.context || fn.inferContext();
            var pal = fn.selectPalette(ctx);
            if (pal) {
                fn.applyPalette(pal);
                fn.savePref(C.SK_PALETTE, pal.id);
                if (_i.dom.paletteSelect) { _i.dom.paletteSelect.value = pal.id; }
            } else {
                console.warn('[themeRand:randomize] selectPalette() returned null.');
            }
        } catch (e) {
            console.error('[themeRand:randomize] randomizePalette() failed:', e);
        }
    };

    console.log('[themeRand:randomize] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
