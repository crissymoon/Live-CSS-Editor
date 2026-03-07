/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* Mouse and keyboard input handlers. */

import { CANVAS_W, CANVAS_H, MIN_SIZE } from './constants.js';
import { st, saveToStorage } from './state.js';
import { byId, byGuideId } from './elements.js';
import {
    clampToLockedGuides, clampGuideThroughElements,
    clampResizeToLockedGuides, proposedConflicts, fitsInParent, tryApply
} from './geometry.js';
import { render, renderGuides, updateXY } from './render.js';

export function onMousemove(e) {
    /* ── Guide dragging ── */
    if (st.guideDrag) {
        var g = byGuideId(st.guideDrag.guideId);
        if (!g) { st.guideDrag = null; return; }
        var rect = st.canvasEl.getBoundingClientRect();
        var pos;
        if (g.axis === 'h') {
            pos = Math.round(e.clientY - rect.top);
            pos = Math.max(0, Math.min(pos, CANVAS_H));
        } else {
            pos = Math.round(e.clientX - rect.left);
            pos = Math.max(0, Math.min(pos, CANVAS_W));
        }
        if (g.locked) {
            pos = clampGuideThroughElements(g, pos);
        }
        g.pos = pos;
        st.guideDrag.moved = true;
        var gDiv = st.canvasEl.querySelector('[data-guide-id="' + g.id + '"]');
        if (gDiv) {
            if (g.axis === 'h') gDiv.style.top  = (pos - 4) + 'px';
            else                gDiv.style.left = (pos - 4) + 'px';
            var lbl = gDiv.querySelector('.wf-guide-label');
            if (lbl) lbl.textContent = pos + 'px';
        }
        return;
    }

    /* ── Element drag / resize ── */
    if (!st.drag) return;
    if (st.rafId) cancelAnimationFrame(st.rafId);
    st.rafId = requestAnimationFrame(function () {
        st.rafId = null;
        var el = byId(st.drag.elId);
        if (!el) return;
        var dx = e.clientX - st.drag.sx;
        var dy = e.clientY - st.drag.sy;

        if (st.drag.mode === 'move') {
            var nx = st.drag.ox + dx;
            var ny = st.drag.oy + dy;
            var gc = clampToLockedGuides(el, nx, ny, el.w, el.h);
            nx = gc.nx; ny = gc.ny;
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
            var nx = st.drag.ox, ny = st.drag.oy, nw = st.drag.ow, nh = st.drag.oh;
            var h = st.drag.handle;

            if (h.indexOf('e') !== -1) { nw = Math.max(MIN_SIZE, st.drag.ow + dx); }
            if (h.indexOf('s') !== -1) { nh = Math.max(MIN_SIZE, st.drag.oh + dy); }
            if (h.indexOf('w') !== -1) {
                var newW = Math.max(MIN_SIZE, st.drag.ow - dx);
                nx = st.drag.ox + st.drag.ow - newW;
                nw = newW;
            }
            if (h.indexOf('n') !== -1) {
                var newH = Math.max(MIN_SIZE, st.drag.oh - dy);
                ny = st.drag.oy + st.drag.oh - newH;
                nh = newH;
            }

            var rc = clampResizeToLockedGuides(el, nx, ny, nw, nh, h);
            nx = rc.nx; ny = rc.ny;
            nw = Math.max(MIN_SIZE, rc.nw);
            nh = Math.max(MIN_SIZE, rc.nh);

            if (!proposedConflicts(el, nx, ny, nw, nh) &&
                fitsInParent(el, nx, ny, nw, nh)) {
                el.x = nx; el.y = ny; el.w = nw; el.h = nh;
            }
        }

        /* fast DOM update: only reposition the dragged element div */
        var div = st.canvasEl.querySelector('[data-id="' + el.id + '"]');
        if (div) {
            div.style.left   = el.x + 'px';
            div.style.top    = el.y + 'px';
            div.style.width  = el.w + 'px';
            div.style.height = el.h + 'px';
        }
        updateXY(el);
    });
}

export function onMouseup() {
    if (st.guideDrag) {
        var moved = st.guideDrag.moved;
        st.guideDrag = null;
        if (moved) {
            saveToStorage();
            renderGuides();
        }
        return;
    }
    if (!st.drag) return;
    st.drag = null;
    if (st.rafId) { cancelAnimationFrame(st.rafId); st.rafId = null; }
    render(); /* full re-render restores handles */
}

/* Attach the arrow-key handler. Call once after overlay is available. */
export function setupKeyboard(overlay) {
    document.addEventListener('keydown', function (e) {
        if (overlay.classList.contains('hidden')) return;
        if (!st.selId && !st.selGuideId) return;
        var tag = document.activeElement && document.activeElement.tagName;
        if (tag === 'INPUT' || tag === 'SELECT' || tag === 'TEXTAREA') return;

        var code = e.keyCode;
        if (code < 37 || code > 40) return;   /* not an arrow key */

        e.preventDefault();
        var step = e.shiftKey ? 10 : 1;

        /* guide arrow key movement */
        if (st.selGuideId) {
            var g = byGuideId(st.selGuideId);
            if (!g) return;
            if (g.axis === 'h') {
                if (code === 38) g.pos = Math.max(0, g.pos - step);
                if (code === 40) g.pos = Math.min(CANVAS_H, g.pos + step);
            } else {
                if (code === 37) g.pos = Math.max(0, g.pos - step);
                if (code === 39) g.pos = Math.min(CANVAS_W, g.pos + step);
            }
            saveToStorage();
            renderGuides();
            return;
        }

        /* element arrow key movement */
        var el = byId(st.selId);
        if (!el) return;

        var nx = el.x, ny = el.y;
        if (code === 37) nx -= step;
        if (code === 39) nx += step;
        if (code === 38) ny -= step;
        if (code === 40) ny += step;

        tryApply(el, nx, ny, el.w, el.h);
        render();
    });
}
