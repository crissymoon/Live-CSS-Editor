/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* Mutable state shared across all wireframe sub-modules.
   Import `st` and mutate its properties; never re-assign `st` itself. */

import { LS_KEY } from './constants.js';

export var st = {
    elements:    [],
    nextId:      1,
    selId:       null,
    drag:        null,
    rafId:       null,
    guides:      [],
    nextGuideId: 1,
    guideDrag:   null,
    selGuideId:  null,
    /* DOM refs -- populated by init() */
    overlay:  null,
    canvasEl: null,
    propsEl:  null,
    rulerH:   null,
    rulerV:   null
};

export function saveToStorage() {
    try {
        localStorage.setItem(LS_KEY, JSON.stringify({
            version:     1,
            nextId:      st.nextId,
            nextGuideId: st.nextGuideId,
            elements:    st.elements,
            guides:      st.guides
        }));
    } catch (e) {}
}

export function loadFromStorage() {
    try {
        var raw = localStorage.getItem(LS_KEY);
        if (!raw) return;
        var data = JSON.parse(raw);
        if (!data || !Array.isArray(data.elements)) return;
        st.elements    = data.elements;
        st.nextId      = data.nextId      || (st.elements.length + 1);
        st.guides      = Array.isArray(data.guides) ? data.guides : [];
        st.nextGuideId = data.nextGuideId || (st.guides.length + 1);
    } catch (e) {}
}
