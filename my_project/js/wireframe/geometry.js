/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* Collision math, guide clamping, tryApply, and findFreePosition. */

import { GAP, MIN_SIZE, CANVAS_W, CANVAS_H } from './constants.js';
import { st } from './state.js';
import { siblingsOf, byId } from './elements.js';

/* Collision box: border-box + margins */
export function cbox(el) {
    return {
        l: el.x - el.ml,
        t: el.y - el.mt,
        r: el.x + el.w + el.mr,
        b: el.y + el.h + el.mb
    };
}

/* Two boxes conflict if they overlap OR their gap is < GAP */
export function boxesConflict(a, b) {
    return a.r + GAP > b.l && b.r + GAP > a.l &&
           a.b + GAP > b.t && b.b + GAP > a.t;
}

export function proposedConflicts(el, nx, ny, nw, nh) {
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
export function fitsInParent(el, nx, ny, nw, nh) {
    if (!el.parentId) return true;
    var par = byId(el.parentId);
    if (!par) return true;
    return (nx - el.ml) >= par.pl &&
           (ny - el.mt) >= par.pt &&
           (nx + nw + el.mr) <= (par.w - par.pr) &&
           (ny + nh + el.mb) <= (par.h - par.pb);
}

/* Clamp a proposed resize so no edge crosses a locked guide barrier. */
export function clampResizeToLockedGuides(el, nx, ny, nw, nh, handle) {
    for (var i = 0; i < st.guides.length; i++) {
        var g = st.guides[i];
        if (!g.locked) continue;
        if (g.axis === 'h') {
            var Y = g.pos;
            if (handle.indexOf('s') !== -1) {
                if (el.y < Y && ny + nh > Y) {
                    nh = Y - ny;
                }
            }
            if (handle.indexOf('n') !== -1) {
                if (el.y + el.h > Y && ny < Y) {
                    var newH = (el.y + el.h) - Y;
                    ny = Y;
                    nh = newH;
                }
            }
        } else {
            var X = g.pos;
            if (handle.indexOf('e') !== -1) {
                if (el.x < X && nx + nw > X) {
                    nw = X - nx;
                }
            }
            if (handle.indexOf('w') !== -1) {
                if (el.x + el.w > X && nx < X) {
                    var newW = (el.x + el.w) - X;
                    nx = X;
                    nw = newW;
                }
            }
        }
    }
    return { nx: nx, ny: ny, nw: nw, nh: nh };
}

/* Clamp a locked guide's proposed new position so it cannot pass through
   any existing element edge. Returns the clamped position value. */
export function clampGuideThroughElements(g, proposedPos) {
    var oldPos = g.pos;
    var best   = proposedPos;
    var moving = proposedPos > oldPos;
    st.elements.forEach(function (el) {
        if (el.parentId) return;   /* only root elements for simplicity */
        if (g.axis === 'h') {
            if (moving) {
                if (el.y > oldPos && el.y < best) best = el.y;
            } else {
                var bottom = el.y + el.h;
                if (bottom < oldPos && bottom > best) best = bottom;
            }
        } else {
            if (moving) {
                if (el.x > oldPos && el.x < best) best = el.x;
            } else {
                var right = el.x + el.w;
                if (right < oldPos && right > best) best = right;
            }
        }
    });
    return best;
}

/* Stop a proposed move from crossing any locked guide barrier. */
export function clampToLockedGuides(el, nx, ny, nw, nh) {
    for (var i = 0; i < st.guides.length; i++) {
        var g = st.guides[i];
        if (!g.locked) continue;
        if (g.axis === 'h') {
            var Y = g.pos;
            if (el.y + el.h <= Y) {
                ny = Math.min(ny, Y - nh);
            } else if (el.y >= Y) {
                ny = Math.max(ny, Y);
            }
        } else {
            var X = g.pos;
            if (el.x + el.w <= X) {
                nx = Math.min(nx, X - nw);
            } else if (el.x >= X) {
                nx = Math.max(nx, X);
            }
        }
    }
    return { nx: nx, ny: ny };
}

/* Try to move/resize el to (nx,ny,nw,nh). Applies if valid. */
export function tryApply(el, nx, ny, nw, nh) {
    nw = Math.max(MIN_SIZE, nw);
    nh = Math.max(MIN_SIZE, nh);

    if (!el.parentId) {
        nx = Math.max(0, Math.min(nx, CANVAS_W - nw));
        ny = Math.max(0, Math.min(ny, CANVAS_H - nh));
    }

    var gc = clampToLockedGuides(el, nx, ny, nw, nh);
    nx = gc.nx; ny = gc.ny;

    if (!proposedConflicts(el, nx, ny, nw, nh) &&
        fitsInParent(el, nx, ny, nw, nh)) {
        el.x = nx; el.y = ny; el.w = nw; el.h = nh;
    }
}

/* Auto-position a new element to avoid collisions. */
export function findFreePosition(el) {
    var x = 40, y = 40;
    var attempts = 0;
    while (attempts < 60) {
        if (!proposedConflicts(
            { id: el.id, parentId: null, ml: el.ml, mr: el.mr, mt: el.mt, mb: el.mb },
            x, y, el.w, el.h)) {
            break;
        }
        x += 28; y += 28;
        if (x + el.w + el.mr > CANVAS_W - 20) { x = 40; y += 60; }
        attempts++;
    }
    el.x = x; el.y = y;
}
