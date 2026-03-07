/**
 * indent-guide.js — Custom indent guides for CodeMirror 5
 *
 * Draws subtle vertical guide lines ONLY within the leading whitespace
 * of each line. Provides a settings panel to change color, opacity,
 * thickness, style, step size, and toggle visibility.
 *
 * Usage:
 *   LiveCSS.indentGuide.attach(cmInstance)   — register an editor
 *   LiveCSS.indentGuide.init()               — wire up settings panel + button
 *
 * Attached to window.LiveCSS.indentGuide
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.indentGuide = (function () {

    var GUIDE_CLASS = 'cm-indent-guide';
    var STORAGE_KEY = 'livecss-indent-guide';

    // Default options
    var defaults = {
        visible:       true,
        color:         '#5a41b4',
        opacity:       18,
        thickness:     1,
        style:         'solid',
        step:          2,
        rulerOn:       true,
        rulerCol:      96,
        rulerColor:    '#4d31bf',
        rulerOpacity:  30,
        searchOutline: false
    };

    // Active options (merged with saved)
    var opts = {};

    // All attached CM instances
    var editors = [];

    // ── Persistence ─────────────────────────────────────────

    function loadOpts() {
        try {
            var raw = localStorage.getItem(STORAGE_KEY);
            if (raw) {
                var saved = JSON.parse(raw);
                for (var k in defaults) {
                    opts[k] = saved.hasOwnProperty(k) ? saved[k] : defaults[k];
                }
                return;
            }
        } catch (e) { /* ignore */ }
        for (var k in defaults) { opts[k] = defaults[k]; }
    }

    function saveOpts() {
        try { localStorage.setItem(STORAGE_KEY, JSON.stringify(opts)); } catch (e) { /* */ }
    }

    // ── Hex-to-rgba helper ──────────────────────────────────

    function hexToRgba(hex, a) {
        hex = hex.replace('#', '');
        var r = parseInt(hex.substring(0, 2), 16);
        var g = parseInt(hex.substring(2, 4), 16);
        var b = parseInt(hex.substring(4, 6), 16);
        return 'rgba(' + r + ',' + g + ',' + b + ',' + (a / 100) + ')';
    }

    // ── Build border-left value from current opts ───────────

    function borderVal() {
        return opts.thickness + 'px ' + opts.style + ' ' + hexToRgba(opts.color, opts.opacity);
    }

    // ── Force all editors to re-render with new guides ──────

    function refreshAll() {
        // Update CSS custom property on existing guide elements via a style rule
        updateDynamicStyle();
        editors.forEach(function (cm) { cm.refresh(); });
    }

    // Single <style> tag for dynamic guide appearance
    var dynStyle = null;
    var RULER_CLASS = 'cm-col-ruler';

    function rulerBorderVal() {
        return '1px solid ' + hexToRgba(opts.rulerColor, opts.rulerOpacity);
    }

    function updateDynamicStyle() {
        if (!dynStyle) {
            dynStyle = document.createElement('style');
            dynStyle.id = 'indent-guide-dynamic';
            document.head.appendChild(dynStyle);
        }
        var css = '';
        if (!opts.visible) {
            css += '.' + GUIDE_CLASS + ' { display: none !important; }\n';
        } else {
            css += '.' + GUIDE_CLASS + ' { border-left: ' + borderVal() + ' !important; display: block; }\n';
        }
        if (!opts.rulerOn) {
            css += '.' + RULER_CLASS + ' { display: none !important; }\n';
        } else {
            css += '.' + RULER_CLASS + ' { border-left: ' + rulerBorderVal() + ' !important; display: block; }\n';
        }
        // Search outline is handled via a body class toggle (search-outline-on)
        // so that static CSS rules in header.css do the visual work instead of
        // injecting outline rules here (more reliable across browsers and CSSOM
        // ordering).
        try {
            document.body.classList.toggle('search-outline-on', !!opts.searchOutline);
        } catch (e) {
            console.error('[indent-guide] updateDynamicStyle: body class toggle failed:', e);
        }
        dynStyle.textContent = css;
    }

    // ── Attach to a CM instance ─────────────────────────────

    function attach(cm) {
        editors.push(cm);

        // Cache wrap width outside of renderLine so we never call
        // getScrollInfo() (a forced DOM reflow) on every rendered line.
        // Updated on viewportChange, refresh, and panel resize events.
        var cachedWrapWidth = 0;
        // Cache char width and line height so we don't re-read them every
        // renderLine call either.
        var cachedCharW = 0;
        var cachedLineH = 0;

        function updateMetrics() {
            try {
                var info = cm.getScrollInfo();
                cachedWrapWidth = (info && info.clientWidth > 0) ? info.clientWidth : 0;
            } catch (e) {
                console.error('[indent-guide] updateMetrics (scrollInfo) error:', e);
            }
            try {
                var cw = cm.defaultCharWidth();
                if (cw && cw > 0) { cachedCharW = cw; }
            } catch (e) {
                console.error('[indent-guide] updateMetrics (charWidth) error:', e);
            }
            try {
                var lh = cm.defaultTextHeight();
                if (lh && lh > 0) { cachedLineH = lh; }
            } catch (e) {
                console.error('[indent-guide] updateMetrics (textHeight) error:', e);
            }
        }

        updateMetrics();

        // Update cached metrics on events that change the layout dimensions.
        // These do NOT fire on every scroll position tick so they are cheap.
        cm.on('viewportChange', updateMetrics);
        cm.on('refresh',        updateMetrics);
        // Also catch panel resize: the outer element fires a native resize.
        try {
            var scroller = cm.getScrollerElement && cm.getScrollerElement();
            if (scroller && window.ResizeObserver) {
                var ro = new ResizeObserver(function () { updateMetrics(); cm.refresh(); });
                ro.observe(scroller);
            }
        } catch (e) {
            console.warn('[indent-guide] ResizeObserver setup failed (non-fatal):', e);
        }

        cm.on('renderLine', function (instance, line, el) {
            try {
                var charW = cachedCharW;
                var lineH = cachedLineH;

                // Fallback: read live if cache is not yet populated.
                if (!charW || charW <= 0) {
                    charW = instance.defaultCharWidth();
                    if (!charW || charW <= 0) {
                        console.warn('[indent-guide] defaultCharWidth unavailable, skipping line');
                        return;
                    }
                }
                if (!lineH || lineH <= 0) {
                    lineH = instance.defaultTextHeight();
                    if (!lineH || lineH <= 0) {
                        console.warn('[indent-guide] defaultTextHeight unavailable, skipping line');
                        return;
                    }
                }

                // Guides must not render past the wrap boundary, otherwise they
                // appear over word-wrapped continuation text.  Use cached value;
                // no DOM read happens here at all.
                var wrapCutoff = cachedWrapWidth > 0 ? cachedWrapWidth : Infinity;

                // -- Indent guides --
                if (opts.visible) {
                    var text = line.text;
                    if (text.length) {
                        var tabSize = instance.getOption('tabSize') || 2;
                        var currentStep = opts.step || 2;

                        var leadingCols = 0;
                        for (var i = 0; i < text.length; i++) {
                            if (text[i] === ' ') {
                                leadingCols++;
                            } else if (text[i] === '\t') {
                                leadingCols += tabSize - (leadingCols % tabSize);
                            } else {
                                break;
                            }
                        }

                        if (leadingCols >= currentStep) {
                            // guideCount is already capped by this line's own
                            // indentation -- no extra maxIndent cutoff needed.
                            var guideCount = Math.floor(leadingCols / currentStep);
                            for (var g = 1; g <= guideCount; g++) {
                                var col  = g * currentStep;
                                var left = col * charW;

                                // Stop at the wrap boundary so guides never
                                // appear over wrapped continuation content.
                                if (wrapCutoff < Infinity && left >= wrapCutoff) { break; }

                                var span = document.createElement('span');
                                span.className = GUIDE_CLASS;
                                span.style.left = left + 'px';
                                span.style.height = lineH + 'px';
                                el.appendChild(span);
                            }
                        }
                    }
                }

                // -- Column ruler --
                if (opts.rulerOn && opts.rulerCol > 0) {
                    var rulerLeft = opts.rulerCol * charW;
                    var ruler = document.createElement('span');
                    ruler.className = RULER_CLASS;
                    ruler.style.left = rulerLeft + 'px';
                    el.appendChild(ruler);
                }
            } catch (e) {
                console.error('[indent-guide] renderLine error:', e);
            }
        });
    }

    // ── Settings panel wiring ───────────────────────────────

    function init() {
        loadOpts();
        updateDynamicStyle();

        var panel   = document.getElementById('guideTool');
        var btn     = document.getElementById('guidesBtn');
        var closeEl = document.getElementById('guideClose');

        if (!panel || !btn) return;

        // Toggle panel on button click
        btn.addEventListener('click', function () {
            panel.classList.toggle('hidden');
            saveGuideToolState(panel);
        });

        closeEl.addEventListener('click', function () {
            panel.classList.add('hidden');
            saveGuideToolState(panel);
        });

        // Make header draggable
        makeDraggable(panel, panel.querySelector('.guide-header'));

        // Restore saved visibility and position
        var savedUI = LiveCSS.storage.loadUIState() || {};
        if (savedUI.guideTool) {
            if (savedUI.guideTool.visible) {
                panel.classList.remove('hidden');
            }
            if (savedUI.guideTool.left) {
                panel.style.left  = savedUI.guideTool.left;
                panel.style.top   = savedUI.guideTool.top;
                panel.style.right = 'auto';
            }
        }

        // Bind controls
        var toggleEl    = document.getElementById('guideToggle');
        var colorEl     = document.getElementById('guideColor');
        var opacityEl   = document.getElementById('guideOpacity');
        var opacityVal  = document.getElementById('guideOpacityVal');
        var thickEl     = document.getElementById('guideThickness');
        var thickVal    = document.getElementById('guideThicknessVal');
        var styleEl     = document.getElementById('guideStyle');
        var stepEl      = document.getElementById('guideStep');

        // Sync controls to current opts
        if (toggleEl)   toggleEl.checked  = opts.visible;
        if (colorEl)    colorEl.value     = opts.color;
        if (opacityEl)  { opacityEl.value = opts.opacity; opacityVal.textContent = opts.opacity + '%'; }
        if (thickEl)    { thickEl.value   = opts.thickness; thickVal.textContent = opts.thickness + 'px'; }
        if (styleEl)    styleEl.value     = opts.style;
        if (stepEl)     stepEl.value      = String(opts.step);

        // Visible toggle
        toggleEl.addEventListener('change', function () {
            opts.visible = toggleEl.checked;
            saveOpts();
            refreshAll();
        });

        // Color
        colorEl.addEventListener('input', function () {
            opts.color = colorEl.value;
            saveOpts();
            refreshAll();
        });

        // Opacity
        opacityEl.addEventListener('input', function () {
            opts.opacity = parseInt(opacityEl.value, 10);
            opacityVal.textContent = opts.opacity + '%';
            saveOpts();
            refreshAll();
        });

        // Thickness
        thickEl.addEventListener('input', function () {
            opts.thickness = parseInt(thickEl.value, 10);
            thickVal.textContent = opts.thickness + 'px';
            saveOpts();
            refreshAll();
        });

        // Style
        styleEl.addEventListener('change', function () {
            opts.style = styleEl.value;
            saveOpts();
            refreshAll();
        });

        // Step — need to re-render lines, so do a full setValue-round-trip
        stepEl.addEventListener('change', function () {
            opts.step = parseInt(stepEl.value, 10);
            saveOpts();
            refreshAll();
        });

        // ── Column ruler controls ───────────────────────────
        var rulerToggle   = document.getElementById('guideRulerToggle');
        var rulerColEl    = document.getElementById('guideRulerCol');
        var rulerColorEl  = document.getElementById('guideRulerColor');
        var rulerOpEl     = document.getElementById('guideRulerOpacity');
        var rulerOpVal    = document.getElementById('guideRulerOpacityVal');

        if (rulerToggle) rulerToggle.checked = opts.rulerOn;
        if (rulerColEl)  rulerColEl.value    = opts.rulerCol;
        if (rulerColorEl) rulerColorEl.value = opts.rulerColor;
        if (rulerOpEl)   { rulerOpEl.value   = opts.rulerOpacity; rulerOpVal.textContent = opts.rulerOpacity + '%'; }

        rulerToggle.addEventListener('change', function () {
            opts.rulerOn = rulerToggle.checked;
            saveOpts();
            refreshAll();
        });

        rulerColEl.addEventListener('change', function () {
            var v = parseInt(rulerColEl.value, 10);
            if (isNaN(v) || v < 1) v = 96;
            opts.rulerCol = v;
            rulerColEl.value = v;
            saveOpts();
            refreshAll();
        });

        rulerColorEl.addEventListener('input', function () {
            opts.rulerColor = rulerColorEl.value;
            saveOpts();
            refreshAll();
        });

        rulerOpEl.addEventListener('input', function () {
            opts.rulerOpacity = parseInt(rulerOpEl.value, 10);
            rulerOpVal.textContent = opts.rulerOpacity + '%';
            saveOpts();
            refreshAll();
        });

        // ── Search outline control ──────────────────────────
        var searchOutlineEl = document.getElementById('guideSearchOutline');
        if (!searchOutlineEl) {
            console.warn('[indent-guide] guideSearchOutline checkbox not found in DOM');
        } else {
            try {
                searchOutlineEl.checked = !!opts.searchOutline;
            } catch (e) {
                console.error('[indent-guide] guideSearchOutline sync failed:', e);
            }
            searchOutlineEl.addEventListener('change', function () {
                try {
                    opts.searchOutline = searchOutlineEl.checked;
                    saveOpts();
                    updateDynamicStyle();
                    console.log('[indent-guide] searchOutline set to', opts.searchOutline);
                } catch (e) {
                    console.error('[indent-guide] guideSearchOutline change handler failed:', e);
                }
            });
        }
    }

    // ── Simple drag helper (same pattern as harmony tool) ───

    function makeDraggable(el, handle) {
        var ox, oy, sx, sy;
        handle.addEventListener('mousedown', function (e) {
            if (e.target.closest('button')) return;
            e.preventDefault();
            var rect = el.getBoundingClientRect();
            ox = e.clientX; oy = e.clientY;
            sx = rect.left; sy = rect.top;
            document.addEventListener('mousemove', onMove);
            document.addEventListener('mouseup', onUp);
        });
        function onMove(e) {
            el.style.left = (sx + e.clientX - ox) + 'px';
            el.style.top  = (sy + e.clientY - oy) + 'px';
            el.style.right = 'auto';
        }
        function onUp() {
            document.removeEventListener('mousemove', onMove);
            document.removeEventListener('mouseup', onUp);
            saveGuideToolState(el);
        }
    }

    function saveGuideToolState(panelEl) {
        var state = LiveCSS.storage.loadUIState() || {};
        state.guideTool = {
            visible: !panelEl.classList.contains('hidden'),
            left:    panelEl.style.left,
            top:     panelEl.style.top
        };
        LiveCSS.storage.saveUIState(state);
    }

    return { attach: attach, init: init };

}());
