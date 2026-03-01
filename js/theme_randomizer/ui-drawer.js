/**
 * theme_randomizer/ui-drawer.js
 *
 * Settings drawer: stylesheet picker, palette picker, nudge controls,
 * AI preview, context detail, UX guidance, and open/close logic.
 * Requires: constants.js, state.js, utils.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:ui-drawer] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    // -----------------------------------------------------------------------
    // Build
    // -----------------------------------------------------------------------

    _i.fn.buildDrawer = function buildDrawer() {
        var dom = _i.dom;
        var fn  = _i.fn;
        try {
            var overlay = fn.mk('div', 'lc-drawer-overlay');
            overlay.id  = 'lcDrawerOverlay';
            document.body.appendChild(overlay);

            var drawer = fn.mk('div', 'lc-drawer');
            drawer.id  = 'lcDrawer';
            drawer.innerHTML = fn.buildDrawerHTML();
            document.body.appendChild(drawer);

            dom.drawerOverlay = overlay;
            dom.drawer        = drawer;

            dom.sheetSelect      = drawer.querySelector('#lcSheetSelect');
            dom.paletteSelect    = drawer.querySelector('#lcPaletteSelect');
            dom.drawerRandBtn    = drawer.querySelector('#lcDrawerRand');
            dom.drawerNudgeBtn   = drawer.querySelector('#lcDrawerNudge');
            dom.drawerClearNudge = drawer.querySelector('#lcDrawerClearNudge');
            dom.drawerAiPreview  = drawer.querySelector('#lcDrawerAiPreview');
            dom.drawerClose      = drawer.querySelector('#lcDrawerClose');
            dom.contextDetail    = drawer.querySelector('#lcContextDetail');
            dom.uxInfo           = drawer.querySelector('#lcUxInfo');
            dom.nudgeStatus      = drawer.querySelector('#lcNudgeStatus');
            dom.aiPreviewStatus  = drawer.querySelector('#lcAiPreviewStatus');

            fn.populateSheetSelect();
            fn.populatePaletteSelect();
            fn.renderContextDetail();
            fn.renderUxInfo();

            // Guard each event-listener attachment
            if (dom.sheetSelect) {
                dom.sheetSelect.addEventListener('change', function () {
                    try {
                        var C    = _i.constants;
                        var S    = _i.state;
                        var name = dom.sheetSelect.value;
                        if (name === '') { fn.applySheet('', ''); fn.clearPref(C.SK_SHEET); return; }
                        var entry = S.sheets.find(function (s) { return s.name === name; });
                        if (entry) {
                            fn.applySheet(entry.path, entry.name);
                            fn.savePref(C.SK_SHEET, entry.name);
                        }
                    } catch (e) {
                        console.error('[themeRand:ui-drawer] sheetSelect change handler failed:', e);
                    }
                });
            }

            if (dom.paletteSelect) {
                dom.paletteSelect.addEventListener('change', function () {
                    try {
                        var C  = _i.constants;
                        var id = dom.paletteSelect.value;
                        if (!id) { return; }
                        var pal = fn.findPalette(id);
                        if (pal) {
                            fn.applyPalette(pal);
                            fn.savePref(C.SK_PALETTE, pal.id);
                        }
                    } catch (e) {
                        console.error('[themeRand:ui-drawer] paletteSelect change handler failed:', e);
                    }
                });
            }

            if (dom.drawerRandBtn) {
                dom.drawerRandBtn.addEventListener('click', function () { fn.randomizePalette(); });
            }

            if (dom.drawerNudgeBtn) {
                dom.drawerNudgeBtn.addEventListener('click', function () { fn.nudgeActiveTheme(); });
            }

            if (dom.drawerClearNudge) {
                dom.drawerClearNudge.addEventListener('click', function () { fn.clearNudge(); });
            }

            if (dom.drawerAiPreview) {
                dom.drawerAiPreview.addEventListener('click', function () { fn.requestAiPreview(); });
            }

            if (dom.drawerClose) {
                dom.drawerClose.addEventListener('click', function () { fn.closeDrawer(); });
            }

            overlay.addEventListener('click', function () { fn.closeDrawer(); });

        } catch (e) {
            console.error('[themeRand:ui-drawer] buildDrawer() failed:', e);
        }
    };

    _i.fn.buildDrawerHTML = function buildDrawerHTML() {
        return [
            '<div class="lc-drawer-header">',
            '  <span class="lc-drawer-title">Theme Settings</span>',
            '  <button class="lc-ctx-btn" id="lcDrawerClose">Close</button>',
            '</div>',
            '<div class="lc-drawer-body">',
            '  <div class="lc-drawer-section">',
            '    <div class="lc-field-label">Stylesheet</div>',
            '    <select id="lcSheetSelect" class="lc-select">',
            '      <option value="">None</option>',
            '    </select>',
            '    <div class="lc-field-note">Files are read from style-sheets/ automatically.</div>',
            '  </div>',
            '  <div class="lc-drawer-section">',
            '    <div class="lc-field-label">Color Palette</div>',
            '    <select id="lcPaletteSelect" class="lc-select"></select>',
            '    <button id="lcDrawerRand" class="lc-drawer-btn">Randomize Palette</button>',
            '  </div>',
            '  <div class="lc-drawer-section">',
            '    <div class="lc-field-label">Harmony Nudge</div>',
            '    <div class="lc-field-note">Apply a small color-harmony offset to the active theme variables. Each click produces a subtle variation that preserves the theme character.</div>',
            '    <button id="lcDrawerNudge" class="lc-drawer-btn">Nudge Theme</button>',
            '    <button id="lcDrawerClearNudge" class="lc-drawer-btn lc-drawer-btn-sm" style="margin-top:4px">Clear Nudge</button>',
            '    <div id="lcNudgeStatus" class="lc-field-note lc-nudge-status"></div>',
            '  </div>',
            '  <div class="lc-drawer-section">',
            '    <div class="lc-field-label">AI Unique Preview</div>',
            '    <div class="lc-field-note">Generate a unique, modern HTML preview page using the active theme and current nudge state. The result is auto-saved to style-sheets/backups/ with a timestamp.</div>',
            '    <button id="lcDrawerAiPreview" class="lc-drawer-btn">Generate + Backup Preview</button>',
            '    <div id="lcAiPreviewStatus" class="lc-field-note lc-ai-status"></div>',
            '  </div>',
            '  <div class="lc-drawer-section">',
            '    <div class="lc-field-label">Current Context</div>',
            '    <div id="lcContextDetail" class="lc-ctx-detail"></div>',
            '  </div>',
            '  <div class="lc-drawer-section">',
            '    <div class="lc-field-label">UX Guidance</div>',
            '    <div id="lcUxInfo" class="lc-ux-info"></div>',
            '  </div>',
            '</div>',
        ].join('');
    };

    // -----------------------------------------------------------------------
    // Populate selects
    // -----------------------------------------------------------------------

    _i.fn.populateSheetSelect = function populateSheetSelect() {
        var S   = _i.state;
        var dom = _i.dom;
        if (!dom.sheetSelect) { return; }
        try {
            var sel = dom.sheetSelect;
            while (sel.options.length > 1) { sel.remove(1); }
            S.sheets.forEach(function (s) {
                var o = document.createElement('option');
                o.value       = s.name;
                o.textContent = s.label;
                if (s.name === S.activeSheet) { o.selected = true; }
                sel.appendChild(o);
            });
        } catch (e) {
            console.error('[themeRand:ui-drawer] populateSheetSelect() failed:', e);
        }
    };

    _i.fn.populatePaletteSelect = function populatePaletteSelect() {
        var S   = _i.state;
        var dom = _i.dom;
        if (!dom.paletteSelect || !S.data || !S.data.palettes) { return; }
        try {
            dom.paletteSelect.innerHTML = '';
            S.data.palettes.forEach(function (p) {
                var o = document.createElement('option');
                o.value       = p.id;
                o.textContent = p.name + ' -- ' + p.description.slice(0, 48);
                if (p.id === S.activePalette) { o.selected = true; }
                dom.paletteSelect.appendChild(o);
            });
        } catch (e) {
            console.error('[themeRand:ui-drawer] populatePaletteSelect() failed:', e);
        }
    };

    // -----------------------------------------------------------------------
    // Info panels
    // -----------------------------------------------------------------------

    _i.fn.renderContextDetail = function renderContextDetail() {
        var S   = _i.state;
        var dom = _i.dom;
        if (!dom.contextDetail || !S.context) { return; }
        try {
            var rows = [
                ['Date',      S.context.date_long    || ''],
                ['Time',      S.context.time_24      || ''],
                ['Timezone',  S.context.timezone     || ''],
                ['Locale',    S.context.locale       || ''],
                ['Region',    S.context.cultural_area || ''],
                ['Season',    S.context.season       || ''],
                ['Direction', S.context.direction    || 'ltr'],
            ];
            dom.contextDetail.innerHTML = rows.map(function (r) {
                return '<div class="lc-cd-row"><span class="lc-cd-key">' + r[0] +
                       '</span><span class="lc-cd-val">' + (r[1] || '--') + '</span></div>';
            }).join('');
        } catch (e) {
            console.error('[themeRand:ui-drawer] renderContextDetail() failed:', e);
        }
    };

    _i.fn.renderUxInfo = function renderUxInfo() {
        var S   = _i.state;
        var dom = _i.dom;
        if (!dom.uxInfo || !S.data || !S.data.ux_patterns) { return; }
        try {
            var pat  = S.data.ux_patterns;
            var html = [];

            if (pat.typography && pat.typography.rules) {
                html.push('<div class="lc-ux-section">Typography</div>');
                pat.typography.rules.slice(0, 3).forEach(function (r) {
                    html.push('<p class="lc-ux-rule">' + r + '</p>');
                });
            }
            if (pat.color_usage && pat.color_usage.rules) {
                html.push('<div class="lc-ux-section">Color Usage</div>');
                pat.color_usage.rules.slice(0, 3).forEach(function (r) {
                    html.push('<p class="lc-ux-rule">' + r + '</p>');
                });
            }
            if (pat.accessibility && pat.accessibility.rules) {
                html.push('<div class="lc-ux-section">Accessibility</div>');
                pat.accessibility.rules.slice(0, 2).forEach(function (r) {
                    html.push('<p class="lc-ux-rule">' + r + '</p>');
                });
            }
            dom.uxInfo.innerHTML = html.join('');
        } catch (e) {
            console.error('[themeRand:ui-drawer] renderUxInfo() failed:', e);
        }
    };

    // -----------------------------------------------------------------------
    // Open / close / toggle
    // -----------------------------------------------------------------------

    _i.fn.openDrawer = function openDrawer() {
        var S   = _i.state;
        var dom = _i.dom;
        var C   = _i.constants;
        try {
            S.panelOpen = true;
            if (dom.drawer)        { dom.drawer.classList.add('lc-drawer-open'); }
            if (dom.drawerOverlay) { dom.drawerOverlay.classList.add('lc-overlay-open'); }
            try { localStorage.setItem(C.SK_OPEN, '1'); } catch (e) {}
            _i.fn.renderContextDetail();
        } catch (e) {
            console.error('[themeRand:ui-drawer] openDrawer() failed:', e);
        }
    };

    _i.fn.closeDrawer = function closeDrawer() {
        var S   = _i.state;
        var dom = _i.dom;
        var C   = _i.constants;
        try {
            S.panelOpen = false;
            if (dom.drawer)        { dom.drawer.classList.remove('lc-drawer-open'); }
            if (dom.drawerOverlay) { dom.drawerOverlay.classList.remove('lc-overlay-open'); }
            try { localStorage.removeItem(C.SK_OPEN); } catch (e) {}
        } catch (e) {
            console.error('[themeRand:ui-drawer] closeDrawer() failed:', e);
        }
    };

    _i.fn.toggleDrawer = function toggleDrawer() {
        var S = _i.state;
        if (S.panelOpen) { _i.fn.closeDrawer(); } else { _i.fn.openDrawer(); }
    };

    console.log('[themeRand:ui-drawer] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
