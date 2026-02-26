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
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.indentGuide = (function () {

    var GUIDE_CLASS = 'cm-indent-guide';
    var STORAGE_KEY = 'livecss-indent-guide';

    // Default options
    var defaults = {
        visible:      true,
        color:        '#5a41b4',
        opacity:      18,
        thickness:    1,
        style:        'solid',
        step:         2,
        rulerOn:      true,
        rulerCol:     96,
        rulerColor:   '#4d31bf',
        rulerOpacity: 30
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
        dynStyle.textContent = css;
    }

    // ── Attach to a CM instance ─────────────────────────────

    function attach(cm) {
        editors.push(cm);

        cm.on('renderLine', function (instance, line, el) {
            var charW = instance.defaultCharWidth();
            if (!charW || charW <= 0) return;

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
                        var guideCount = Math.floor(leadingCols / currentStep);
                        for (var g = 1; g <= guideCount; g++) {
                            var col  = g * currentStep;
                            var left = col * charW;
                            var span = document.createElement('span');
                            span.className = GUIDE_CLASS;
                            span.style.left = left + 'px';
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
