/**
 * theme_randomizer/palette.js
 *
 * Palette selection, lookup, and application to :root CSS custom properties.
 * Requires: constants.js, state.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:palette] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    /**
     * Pick the best palette for the given context descriptor.
     * Falls back to a random item from all palettes if no culture/time match is found.
     * @param {object} ctx
     * @returns {object|null}
     */
    _i.fn.selectPalette = function selectPalette(ctx) {
        var S = _i.state;
        if (!S.data || !S.data.palettes || !S.data.palettes.length) {
            console.warn('[themeRand:palette] selectPalette() called before data was loaded.');
            return null;
        }

        try {
            var culturalAdj = S.data.cultural_adjustments && S.data.cultural_adjustments[ctx.cultural_area];
            var preferred   = culturalAdj ? (culturalAdj.preferred_palettes || []) : [];

            var candidates = S.data.palettes.filter(function (p) {
                var cAff  = p.cultural_affinity || [];
                var times = p.time_of_day       || [];
                var cMatch = cAff.indexOf(ctx.cultural_area) !== -1 || cAff.indexOf('global') !== -1;
                var tMatch = times.length === 0 || times.indexOf(ctx.time_of_day) !== -1;
                return cMatch && tMatch;
            });

            var preferred_filtered = candidates.filter(function (p) {
                return preferred.indexOf(p.id) !== -1;
            });

            var pool = preferred_filtered.length ? preferred_filtered : candidates;
            if (!pool.length) { pool = S.data.palettes; }

            return pool[Math.floor(Math.random() * pool.length)];
        } catch (e) {
            console.error('[themeRand:palette] selectPalette() error:', e);
            return S.data.palettes[0] || null;
        }
    };

    /**
     * Apply palette CSS custom properties to :root.
     * @param {object} palette
     */
    _i.fn.applyPalette = function applyPalette(palette) {
        var S   = _i.state;
        var dom = _i.dom;
        if (!palette || !palette.vars) {
            console.warn('[themeRand:palette] applyPalette() received empty palette.');
            return;
        }
        try {
            var root = document.documentElement;
            Object.keys(palette.vars).forEach(function (prop) {
                root.style.setProperty(prop, palette.vars[prop]);
            });
            S.activePalette = palette.id;
            if (dom.paletteSelect) { dom.paletteSelect.value = palette.id; }
            if (dom.contextBar)    { _i.fn.updateContextBarPalette(palette); }
        } catch (e) {
            console.error('[themeRand:palette] applyPalette() error for "' + (palette.id || '?') + '":', e);
        }
    };

    /**
     * Find a palette by id in the loaded data.
     * @param {string} id
     * @returns {object|null}
     */
    _i.fn.findPalette = function findPalette(id) {
        var S = _i.state;
        if (!S.data || !S.data.palettes) { return null; }
        for (var i = 0; i < S.data.palettes.length; i++) {
            if (S.data.palettes[i].id === id) { return S.data.palettes[i]; }
        }
        return null;
    };

    console.log('[themeRand:palette] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
