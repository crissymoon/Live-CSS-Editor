/**
 * color-harmony.js — Draggable floating Color Harmony tool
 * Attached to window.LiveCSS.colorHarmony
 *
 * Shows computed color harmonies (Complementary, Analogous, Triadic,
 * Split-Complementary, Tetradic, Square, Monochromatic) for the
 * base color selected with the picker at the bottom of the panel.
 * Clicking a swatch copies its hex value.
 *
 * Call LiveCSS.colorHarmony.init() after DOM is ready.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.colorHarmony = (function () {
    'use strict';

    // ── State ─────────────────────────────────────────────────────

    var panel, picker, hexDisplay, swatchesEl, modeBtns;
    var currentMode = 'complementary';
    var drag = { active: false, sx: 0, sy: 0, ox: 0, oy: 0 };

    // ── Color math ────────────────────────────────────────────────

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
        // Prefer shared implementation if available
        if (window.LiveCSS.colorSwatch) {
            return window.LiveCSS.colorSwatch.hslToHex(h, s, l);
        }
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
        hex = hex.replace('#', '');
        if (hex.length === 3) {
            hex = hex[0]+hex[0] + hex[1]+hex[1] + hex[2]+hex[2];
        }
        var r = parseInt(hex.substring(0, 2), 16) / 255;
        var g = parseInt(hex.substring(2, 4), 16) / 255;
        var b = parseInt(hex.substring(4, 6), 16) / 255;
        var max = Math.max(r, g, b);
        var min = Math.min(r, g, b);
        var h, s;
        var l = (max + min) / 2;
        if (max === min) {
            h = s = 0;
        } else {
            var d = max - min;
            s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
            switch (max) {
                case r: h = ((g - b) / d + (g < b ? 6 : 0)) / 6; break;
                case g: h = ((b - r) / d + 2) / 6;               break;
                case b: h = ((r - g) / d + 4) / 6;               break;
                default: h = 0;
            }
        }
        return { h: h * 360, s: s * 100, l: l * 100 };
    }

    // ── Harmony definitions ───────────────────────────────────────

    var HARMONY_MODES = {
        complementary: function (h, s, l) {
            return [
                { h: h,       s: s,       l: l,                 label: 'Base'       },
                { h: h + 180, s: s,       l: l,                 label: 'Complement' }
            ];
        },
        analogous: function (h, s, l) {
            return [
                { h: h - 30,  s: s,       l: l,                 label: '\u221230'   },
                { h: h - 15,  s: s,       l: l,                 label: '\u221215'   },
                { h: h,       s: s,       l: l,                 label: 'Base'       },
                { h: h + 15,  s: s,       l: l,                 label: '+15'        },
                { h: h + 30,  s: s,       l: l,                 label: '+30'        }
            ];
        },
        triadic: function (h, s, l) {
            return [
                { h: h,       s: s,       l: l,                 label: 'Base'       },
                { h: h + 120, s: s,       l: l,                 label: '+120'       },
                { h: h + 240, s: s,       l: l,                 label: '+240'       }
            ];
        },
        'split-comp': function (h, s, l) {
            return [
                { h: h,       s: s,       l: l,                 label: 'Base'       },
                { h: h + 150, s: s,       l: l,                 label: '+150'       },
                { h: h + 210, s: s,       l: l,                 label: '+210'       }
            ];
        },
        tetradic: function (h, s, l) {
            return [
                { h: h,       s: s,       l: l,                 label: 'Base'       },
                { h: h + 60,  s: s,       l: l,                 label: '+60'        },
                { h: h + 180, s: s,       l: l,                 label: '+180'       },
                { h: h + 240, s: s,       l: l,                 label: '+240'       }
            ];
        },
        square: function (h, s, l) {
            return [
                { h: h,       s: s,       l: l,                 label: 'Base'       },
                { h: h + 90,  s: s,       l: l,                 label: '+90'        },
                { h: h + 180, s: s,       l: l,                 label: '+180'       },
                { h: h + 270, s: s,       l: l,                 label: '+270'       }
            ];
        },
        monochromatic: function (h, s, l) {
            return [
                { h: h, s: s, l: Math.max(5,  l - 40), label: 'Dark 2'   },
                { h: h, s: s, l: Math.max(5,  l - 20), label: 'Dark 1'   },
                { h: h, s: s, l: l,                     label: 'Base'     },
                { h: h, s: s, l: Math.min(95, l + 20), label: 'Light 1'  },
                { h: h, s: s, l: Math.min(95, l + 40), label: 'Light 2'  }
            ];
        }
    };

    function computeHarmony(hexColor, mode) {
        var hsl = hexToHsl(hexColor);
        var fn  = HARMONY_MODES[mode] || HARMONY_MODES.complementary;
        return fn(hsl.h, hsl.s, hsl.l).map(function (c) {
            return { hex: hslToHex(c.h, c.s, c.l), label: c.label };
        });
    }

    // ── Render swatches ───────────────────────────────────────────

    function render() {
        var hexColor = picker.value;
        hexDisplay.textContent = hexColor;

        var colors  = computeHarmony(hexColor, currentMode);
        swatchesEl.innerHTML = '';

        colors.forEach(function (c) {
            var item   = document.createElement('div');
            item.className = 'harmony-swatch-item';

            var block  = document.createElement('div');
            block.className = 'harmony-swatch-block';
            block.style.background = c.hex;
            block.title = 'Click to copy ' + c.hex;

            var hexLbl = document.createElement('div');
            hexLbl.className = 'harmony-swatch-hex';
            hexLbl.textContent = c.hex;

            var nameLbl = document.createElement('div');
            nameLbl.className = 'harmony-swatch-lbl';
            nameLbl.textContent = c.label;

            // Copy to clipboard on click
            block.addEventListener('click', function () {
                try {
                    if (navigator.clipboard) {
                        navigator.clipboard.writeText(c.hex);
                    }
                } catch (e) { /* clipboard not available */ }
                block.classList.add('harmony-swatch-flash');
                setTimeout(function () {
                    block.classList.remove('harmony-swatch-flash');
                }, 350);
            });

            item.appendChild(block);
            item.appendChild(hexLbl);
            item.appendChild(nameLbl);
            swatchesEl.appendChild(item);
        });
    }

    // ── Drag ─────────────────────────────────────────────────────

    function onHeaderMousedown(e) {
        if (e.target.classList.contains('harmony-close')) { return; }
        drag.active = true;
        drag.sx = e.clientX;
        drag.sy = e.clientY;
        drag.ox = panel.offsetLeft;
        drag.oy = panel.offsetTop;
        e.preventDefault();
    }

    function onDocMousemove(e) {
        if (!drag.active) { return; }
        panel.style.left = (drag.ox + e.clientX - drag.sx) + 'px';
        panel.style.top  = (drag.oy + e.clientY - drag.sy) + 'px';
    }

    function onDocMouseup() {
        if (drag.active) {
            drag.active = false;
            saveToolState();
        }
    }

    function saveToolState() {
        var state = LiveCSS.storage.loadUIState() || {};
        state.harmony = {
            visible: !panel.classList.contains('hidden'),
            left:    panel.style.left,
            top:     panel.style.top,
            color:   picker ? picker.value : '#4d31bf',
            mode:    currentMode
        };
        LiveCSS.storage.saveUIState(state);
    }

    function positionNear(btn) {
        var rect = btn.getBoundingClientRect();
        var pw   = panel.offsetWidth  || 380;
        var ph   = panel.offsetHeight || 480;
        var left = Math.max(4, Math.min(rect.right - pw, window.innerWidth - pw - 4));
        var top  = Math.min(rect.bottom + 6, window.innerHeight - ph - 4);
        panel.style.left = left + 'px';
        panel.style.top  = top  + 'px';
    }

    // ── Init ──────────────────────────────────────────────────────

    function init() {
        panel      = document.getElementById('harmonyTool');
        picker     = document.getElementById('harmonyPicker');
        hexDisplay = document.getElementById('harmonyHexDisplay');
        swatchesEl = document.getElementById('harmonySwatches');
        modeBtns   = panel.querySelectorAll('.harmony-mode-btn');

        // Toggle open/close
        document.getElementById('harmonyBtn').addEventListener('click', function () {
            if (panel.classList.contains('hidden')) {
                panel.classList.remove('hidden');
                // Use saved position, or position near button
                var uiState = LiveCSS.storage.loadUIState() || {};
                if (uiState.harmony && uiState.harmony.left) {
                    panel.style.left = uiState.harmony.left;
                    panel.style.top  = uiState.harmony.top;
                } else {
                    positionNear(this);
                }
                render();
            } else {
                panel.classList.add('hidden');
            }
            saveToolState();
        });

        document.getElementById('harmonyClose').addEventListener('click', function () {
            panel.classList.add('hidden');
            saveToolState();
        });

        // Mode selection
        modeBtns.forEach(function (btn) {
            btn.addEventListener('click', function () {
                modeBtns.forEach(function (b) { b.classList.remove('active'); });
                this.classList.add('active');
                currentMode = this.dataset.mode;
                render();
            });
        });

        // Color picker drives the harmony display
        picker.addEventListener('input', render);

        // Drag
        panel.querySelector('.harmony-header').addEventListener('mousedown', onHeaderMousedown);
        document.addEventListener('mousemove', onDocMousemove);
        document.addEventListener('mouseup', onDocMouseup);

        // Restore saved visibility and position
        var savedUI = LiveCSS.storage.loadUIState() || {};
        if (savedUI.harmony) {
            if (savedUI.harmony.visible) {
                panel.classList.remove('hidden');
                if (savedUI.harmony.left) {
                    panel.style.left = savedUI.harmony.left;
                    panel.style.top  = savedUI.harmony.top;
                }
            }
            if (savedUI.harmony.mode) {
                currentMode = savedUI.harmony.mode;
                modeBtns.forEach(function (b) {
                    b.classList.toggle('active', b.dataset.mode === currentMode);
                });
            }
            if (savedUI.harmony.color && picker) {
                picker.value = savedUI.harmony.color;
                if (hexDisplay) { hexDisplay.textContent = savedUI.harmony.color; }
            }
            if (!panel.classList.contains('hidden')) { render(); }
        }
    }

    return { init: init };

}());
