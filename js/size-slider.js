/**
 * size-slider.js — Inline size-value diamonds with slider popover
 * Attached to window.LiveCSS.sizeSlider
 *
 * Scans each editor for lines that contain a known CSS sizing property,
 * then marks every numeric value (with or without unit) with a diamond widget.
 * Clicking the diamond opens a slider popover for live adjustment.
 *
 * Call LiveCSS.sizeSlider.init() after editor.init().
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.sizeSlider = (function () {
    'use strict';

    /* ================================================================
       CSS properties that accept length / size / numeric values
       ================================================================ */
    var SIZE_PROPS = [
        /* box-model */
        'width', 'height', 'min-width', 'min-height', 'max-width', 'max-height',
        'padding', 'padding-top', 'padding-right', 'padding-bottom', 'padding-left',
        'padding-block', 'padding-block-start', 'padding-block-end',
        'padding-inline', 'padding-inline-start', 'padding-inline-end',
        'margin', 'margin-top', 'margin-right', 'margin-bottom', 'margin-left',
        'margin-block', 'margin-block-start', 'margin-block-end',
        'margin-inline', 'margin-inline-start', 'margin-inline-end',
        /* position */
        'top', 'right', 'bottom', 'left',
        'inset', 'inset-block', 'inset-block-start', 'inset-block-end',
        'inset-inline', 'inset-inline-start', 'inset-inline-end',
        /* logical size */
        'block-size', 'inline-size', 'min-block-size', 'min-inline-size',
        'max-block-size', 'max-inline-size',
        /* flex / grid */
        'flex', 'flex-basis', 'flex-grow', 'flex-shrink',
        'gap', 'row-gap', 'column-gap',
        'grid-template-columns', 'grid-template-rows',
        'grid-auto-columns', 'grid-auto-rows',
        'grid-gap', 'grid-column-gap', 'grid-row-gap',
        /* typography */
        'font-size', 'font', 'line-height', 'letter-spacing', 'word-spacing',
        'text-indent', 'text-decoration-thickness', 'text-underline-offset',
        /* border / outline */
        'border', 'border-width',
        'border-top-width', 'border-right-width',
        'border-bottom-width', 'border-left-width',
        'border-spacing', 'border-image-width',
        'outline-width', 'outline-offset',
        /* background */
        'background-size', 'background-position',
        'background-position-x', 'background-position-y',
        /* radius */
        'border-radius',
        'border-top-left-radius', 'border-top-right-radius',
        'border-bottom-left-radius', 'border-bottom-right-radius',
        /* shadow */
        'box-shadow', 'text-shadow',
        /* columns */
        'column-width', 'column-rule-width',
        /* transition / animation */
        'transition', 'transition-duration', 'transition-delay',
        'animation', 'animation-duration', 'animation-delay',
        /* scroll */
        'scroll-margin', 'scroll-margin-top', 'scroll-margin-right',
        'scroll-margin-bottom', 'scroll-margin-left',
        'scroll-padding', 'scroll-padding-top', 'scroll-padding-right',
        'scroll-padding-bottom', 'scroll-padding-left',
        /* transform */
        'perspective', 'translate', 'rotate',
        /* box-sizing (for inline declarations like `margin: 0; padding: 0;`) */
        'box-sizing',
        /* other */
        'z-index', 'order', 'opacity',
        'tab-size', 'shape-margin', 'object-position',
        'stroke-width', 'stroke-dashoffset', 'stroke-dasharray'
    ];

    /* Properties where bare numbers should stay unitless on output */
    var UNITLESS_PROPS_RE = /\b(flex|flex-grow|flex-shrink|line-height|z-index|order|opacity|font-weight|orphans|widows)\b/i;

    /* Build combined list: kebab-case + camelCase for JS support */
    var ALL_TERMS = SIZE_PROPS.slice();
    (function () {
        for (var i = 0; i < SIZE_PROPS.length; i++) {
            var camel = SIZE_PROPS[i].replace(/-([a-z])/g, function (_, c) {
                return c.toUpperCase();
            });
            if (camel !== SIZE_PROPS[i]) ALL_TERMS.push(camel);
        }
    })();

    /* Does the line mention any sizing property? */
    var PROP_RE = new RegExp('(?:' + ALL_TERMS.join('|') + ')', 'i');

    /* ── Regexes ────────────────────────────────────────────────── */

    /* Values WITH a CSS unit. (?!\w) instead of \b so % works. */
    var UNIT_RE = /(^|[\s:;,\(\/-])(-?\d+(?:\.\d+)?)(px|em|rem|%|vw|vh|vmin|vmax|ch|ex|cm|mm|in|pt|pc|fr|dvh|dvw|svh|svw|deg|turn|ms|s)(?!\w)/g;

    /* Bare numbers with NO unit — followed by value terminators */
    var BARE_RE = /(^|[\s:;,\(\/-])(-?\d+(?:\.\d+)?)(?=[\s;,\)}\]\/]|$)/g;

    /* ── Slider config per unit ─────────────────────────────────── */
    var UNIT_CONF = {
        'px':   { min: -200, max: 1000, step: 1 },
        'em':   { min: -10,  max: 30,   step: 0.05 },
        'rem':  { min: -10,  max: 30,   step: 0.05 },
        '%':    { min: -100, max: 300,   step: 1 },
        'vw':   { min: 0,    max: 100,  step: 1 },
        'vh':   { min: 0,    max: 100,  step: 1 },
        'vmin': { min: 0,    max: 100,  step: 1 },
        'vmax': { min: 0,    max: 100,  step: 1 },
        'ch':   { min: 0,    max: 100,  step: 1 },
        'ex':   { min: 0,    max: 100,  step: 1 },
        'cm':   { min: 0,    max: 50,   step: 0.1 },
        'mm':   { min: 0,    max: 500,  step: 1 },
        'in':   { min: 0,    max: 20,   step: 0.1 },
        'pt':   { min: 0,    max: 500,  step: 1 },
        'pc':   { min: 0,    max: 100,  step: 1 },
        'fr':   { min: 0,    max: 10,   step: 0.1 },
        'dvh':  { min: 0,    max: 100,  step: 1 },
        'dvw':  { min: 0,    max: 100,  step: 1 },
        'svh':  { min: 0,    max: 100,  step: 1 },
        'svw':  { min: 0,    max: 100,  step: 1 },
        'deg':  { min: 0,    max: 360,  step: 1 },
        'turn': { min: 0,    max: 1,    step: 0.01 },
        's':    { min: 0,    max: 10,   step: 0.01 },
        'ms':   { min: 0,    max: 5000, step: 10 },
        /* bare-number defaults */
        '':     { min: -100, max: 100,  step: 1 },
        '_px':  { min: -200, max: 1000, step: 1 }
    };

    /* ================================================================
       Helpers
       ================================================================ */

    /* Is a character index inside (unmatched) parentheses? */
    function insideParens(text, idx) {
        var depth = 0;
        for (var i = 0; i < idx; i++) {
            if (text.charCodeAt(i) === 40) depth++;       /* ( */
            else if (text.charCodeAt(i) === 41) depth--;  /* ) */
        }
        return depth > 0;
    }

    /* Does this line (or its owning property) take unitless numbers? */
    function isUnitlessPropLine(cm, lineNo, text) {
        if (UNITLESS_PROPS_RE.test(text)) return true;
        /* also check backwards if property is on a prior line */
        for (var i = lineNo - 1; i >= Math.max(0, lineNo - 12); i--) {
            var prev = cm.getLine(i);
            if (!prev) continue;
            if (/[;{}]/.test(prev)) {
                var tail = prev.slice(Math.max(
                    prev.lastIndexOf(';'),
                    prev.lastIndexOf('{'),
                    prev.lastIndexOf('}')
                ) + 1);
                return UNITLESS_PROPS_RE.test(tail);
            }
            if (UNITLESS_PROPS_RE.test(prev)) return true;
        }
        return false;
    }

    /* ================================================================
       Popover state
       ================================================================ */
    var activePopover = null;
    var activeDiamond = null;
    var popoverOpen   = false;
    var pendingRescan = null;
    var pendingCommit = null;

    function closePopover() {
        if (!popoverOpen) return;

        if (activePopover && activePopover.parentNode) {
            activePopover.parentNode.removeChild(activePopover);
        }
        activePopover = null;
        activeDiamond = null;
        popoverOpen   = false;

        if (pendingCommit) {
            var pc = pendingCommit;
            pendingCommit = null;
            try {
                var pos = pc.mark && pc.mark.find();
                if (pos) pc.cm.replaceRange(pc.newVal, pos.from, pos.to);
            } catch (e) { /* mark may have been cleared */ }
        }

        if (pendingRescan) {
            var fn = pendingRescan;
            pendingRescan = null;
            setTimeout(fn, 80);
        }
    }

    /* Escape = cancel (no commit) */
    document.addEventListener('keydown', function (e) {
        if (e.keyCode === 27 && popoverOpen) {
            pendingCommit = null;
            closePopover();
        }
    });

    /* Click outside = close & commit */
    document.addEventListener('mousedown', function (e) {
        if (!popoverOpen || !activePopover) return;
        if (activePopover.contains(e.target)) return;
        var t = e.target;
        while (t) {
            if (t.classList && t.classList.contains('cm-size-diamond')) return;
            t = t.parentNode;
        }
        closePopover();
    });

    /* ================================================================
       Slider popover
       ================================================================ */

    /**
     * @param {Element}  diamond
     * @param {string}   numStr    — original numeric text
     * @param {string}   unit      — original unit string ('' for bare numbers)
     * @param {boolean}  isBare    — true when the number had no unit
     * @param {boolean}  unitless  — true when the property is unitless (flex, z-index, etc.)
     * @param {object}   mark      — CodeMirror TextMarker
     * @param {object}   cm        — CodeMirror editor
     */
    function openSlider(diamond, numStr, unit, isBare, unitless, mark, cm) {
        closePopover();

        var rect = diamond.getBoundingClientRect();

        /* Choose slider config */
        var confKey = unit;
        if (isBare && !unitless) confKey = '_px';   /* bare on length prop → px range */
        if (isBare && unitless)  confKey = '';       /* bare on unitless prop → small range */
        var conf  = UNIT_CONF[confKey] || UNIT_CONF['px'];
        var cur   = parseFloat(numStr);
        var sMin  = conf.min;
        var sMax  = conf.max;
        var sStep = conf.step;
        if (cur < sMin) sMin = Math.floor(cur * 2);
        if (cur > sMax) sMax = Math.ceil(cur * 2);

        /* ── DOM ── */
        var pop = document.createElement('div');
        pop.className = 'cm-size-popover';

        var head = document.createElement('div');
        head.className = 'cm-size-pop-head';

        var valField = document.createElement('input');
        valField.type       = 'text';
        valField.className  = 'cm-size-pop-val';
        valField.value      = numStr;
        valField.spellcheck = false;

        /* Show the effective unit label */
        var displayUnit = unit;
        if (isBare && !unitless) displayUnit = 'px';
        if (isBare && unitless)  displayUnit = '';

        var unitLbl = document.createElement('span');
        unitLbl.className   = 'cm-size-pop-unit';
        unitLbl.textContent = displayUnit;

        var closeBtn = document.createElement('button');
        closeBtn.className   = 'cm-size-pop-close';
        closeBtn.textContent = '\u00d7';
        closeBtn.title       = 'Close';

        head.appendChild(valField);
        head.appendChild(unitLbl);
        head.appendChild(closeBtn);

        var row = document.createElement('div');
        row.className = 'cm-size-pop-row';

        var minLbl = document.createElement('span');
        minLbl.className   = 'cm-size-pop-range';
        minLbl.textContent = sMin;

        var slider = document.createElement('input');
        slider.type      = 'range';
        slider.className = 'cm-size-pop-slider';
        slider.min   = sMin;
        slider.max   = sMax;
        slider.step  = sStep;
        slider.value = cur;

        var maxLbl = document.createElement('span');
        maxLbl.className   = 'cm-size-pop-range';
        maxLbl.textContent = sMax;

        row.appendChild(minLbl);
        row.appendChild(slider);
        row.appendChild(maxLbl);

        pop.appendChild(head);
        pop.appendChild(row);
        document.body.appendChild(pop);

        activePopover = pop;
        activeDiamond = diamond;
        popoverOpen   = true;

        /* position next to diamond */
        var pw   = pop.offsetWidth  || 240;
        var ph   = pop.offsetHeight || 60;
        var left = rect.right + 8;
        var top  = rect.top - 4;
        if (left + pw > window.innerWidth  - 8) left = rect.left - pw - 8;
        if (top  + ph > window.innerHeight - 8) top  = window.innerHeight - ph - 8;
        pop.style.left = Math.max(4, left) + 'px';
        pop.style.top  = Math.max(4, top)  + 'px';

        /* ── live preview (CSS editor only) ── */
        var cssOffset = null;
        var cssLen    = null;
        try {
            var cssEd = LiveCSS.editor.getCssEditor && LiveCSS.editor.getCssEditor();
            if (cm === cssEd && mark) {
                var mp = mark.find();
                if (mp) {
                    cssOffset = cm.indexFromPos(mp.from);
                    cssLen    = cm.indexFromPos(mp.to) - cssOffset;
                }
            }
        } catch (ignore) { /* non-critical */ }

        function livePreview(newText) {
            if (cssOffset === null) return;
            try {
                var frame = document.getElementById('previewFrame');
                if (!frame) return;
                var fd = frame.contentDocument ||
                         (frame.contentWindow && frame.contentWindow.document);
                if (!fd) return;
                var st = fd.querySelector('style');
                if (!st) return;
                var css = st.textContent;
                st.textContent =
                    css.slice(0, cssOffset) + newText +
                    css.slice(cssOffset + cssLen);
                cssLen = newText.length;
            } catch (ignore) { /* sandbox */ }
        }

        function fmt(v) {
            if (sStep >= 1) return String(Math.round(v));
            var s = v.toFixed(2).replace(/\.?0+$/, '');
            return s || '0';
        }

        /**
         * Build the output string for committing:
         * - Has unit:   "14px", "1.5em", "100%"
         * - Bare + length prop:  "0" when zero, "10px" when nonzero
         * - Bare + unitless:     always bare number "2", "0", "0.5"
         */
        function buildOutput(v) {
            var num = fmt(v);
            if (!isBare) return num + unit;               /* normal unit value */
            if (unitless) return num;                      /* flex, z-index, etc. */
            /* bare number on a length property */
            if (parseFloat(num) === 0) return '0';         /* 0 stays bare */
            return num + 'px';                             /* nonzero gets px */
        }

        function apply(v) {
            var text = buildOutput(v);
            valField.value = fmt(v);
            livePreview(text);
            pendingCommit = { newVal: text, mark: mark, cm: cm };
        }

        slider.addEventListener('input', function () {
            apply(parseFloat(slider.value));
        });

        valField.addEventListener('input', function () {
            var v = parseFloat(valField.value);
            if (!isNaN(v)) { slider.value = v; apply(v); }
        });
        valField.addEventListener('keydown', function (e) {
            if (e.keyCode === 13) closePopover();
            else if (e.keyCode === 27) { pendingCommit = null; closePopover(); }
        });

        closeBtn.addEventListener('mousedown', function (e) {
            e.preventDefault();
            closePopover();
        });
    }

    /* ================================================================
       Widget factory — diamond + original text span
       ================================================================ */
    function createWidget(displayText, numStr, unit, isBare, unitless, cm) {
        var wrap = document.createElement('span');
        wrap.className = 'cm-size-wrap';

        var diamond = document.createElement('span');
        diamond.className = 'cm-size-diamond';

        var letter = document.createElement('span');
        letter.className   = 'cm-size-letter';
        letter.textContent = 'S';
        diamond.appendChild(letter);

        var txt = document.createElement('span');
        txt.className   = 'cm-size-text';
        txt.textContent = displayText;

        wrap.appendChild(diamond);
        wrap.appendChild(txt);

        diamond.addEventListener('mousedown', function (e) {
            e.preventDefault();
            e.stopPropagation();
            if (activeDiamond === diamond) { closePopover(); return; }
            openSlider(diamond, numStr, unit, isBare, unitless, wrap._cmMark, cm);
        });

        return wrap;
    }

    /* ================================================================
       Per-editor scanner
       ================================================================ */

    /** Does this line belong to a sizing-property declaration?
        Handles multi-line values by looking backwards. */
    function isSizeLine(cm, lineNo, text) {
        if (PROP_RE.test(text)) return true;
        for (var i = lineNo - 1; i >= Math.max(0, lineNo - 12); i--) {
            var prev = cm.getLine(i);
            if (!prev) continue;
            if (/[;{}]/.test(prev)) {
                var tail = prev.slice(Math.max(
                    prev.lastIndexOf(';'),
                    prev.lastIndexOf('{'),
                    prev.lastIndexOf('}')
                ) + 1);
                return PROP_RE.test(tail);
            }
            if (PROP_RE.test(prev)) return true;
        }
        return false;
    }

    function attachSizeSliders(cm) {
        var marks = [];

        function clearAll() {
            for (var i = 0; i < marks.length; i++) {
                try { marks[i].clear(); } catch (e) { /* */ }
            }
            marks = [];
        }

        function addMark(displayText, numStr, unit, isBare, unitless, fromPos, toPos) {
            var w  = createWidget(displayText, numStr, unit, isBare, unitless, cm);
            var mk = cm.markText(fromPos, toPos, {
                replacedWith:     w,
                handleMouseEvents: false,
                inclusiveLeft:    false,
                inclusiveRight:   false
            });
            w._cmMark = mk;
            marks.push(mk);
        }

        function scan() {
            if (popoverOpen) { pendingRescan = scan; return; }
            clearAll();

            var vp   = cm.getViewport();
            var from = Math.max(0, vp.from - 10);
            var to   = Math.min(cm.lineCount(), vp.to + 10);

            for (var ln = from; ln < to; ln++) {
                var text = cm.getLine(ln);
                if (!text) continue;
                if (!isSizeLine(cm, ln, text)) continue;

                var unitless = isUnitlessPropLine(cm, ln, text);

                /* ── Pass 1: values WITH a unit ── */
                var unitRanges = [];   /* track columns already marked */
                UNIT_RE.lastIndex = 0;
                var m;
                while ((m = UNIT_RE.exec(text)) !== null) {
                    var prefix = m[1];
                    var numStr = m[2];
                    var unit   = m[3];
                    var full   = numStr + unit;
                    var ch0    = m.index + prefix.length;
                    var ch1    = ch0 + full.length;

                    unitRanges.push({ from: ch0, to: ch1 });
                    addMark(full, numStr, unit, false, false,
                            { line: ln, ch: ch0 }, { line: ln, ch: ch1 });
                }

                /* ── Pass 2: bare numbers (no unit) ── */
                BARE_RE.lastIndex = 0;
                while ((m = BARE_RE.exec(text)) !== null) {
                    var bPrefix = m[1];
                    var bNum    = m[2];
                    var bCh0    = m.index + bPrefix.length;
                    var bCh1    = bCh0 + bNum.length;

                    /* Skip if already covered by a unit match */
                    var overlap = false;
                    for (var r = 0; r < unitRanges.length; r++) {
                        if (bCh0 < unitRanges[r].to && bCh1 > unitRanges[r].from) {
                            overlap = true;
                            break;
                        }
                    }
                    if (overlap) continue;

                    /* Skip numbers inside parentheses (rgb, rgba, calc args, var) */
                    if (insideParens(text, bCh0)) continue;

                    addMark(bNum, bNum, '', true, unitless,
                            { line: ln, ch: bCh0 }, { line: ln, ch: bCh1 });
                }
            }
        }

        var debounced = LiveCSS.utils.debounce(scan, 300);
        cm.on('change',         debounced);
        cm.on('viewportChange', debounced);
        cm.on('scroll',         debounced);

        /* initial paint */
        setTimeout(scan, 600);
    }

    /* ================================================================
       Public API
       ================================================================ */
    function init() {
        var ed = LiveCSS.editor;
        if (!ed) return;
        try { if (ed.getCssEditor)  attachSizeSliders(ed.getCssEditor());  } catch (e) { /* */ }
        try { if (ed.getHtmlEditor) attachSizeSliders(ed.getHtmlEditor()); } catch (e) { /* */ }
        try { if (ed.getJsEditor)   attachSizeSliders(ed.getJsEditor());   } catch (e) { /* */ }
    }

    return { init: init };

}());
