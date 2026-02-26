/**
 * wireframe.js — Interactive wireframe / layout prototyping tool
 * Attached to window.LiveCSS.wireframe
 *
 * Features:
 *  - Add, move, resize rectangles on a 1200×900 canvas
 *  - Nest elements (children positioned inside parent's box)
 *  - Adjust margins and padding per element
 *  - Anti-overlap: sibling collision boxes (border + margins) must stay
 *    at least GAP px apart — elements cannot touch or overlap
 *  - Color pickers for background and border color
 *  - Label each element
 *  - Cascade delete (deleting a parent removes its children)
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.wireframe = (function () {

    /* ── Constants ─────────────────────────────────────────── */
    var GAP      = 4;     /* min px gap between siblings (incl. margins) */
    var MIN_SIZE = 48;    /* min element width / height                   */
    var CANVAS_W = 1200;
    var CANVAS_H = 900;

    /* ── State ─────────────────────────────────────────────── */
    var elements = [];
    var nextId   = 1;
    var selId    = null;
    var drag     = null;
    var rafId    = null;

    var LS_KEY   = 'livecss_wireframe_v1';
    /*
     * drag shape:
     * { mode:'move'|'resize', elId, handle(resize only),
     *   sx, sy (start mouse page coords),
     *   ox, oy, ow, oh (original el geometry) }
     */

    /* ── DOM refs ────────────────────────────────────────────── */
    var overlay, canvasEl, propsEl;

    /* ── Element factory ─────────────────────────────────────── */
    function makeEl(parentId, x, y) {
        return {
            id:          'el_' + (nextId++),
            parentId:    parentId || null,
            x:           x !== undefined ? x : 40,
            y:           y !== undefined ? y : 40,
            w:           200,
            h:           140,
            label:       'Box',
            bgColor:     '#1a0f3d',
            borderColor: '#4d31bf',
            borderWidth: 1,
            mt: 8, mr: 8, mb: 8, ml: 8,
            pt: 8, pr: 8, pb: 8, pl: 8
        };
    }

    /* ── Look-up helpers ─────────────────────────────────────── */
    function byId(id) {
        for (var i = 0; i < elements.length; i++) {
            if (elements[i].id === id) return elements[i];
        }
        return null;
    }

    function siblingsOf(el) {
        return elements.filter(function (e) {
            return e.parentId === el.parentId && e.id !== el.id;
        });
    }

    function childrenOf(parentId) {
        return elements.filter(function (e) { return e.parentId === parentId; });
    }

    function isDescendant(el, ancestorId) {
        var pid = el.parentId;
        while (pid) {
            if (pid === ancestorId) return true;
            var p = byId(pid);
            pid = p ? p.parentId : null;
        }
        return false;
    }

    /* ── Collision math ──────────────────────────────────────── */

    /* Collision box: border-box + margins */
    function cbox(el) {
        return {
            l: el.x - el.ml,
            t: el.y - el.mt,
            r: el.x + el.w + el.mr,
            b: el.y + el.h + el.mb
        };
    }

    /* Two boxes conflict if they overlap OR their gap is < GAP */
    function boxesConflict(a, b) {
        return a.r + GAP > b.l && b.r + GAP > a.l &&
               a.b + GAP > b.t && b.b + GAP > a.t;
    }

    function proposedConflicts(el, nx, ny, nw, nh) {
        var p = {
            l: nx - el.ml, t: ny - el.mt,
            r: nx + nw + el.mr, b: ny + nh + el.mb
        };
        var sibs = siblingsOf(el);
        for (var i = 0; i < sibs.length; i++) {
            if (boxesConflict(p, cbox(sibs[i]))) return true;
        }
        return false;
    }

    /* Check child fits inside parent's content area (inside padding) */
    function fitsInParent(el, nx, ny, nw, nh) {
        if (!el.parentId) return true;
        var par = byId(el.parentId);
        if (!par) return true;
        /* occupied area including margins must sit inside content area */
        return (nx - el.ml) >= par.pl &&
               (ny - el.mt) >= par.pt &&
               (nx + nw + el.mr) <= (par.w - par.pr) &&
               (ny + nh + el.mb) <= (par.h - par.pb);
    }

    /* Try to move/resize el to (nx,ny,nw,nh). Applies if valid. */
    function tryApply(el, nx, ny, nw, nh) {
        nw = Math.max(MIN_SIZE, nw);
        nh = Math.max(MIN_SIZE, nh);

        if (!el.parentId) {
            /* clamp within canvas */
            nx = Math.max(0, Math.min(nx, CANVAS_W - nw));
            ny = Math.max(0, Math.min(ny, CANVAS_H - nh));
        }

        if (!proposedConflicts(el, nx, ny, nw, nh) &&
            fitsInParent(el, nx, ny, nw, nh)) {
            el.x = nx; el.y = ny; el.w = nw; el.h = nh;
        }
    }

    /* ── localStorage autosave ──────────────────────────────── */
    function saveToStorage() {
        try {
            localStorage.setItem(LS_KEY, JSON.stringify({
                version: 1,
                nextId:  nextId,
                elements: elements
            }));
        } catch (e) {}
    }

    function loadFromStorage() {
        try {
            var raw = localStorage.getItem(LS_KEY);
            if (!raw) return;
            var data = JSON.parse(raw);
            if (!data || !Array.isArray(data.elements)) return;
            elements = data.elements;
            nextId   = data.nextId || (elements.length + 1);
        } catch (e) {}
    }

    /* ── JSON export ─────────────────────────────────────────── */
    function saveJSON() {
        var payload = JSON.stringify({
            version:  1,
            nextId:   nextId,
            elements: elements
        }, null, 2);
        var blob = new Blob([payload], { type: 'application/json' });
        var url  = URL.createObjectURL(blob);
        var a    = document.createElement('a');
        a.href     = url;
        a.download = 'wireframe.wf.json';
        a.click();
        setTimeout(function () { URL.revokeObjectURL(url); }, 2000);
    }

    /* ── JSON import ─────────────────────────────────────────── */
    function loadJSON(file) {
        var reader = new FileReader();
        reader.onload = function (e) {
            try {
                var data = JSON.parse(e.target.result);
                if (!data || !Array.isArray(data.elements)) {
                    alert('Invalid wireframe JSON file.');
                    return;
                }
                elements = data.elements;
                nextId   = data.nextId || (elements.length + 1);
                selId    = null;
                saveToStorage();
                render();
            } catch (ex) {
                alert('Could not parse JSON: ' + ex.message);
            }
        };
        reader.readAsText(file);
    }

    /* ── Context / description generator ────────────────────── */
    function buildContext() {
        if (!elements.length) return '/* No wireframe elements */';

        var lines = [];
        lines.push('/* =============================================');
        lines.push('   Wireframe Layout — ' + elements.length + ' element(s)');
        lines.push('   Canvas: ' + CANVAS_W + 'px x ' + CANVAS_H + 'px');
        lines.push('   ============================================= */');
        lines.push('');

        /* Build indented tree */
        function describe(el, indent) {
            var pfx = indent;
            lines.push(pfx + '/* ' + el.label + ' */');
            lines.push(pfx + '.el-' + el.id + ' {');
            lines.push(pfx + '    position: absolute;');
            lines.push(pfx + '    left: '    + el.x + 'px;');
            lines.push(pfx + '    top: '     + el.y + 'px;');
            lines.push(pfx + '    width: '   + el.w + 'px;');
            lines.push(pfx + '    height: '  + el.h + 'px;');
            lines.push(pfx + '    margin: '  + el.mt + 'px ' + el.mr + 'px ' + el.mb + 'px ' + el.ml + 'px;');
            lines.push(pfx + '    padding: ' + el.pt + 'px ' + el.pr + 'px ' + el.pb + 'px ' + el.pl + 'px;');
            lines.push(pfx + '    background: ' + el.bgColor + ';');
            lines.push(pfx + '    border: ' + el.borderWidth + 'px solid ' + el.borderColor + ';');
            lines.push(pfx + '}');

            var kids = childrenOf(el.id);
            kids.forEach(function (k) { describe(k, indent + '    '); });
        }

        var roots = elements.filter(function (e) { return !e.parentId; });
        roots.forEach(function (el) {
            describe(el, '');
            lines.push('');
        });

        /* Append raw JSON payload as a comment block */
        lines.push('/* --- Raw wireframe JSON ---');
        lines.push(JSON.stringify({ version: 1, nextId: nextId, elements: elements }, null, 2));
        lines.push('--- end wireframe JSON --- */');

        return lines.join('\n');
    }

    /* ── Delete ──────────────────────────────────────────────── */
    function deleteEl(id) {
        /* cascade: collect id + all descendants */
        var toDelete = [id];
        var changed = true;
        while (changed) {
            changed = false;
            elements.forEach(function (e) {
                if (toDelete.indexOf(e.parentId) !== -1 &&
                    toDelete.indexOf(e.id) === -1) {
                    toDelete.push(e.id);
                    changed = true;
                }
            });
        }
        elements = elements.filter(function (e) {
            return toDelete.indexOf(e.id) === -1;
        });
        if (toDelete.indexOf(selId) !== -1) selId = null;
        render();
    }

    /* ── Rendering ───────────────────────────────────────────── */
    function render() {
        saveToStorage();
        canvasEl.innerHTML = '';

        var roots = elements.filter(function (e) { return !e.parentId; });
        roots.forEach(function (el) { renderEl(el, canvasEl); });
        renderProps();
    }

    function renderEl(el, container) {
        var div = document.createElement('div');
        div.className = 'wf-el' + (el.id === selId ? ' wf-selected' : '');
        div.dataset.id = el.id;
        div.style.left        = el.x + 'px';
        div.style.top         = el.y + 'px';
        div.style.width       = el.w + 'px';
        div.style.height      = el.h + 'px';
        div.style.background  = el.bgColor;
        div.style.borderColor = el.borderColor;
        div.style.borderWidth = el.borderWidth + 'px';
        div.style.borderStyle = 'solid';
        div.style.boxSizing   = 'border-box';

        /* margin indicator */
        if (el.mt || el.mr || el.mb || el.ml) {
            var mb = document.createElement('div');
            mb.className = 'wf-margin-box';
            mb.style.top    = (-el.mt) + 'px';
            mb.style.left   = (-el.ml) + 'px';
            mb.style.right  = (-el.mr) + 'px';
            mb.style.bottom = (-el.mb) + 'px';
            div.appendChild(mb);
        }

        /* padding indicator */
        if (el.pt || el.pr || el.pb || el.pl) {
            var pb = document.createElement('div');
            pb.className = 'wf-padding-box';
            pb.style.top    = el.pt + 'px';
            pb.style.left   = el.pl + 'px';
            pb.style.right  = el.pr + 'px';
            pb.style.bottom = el.pb + 'px';
            div.appendChild(pb);
        }

        /* label */
        var lbl = document.createElement('div');
        lbl.className = 'wf-label';
        lbl.textContent = el.label;
        div.appendChild(lbl);

        /* resize handles (only for selected element) */
        if (el.id === selId) {
            ['nw','n','ne','e','se','s','sw','w'].forEach(function (h) {
                var hd = document.createElement('div');
                hd.className       = 'wf-handle wf-handle-' + h;
                hd.dataset.handle  = h;
                hd.dataset.elid    = el.id;
                div.appendChild(hd);
            });
        }

        /* mousedown: start move (if not on a handle) */
        div.addEventListener('mousedown', function (e) {
            if (e.target.classList.contains('wf-handle')) return;
            e.stopPropagation();
            e.preventDefault();
            selId = el.id;
            drag  = {
                mode: 'move', elId: el.id,
                sx: e.clientX, sy: e.clientY,
                ox: el.x, oy: el.y,
                ow: el.w, oh: el.h
            };
            render();
        });

        /* nested children */
        childrenOf(el.id).forEach(function (kid) {
            renderEl(kid, div);
        });

        container.appendChild(div);
    }

    /* ── Properties panel ────────────────────────────────────── */
    function escHtml(s) {
        return String(s)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }

    function renderProps() {
        if (!propsEl) return;
        if (!selId) {
            propsEl.innerHTML = '<div class="wf-props-empty">Click an element to select it</div>';
            return;
        }
        var el = byId(selId);
        if (!el) { propsEl.innerHTML = ''; return; }

        /* build parent <select> options */
        var parentOpts = '<option value="">— canvas (no parent) —</option>';
        elements.forEach(function (e) {
            if (e.id !== el.id && !isDescendant(e, el.id)) {
                parentOpts += '<option value="' + e.id + '"' +
                    (el.parentId === e.id ? ' selected' : '') +
                    '>' + escHtml(e.label) + '</option>';
            }
        });

        propsEl.innerHTML =
            /* label + parent */
            '<div class="wf-props-section">' +
              '<div class="wf-props-section-title">Element</div>' +
              '<div class="wf-props-row"><label>Label</label>' +
                '<input class="wf-pi" type="text" id="wfpLabel" value="' + escHtml(el.label) + '">' +
              '</div>' +
              '<div class="wf-props-row"><label>Parent</label>' +
                '<select class="wf-pi" id="wfpParent">' + parentOpts + '</select>' +
              '</div>' +
            '</div>' +

            /* position + size */
            '<div class="wf-props-section">' +
              '<div class="wf-props-section-title">Position &amp; Size</div>' +
              '<div class="wf-props-row wf-props-row-half">' +
                '<label>X</label><input class="wf-pi" type="number" id="wfpX" value="' + el.x + '">' +
                '<label>Y</label><input class="wf-pi" type="number" id="wfpY" value="' + el.y + '">' +
              '</div>' +
              '<div class="wf-props-row wf-props-row-half">' +
                '<label>W</label><input class="wf-pi" type="number" id="wfpW" value="' + el.w + '">' +
                '<label>H</label><input class="wf-pi" type="number" id="wfpH" value="' + el.h + '">' +
              '</div>' +
            '</div>' +

            /* margin */
            '<div class="wf-props-section">' +
              '<div class="wf-props-section-title">Margin</div>' +
              '<div class="wf-props-sublabel">' +
                '<span>top</span><span>right</span><span>bottom</span><span>left</span>' +
              '</div>' +
              '<div class="wf-props-row wf-props-row-quad">' +
                '<input class="wf-pi" type="number" id="wfpMt" min="0" value="' + el.mt + '">' +
                '<input class="wf-pi" type="number" id="wfpMr" min="0" value="' + el.mr + '">' +
                '<input class="wf-pi" type="number" id="wfpMb" min="0" value="' + el.mb + '">' +
                '<input class="wf-pi" type="number" id="wfpMl" min="0" value="' + el.ml + '">' +
              '</div>' +
            '</div>' +

            /* padding */
            '<div class="wf-props-section">' +
              '<div class="wf-props-section-title">Padding</div>' +
              '<div class="wf-props-sublabel">' +
                '<span>top</span><span>right</span><span>bottom</span><span>left</span>' +
              '</div>' +
              '<div class="wf-props-row wf-props-row-quad">' +
                '<input class="wf-pi" type="number" id="wfpPt" min="0" value="' + el.pt + '">' +
                '<input class="wf-pi" type="number" id="wfpPr" min="0" value="' + el.pr + '">' +
                '<input class="wf-pi" type="number" id="wfpPb" min="0" value="' + el.pb + '">' +
                '<input class="wf-pi" type="number" id="wfpPl" min="0" value="' + el.pl + '">' +
              '</div>' +
            '</div>' +

            /* colors */
            '<div class="wf-props-section">' +
              '<div class="wf-props-section-title">Appearance</div>' +
              '<div class="wf-props-row">' +
                '<label>BG color</label>' +
                '<input class="wf-pi wf-pi-color" type="color" id="wfpBg" value="' + el.bgColor + '">' +
                '<span class="wf-pi-hex" id="wfpBgHex">' + el.bgColor + '</span>' +
              '</div>' +
              '<div class="wf-props-row">' +
                '<label>Border</label>' +
                '<input class="wf-pi wf-pi-color" type="color" id="wfpBorder" value="' + el.borderColor + '">' +
                '<span class="wf-pi-hex" id="wfpBorderHex">' + el.borderColor + '</span>' +
              '</div>' +
              '<div class="wf-props-row">' +
                '<label>Border W</label>' +
                '<input class="wf-pi" type="number" id="wfpBW" min="0" max="20" value="' + el.borderWidth + '">' +
              '</div>' +
            '</div>' +

            /* delete */
            '<div class="wf-props-section">' +
              '<button class="wf-btn-del" id="wfpDelete">Delete Element</button>' +
            '</div>';

        /* bind inputs */
        bindStr('wfpLabel', function (v) { el.label = v; renderCanvas(); });

        bindNum('wfpX', function (v) { tryApply(el, v,    el.y, el.w, el.h); renderCanvas(); updateXY(el); });
        bindNum('wfpY', function (v) { tryApply(el, el.x, v,    el.w, el.h); renderCanvas(); updateXY(el); });
        bindNum('wfpW', function (v) { tryApply(el, el.x, el.y, v,    el.h); renderCanvas(); updateXY(el); });
        bindNum('wfpH', function (v) { tryApply(el, el.x, el.y, el.w, v   ); renderCanvas(); updateXY(el); });

        bindNum('wfpMt', function (v) { el.mt = Math.max(0, v); renderCanvas(); });
        bindNum('wfpMr', function (v) { el.mr = Math.max(0, v); renderCanvas(); });
        bindNum('wfpMb', function (v) { el.mb = Math.max(0, v); renderCanvas(); });
        bindNum('wfpMl', function (v) { el.ml = Math.max(0, v); renderCanvas(); });

        bindNum('wfpPt', function (v) { el.pt = Math.max(0, v); renderCanvas(); });
        bindNum('wfpPr', function (v) { el.pr = Math.max(0, v); renderCanvas(); });
        bindNum('wfpPb', function (v) { el.pb = Math.max(0, v); renderCanvas(); });
        bindNum('wfpPl', function (v) { el.pl = Math.max(0, v); renderCanvas(); });

        bindStr('wfpBg', function (v) {
            el.bgColor = v;
            var hex = document.getElementById('wfpBgHex');
            if (hex) hex.textContent = v;
            renderCanvas();
        });
        bindStr('wfpBorder', function (v) {
            el.borderColor = v;
            var hex = document.getElementById('wfpBorderHex');
            if (hex) hex.textContent = v;
            renderCanvas();
        });
        bindNum('wfpBW', function (v) { el.borderWidth = Math.max(0, v); renderCanvas(); });

        var parentSel = document.getElementById('wfpParent');
        if (parentSel) {
            parentSel.addEventListener('change', function () {
                el.parentId = this.value || null;
                render();
            });
        }

        var delBtn = document.getElementById('wfpDelete');
        if (delBtn) {
            delBtn.addEventListener('click', function () { deleteEl(el.id); });
        }
    }

    /* Only re-render the canvas elements (not props panel) — for color / margin changes */
    function renderCanvas() {
        canvasEl.innerHTML = '';
        var roots = elements.filter(function (e) { return !e.parentId; });
        roots.forEach(function (el) { renderEl(el, canvasEl); });
    }

    /* Update only the x/y/w/h prop inputs without full props re-render */
    function updateXY(el) {
        var fields = { wfpX: el.x, wfpY: el.y, wfpW: el.w, wfpH: el.h };
        Object.keys(fields).forEach(function (id) {
            var inp = document.getElementById(id);
            if (inp && document.activeElement !== inp) inp.value = fields[id];
        });
    }

    /* ── Input binding helpers ───────────────────────────────── */
    function bindStr(id, fn) {
        var inp = document.getElementById(id);
        if (inp) inp.addEventListener('input', function () { fn(this.value); });
    }

    function bindNum(id, fn) {
        var inp = document.getElementById(id);
        if (inp) {
            inp.addEventListener('input', function () {
                var v = parseInt(this.value, 10);
                if (!isNaN(v)) fn(v);
            });
        }
    }

    /* ── Mouse handling ──────────────────────────────────────── */
    function onMousemove(e) {
        if (!drag) return;
        if (rafId) cancelAnimationFrame(rafId);
        rafId = requestAnimationFrame(function () {
            rafId = null;
            var el = byId(drag.elId);
            if (!el) return;
            var dx = e.clientX - drag.sx;
            var dy = e.clientY - drag.sy;

            if (drag.mode === 'move') {
                var nx = drag.ox + dx;
                var ny = drag.oy + dy;
                /* try full move, fall back to axis-locked moves */
                if (!proposedConflicts(el, nx, ny, el.w, el.h) &&
                    fitsInParent(el, nx, ny, el.w, el.h)) {
                    el.x = nx; el.y = ny;
                } else {
                    if (!proposedConflicts(el, nx, el.y, el.w, el.h) &&
                        fitsInParent(el, nx, el.y, el.w, el.h)) {
                        el.x = nx;
                    }
                    if (!proposedConflicts(el, el.x, ny, el.w, el.h) &&
                        fitsInParent(el, el.x, ny, el.w, el.h)) {
                        el.y = ny;
                    }
                }
            } else {
                /* resize */
                var nx = drag.ox, ny = drag.oy, nw = drag.ow, nh = drag.oh;
                var h = drag.handle;

                if (h.indexOf('e') !== -1) {
                    nw = Math.max(MIN_SIZE, drag.ow + dx);
                }
                if (h.indexOf('s') !== -1) {
                    nh = Math.max(MIN_SIZE, drag.oh + dy);
                }
                if (h.indexOf('w') !== -1) {
                    var newW = Math.max(MIN_SIZE, drag.ow - dx);
                    nx = drag.ox + drag.ow - newW;
                    nw = newW;
                }
                if (h.indexOf('n') !== -1) {
                    var newH = Math.max(MIN_SIZE, drag.oh - dy);
                    ny = drag.oy + drag.oh - newH;
                    nh = newH;
                }

                if (!proposedConflicts(el, nx, ny, nw, nh) &&
                    fitsInParent(el, nx, ny, nw, nh)) {
                    el.x = nx; el.y = ny; el.w = nw; el.h = nh;
                }
            }

            /* fast DOM update: only move the dragged element div */
            var div = canvasEl.querySelector('[data-id="' + el.id + '"]');
            if (div) {
                div.style.left   = el.x + 'px';
                div.style.top    = el.y + 'px';
                div.style.width  = el.w + 'px';
                div.style.height = el.h + 'px';
            }
            updateXY(el);
        });
    }

    function onMouseup() {
        if (!drag) return;
        drag = null;
        if (rafId) { cancelAnimationFrame(rafId); rafId = null; }
        render(); /* full re-render with handles */
    }

    /* ── Auto-position new elements to avoid conflicts ────────── */
    function findFreePosition(el) {
        var x = 40, y = 40;
        var attempts = 0;
        while (attempts < 60) {
            if (!proposedConflicts({ id: el.id, parentId: null,
                    ml: el.ml, mr: el.mr, mt: el.mt, mb: el.mb },
                    x, y, el.w, el.h)) {
                break;
            }
            x += 28; y += 28;
            if (x + el.w + el.mr > CANVAS_W - 20) { x = 40; y += 60; }
            attempts++;
        }
        el.x = x; el.y = y;
    }

    /* ── Public API ──────────────────────────────────────────── */
    function init() {
        overlay   = document.getElementById('wireframeOverlay');
        canvasEl  = document.getElementById('wfCanvas');
        propsEl   = document.getElementById('wfProps');

        var openBtn    = document.getElementById('wireframeBtn');
        var closeBtn   = document.getElementById('wfCloseBtn');
        var addBtn     = document.getElementById('wfAddBtn');
        var clearBtn   = document.getElementById('wfClearBtn');
        var saveBtn    = document.getElementById('wfSaveBtn');
        var loadBtn    = document.getElementById('wfLoadBtn');
        var fileInput  = document.getElementById('wfFileInput');
        var contextBtn = document.getElementById('wfContextBtn');

        if (!overlay || !canvasEl || !propsEl || !openBtn) return;

        /* Restore last session from localStorage */
        loadFromStorage();

        openBtn.addEventListener('click', function () {
            overlay.classList.remove('hidden');
            render();
        });

        closeBtn.addEventListener('click', function () {
            overlay.classList.add('hidden');
        });

        /* close on overlay background click */
        overlay.addEventListener('mousedown', function (e) {
            if (e.target === overlay) overlay.classList.add('hidden');
        });

        addBtn.addEventListener('click', function () {
            var el = makeEl(null);
            findFreePosition(el);
            elements.push(el);
            selId = el.id;
            render();
        });

        clearBtn.addEventListener('click', function () {
            if (elements.length === 0 || window.confirm('Clear all elements?')) {
                elements = [];
                selId    = null;
                render();
            }
        });

        /* Save JSON */
        saveBtn.addEventListener('click', function () { saveJSON(); });

        /* Load JSON — trigger hidden file input */
        loadBtn.addEventListener('click', function () {
            if (fileInput) { fileInput.value = ''; fileInput.click(); }
        });
        if (fileInput) {
            fileInput.addEventListener('change', function () {
                if (this.files && this.files[0]) loadJSON(this.files[0]);
            });
        }

        /* Copy Context */
        contextBtn.addEventListener('click', function () {
            var text = buildContext();
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(text).then(function () {
                    var orig = contextBtn.textContent;
                    contextBtn.textContent = 'Copied!';
                    setTimeout(function () { contextBtn.textContent = orig; }, 1600);
                });
            } else {
                /* fallback: show in a temporary textarea */
                var ta = document.createElement('textarea');
                ta.value = text;
                ta.style.cssText = 'position:fixed;top:80px;left:50%;transform:translateX(-50%);width:600px;height:320px;z-index:99999;background:#0c071c;color:#d0c8f8;border:1px solid #4d31bf;font-family:monospace;font-size:12px;padding:10px;';
                document.body.appendChild(ta);
                ta.select();
                document.execCommand('copy');
                setTimeout(function () { ta.remove(); }, 4000);
                var orig = contextBtn.textContent;
                contextBtn.textContent = 'Copied!';
                setTimeout(function () { contextBtn.textContent = orig; }, 1600);
            }
        });

        /* deselect when clicking empty canvas */
        canvasEl.addEventListener('mousedown', function (e) {
            if (e.target === canvasEl) {
                selId = null;
                render();
            }
        });

        /* handle resize-handle mousedown (delegated from canvas) */
        canvasEl.addEventListener('mousedown', function (e) {
            var hd = e.target.closest ? e.target.closest('.wf-handle') : null;
            if (!hd) return;
            e.stopPropagation();
            e.preventDefault();
            var elId = hd.dataset.elid;
            var el   = byId(elId);
            if (!el) return;
            drag = {
                mode:   'resize',
                elId:   elId,
                handle: hd.dataset.handle,
                sx: e.clientX, sy: e.clientY,
                ox: el.x, oy: el.y,
                ow: el.w, oh: el.h
            };
        });

        document.addEventListener('mousemove', onMousemove);
        document.addEventListener('mouseup',   onMouseup);
    }

    return { init: init };

}());
