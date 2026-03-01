/**
 * theme_randomizer/loader.js
 *
 * Fetch all remote data (theme JSON, stylesheet list, server context) and
 * restore or auto-select a palette/stylesheet on first load.
 * Requires: constants.js, state.js, context.js, palette.js, sheets.js,
 *           persistence.js, ui-context-bar.js, ui-drawer.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:loader] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    // -----------------------------------------------------------------------
    // Public: restoreOrRandomize
    // -----------------------------------------------------------------------

    _i.fn.restoreOrRandomize = function restoreOrRandomize() {
        var S  = _i.state;
        var C  = _i.constants;
        var fn = _i.fn;

        var savedPalette = '';
        var savedSheet   = '';
        try {
            savedPalette = localStorage.getItem(C.SK_PALETTE) || '';
            savedSheet   = localStorage.getItem(C.SK_SHEET)   || '';
        } catch (e) {
            console.warn('[themeRand:loader] localStorage read failed:', e);
        }

        // Apply saved stylesheet first
        if (savedSheet) {
            var sheetEntry = S.sheets.find(function (s) { return s.name === savedSheet; });
            if (sheetEntry) {
                fn.applySheet(sheetEntry.path, sheetEntry.name);
            } else {
                console.warn('[themeRand:loader] Saved sheet "' + savedSheet + '" not found in loaded sheets list.');
            }
        }

        // Apply saved palette or auto-select
        if (savedPalette) {
            var pal = fn.findPalette(savedPalette);
            if (pal) {
                fn.applyPalette(pal);
                return;
            } else {
                console.warn('[themeRand:loader] Saved palette "' + savedPalette + '" not found in data.');
            }
        }

        // No valid saved preference -- auto-select based on context
        if (S.data) {
            var auto = fn.selectPalette(S.context);
            if (auto) {
                fn.applyPalette(auto);
            } else {
                console.warn('[themeRand:loader] selectPalette() returned null during auto-select.');
            }
        }
    };

    // -----------------------------------------------------------------------
    // Public: loadAll
    // -----------------------------------------------------------------------

    _i.fn.loadAll = function loadAll() {
        var S  = _i.state;
        var C  = _i.constants;
        var fn = _i.fn;

        // Immediate client-side context
        S.context = fn.inferContext();

        // Enrich with server context (non-blocking, best effort)
        fetch(C.CONTEXT_API, {
            headers: { 'X-Timezone': S.context.timezone }
        })
        .then(function (r) { return r.ok ? r.json() : null; })
        .then(function (d) {
            if (d) {
                S.context = Object.assign({}, S.context, d);
                if (_i.dom.contextBar) { fn.renderContextBar(); }
            }
        })
        .catch(function (e) {
            console.warn('[themeRand:loader] Context API unavailable (non-fatal):', e);
        });

        // Parallel: theme JSON + stylesheet list
        var pRand = fetch(C.RANDOMIZER_DATA)
            .then(function (r) {
                if (!r.ok) { throw new Error('HTTP ' + r.status); }
                return r.json();
            })
            .catch(function (e) {
                console.error('[themeRand:loader] Failed to load ' + C.RANDOMIZER_DATA + ':', e);
                return null;
            });

        var pSheets = fetch(C.SHEETS_API)
            .then(function (r) {
                if (!r.ok) { throw new Error('HTTP ' + r.status); }
                return r.json();
            })
            .catch(function (e) {
                console.error('[themeRand:loader] Failed to load ' + C.SHEETS_API + ':', e);
                return { stylesheets: [] };
            });

        Promise.all([pRand, pSheets])
            .then(function (results) {
                S.data   = results[0];
                S.sheets = (results[1] && results[1].stylesheets) ? results[1].stylesheets : [];

                if (!S.data) {
                    console.error('[themeRand:loader] Randomizer data is null -- UI may be limited.');
                }

                fn.buildUI();
                fn.restoreOrRandomize();
            })
            .catch(function (e) {
                console.error('[themeRand:loader] Promise.all failed in loadAll():', e);
            });
    };

    // -----------------------------------------------------------------------
    // Internal: build all UI components
    // -----------------------------------------------------------------------

    _i.fn.buildUI = function buildUI() {
        _i.fn.buildContextBar();
        _i.fn.buildDrawer();
    };

    console.log('[themeRand:loader] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
