/**
 * theme_randomizer/ui-context-bar.js
 *
 * Builds and updates the slim fixed context bar at the bottom-left.
 * Clicking "Theme" opens the settings drawer.
 * Requires: constants.js, state.js, utils.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:ui-context-bar] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    // -----------------------------------------------------------------------
    // Build
    // -----------------------------------------------------------------------

    _i.fn.buildContextBar = function buildContextBar() {
        var dom = _i.dom;
        var fn  = _i.fn;
        try {
            var bar = fn.mk('div', 'lc-ctx-bar');
            bar.id  = 'lcContextBar';

            var localeChip = fn.mk('span', 'lc-ctx-chip lc-ctx-locale');
            localeChip.id  = 'lcCtxLocale';

            var timeChip = fn.mk('span', 'lc-ctx-chip lc-ctx-time');
            timeChip.id  = 'lcCtxTime';

            var paletteChip = fn.mk('span', 'lc-ctx-chip lc-ctx-palette');
            paletteChip.id  = 'lcCtxPalette';

            var nudgeChip = fn.mk('span', 'lc-ctx-chip lc-ctx-nudge');
            nudgeChip.id  = 'lcCtxNudge';
            nudgeChip.textContent = '';

            var randBtn = fn.mk('button', 'lc-ctx-btn lc-rand-btn');
            randBtn.id          = 'lcRandBtn';
            randBtn.title       = 'Randomize palette';
            randBtn.textContent = 'Rand';

            var nudgeBtn = fn.mk('button', 'lc-ctx-btn lc-nudge-btn');
            nudgeBtn.id         = 'lcNudgeBtn';
            nudgeBtn.title      = 'Nudge active theme colors slightly';
            nudgeBtn.textContent = 'Nudge';

            var aiPreviewBtn = fn.mk('button', 'lc-ctx-btn lc-ai-preview-btn');
            aiPreviewBtn.id         = 'lcAiPreviewBtn';
            aiPreviewBtn.title      = 'Ask AI to generate a unique modern preview and save a timestamped backup';
            aiPreviewBtn.textContent = 'AI Preview';

            var settingsBtn = fn.mk('button', 'lc-ctx-btn lc-settings-btn');
            settingsBtn.id         = 'lcSettingsBtn';
            settingsBtn.title      = 'Theme settings';
            settingsBtn.textContent = 'Theme';

            bar.appendChild(localeChip);
            bar.appendChild(timeChip);
            bar.appendChild(paletteChip);
            bar.appendChild(nudgeChip);
            bar.appendChild(randBtn);
            bar.appendChild(nudgeBtn);
            bar.appendChild(aiPreviewBtn);
            bar.appendChild(settingsBtn);
            document.body.appendChild(bar);

            dom.contextBar   = bar;
            dom.localeChip   = localeChip;
            dom.timeChip     = timeChip;
            dom.paletteChip  = paletteChip;
            dom.nudgeChip    = nudgeChip;
            dom.nudgeBtn     = nudgeBtn;
            dom.aiPreviewBtn = aiPreviewBtn;

            randBtn.addEventListener('click', function () {
                fn.randomizePalette();
            });

            settingsBtn.addEventListener('click', function () {
                fn.toggleDrawer();
            });

            nudgeBtn.addEventListener('click', function () {
                fn.nudgeActiveTheme();
            });

            aiPreviewBtn.addEventListener('click', function () {
                fn.requestAiPreview();
            });

            fn.renderContextBar();
        } catch (e) {
            console.error('[themeRand:ui-context-bar] buildContextBar() failed:', e);
        }
    };

    // -----------------------------------------------------------------------
    // Render / update helpers
    // -----------------------------------------------------------------------

    _i.fn.renderContextBar = function renderContextBar() {
        var S   = _i.state;
        var dom = _i.dom;
        var fn  = _i.fn;
        if (!S.context) { return; }
        try {
            if (dom.localeChip) { dom.localeChip.textContent = S.context.locale || 'en-US'; }
            if (dom.timeChip)   {
                dom.timeChip.textContent = (S.context.time_of_day || '') + '  ' + (S.context.time_24 || '');
            }
            fn.updateContextBarPalette(fn.findPalette(S.activePalette));
        } catch (e) {
            console.error('[themeRand:ui-context-bar] renderContextBar() failed:', e);
        }
    };

    _i.fn.updateContextBarPalette = function updateContextBarPalette(palette) {
        var dom = _i.dom;
        if (dom.paletteChip) {
            dom.paletteChip.textContent = palette ? palette.name : 'Default';
        }
    };

    console.log('[themeRand:ui-context-bar] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
