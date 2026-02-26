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
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.colorSwatch = (function () {
    'use strict';

    // ── Color regex (hex, rgb/rgba, hsl/hsla, common named colors) ─

    var COLOR_RE = /#(?:[0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{4}|[0-9a-fA-F]{3})\b|rgba?\(\s*[\d.]+\s*,\s*[\d.]+\s*,\s*[\d.]+(?:\s*,\s*[\d.]+)?\s*\)|hsla?\(\s*[\d.]+\s*,\s*[\d.]+%\s*,\s*[\d.]+%(?:\s*,\s*[\d.]+)?\s*\)|\b(?:aqua|black|blue|fuchsia|gray|grey|green|lime|maroon|navy|olive|orange|purple|red|silver|teal|white|yellow|coral|salmon|crimson|gold|indigo|ivory|khaki|lavender|magenta|pink|plum|cyan|tan|violet|turquoise|beige|brown)\b/gi;

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

    // ── Inline color popover ─────────────────────────────────────
    // Shows a small floating panel right beside the diamond with a
    // visible large color swatch and a hex text field.
    // Avoids relying on the native browser color-picker dialog entirely.

    var activePopover = null;

    function closePopover() {
        if (activePopover && activePopover.parentNode) {
            activePopover.parentNode.removeChild(activePopover);
        }
        activePopover = null;
    }

    function openPicker(diamond, hexVal, onPick) {
        closePopover();

        var rect = diamond.getBoundingClientRect();

        var pop = document.createElement('div');
        pop.className = 'cm-color-popover';
        pop.id = '_cmColorPopover';

        // -- Header row: hex display + close button
        var head = document.createElement('div');
        head.className = 'cm-color-pop-head';

        var hexField = document.createElement('input');
        hexField.type      = 'text';
        hexField.className = 'cm-color-pop-hex';
        hexField.value     = hexVal;
        hexField.maxLength = 7;
        hexField.spellcheck = false;

        var closeBtn = document.createElement('button');
        closeBtn.className   = 'cm-color-pop-close';
        closeBtn.textContent = '\u00d7';  // ×
        closeBtn.title       = 'Close';

        head.appendChild(hexField);
        head.appendChild(closeBtn);

        // -- Large native color input (visible swatch area)
        var colorInp = document.createElement('input');
        colorInp.type      = 'color';
        colorInp.className = 'cm-color-pop-input';
        colorInp.value     = hexVal;

        pop.appendChild(head);
        pop.appendChild(colorInp);

        // Position beside the diamond
        document.body.appendChild(pop);
        activePopover = pop;

        var pw = pop.offsetWidth  || 180;
        var ph = pop.offsetHeight || 80;
        var left = rect.right + 8;
        var top  = rect.top  - 4;
        // Flip left if too close to right edge
        if (left + pw > window.innerWidth - 8) { left = rect.left - pw - 8; }
        // Flip up if too close to bottom
        if (top + ph > window.innerHeight - 8)  { top  = window.innerHeight - ph - 8; }
        pop.style.left = Math.max(4, left) + 'px';
        pop.style.top  = Math.max(4, top)  + 'px';

        // -- Live update from color wheel
        // Track when the native picker last closed so the outsideClick guard
        // doesn't fire immediately after the OS dialog is dismissed.
        var lastPickTime = 0;

        colorInp.addEventListener('input', function () {
            hexField.value = colorInp.value;
            onPick(colorInp.value, false);
        });
        // 'change' fires when the OS native picker dialog is dismissed.
        // DO NOT treat this as final (don't close the popover) — the user
        // should be able to re-open the picker as many times as they like.
        colorInp.addEventListener('change', function () {
            lastPickTime = Date.now();
            hexField.value = colorInp.value;
            onPick(colorInp.value, false);
        });

        // -- Manual hex field
        hexField.addEventListener('input', function () {
            var v = hexField.value.trim();
            if (/^#[0-9a-fA-F]{6}$/.test(v)) {
                colorInp.value = v;
                onPick(v, false);
            }
        });
        hexField.addEventListener('keydown', function (e) {
            if (e.keyCode === 13) {
                var v = hexField.value.trim();
                if (/^#[0-9a-fA-F]{6}$/.test(v)) { onPick(v, true); }
                closePopover();
            } else if (e.keyCode === 27) {
                closePopover();
            }
        });

        // -- Close button
        closeBtn.addEventListener('mousedown', function (e) {
            e.preventDefault();
            closePopover();
        });

        // -- Close on outside click.
        // Guard: ignore any mousedown that arrives within 350 ms of the native
        // picker dialog closing — the browser synthesises one to restore focus.
        setTimeout(function () {
            document.addEventListener('mousedown', function outsideClick(e) {
                if (Date.now() - lastPickTime < 350) { return; }
                if (activePopover && !activePopover.contains(e.target)) {
                    closePopover();
                    document.removeEventListener('mousedown', outsideClick);
                }
            });
        }, 50);

        // Focus the color input so the native picker is ready for keyboard
        setTimeout(function () { colorInp.focus(); }, 30);
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

            var mark = wrap._cmMark;

            openPicker(diamond, hexVal, function (newHex, isFinal) {
                // Update hexVal so re-open shows current color
                hexVal = newHex;
                diamond.style.background = newHex;
                if (!mark) { return; }
                var pos = mark.find();
                if (!pos) { return; }
                cm.replaceRange(newHex, pos.from, pos.to);
                if (isFinal) { cm.focus(); }
            });
        });

        return wrap;
    }

    // ── Per-editor scanner ────────────────────────────────────────

    function attachSwatches(cm) {
        var activeMarks = [];

        function clearMarks() {
            for (var i = 0; i < activeMarks.length; i++) {
                try { activeMarks[i].clear(); } catch (e) { /* already cleared */ }
            }
            activeMarks = [];
        }

        function scan() {
            clearMarks();
            var vp   = cm.getViewport();
            var from = Math.max(0, vp.from - 10);
            var to   = Math.min(cm.lineCount(), vp.to + 10);

            for (var lineNo = from; lineNo < to; lineNo++) {
                var text = cm.getLine(lineNo);
                if (!text) { continue; }

                COLOR_RE.lastIndex = 0;
                var m;
                while ((m = COLOR_RE.exec(text)) !== null) {
                    var colorStr = m[0];
                    var hexVal   = colorToHex(colorStr);
                    if (!hexVal) { continue; }

                    var fromPos = { line: lineNo, ch: m.index };
                    var toPos   = { line: lineNo, ch: m.index + colorStr.length };

                    // IIFE to capture value of colorStr/hexVal/fromPos/toPos per iteration
                    activeMarks.push((function (cs, hv, fp, tp) {
                        var widget = createWidget(cs, hv, cm);
                        var mark   = cm.markText(fp, tp, {
                            replacedWith:     widget,
                            handleMouseEvents: false,
                            inclusiveLeft:    false,
                            inclusiveRight:   false
                        });
                        // Back-reference for the click handler
                        widget._cmMark = mark;
                        return mark;
                    })(colorStr, hexVal, fromPos, toPos));
                }
            }
        }

        var debouncedScan = LiveCSS.utils.debounce(scan, 250);

        cm.on('change',         debouncedScan);
        cm.on('viewportChange', debouncedScan);
        cm.on('scroll',         debouncedScan);

        // Initial scan after editors have rendered
        setTimeout(scan, 500);
    }

    // ── Public ────────────────────────────────────────────────────

    function init() {
        var fns = ['getCssEditor', 'getHtmlEditor', 'getJsEditor'];
        fns.forEach(function (fn) {
            var cm = LiveCSS.editor[fn] && LiveCSS.editor[fn]();
            if (cm) { attachSwatches(cm); }
        });
    }

    return {
        init:       init,
        colorToHex: colorToHex,
        hslToHex:   hslToHex
    };

}());
