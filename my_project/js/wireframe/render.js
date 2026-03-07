/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* All DOM rendering: canvas elements, guides, rulers, and properties panel.
   Also owns deleteEl since it drives a full re-render. */

import { CANVAS_W, CANVAS_H } from './constants.js';
import { st, saveToStorage } from './state.js';
import { byId, childrenOf, isDescendant } from './elements.js';
import { tryApply } from './geometry.js';
import { escHtml, buildPropsHTML, bindStr, bindNum } from './props.js';

/* ── Guide ruler markers ──────────────────────────────────── */

export function renderGuideMarkers() {
    if (!st.rulerH || !st.rulerV) return;

    /* remove stale markers */
    var oldH = st.rulerH.querySelectorAll('.wf-gmark');
    for (var i = 0; i < oldH.length; i++) oldH[i].parentNode.removeChild(oldH[i]);
    var oldV = st.rulerV.querySelectorAll('.wf-gmark');
    for (var i = 0; i < oldV.length; i++) oldV[i].parentNode.removeChild(oldV[i]);

    st.guides.forEach(function (g) {
        var m = document.createElement('div');
        var lockedCls = g.locked ? ' wf-gmark-locked' : '';

        if (g.axis === 'v') {
            /* vertical guide at X -> marker on horizontal ruler */
            m.className  = 'wf-gmark wf-gmark-h' + lockedCls;
            m.style.left = g.pos + 'px';
            st.rulerH.appendChild(m);
        } else {
            /* horizontal guide at Y -> marker on vertical ruler */
            m.className = 'wf-gmark wf-gmark-v' + lockedCls;
            m.style.top = g.pos + 'px';
            st.rulerV.appendChild(m);
        }

        var lbl = document.createElement('span');
        lbl.className   = 'wf-gmark-label';
        lbl.textContent = g.pos + '';
        m.appendChild(lbl);

        m.addEventListener('mousedown', function (e) {
            e.stopPropagation();
            e.preventDefault();
        });
        m.addEventListener('click', function (e) {
            e.stopPropagation();
            g.locked = !g.locked;
            saveToStorage();
            renderGuideMarkers();
            renderGuides();
        });
    });
}

/* ── Guide lines ──────────────────────────────────────────── */

export function renderGuides() {
    /* remove existing guide divs */
    var old = st.canvasEl.querySelectorAll('.wf-guide-h, .wf-guide-v');
    for (var i = 0; i < old.length; i++) old[i].parentNode.removeChild(old[i]);

    st.guides.forEach(function (g) {
        var outer = document.createElement('div');
        outer.className = 'wf-guide-' + g.axis +
                          (g.id === st.selGuideId ? ' wf-guide-selected' : '') +
                          (g.locked ? ' wf-guide-locked' : '');
        outer.dataset.guideId = g.id;

        if (g.axis === 'h') {
            outer.style.top  = (g.pos - 4) + 'px';
        } else {
            outer.style.left = (g.pos - 4) + 'px';
        }

        var line = document.createElement('div');
        line.className = 'wf-guide-line';

        var lbl = document.createElement('span');
        lbl.className   = 'wf-guide-label';
        lbl.textContent = g.pos + 'px';

        outer.appendChild(line);
        outer.appendChild(lbl);

        outer.addEventListener('mousedown', function (e) {
            e.stopPropagation();
            e.preventDefault();
            st.selGuideId = g.id;
            st.selId      = null;
            var allGuides = st.canvasEl.querySelectorAll('[data-guide-id]');
            for (var i = 0; i < allGuides.length; i++) {
                var gid = parseInt(allGuides[i].dataset.guideId, 10);
                allGuides[i].classList.toggle('wf-guide-selected', gid === g.id);
            }
            st.guideDrag = { guideId: g.id, axis: g.axis, moved: false };
        });

        st.canvasEl.appendChild(outer);
    });

    renderGuideMarkers();
}

/* ── Rulers ───────────────────────────────────────────────── */

export function renderRulers() {
    if (!st.rulerH || !st.rulerV) return;

    var hHtml = '';
    for (var x = 0; x <= CANVAS_W; x += 25) {
        var major = (x % 100 === 0);
        hHtml += '<div class="wf-tick wf-tick-h' + (major ? ' wf-tick-major' : '') +
                 '" style="left:' + x + 'px">' +
                 (major ? '<span class="wf-tick-label">' + x + '</span>' : '') +
                 '</div>';
    }
    st.rulerH.innerHTML = hHtml;

    var vHtml = '';
    for (var y = 0; y <= CANVAS_H; y += 25) {
        var majorY = (y % 100 === 0);
        vHtml += '<div class="wf-tick wf-tick-v' + (majorY ? ' wf-tick-major' : '') +
                 '" style="top:' + y + 'px">' +
                 (majorY ? '<span class="wf-tick-label">' + y + '</span>' : '') +
                 '</div>';
    }
    st.rulerV.innerHTML = vHtml;

    renderGuideMarkers();
}

/* ── Full re-render ───────────────────────────────────────── */

export function render() {
    saveToStorage();
    st.canvasEl.innerHTML = '';
    var roots = st.elements.filter(function (e) { return !e.parentId; });
    roots.forEach(function (el) { renderEl(el, st.canvasEl); });
    renderGuides();
    renderProps();
}

/* ── Single element ───────────────────────────────────────── */

export function renderEl(el, container) {
    var div = document.createElement('div');
    div.className       = 'wf-el' + (el.id === st.selId ? ' wf-selected' : '');
    div.dataset.id      = el.id;
    div.style.left        = el.x + 'px';
    div.style.top         = el.y + 'px';
    div.style.width       = el.w + 'px';
    div.style.height      = el.h + 'px';
    div.style.background  = el.bgColor;
    div.style.borderColor = el.borderColor;
    div.style.borderWidth  = el.borderWidth + 'px';
    div.style.borderStyle  = 'solid';
    div.style.boxSizing    = 'border-box';
    div.style.borderRadius = (el.borderRadius || 0) + 'px';

    /* margin indicator */
    if (el.mt || el.mr || el.mb || el.ml) {
        var mb = document.createElement('div');
        mb.className    = 'wf-margin-box';
        mb.style.top    = (-el.mt) + 'px';
        mb.style.left   = (-el.ml) + 'px';
        mb.style.right  = (-el.mr) + 'px';
        mb.style.bottom = (-el.mb) + 'px';
        div.appendChild(mb);
    }

    /* padding indicator */
    if (el.pt || el.pr || el.pb || el.pl) {
        var pb = document.createElement('div');
        pb.className    = 'wf-padding-box';
        pb.style.top    = el.pt + 'px';
        pb.style.left   = el.pl + 'px';
        pb.style.right  = el.pr + 'px';
        pb.style.bottom = el.pb + 'px';
        div.appendChild(pb);
    }

    /* label */
    var lbl = document.createElement('div');
    lbl.className   = 'wf-label';
    lbl.textContent = el.label;
    div.appendChild(lbl);

    /* resize handles (selected element only) */
    if (el.id === st.selId) {
        ['nw','n','ne','e','se','s','sw','w'].forEach(function (h) {
            var hd = document.createElement('div');
            hd.className      = 'wf-handle wf-handle-' + h;
            hd.dataset.handle = h;
            hd.dataset.elid   = el.id;
            div.appendChild(hd);
        });
    }

    /* mousedown: start move (if not on a handle) */
    div.addEventListener('mousedown', function (e) {
        if (e.target.classList.contains('wf-handle')) return;
        e.stopPropagation();
        e.preventDefault();
        st.selId      = el.id;
        st.selGuideId = null;
        st.drag = {
            mode: 'move', elId: el.id,
            sx: e.clientX, sy: e.clientY,
            ox: el.x, oy: el.y,
            ow: el.w, oh: el.h
        };
        render();
    });

    /* nested children */
    childrenOf(el.id).forEach(function (kid) { renderEl(kid, div); });

    container.appendChild(div);
}

/* ── Canvas-only re-render (skips props panel) ────────────── */

export function renderCanvas() {
    st.canvasEl.innerHTML = '';
    var roots = st.elements.filter(function (e) { return !e.parentId; });
    roots.forEach(function (el) { renderEl(el, st.canvasEl); });
    renderGuides();
}

/* ── X/Y inputs update without full props re-render ──────── */

export function updateXY(el) {
    var fields = { wfpX: el.x, wfpY: el.y, wfpW: el.w, wfpH: el.h };
    Object.keys(fields).forEach(function (id) {
        var inp = document.getElementById(id);
        if (inp && document.activeElement !== inp) inp.value = fields[id];
    });
}

/* ── Properties panel ─────────────────────────────────────── */

export function renderProps() {
    if (!st.propsEl) return;
    if (!st.selId) {
        st.propsEl.innerHTML = '<div class="wf-props-empty">Click an element to select it</div>';
        return;
    }
    var el = byId(st.selId);
    if (!el) { st.propsEl.innerHTML = ''; return; }

    var eligibleParents = st.elements.filter(function (e) {
        return e.id !== el.id && !isDescendant(e, el.id);
    });

    st.propsEl.innerHTML = buildPropsHTML(el, eligibleParents);

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
    bindNum('wfpBW', function (v) { el.borderWidth  = Math.max(0, v); renderCanvas(); });
    bindNum('wfpBR', function (v) { el.borderRadius = Math.max(0, v); renderCanvas(); });

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

/* ── Cascade delete ───────────────────────────────────────── */

export function deleteEl(id) {
    var toDelete = [id];
    var changed  = true;
    while (changed) {
        changed = false;
        st.elements.forEach(function (e) {
            if (toDelete.indexOf(e.parentId) !== -1 &&
                toDelete.indexOf(e.id) === -1) {
                toDelete.push(e.id);
                changed = true;
            }
        });
    }
    st.elements = st.elements.filter(function (e) {
        return toDelete.indexOf(e.id) === -1;
    });
    if (toDelete.indexOf(st.selId) !== -1) st.selId = null;
    render();
}
