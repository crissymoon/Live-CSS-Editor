/**
 * theme_randomizer/index.js  --  LiveCSS.themeRand public API
 *
 * Assembles all themeRand sub-modules into the public LiveCSS.themeRand
 * object.  Must be loaded LAST, after all other theme_randomizer/*.js files.
 *
 * Required load order:
 *   1. theme_randomizer/constants.js
 *   2. theme_randomizer/state.js
 *   3. theme_randomizer/utils.js
 *   4. theme_randomizer/context.js
 *   5. theme_randomizer/palette.js
 *   6. theme_randomizer/sheets.js
 *   7. theme_randomizer/persistence.js
 *   8. theme_randomizer/ui-styles.js
 *   9. theme_randomizer/ui-context-bar.js
 *  10. theme_randomizer/ui-drawer.js
 *  11. theme_randomizer/randomize.js
 *  12. theme_randomizer/nudge.js
 *  13. theme_randomizer/ai-preview.js
 *  14. theme_randomizer/loader.js
 *  15. theme_randomizer/index.js    <-- this file
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;

    if (!_i) {
        console.error('[themeRand:index] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    // Verify required sub-modules registered their functions
    var required = [
        'inferContext', 'selectPalette', 'applyPalette', 'findPalette',
        'applySheet', 'savePref', 'clearPref', 'injectStyles',
        'buildContextBar', 'renderContextBar', 'updateContextBarPalette',
        'buildDrawer', 'buildDrawerHTML', 'openDrawer', 'closeDrawer', 'toggleDrawer',
        'randomizePalette', 'nudgeActiveTheme', 'clearNudge',
        'requestAiPreview', 'loadAll', 'buildUI', 'restoreOrRandomize',
    ];
    required.forEach(function (name) {
        if (typeof _i.fn[name] !== 'function') {
            console.error('[themeRand:index] Missing required function "' + name + '" -- check load order.');
        }
    });

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    var themeRand = {

        /**
         * Initialise the module.  Call after DOM ready.
         * Options:
         *   randomizerData -- path to theme-randomizer.json  (default: 'ai/theme-randomizer.json')
         *   sheetsApi      -- path to stylesheets scanner PHP (default: 'data/stylesheets.php?json')
         *   contextApi     -- path to context-time.php        (default: 'ai/context-time.php')
         */
        init: function (opts) {
            opts = opts || {};
            try {
                var C = _i.constants;
                if (opts.randomizerData) { C.RANDOMIZER_DATA = opts.randomizerData; }
                if (opts.sheetsApi)      { C.SHEETS_API      = opts.sheetsApi;      }
                if (opts.contextApi)     { C.CONTEXT_API     = opts.contextApi;     }

                _i.fn.injectStyles();
                _i.fn.loadAll();
            } catch (e) {
                console.error('[themeRand:index] init() failed:', e);
            }
        },

        /** Trigger a new random palette selection. */
        randomize: function () { _i.fn.randomizePalette(); },

        /** Nudge the active theme colors with small harmony offsets. */
        nudgeTheme: function () { _i.fn.nudgeActiveTheme(); },

        /** Clear the current harmony nudge and restore original theme variables. */
        clearNudge: function () { _i.fn.clearNudge(); },

        /** Ask the AI to generate a unique preview and save a timestamped backup. */
        requestAiPreview: function () { _i.fn.requestAiPreview(); },

        /** Manually apply a palette by id (e.g. 'night-ink'). */
        applyPalette: function (id) {
            try {
                var pal = _i.fn.findPalette(id);
                if (pal) {
                    _i.fn.applyPalette(pal);
                    _i.fn.savePref(_i.constants.SK_PALETTE, pal.id);
                } else {
                    console.warn('[themeRand] applyPalette(): palette "' + id + '" not found.');
                }
            } catch (e) {
                console.error('[themeRand] applyPalette() failed:', e);
            }
        },

        /** Apply a stylesheet by name (filename without path prefix). */
        applySheet: function (name) {
            try {
                var S = _i.state;
                var entry = S.sheets.find(function (s) { return s.name === name; });
                if (entry) {
                    _i.fn.applySheet(entry.path, entry.name);
                    _i.fn.savePref(_i.constants.SK_SHEET, entry.name);
                } else {
                    console.warn('[themeRand] applySheet(): sheet "' + name + '" not found in loaded list.');
                }
            } catch (e) {
                console.error('[themeRand] applySheet() failed:', e);
            }
        },

        /** Clear all persisted preferences and re-pick automatically. */
        resetPreferences: function () {
            try {
                var C = _i.constants;
                _i.fn.clearPref(C.SK_PALETTE);
                _i.fn.clearPref(C.SK_SHEET);
                _i.fn.applySheet('', '');
                _i.fn.restoreOrRandomize();
            } catch (e) {
                console.error('[themeRand] resetPreferences() failed:', e);
            }
        },

        openDrawer:  function () { _i.fn.openDrawer(); },
        closeDrawer: function () { _i.fn.closeDrawer(); },

        /** Return the currently inferred context descriptor. */
        getContext:  function () { return _i.state.context; },

        /** Return all loaded palette descriptors. */
        getPalettes: function () { return _i.state.data ? _i.state.data.palettes : []; },

        /** Return all scanned stylesheet descriptors. */
        getSheets:   function () { return _i.state.sheets; },
    };

    LiveCSS.themeRand = themeRand;

    console.log('[themeRand:index] LiveCSS.themeRand ready');

}(window.LiveCSS = window.LiveCSS || {}));
