/**
 * color-swatch.js — Inline color swatches in CodeMirror editors
 * Attached to window.LiveCSS.colorSwatch
 *
 * Scans all three editors for color values (hex, rgb, hsl, named).
 * Renders a small diamond-shaped swatch before each color token.
 * Clicking the swatch opens a native color picker to edit the value.
 *
 * Call LiveCSS.colorSwatch.init() after editor.init().
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.colorSwatch = (function () {
    'use strict';

    // ── Color regex (hex, rgb/rgba, hsl/hsla, common named colors) ─

    var COLOR_RE = /#(?:[0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{4}|[0-9a-fA-F]{3})\b|rgba?\(\s*[\d.]+\s*,\s*[\d.]+\s*,\s*[\d.]+(?:\s*,\s*[\d.]+)?\s*\)|hsla?\(\s*[\d.]+\s*,\s*[\d.]+%\s*,\s*[\d.]+%(?:\s*,\s*[\d.]+)?\s*\)|\b(?:aqua|black|blue|fuchsia|gray|grey|green|lime|maroon|navy|olive|orange|purple|red|silver|teal|white|yellow|coral|salmon|crimson|gold|indigo|ivory|khaki|lavender|magenta|pink|plum|cyan|tan|violet|turquoise|beige|brown)\b/gi;

    // Matches var(--property-name) tokens so they can be resolved and swatched.
    var VAR_RE = /var\(\s*(--[a-zA-Z0-9_-]+)\s*\)/g;

    // Common named colors → hex
    var NAMED = {
        aqua:'#00ffff',black:'#000000',blue:'#0000ff',fuchsia:'#ff00ff',
        gray:'#808080',grey:'#808080',green:'#008000',lime:'#00ff00',
        maroon:'#800000',navy:'#000080',olive:'#808000',orange:'#ffa500',
        purple:'#800080',red:'#ff0000',silver:'#c0c0c0',teal:'#008080',
        white:'#ffffff',yellow:'#ffff00',coral:'#ff7f50',salmon:'#fa8072',
        crimson:'#dc143c',gold:'#ffd700',indigo:'#4b0082',ivory:'#fffff0',
        khaki:'#f0e68c',lavender:'#e6e6fa',magenta:'#ff00ff',pink:'#ffc0cb',
        plum:'#dda0dd',cyan:'#00ffff',tan:'#d2b48c',violet:'#ee82ee',
        turquoise:'#40e0d0',beige:'#f5f5dc',brown:'#a52a2a'
    };

    // ── Conversion helpers ────────────────────────────────────────

    function toHex2(n) {
        var h = Math.round(Math.max(0, Math.min(255, n))).toString(16);
        return h.length === 1 ? '0' + h : h;
    }

    function hue(p, q, t) {
        if (t < 0) { t += 1; }
        if (t > 1) { t -= 1; }
        if (t < 1 / 6) { return p + (q - p) * 6 * t; }
        if (t < 1 / 2) { return q; }
        if (t < 2 / 3) { return p + (q - p) * (2 / 3 - t) * 6; }
        return p;
    }

    function hslToHex(h, s, l) {
        // h: 0-360, s: 0-100, l: 0-100
        h = ((h % 360) + 360) % 360 / 360;
        s = s / 100;
        l = l / 100;
        var r, g, b;
        if (s === 0) {
            r = g = b = l;
        } else {
            var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            var p = 2 * l - q;
            r = hue(p, q, h + 1 / 3);
            g = hue(p, q, h);
            b = hue(p, q, h - 1 / 3);
        }
        return '#' + toHex2(r * 255) + toHex2(g * 255) + toHex2(b * 255);
    }

    function hexToHsl(hex) {
        var r = parseInt(hex.slice(1, 3), 16) / 255;
        var g = parseInt(hex.slice(3, 5), 16) / 255;
        var b = parseInt(hex.slice(5, 7), 16) / 255;
        var max = Math.max(r, g, b), min = Math.min(r, g, b);
        var h, s, l = (max + min) / 2;
        if (max === min) {
            h = s = 0;
        } else {
            var d = max - min;
            s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
            switch (max) {
                case r: h = (g - b) / d + (g < b ? 6 : 0); break;
                case g: h = (b - r) / d + 2; break;
                case b: h = (r - g) / d + 4; break;
            }
            h /= 6;
        }
        return { h: Math.round(h * 360), s: Math.round(s * 100), l: Math.round(l * 100) };
    }

    function colorToHex(str) {
        str = str.trim();

        // Named
        var low = str.toLowerCase();
        if (NAMED[low]) { return NAMED[low]; }

        // Hex 3: #rgb
        var m3 = str.match(/^#([0-9a-fA-F]{3})$/);
        if (m3) {
            var c = m3[1];
            return '#' + c[0]+c[0] + c[1]+c[1] + c[2]+c[2];
        }
        // Hex 4: #rgba (drop alpha)
        var m4 = str.match(/^#([0-9a-fA-F]{4})$/);
        if (m4) {
            var c4 = m4[1];
            return '#' + c4[0]+c4[0] + c4[1]+c4[1] + c4[2]+c4[2];
        }
        // Hex 6 or 8 (strip alpha from 8)
        var m6 = str.match(/^#([0-9a-fA-F]{6})([0-9a-fA-F]{2})?$/);
        if (m6) { return ('#' + m6[1]).toLowerCase(); }

        // rgb / rgba
        var rgb = str.match(/rgba?\(\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)/i);
        if (rgb) {
            return '#' + toHex2(+rgb[1]) + toHex2(+rgb[2]) + toHex2(+rgb[3]);
        }

        // hsl / hsla
        var hsl = str.match(/hsla?\(\s*([\d.]+)\s*,\s*([\d.]+)%\s*,\s*([\d.]+)%/i);
        if (hsl) {
            return hslToHex(+hsl[1], +hsl[2], +hsl[3]);
        }

        return null;
    }

    // ── CSS variable map builder ──────────────────────────────────
    // Scans every line of a CM instance for  --prop: <color>;  declarations.
    // Returns a plain object keyed by property name, value = { hex, line }.
    // Only single-token color values are resolved; var()-valued vars are skipped
    // to avoid infinite-loop risk on circular references.

    function buildVarMap(cm) {
        var map = {};
        try {
            var count = cm.lineCount();
            for (var i = 0; i < count; i++) {
                var line = cm.getLine(i);
                if (!line) { continue; }
                // Match:  --property: <value>  (value may end at ; or end of line)
                var vm = line.match(/^\s*(--[a-zA-Z0-9_-]+)\s*:\s*(.+?)\s*(?:;.*)?$/);
                if (!vm) { continue; }
                var prop = vm[1];
                var val  = vm[2].trim();
                // Only store if the value resolves to a concrete color.
                // Ignore values that are themselves var() references.
                if (/^var\(/.test(val)) { continue; }
                var hex = colorToHex(val);
                if (hex) { map[prop] = { hex: hex, line: i }; }
            }
        } catch (e) {
            console.error('[colorSwatch] buildVarMap error:', e);
        }
        return map;
    }

    // ── Widget factory (literal colors) ──────────────────────────
    // Shows a small floating panel right beside the diamond with a
    // visible large color swatch and a hex text field.
    // Avoids relying on the native browser color-picker dialog entirely.

    // Module-level enabled flag.  When false, all scans are no-ops and marks
    // are cleared.  Toggled externally via setEnabled().
    var enabled = true;

    // Registry of per-editor scan helpers so setEnabled() can reach them all.
    var allInstances = [];

    var activePopover  = null;
    var activeDiamond  = null;
    var popoverOpen    = false;
    var pendingRescan  = null;   // scan function to call after close
    var pendingCommit  = null;   // {hex, mark, cm} — written to editor on close

    function closePopover() {
        if (activePopover && activePopover.parentNode) {
            try { activePopover.parentNode.removeChild(activePopover); } catch (e) {
                console.warn('[colorSwatch] closePopover: removeChild failed:', e);
            }
        }
        activePopover = null;
        activeDiamond = null;
        popoverOpen   = false;

        // Commit the chosen color into the editor text
        if (pendingCommit) {
            var pc = pendingCommit;
            pendingCommit = null;
            try {
                if (pc.mark) {
                    var pos = pc.mark.find();
                    if (pos) {
                        pc.cm.replaceRange(pc.hex, pos.from, pos.to);
                        console.log('[colorSwatch] committed color', pc.hex);
                    } else {
                        console.warn('[colorSwatch] commit skipped -- mark was cleared before close');
                    }
                }
            } catch (e) {
                console.error('[colorSwatch] failed to commit color on close:', e);
            }
        }

        // Run the deferred rescan so the editor re-renders swatches
        if (pendingRescan) {
            var fn = pendingRescan;
            pendingRescan = null;
            setTimeout(fn, 60);
        }
    }

    function openPicker(diamond, hexVal, mark, cm) {
        closePopover();

        var rect = diamond.getBoundingClientRect();
        var hsl  = hexToHsl(hexVal);
        var H = hsl.h, S = hsl.s, L = hsl.l;
        var origHex = hexVal;   // the hex in the editor text right now

        var pop = document.createElement('div');
        pop.className = 'cm-color-popover';
        pop.id = '_cmColorPopover';

        // -- Header row: hex field + close button
        var head = document.createElement('div');
        head.className = 'cm-color-pop-head';

        var hexField = document.createElement('input');
        hexField.type       = 'text';
        hexField.className  = 'cm-color-pop-hex';
        hexField.value      = hexVal;
        hexField.maxLength  = 7;
        hexField.spellcheck = false;

        var closeBtn = document.createElement('button');
        closeBtn.className   = 'cm-color-pop-close';
        closeBtn.textContent = '\u00d7';
        closeBtn.title       = 'Close';

        head.appendChild(hexField);
        head.appendChild(closeBtn);

        // -- Preview swatch
        var preview = document.createElement('div');
        preview.className        = 'cm-color-pop-preview';
        preview.style.background = hexVal;

        // -- HSL slider rows (no native OS dialog)
        function makeRow(label, min, max, val, cls) {
            var row = document.createElement('div');
            row.className = 'cm-color-pop-row';
            var lbl = document.createElement('span');
            lbl.className   = 'cm-color-pop-label';
            lbl.textContent = label;
            var inp = document.createElement('input');
            inp.type      = 'range';
            inp.className = 'cm-color-pop-slider ' + cls;
            inp.min = min; inp.max = max; inp.value = val;
            row.appendChild(lbl);
            row.appendChild(inp);
            return { row: row, inp: inp };
        }

        var hr = makeRow('H', 0, 360, H, 'cm-pop-h');
        var sr = makeRow('S', 0, 100, S, 'cm-pop-s');
        var lr = makeRow('L', 0, 100, L, 'cm-pop-l');

        pop.appendChild(head);
        pop.appendChild(preview);
        pop.appendChild(hr.row);
        pop.appendChild(sr.row);
        pop.appendChild(lr.row);

        document.body.appendChild(pop);
        activePopover = pop;
        activeDiamond = diamond;
        popoverOpen   = true;

        var pw = pop.offsetWidth  || 210;
        var ph = pop.offsetHeight || 140;
        var left = rect.right + 8;
        var top  = rect.top  - 4;
        if (left + pw > window.innerWidth  - 8) { left = rect.left - pw - 8; }
        if (top  + ph > window.innerHeight - 8) { top  = window.innerHeight - ph - 8; }
        pop.style.left = Math.max(4, left) + 'px';
        pop.style.top  = Math.max(4, top)  + 'px';

        // -- Update slider track gradients to reflect current HSL
        function updateSliderBgs() {
            hr.inp.style.background =
                'linear-gradient(to right,' +
                'hsl(0,'   + S + '%,' + L + '%),' +
                'hsl(60,'  + S + '%,' + L + '%),' +
                'hsl(120,' + S + '%,' + L + '%),' +
                'hsl(180,' + S + '%,' + L + '%),' +
                'hsl(240,' + S + '%,' + L + '%),' +
                'hsl(300,' + S + '%,' + L + '%),' +
                'hsl(360,' + S + '%,' + L + '%)'  + ')';
            sr.inp.style.background =
                'linear-gradient(to right,' +
                'hsl(' + H + ',0%,'   + L + '%),' +
                'hsl(' + H + ',100%,' + L + '%))' ;
            lr.inp.style.background =
                'linear-gradient(to right,' +
                'hsl(' + H + ',' + S + '%,0%),'   +
                'hsl(' + H + ',' + S + '%,50%),'  +
                'hsl(' + H + ',' + S + '%,100%))';
        }

        // -- Live-update the preview iframe without touching the editor.
        // Use the exact character offset of this specific token so that
        // other properties sharing the same hex value are not affected.
        // livePreview patches the preview iframe user CSS style tag without
        // rebuilding the entire srcdoc. The mark position is recomputed fresh
        // on every call so it is always correct, even when updatePreview() has
        // rebuilt the iframe (and the style tag content) between two drag events.
        // This eliminates the previewCssLen / offset drift race condition.
        function livePreview(newHex) {
            try {
                var cssEd = LiveCSS.editor.getCssEditor && LiveCSS.editor.getCssEditor();
                if (!cssEd || cm !== cssEd) { return; }
                if (!mark) { console.warn('[colorSwatch] livePreview: no mark reference'); return; }
                var mpos = mark.find();
                if (!mpos) {
                    console.warn('[colorSwatch] livePreview: mark was cleared -- rescan may have run while picker was open');
                    return;
                }
                var from    = cm.indexFromPos(mpos.from);
                var to      = cm.indexFromPos(mpos.to);
                var curCss  = cm.getValue();
                var patched = curCss.slice(0, from) + newHex + curCss.slice(to);
                if (LiveCSS.editor.setPreviewCss) {
                    LiveCSS.editor.setPreviewCss(patched);
                } else {
                    console.warn('[colorSwatch] LiveCSS.editor.setPreviewCss not available -- upgrade editor.js');
                }
            } catch (e) {
                console.error('[colorSwatch] livePreview failed:', e);
            }
        }

        function applyColor() {
            var hex = hslToHex(H, S, L);
            hexField.value           = hex;
            preview.style.background = hex;
            diamond.style.background = hex;
            updateSliderBgs();
            livePreview(hex);
            // Stage for commit on close
            pendingCommit = { hex: hex, mark: mark, cm: cm };
        }

        updateSliderBgs();

        hr.inp.addEventListener('input', function () { H = +hr.inp.value; applyColor(); });
        sr.inp.addEventListener('input', function () { S = +sr.inp.value; applyColor(); });
        lr.inp.addEventListener('input', function () { L = +lr.inp.value; applyColor(); });

        // -- Manual hex field
        hexField.addEventListener('input', function () {
            var v = hexField.value.trim();
            if (/^#[0-9a-fA-F]{6}$/.test(v)) {
                var p = hexToHsl(v);
                H = p.h; S = p.s; L = p.l;
                hr.inp.value = H; sr.inp.value = S; lr.inp.value = L;
                preview.style.background = v;
                diamond.style.background = v;
                updateSliderBgs();
                livePreview(v);
                pendingCommit = { hex: v, mark: mark, cm: cm };
            }
        });
        hexField.addEventListener('keydown', function (e) {
            if (e.keyCode === 13) {
                var v = hexField.value.trim();
                if (/^#[0-9a-fA-F]{6}$/.test(v)) {
                    pendingCommit = { hex: v, mark: mark, cm: cm };
                }
                closePopover();
            } else if (e.keyCode === 27) {
                pendingCommit = null;  // discard changes
                closePopover();
            }
        });

        // -- Close button
        closeBtn.addEventListener('mousedown', function (e) {
            e.preventDefault();
            closePopover();
        });
        // No outside-click listener — popover stays open until the user
        // clicks the diamond again (toggle), presses Escape, or clicks X.
    }

    // ── Widget factory ────────────────────────────────────────────

    function createWidget(colorStr, hexVal, cm) {
        var wrap = document.createElement('span');
        wrap.className = 'cm-color-wrap';

        // Diamond swatch
        var diamond = document.createElement('span');
        diamond.className = 'cm-color-diamond';
        diamond.style.background = hexVal;
        diamond.title = colorStr;

        // Original text
        var txt = document.createElement('span');
        txt.className   = 'cm-color-text';
        txt.textContent = colorStr;

        wrap.appendChild(diamond);
        wrap.appendChild(txt);

        diamond.addEventListener('mousedown', function (e) {
            e.preventDefault();
            e.stopPropagation();

            // Toggle: clicking the same diamond closes the popover
            if (activeDiamond === diamond) {
                closePopover();
                return;
            }

            var mark = wrap._cmMark;
            openPicker(diamond, hexVal, mark, cm);
        });

        return wrap;
    }

    // ── Widget factory (var() references) ────────────────────────
    // Shows a read-only swatch for a var(--property) token.
    // Click jumps to the line where the variable is defined.

    function createVarWidget(varStr, hexVal, defLine, cm) {
        var wrap = document.createElement('span');
        wrap.className = 'cm-color-wrap cm-color-wrap-var';

        var diamond = document.createElement('span');
        diamond.className = 'cm-color-diamond cm-color-diamond-var';
        diamond.style.background = hexVal;
        diamond.title = varStr + ' = ' + hexVal + '  (click to go to definition)';

        var txt = document.createElement('span');
        txt.className   = 'cm-color-text';
        txt.textContent = varStr;

        wrap.appendChild(diamond);
        wrap.appendChild(txt);

        diamond.addEventListener('mousedown', function (e) {
            e.preventDefault();
            e.stopPropagation();
            try {
                var pos = { line: defLine, ch: 0 };
                cm.setCursor(pos);
                cm.scrollIntoView(pos, 80);
                cm.focus();
                console.log('[colorSwatch] var() definition jump to line', defLine + 1);
            } catch (err) {
                console.error('[colorSwatch] var() definition jump failed:', err);
            }
        });

        return wrap;
    }

    // ── Per-editor scanner ────────────────────────────────────────

    function attachSwatches(cm) {
        // Per-line mark tracking: key = line number (string), value = mark[].
        // This lets viewport-change scans ADD marks for newly visible lines
        // WITHOUT clearing marks that are already visible, eliminating the
        // "blank flash" that appeared on every scroll debounce.
        var marksByLine = {};

        // Resolved CSS custom-property color map for this editor instance.
        // Rebuilt by fullScan so it always reflects the current editor content.
        var varMap = {};

        function clearLineMarks(lineNo) {
            var arr = marksByLine[String(lineNo)];
            if (!arr) { return; }
            for (var i = 0; i < arr.length; i++) {
                try { arr[i].clear(); } catch (e) {
                    console.warn('[colorSwatch] clearLineMarks: mark.clear() failed on line ' + lineNo + ':', e);
                }
            }
            delete marksByLine[String(lineNo)];
        }

        function clearAllMarks() {
            for (var key in marksByLine) {
                var arr = marksByLine[key];
                for (var i = 0; i < arr.length; i++) {
                    try { arr[i].clear(); } catch (e) {
                        console.warn('[colorSwatch] clearAllMarks: mark.clear() failed:', e);
                    }
                }
            }
            marksByLine = {};
        }

        function scanLine(lineNo) {
            // Refresh this single line: clear any existing marks then rebuild.
            clearLineMarks(lineNo);
            var text = cm.getLine(lineNo);
            if (!text) { return; }

            var lineMarks = [];

            // -- Literal colors (hex, rgb, hsl, named) --
            COLOR_RE.lastIndex = 0;
            var m;
            while ((m = COLOR_RE.exec(text)) !== null) {
                var colorStr = m[0];
                var hexVal   = colorToHex(colorStr);
                if (!hexVal) { continue; }

                var fromPos = { line: lineNo, ch: m.index };
                var toPos   = { line: lineNo, ch: m.index + colorStr.length };

                (function (cs, hv, fp, tp) {
                    try {
                        var widget = createWidget(cs, hv, cm);
                        var mark   = cm.markText(fp, tp, {
                            replacedWith:      widget,
                            handleMouseEvents: false,
                            inclusiveLeft:     false,
                            inclusiveRight:    false
                        });
                        widget._cmMark = mark;
                        lineMarks.push(mark);
                    } catch (e) {
                        console.error('[colorSwatch] markText failed at', fp, tp, e);
                    }
                })(colorStr, hexVal, fromPos, toPos);
            }

            // -- var(--property) references resolved through varMap --
            VAR_RE.lastIndex = 0;
            var vm;
            while ((vm = VAR_RE.exec(text)) !== null) {
                var varName  = vm[1];          // '--kb-key-highlight'
                var varStr   = vm[0];          // 'var(--kb-key-highlight)'
                var varEntry = varMap[varName];
                if (!varEntry) { continue; }   // unknown or non-color variable

                var vFrom = { line: lineNo, ch: vm.index };
                var vTo   = { line: lineNo, ch: vm.index + varStr.length };

                (function (vs, ve, fp, tp) {
                    try {
                        var widget = createVarWidget(vs, ve.hex, ve.line, cm);
                        var mark   = cm.markText(fp, tp, {
                            replacedWith:      widget,
                            handleMouseEvents: false,
                            inclusiveLeft:     false,
                            inclusiveRight:    false
                        });
                        widget._cmMark = mark;
                        lineMarks.push(mark);
                    } catch (e) {
                        console.error('[colorSwatch] markText (var) failed at', fp, tp, e);
                    }
                })(varStr, varEntry, vFrom, vTo);
            }

            if (lineMarks.length) {
                marksByLine[String(lineNo)] = lineMarks;
            }
        }

        // Full rescan: called after content changes.  Clears everything and
        // rebuilds marks for the current viewport.
        function fullScan() {
            if (!enabled) { return; }
            if (popoverOpen) { pendingRescan = fullScan; return; }
            // Rebuild the var map first so scanLine can resolve var() tokens.
            try {
                varMap = buildVarMap(cm);
            } catch (e) {
                console.error('[colorSwatch] fullScan: buildVarMap failed:', e);
                varMap = {};
            }
            clearAllMarks();
            try {
                var vp   = cm.getViewport();
                var from = Math.max(0, vp.from - 10);
                var to   = Math.min(cm.lineCount(), vp.to + 10);
                for (var ln = from; ln < to; ln++) {
                    scanLine(ln);
                }
            } catch (e) {
                console.error('[colorSwatch] fullScan error:', e);
            }
        }

        // Incremental viewport scan: called on scroll / viewportChange.
        // Only adds marks for newly visible lines; does NOT clear marks for
        // lines still on screen, so there is no visible flash.
        function viewportScan() {
            if (!enabled) { return; }
            if (popoverOpen) { pendingRescan = viewportScan; return; }
            try {
                var vp   = cm.getViewport();
                var from = Math.max(0, vp.from - 10);
                var to   = Math.min(cm.lineCount(), vp.to + 10);

                // Remove marks for lines now well outside the viewport so
                // we don't accumulate stale marks indefinitely.
                var evict = 30;
                for (var key in marksByLine) {
                    var n = parseInt(key, 10);
                    if (n < from - evict || n >= to + evict) {
                        clearLineMarks(n);
                    }
                }

                // Add marks for lines entering the viewport that are not yet marked.
                for (var ln = from; ln < to; ln++) {
                    if (!marksByLine[String(ln)]) {
                        scanLine(ln);
                    }
                }
            } catch (e) {
                console.error('[colorSwatch] viewportScan error:', e);
            }
        }

        var debouncedFullScan     = LiveCSS.utils.debounce(fullScan,     250);
        var debouncedViewportScan = LiveCSS.utils.debounce(viewportScan,  80);

        // Content changes need a full rescan (existing marks may be stale).
        cm.on('change', debouncedFullScan);
        // Viewport changes (including scroll) use the non-destructive incremental scan.
        // The 'scroll' listener is intentionally omitted -- 'viewportChange' covers it
        // and avoids redundant double-fires.
        cm.on('viewportChange', debouncedViewportScan);

        // Initial scan after editors have rendered.
        setTimeout(fullScan, 500);

        // Register this instance so the global setEnabled() can reach it.
        allInstances.push({ clearAll: clearAllMarks, fullScan: fullScan });
    }

    // ── Global enable / disable ───────────────────────────────────

    function setEnabled(val) {
        try {
            enabled = !!val;
            if (!enabled) {
                // Close any open popover so the user is not left with a dangling picker.
                try { closePopover(); } catch (e) { console.error('[colorSwatch] setEnabled: closePopover failed:', e); }
                // Remove all marks from every attached editor instance.
                allInstances.forEach(function (inst) {
                    try { inst.clearAll(); } catch (e) { console.error('[colorSwatch] setEnabled: clearAll failed:', e); }
                });
                console.log('[colorSwatch] widgets disabled -- all marks cleared');
            } else {
                // Trigger a full rescan on every attached editor instance.
                allInstances.forEach(function (inst) {
                    try { inst.fullScan(); } catch (e) { console.error('[colorSwatch] setEnabled: fullScan failed:', e); }
                });
                console.log('[colorSwatch] widgets enabled -- rescanning all editors');
            }
        } catch (e) {
            console.error('[colorSwatch] setEnabled error:', e);
        }
    }

    // ── Public ────────────────────────────────────────────────────

    function init() {
        if (!window.LiveCSS || !LiveCSS.editor) {
            console.error('[colorSwatch] LiveCSS.editor not available -- call init() after editor.init()');
            return;
        }
        var fns = ['getCssEditor', 'getHtmlEditor', 'getJsEditor'];
        fns.forEach(function (fn) {
            try {
                var cm = LiveCSS.editor[fn] && LiveCSS.editor[fn]();
                if (cm) {
                    attachSwatches(cm);
                } else {
                    console.warn('[colorSwatch] ' + fn + ' not available on LiveCSS.editor');
                }
            } catch (e) {
                console.error('[colorSwatch] failed to attach swatches via ' + fn + ':', e);
            }
        });
    }

    return {
        init:       init,
        setEnabled: setEnabled,
        colorToHex: colorToHex,
        hslToHex:   hslToHex
    };

}());
