/**
 * Theme Randomizer Module -- LiveCSS.themeRand
 *
 * Responsibilities:
 *   1. Fetch ai/theme-randomizer.json for color palettes and UX guidance.
 *   2. Fetch data/stylesheets.php?json to auto-discover available CSS themes
 *      from the style-sheets/ directory. New files there appear automatically.
 *   3. Infer locale, cultural region, and time-of-day from browser APIs.
 *   4. If no user preference is stored, select the best-fit palette.
 *   5. Apply palette color custom properties to :root.
 *   6. Render a compact floating context bar (locale, time, theme) and a
 *      settings drawer for manual selection plus the randomize action.
 *   7. Persist the user selection in localStorage.
 *   8. Expose a public API via LiveCSS.themeRand.
 *
 * No external dependencies.  Requires modern browsers supporting fetch,
 * Intl, CSS custom properties, and localStorage.
 */

'use strict';

(function (LiveCSS) {

    // -----------------------------------------------------------------------
    // Endpoints
    // -----------------------------------------------------------------------

    var RANDOMIZER_DATA = 'ai/theme-randomizer.json';
    var SHEETS_API      = 'data/stylesheets.php?json';
    var CONTEXT_API     = 'ai/context-time.php';

    // -----------------------------------------------------------------------
    // Storage key
    // -----------------------------------------------------------------------

    var SK_PALETTE   = 'lc-rand-palette';
    var SK_SHEET     = 'lc-rand-sheet';
    var SK_OPEN      = 'lc-rand-open';

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    var data       = null;   // parsed theme-randomizer.json
    var sheets     = [];     // array from stylesheets.php
    var context    = null;   // object from context-time.php or inferred
    var activeSheet   = '';  // currently applied stylesheet filename
    var activePalette = '';  // currently applied palette id
    var panelOpen  = false;

    // -----------------------------------------------------------------------
    // DOM references
    // -----------------------------------------------------------------------

    var dom = {};

    // -----------------------------------------------------------------------
    // Locale / cultural inference (client-side, no server call required)
    // -----------------------------------------------------------------------

    /**
     * Return a context descriptor built entirely from browser APIs.
     * The server endpoint (context-time.php) provides richer data but this
     * function works offline or when the server is not available.
     */
    function inferContext() {
        var locale   = navigator.language || 'en-US';
        var parts    = locale.split('-');
        var lang     = parts[0].toLowerCase();
        var region   = (parts[1] || '').toUpperCase();
        var tz       = 'UTC';
        try { tz = Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC'; } catch (e) {}

        var now         = new Date();
        var hour        = now.getHours();
        var timeOfDay   = resolveTimeOfDay(hour);
        var culturalArea = resolveCulturalArea(lang, region);

        return {
            locale:       locale,
            lang:         lang,
            region:       region,
            timezone:     tz,
            time_of_day:  timeOfDay,
            cultural_area: culturalArea,
            direction:    resolveDirection(lang),
            date_long:    now.toLocaleDateString(locale, { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' }),
            time_24:      now.toLocaleTimeString(locale, { hour12: false, hour: '2-digit', minute: '2-digit' }),
        };
    }

    function resolveTimeOfDay(hour) {
        if (hour >= 5  && hour < 12) { return 'Morning'; }
        if (hour >= 12 && hour < 17) { return 'Afternoon'; }
        if (hour >= 17 && hour < 21) { return 'Evening'; }
        return 'Night';
    }

    function resolveDirection(lang) {
        var rtl = ['ar', 'he', 'fa', 'ur', 'ps', 'dv', 'yi'];
        return rtl.indexOf(lang) !== -1 ? 'rtl' : 'ltr';
    }

    function resolveCulturalArea(lang, region) {
        var eastAsian  = ['zh', 'ja', 'ko'];
        var southAsian = ['hi', 'ur', 'bn', 'ta', 'te', 'mr', 'gu', 'kn', 'ml', 'pa', 'si'];
        var rtlLangs   = ['ar', 'he', 'fa', 'ps', 'dv', 'yi'];
        var nordic     = ['sv', 'no', 'nb', 'nn', 'da', 'fi', 'is', 'et', 'lv', 'lt'];
        var slavic     = ['ru', 'pl', 'cs', 'sk', 'uk', 'bg', 'sr', 'hr', 'sl', 'mk', 'bs', 'be'];
        var latin      = ['es', 'pt', 'fr', 'it', 'ro', 'ca', 'gl', 'oc'];
        var germanic   = ['de', 'nl', 'af', 'lb'];
        var latamCarib = ['MX','GT','BZ','HN','SV','NI','CR','PA','CU','DO','PR','JM','TT','HT'];
        var latamSouth = ['AR','BO','BR','CL','CO','EC','PE','PY','UY','VE'];
        var australas  = ['AU','NZ'];

        if (eastAsian.indexOf(lang)  !== -1) { return 'East Asia'; }
        if (southAsian.indexOf(lang) !== -1) { return 'South Asia'; }
        if (rtlLangs.indexOf(lang)   !== -1) { return 'Middle East / North Africa'; }
        if (nordic.indexOf(lang)     !== -1) { return 'Northern Europe'; }
        if (slavic.indexOf(lang)     !== -1) { return 'Eastern Europe'; }
        if (germanic.indexOf(lang)   !== -1) { return 'Western Europe / Germanic'; }
        if (latin.indexOf(lang) !== -1) {
            if (latamCarib.indexOf(region) !== -1) { return 'Latin America / Caribbean'; }
            if (latamSouth.indexOf(region) !== -1) { return 'Latin America / South'; }
            return 'Western Europe / Latin';
        }
        if (australas.indexOf(region) !== -1) { return 'Australasia'; }
        if (region === 'IN') { return 'South Asia'; }
        return 'North America / English';
    }

    // -----------------------------------------------------------------------
    // Palette selection logic
    // -----------------------------------------------------------------------

    /**
     * Pick the best palette based on cultural region and time of day.
     * Falls back to a random item from all palettes if no match found.
     */
    function selectPalette(ctx) {
        if (!data || !data.palettes || !data.palettes.length) { return null; }

        var culturalAdj = data.cultural_adjustments && data.cultural_adjustments[ctx.cultural_area];
        var preferred   = culturalAdj ? (culturalAdj.preferred_palettes || []) : [];

        // Filter palettes that match both cultural affinity and time of day
        var candidates = data.palettes.filter(function (p) {
            var cAff  = p.cultural_affinity || [];
            var times = p.time_of_day || [];
            var cMatch = cAff.indexOf(ctx.cultural_area) !== -1 || cAff.indexOf('global') !== -1;
            var tMatch = times.length === 0 || times.indexOf(ctx.time_of_day) !== -1;
            return cMatch && tMatch;
        });

        // Prefer items that are in the cultural preferred list
        var preferred_filtered = candidates.filter(function (p) {
            return preferred.indexOf(p.id) !== -1;
        });

        var pool = preferred_filtered.length ? preferred_filtered : candidates;

        if (!pool.length) {
            pool = data.palettes;
        }

        return pool[Math.floor(Math.random() * pool.length)];
    }

    /**
     * Apply a palette's CSS custom property set to :root.
     */
    function applyPalette(palette) {
        if (!palette || !palette.vars) { return; }
        var root = document.documentElement;
        Object.keys(palette.vars).forEach(function (prop) {
            root.style.setProperty(prop, palette.vars[prop]);
        });
        activePalette = palette.id;
        if (dom.paletteSelect) { dom.paletteSelect.value = palette.id; }
        if (dom.contextBar)    { updateContextBarPalette(palette); }
    }

    /**
     * Find a palette by id in the loaded data.
     */
    function findPalette(id) {
        if (!data || !data.palettes) { return null; }
        for (var i = 0; i < data.palettes.length; i++) {
            if (data.palettes[i].id === id) { return data.palettes[i]; }
        }
        return null;
    }

    // -----------------------------------------------------------------------
    // Stylesheet application
    // -----------------------------------------------------------------------

    var styleLink = null;

    /**
     * Swap the active application stylesheet.
     * Creates a <link> element and inserts it into <head>; removes the previous one.
     */
    function applySheet(path, filename) {
        if (styleLink) {
            styleLink.parentNode && styleLink.parentNode.removeChild(styleLink);
        }
        if (!path) { styleLink = null; activeSheet = ''; return; }

        var link = document.createElement('link');
        link.rel  = 'stylesheet';
        link.href = path;
        document.head.appendChild(link);
        styleLink  = link;
        activeSheet = filename || path;

        if (dom.sheetSelect) { dom.sheetSelect.value = activeSheet; }
    }

    // -----------------------------------------------------------------------
    // Data loading
    // -----------------------------------------------------------------------

    function loadAll() {
        context = inferContext();    // immediate client-side context

        // Fetch server context to enrich (non-blocking, best effort)
        fetch(CONTEXT_API, {
            headers: { 'X-Timezone': context.timezone }
        })
        .then(function (r) { return r.ok ? r.json() : null; })
        .then(function (d) {
            if (d) {
                context = Object.assign({}, context, d);
                if (dom.contextBar) { renderContextBar(); }
            }
        })
        .catch(function () {});   // silently ignore if endpoint is unavailable

        var pRand = fetch(RANDOMIZER_DATA)
            .then(function (r) { return r.json(); })
            .catch(function () { return null; });

        var pSheets = fetch(SHEETS_API)
            .then(function (r) { return r.json(); })
            .catch(function () { return { stylesheets: [] }; });

        Promise.all([pRand, pSheets]).then(function (results) {
            data   = results[0];
            sheets = (results[1] && results[1].stylesheets) ? results[1].stylesheets : [];

            buildUI();
            restoreOrRandomize();
        });
    }

    function restoreOrRandomize() {
        var savedPalette = '';
        var savedSheet   = '';
        try {
            savedPalette = localStorage.getItem(SK_PALETTE) || '';
            savedSheet   = localStorage.getItem(SK_SHEET)   || '';
        } catch (e) {}

        // Apply stylesheet
        if (savedSheet) {
            var sheetEntry = sheets.find(function (s) { return s.name === savedSheet; });
            if (sheetEntry) { applySheet(sheetEntry.path, sheetEntry.name); }
        }

        // Apply palette
        if (savedPalette) {
            var pal = findPalette(savedPalette);
            if (pal) { applyPalette(pal); return; }
        }

        // No saved preference -- auto-select
        if (data) {
            var auto = selectPalette(context);
            if (auto) { applyPalette(auto); }
        }
    }

    // -----------------------------------------------------------------------
    // UI Construction
    // -----------------------------------------------------------------------

    function buildUI() {
        buildContextBar();
        buildDrawer();
    }

    /**
     * The context bar is a slim fixed strip at the bottom-left of the page.
     * It shows locale, time-of-day, and the active palette name.
     * Clicking it opens the settings drawer.
     */
    function buildContextBar() {
        var bar = mk('div', 'lc-ctx-bar');
        bar.id  = 'lcContextBar';

        var localeChip = mk('span', 'lc-ctx-chip lc-ctx-locale');
        localeChip.id  = 'lcCtxLocale';

        var timeChip = mk('span', 'lc-ctx-chip lc-ctx-time');
        timeChip.id  = 'lcCtxTime';

        var paletteChip = mk('span', 'lc-ctx-chip lc-ctx-palette');
        paletteChip.id  = 'lcCtxPalette';

        var randBtn = mk('button', 'lc-ctx-btn lc-rand-btn');
        randBtn.id       = 'lcRandBtn';
        randBtn.title    = 'Randomize palette';
        randBtn.textContent = 'Rand';

        var settingsBtn = mk('button', 'lc-ctx-btn lc-settings-btn');
        settingsBtn.id       = 'lcSettingsBtn';
        settingsBtn.title    = 'Theme settings';
        settingsBtn.textContent = 'Theme';

        bar.appendChild(localeChip);
        bar.appendChild(timeChip);
        bar.appendChild(paletteChip);
        bar.appendChild(randBtn);
        bar.appendChild(settingsBtn);
        document.body.appendChild(bar);

        dom.contextBar  = bar;
        dom.localeChip  = localeChip;
        dom.timeChip    = timeChip;
        dom.paletteChip = paletteChip;

        randBtn.addEventListener('click', function () {
            randomizePalette();
        });

        settingsBtn.addEventListener('click', function () {
            toggleDrawer();
        });

        renderContextBar();
    }

    function renderContextBar() {
        if (!context) { return; }
        if (dom.localeChip)  { dom.localeChip.textContent  = context.locale || 'en-US'; }
        if (dom.timeChip)    { dom.timeChip.textContent    = (context.time_of_day || '') + '  ' + (context.time_24 || ''); }
        updateContextBarPalette(findPalette(activePalette));
    }

    function updateContextBarPalette(palette) {
        if (dom.paletteChip) {
            dom.paletteChip.textContent = palette ? palette.name : 'Default';
        }
    }

    /**
     * The settings drawer slides in from the right.
     * It contains: stylesheet picker, color palette picker, and UX density info.
     */
    function buildDrawer() {
        var overlay = mk('div', 'lc-drawer-overlay');
        overlay.id  = 'lcDrawerOverlay';
        document.body.appendChild(overlay);

        var drawer = mk('div', 'lc-drawer');
        drawer.id  = 'lcDrawer';
        drawer.innerHTML = buildDrawerHTML();
        document.body.appendChild(drawer);

        dom.drawerOverlay = overlay;
        dom.drawer        = drawer;

        // Wire controls after building
        dom.sheetSelect   = drawer.querySelector('#lcSheetSelect');
        dom.paletteSelect = drawer.querySelector('#lcPaletteSelect');
        dom.drawerRandBtn = drawer.querySelector('#lcDrawerRand');
        dom.drawerClose   = drawer.querySelector('#lcDrawerClose');
        dom.contextDetail = drawer.querySelector('#lcContextDetail');
        dom.uxInfo        = drawer.querySelector('#lcUxInfo');

        populateSheetSelect();
        populatePaletteSelect();
        renderContextDetail();
        renderUxInfo();

        dom.sheetSelect.addEventListener('change', function () {
            var name = dom.sheetSelect.value;
            if (name === '') { applySheet('', ''); clearPref(SK_SHEET); return; }
            var entry = sheets.find(function (s) { return s.name === name; });
            if (entry) {
                applySheet(entry.path, entry.name);
                savePref(SK_SHEET, entry.name);
            }
        });

        dom.paletteSelect.addEventListener('change', function () {
            var id = dom.paletteSelect.value;
            if (!id) { return; }
            var pal = findPalette(id);
            if (pal) {
                applyPalette(pal);
                savePref(SK_PALETTE, pal.id);
            }
        });

        dom.drawerRandBtn.addEventListener('click', function () {
            randomizePalette();
        });

        dom.drawerClose.addEventListener('click', function () {
            closeDrawer();
        });

        overlay.addEventListener('click', function () {
            closeDrawer();
        });
    }

    function buildDrawerHTML() {
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
            '    <div class="lc-field-label">Current Context</div>',
            '    <div id="lcContextDetail" class="lc-ctx-detail"></div>',
            '  </div>',
            '  <div class="lc-drawer-section">',
            '    <div class="lc-field-label">UX Guidance</div>',
            '    <div id="lcUxInfo" class="lc-ux-info"></div>',
            '  </div>',
            '</div>',
        ].join('');
    }

    function populateSheetSelect() {
        if (!dom.sheetSelect) { return; }
        var sel = dom.sheetSelect;
        // Clear and re-populate preserving the "None" option
        while (sel.options.length > 1) { sel.remove(1); }
        sheets.forEach(function (s) {
            var o = document.createElement('option');
            o.value       = s.name;
            o.textContent = s.label;
            if (s.name === activeSheet) { o.selected = true; }
            sel.appendChild(o);
        });
    }

    function populatePaletteSelect() {
        if (!dom.paletteSelect || !data || !data.palettes) { return; }
        dom.paletteSelect.innerHTML = '';
        data.palettes.forEach(function (p) {
            var o = document.createElement('option');
            o.value       = p.id;
            o.textContent = p.name + ' -- ' + p.description.slice(0, 48);
            if (p.id === activePalette) { o.selected = true; }
            dom.paletteSelect.appendChild(o);
        });
    }

    function renderContextDetail() {
        if (!dom.contextDetail || !context) { return; }
        var rows = [
            ['Date',     context.date_long    || ''],
            ['Time',     context.time_24      || ''],
            ['Timezone', context.timezone     || ''],
            ['Locale',   context.locale       || ''],
            ['Region',   context.cultural_area || ''],
            ['Season',   context.season       || ''],
            ['Direction',context.direction    || 'ltr'],
        ];
        dom.contextDetail.innerHTML = rows.map(function (r) {
            return '<div class="lc-cd-row"><span class="lc-cd-key">' +
                   r[0] + '</span><span class="lc-cd-val">' + (r[1] || '--') + '</span></div>';
        }).join('');
    }

    function renderUxInfo() {
        if (!dom.uxInfo || !data || !data.ux_patterns) { return; }
        var pat  = data.ux_patterns;
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
    }

    // -----------------------------------------------------------------------
    // Drawer open / close
    // -----------------------------------------------------------------------

    function openDrawer() {
        panelOpen = true;
        dom.drawer.classList.add('lc-drawer-open');
        dom.drawerOverlay.classList.add('lc-overlay-open');
        try { localStorage.setItem(SK_OPEN, '1'); } catch (e) {}
        renderContextDetail();
    }

    function closeDrawer() {
        panelOpen = false;
        dom.drawer.classList.remove('lc-drawer-open');
        dom.drawerOverlay.classList.remove('lc-overlay-open');
        try { localStorage.removeItem(SK_OPEN); } catch (e) {}
    }

    function toggleDrawer() {
        if (panelOpen) { closeDrawer(); } else { openDrawer(); }
    }

    // -----------------------------------------------------------------------
    // Randomize
    // -----------------------------------------------------------------------

    function randomizePalette() {
        if (!data) { return; }
        var ctx = context || inferContext();
        var pal = selectPalette(ctx);
        if (pal) {
            applyPalette(pal);
            savePref(SK_PALETTE, pal.id);
            if (dom.paletteSelect) { dom.paletteSelect.value = pal.id; }
        }
    }

    // -----------------------------------------------------------------------
    // Persistence
    // -----------------------------------------------------------------------

    function savePref(key, value) {
        try { localStorage.setItem(key, value); } catch (e) {}
    }

    function clearPref(key) {
        try { localStorage.removeItem(key); } catch (e) {}
    }

    // -----------------------------------------------------------------------
    // Styles injection
    // Writes the lc-* component CSS directly into <head> so this module has
    // no external CSS dependency.  All color refs use the active palette vars.
    // -----------------------------------------------------------------------

    function injectStyles() {
        var id   = 'lcRandStyles';
        if (document.getElementById(id)) { return; }

        var css = [
            /* Context bar */
            '.lc-ctx-bar{',
            '  position:fixed;bottom:0;left:0;',
            '  display:flex;align-items:center;gap:4px;',
            '  padding:0 8px;height:26px;',
            '  background:var(--clr-bg-alt,#141620);',
            '  border-top:1px solid var(--clr-border,#2a2d3e);',
            '  border-right:1px solid var(--clr-border,#2a2d3e);',
            '  z-index:8000;',
            '  font-family:system-ui,-apple-system,sans-serif;',
            '}',
            '.lc-ctx-chip{',
            '  font-size:10px;',
            '  color:var(--clr-text-dim,#50536a);',
            '  padding:0 5px;',
            '  border-right:1px solid var(--clr-border,#2a2d3e);',
            '  white-space:nowrap;',
            '}',
            '.lc-ctx-btn{',
            '  height:20px;',
            '  padding:0 7px;',
            '  font-size:10px;',
            '  font-weight:600;',
            '  letter-spacing:0.04em;',
            '  text-transform:uppercase;',
            '  background:transparent;',
            '  border:1px solid var(--clr-border,#2a2d3e);',
            '  color:var(--clr-text-muted,#8890b0);',
            '  cursor:pointer;',
            '  font-family:inherit;',
            '  transition:color 100ms,border-color 100ms;',
            '}',
            '.lc-ctx-btn:hover{',
            '  color:var(--clr-accent,#3da8ff);',
            '  border-color:var(--clr-accent,#3da8ff);',
            '}',
            '.lc-rand-btn{',
            '  background:var(--clr-surface-alt,#22253a);',
            '}',

            /* Drawer overlay */
            '.lc-drawer-overlay{',
            '  position:fixed;inset:0;',
            '  background:var(--clr-overlay,rgba(0,0,0,0.60));',
            '  z-index:8001;',
            '  opacity:0;pointer-events:none;',
            '  transition:opacity 180ms;',
            '}',
            '.lc-overlay-open{opacity:1!important;pointer-events:auto!important;}',

            /* Drawer */
            '.lc-drawer{',
            '  position:fixed;top:0;right:0;bottom:0;',
            '  width:320px;',
            '  background:var(--clr-surface,#1a1c28);',
            '  border-left:1px solid var(--clr-border,#2a2d3e);',
            '  z-index:8002;',
            '  transform:translateX(100%);',
            '  transition:transform 240ms cubic-bezier(0.0,0.0,0.2,1);',
            '  display:flex;flex-direction:column;',
            '  font-family:system-ui,-apple-system,sans-serif;',
            '}',
            '.lc-drawer-open{transform:translateX(0)!important;}',
            '.lc-drawer-header{',
            '  display:flex;align-items:center;justify-content:space-between;',
            '  padding:0 14px;',
            '  height:40px;',
            '  border-bottom:1px solid var(--clr-border,#2a2d3e);',
            '  background:var(--clr-bg-alt,#141620);',
            '  flex-shrink:0;',
            '}',
            '.lc-drawer-title{',
            '  font-size:11px;font-weight:600;letter-spacing:0.06em;text-transform:uppercase;',
            '  color:var(--clr-text-muted,#8890b0);',
            '}',
            '.lc-drawer-body{',
            '  flex:1;overflow-y:auto;padding:14px;',
            '  display:flex;flex-direction:column;gap:16px;',
            '}',
            '.lc-drawer-section{display:flex;flex-direction:column;gap:6px;}',
            '.lc-field-label{',
            '  font-size:10px;font-weight:600;letter-spacing:0.06em;text-transform:uppercase;',
            '  color:var(--clr-text-dim,#50536a);',
            '}',
            '.lc-field-note{',
            '  font-size:10px;color:var(--clr-text-dim,#50536a);',
            '}',
            '.lc-select{',
            '  width:100%;height:30px;',
            '  background:var(--clr-surface-alt,#22253a);',
            '  border:1px solid var(--clr-border,#2a2d3e);',
            '  color:var(--clr-text,#e0e2f0);',
            '  font-size:12px;font-family:inherit;',
            '  padding:0 8px;',
            '  appearance:none;-webkit-appearance:none;',
            '  cursor:pointer;',
            '}',
            '.lc-select:focus{outline:2px solid var(--clr-accent,#3da8ff);outline-offset:0;}',
            '.lc-drawer-btn{',
            '  display:inline-flex;align-items:center;justify-content:center;',
            '  height:28px;padding:0 10px;',
            '  background:var(--clr-surface-alt,#22253a);',
            '  border:1px solid var(--clr-border,#2a2d3e);',
            '  color:var(--clr-text-muted,#8890b0);',
            '  font-size:11px;font-weight:600;letter-spacing:0.04em;',
            '  font-family:inherit;text-transform:uppercase;',
            '  cursor:pointer;',
            '  transition:border-color 100ms,color 100ms;',
            '}',
            '.lc-drawer-btn:hover{',
            '  border-color:var(--clr-accent,#3da8ff);color:var(--clr-accent,#3da8ff);',
            '}',

            /* Context detail rows */
            '.lc-ctx-detail{display:flex;flex-direction:column;gap:3px;}',
            '.lc-cd-row{display:flex;gap:6px;font-size:11px;}',
            '.lc-cd-key{',
            '  width:72px;flex-shrink:0;',
            '  color:var(--clr-text-dim,#50536a);',
            '  font-size:10px;letter-spacing:0.04em;text-transform:uppercase;',
            '}',
            '.lc-cd-val{color:var(--clr-text-muted,#8890b0);word-break:break-word;}',

            /* UX guidance */
            '.lc-ux-info{display:flex;flex-direction:column;gap:4px;}',
            '.lc-ux-section{',
            '  font-size:10px;font-weight:600;letter-spacing:0.05em;text-transform:uppercase;',
            '  color:var(--clr-accent,#3da8ff);margin-top:6px;',
            '}',
            '.lc-ux-rule{',
            '  font-size:11px;color:var(--clr-text-dim,#50536a);line-height:1.5;',
            '}',
        ].join('\n');

        var style    = document.createElement('style');
        style.id     = id;
        style.textContent = css;
        document.head.appendChild(style);
    }

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    function mk(tag, cls) {
        var el = document.createElement(tag);
        if (cls) { el.className = cls; }
        return el;
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    var themeRand = {

        /**
         * Initialize the module.
         * Call after DOM ready.  Options:
         *   randomizerData  -- path to theme-randomizer.json  (default: 'ai/theme-randomizer.json')
         *   sheetsApi       -- path to stylesheets scanner PHP (default: 'data/stylesheets.php?json')
         *   contextApi      -- path to context-time.php        (default: 'ai/context-time.php')
         */
        init: function (opts) {
            opts = opts || {};
            if (opts.randomizerData) { RANDOMIZER_DATA = opts.randomizerData; }
            if (opts.sheetsApi)      { SHEETS_API      = opts.sheetsApi;      }
            if (opts.contextApi)     { CONTEXT_API     = opts.contextApi;     }

            injectStyles();
            loadAll();
        },

        /** Trigger a new random palette selection. */
        randomize: randomizePalette,

        /** Manually apply a palette by id (e.g. 'night-ink'). */
        applyPalette: function (id) {
            var pal = findPalette(id);
            if (pal) {
                applyPalette(pal);
                savePref(SK_PALETTE, pal.id);
            }
        },

        /** Apply a stylesheet by name (slug without .css extension). */
        applySheet: function (name) {
            var entry = sheets.find(function (s) { return s.name === name; });
            if (entry) {
                applySheet(entry.path, entry.name);
                savePref(SK_SHEET, entry.name);
            }
        },

        /** Clear all persisted preferences and re-pick automatically. */
        resetPreferences: function () {
            clearPref(SK_PALETTE);
            clearPref(SK_SHEET);
            applySheet('', '');
            restoreOrRandomize();
        },

        openDrawer:  openDrawer,
        closeDrawer: closeDrawer,

        /** Return the currently inferred context descriptor object. */
        getContext: function () { return context; },

        /** Return all loaded palette descriptors. */
        getPalettes: function () { return data ? data.palettes : []; },

        /** Return all scanned stylesheet descriptors. */
        getSheets: function () { return sheets; },
    };

    LiveCSS.themeRand = themeRand;

}(window.LiveCSS = window.LiveCSS || {}));
