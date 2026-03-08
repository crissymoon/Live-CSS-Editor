/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* Public API: init(), getState(), loadState().
   Wires up DOM event listeners and restores persisted session. */

import { CANVAS_W, CANVAS_H } from './constants.js';
import { st, saveToStorage, loadFromStorage } from './state.js';
import { makeEl, byId } from './elements.js';
import { findFreePosition } from './geometry.js';
import { saveJSON, loadJSON, buildContext } from './io.js';
import { render, renderRulers, renderGuides } from './render.js';
import { onMousemove, onMouseup, setupKeyboard } from './input.js';

export function init() {
    console.log('[wireframe] init called');
    st.overlay  = document.getElementById('wireframeOverlay');
    st.canvasEl = document.getElementById('wfCanvas');
    st.propsEl  = document.getElementById('wfProps');
    st.rulerH   = document.getElementById('wfRulerH');
    st.rulerV   = document.getElementById('wfRulerV');

    var openBtn    = document.getElementById('wireframeBtn');
    var closeBtn   = document.getElementById('wfCloseBtn');
    var addBtn     = document.getElementById('wfAddBtn');
    var clearBtn   = document.getElementById('wfClearBtn');
    var saveBtn    = document.getElementById('wfSaveBtn');
    var loadBtn    = document.getElementById('wfLoadBtn');
    var contextBtn = document.getElementById('wfContextBtn');

    /* Detect standalone mode before the guard so we can adjust requirements */
    var _standalone = document.body && document.body.dataset.wfStandalone === '1';

    /* In standalone mode there is no openBtn -- the page IS the wireframe.
       In embedded mode openBtn must exist or the overlay can never be opened. */
    if (!st.overlay || !st.canvasEl || !st.propsEl || (!_standalone && !openBtn)) {
        console.error('[wireframe] init: required DOM element missing', {
            overlay:    !!st.overlay,
            canvasEl:   !!st.canvasEl,
            propsEl:    !!st.propsEl,
            openBtn:    !!openBtn,
            standalone: _standalone
        });
        return;
    }
    console.log('[wireframe] init: elements found, standalone=' + _standalone + ', wiring buttons');

    /* Restore last session */
    loadFromStorage();

    /* ── Open / close ── */
    /* openBtn only exists in embedded (index.php) mode */
    openBtn && openBtn.addEventListener('click', function () {
        st.overlay.classList.remove('hidden');
        renderRulers();
        render();
    });

    /* In standalone mode the overlay is already visible -- render immediately */
    if (_standalone) {
        renderRulers();
        render();
    }

    closeBtn && closeBtn.addEventListener('click', function () {
        if (_standalone) {
            if (window.history.length > 1) { window.history.back(); }
            else { window.location.href = '../../index.php'; }
        } else {
            st.overlay.classList.add('hidden');
        }
    });

    /* Close on overlay background click */
    st.overlay.addEventListener('mousedown', function (e) {
        if (e.target === st.overlay) st.overlay.classList.add('hidden');
    });

    /* ── Ruler drags: create guides ── */
    if (st.rulerH) {
        st.rulerH.addEventListener('mousedown', function (e) {
            e.preventDefault();
            var rect = st.canvasEl.getBoundingClientRect();
            var pos  = Math.max(0, Math.min(Math.round(e.clientY - rect.top), CANVAS_H));
            var g    = { id: 'g_' + (st.nextGuideId++), axis: 'h', pos: pos, locked: false };
            st.guides.push(g);
            st.guideDrag = { guideId: g.id, axis: 'h' };
            renderGuides();
        });
    }

    if (st.rulerV) {
        st.rulerV.addEventListener('mousedown', function (e) {
            e.preventDefault();
            var rect = st.canvasEl.getBoundingClientRect();
            var pos  = Math.max(0, Math.min(Math.round(e.clientX - rect.left), CANVAS_W));
            var g    = { id: 'g_' + (st.nextGuideId++), axis: 'v', pos: pos, locked: false };
            st.guides.push(g);
            st.guideDrag = { guideId: g.id, axis: 'v' };
            renderGuides();
        });
    }

    /* ── Toolbar actions ── */
    addBtn && addBtn.addEventListener('click', function () {
        var el = makeEl(null);
        findFreePosition(el);
        st.elements.push(el);
        st.selId = el.id;
        render();
    });

    clearBtn && clearBtn.addEventListener('click', function () {
        if (st.elements.length === 0 || window.confirm('Clear all elements?')) {
            st.elements = [];
            st.selId    = null;
            render();
        }
    });

    saveBtn && saveBtn.addEventListener('click', function () { saveJSON(saveBtn); });

    /* Load JSON -- create a temporary file input at body level so
       programmatic .click() works reliably in Tauri's WebView */
    loadBtn && loadBtn.addEventListener('click', function () {
        var tmp = document.createElement('input');
        tmp.type      = 'file';
        tmp.accept    = '.json,.wf.json';
        tmp.style.cssText = 'position:fixed;top:-100px;left:-100px;opacity:0;';
        document.body.appendChild(tmp);
        tmp.addEventListener('change', function () {
            if (tmp.files && tmp.files[0]) loadJSON(tmp.files[0], render);
            document.body.removeChild(tmp);
        });
        tmp.click();
    });

    contextBtn && contextBtn.addEventListener('click', function () {
        var text = buildContext();
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).then(function () {
                var orig = contextBtn.textContent;
                contextBtn.textContent = 'Copied!';
                setTimeout(function () { contextBtn.textContent = orig; }, 1600);
            });
        } else {
            /* fallback: temporary textarea */
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

    /* ── Canvas mouse events ── */

    /* Deselect when clicking empty canvas */
    st.canvasEl.addEventListener('mousedown', function (e) {
        if (e.target === st.canvasEl) {
            st.selId      = null;
            st.selGuideId = null;
            render();
        }
    });

    /* Delegated resize-handle mousedown */
    st.canvasEl.addEventListener('mousedown', function (e) {
        var hd = e.target.closest ? e.target.closest('.wf-handle') : null;
        if (!hd) return;
        e.stopPropagation();
        e.preventDefault();
        var elId = hd.dataset.elid;
        var el   = byId(elId);
        if (!el) return;
        st.drag = {
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

    /* Delegated dblclick on canvas: delete a guide.
       Must be delegated because renderGuides() rebuilds guide DOM between
       click 1 and click 2, which would break per-element dblclick listeners. */
    st.canvasEl.addEventListener('dblclick', function (e) {
        var guideEl = e.target.closest
            ? e.target.closest('.wf-guide-h, .wf-guide-v')
            : null;
        if (!guideEl) return;
        var gid = guideEl.dataset.guideId;
        if (!gid) return;
        e.stopPropagation();
        st.guides = st.guides.filter(function (x) { return x.id !== gid; });
        saveToStorage();
        renderGuides();
    });

    setupKeyboard(st.overlay);
}

/* ── Bridge API ───────────────────────────────────────────── */

export function getState() {
    return {
        version:     1,
        nextId:      st.nextId,
        nextGuideId: st.nextGuideId,
        elements:    st.elements,
        guides:      st.guides
    };
}

export function loadState(data) {
    try {
        if (!data || !Array.isArray(data.elements)) {
            console.warn('[wireframe] loadState: invalid payload - expected .elements array');
            return false;
        }
        st.elements    = data.elements;
        st.nextId      = data.nextId      || (st.elements.length + 1);
        st.guides      = Array.isArray(data.guides) ? data.guides : [];
        st.nextGuideId = data.nextGuideId || (st.guides.length + 1);
        st.selId       = null;
        saveToStorage();
        render();
        console.log('[wireframe] loadState: applied ' + st.elements.length + ' element(s) from bridge update');
        return true;
    } catch (e) {
        console.error('[wireframe] loadState exception: ' + e.message);
        return false;
    }
}
