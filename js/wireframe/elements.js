/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* Element factory, lookup helpers. */

import { st } from './state.js';

export function makeEl(parentId, x, y) {
    return {
        id:           'el_' + (st.nextId++),
        parentId:     parentId || null,
        x:            x !== undefined ? x : 40,
        y:            y !== undefined ? y : 40,
        w:            200,
        h:            140,
        label:        'Box',
        bgColor:      '#1a0f3d',
        borderColor:  '#4d31bf',
        borderWidth:  1,
        borderRadius: 0,
        mt: 8, mr: 8, mb: 8, ml: 8,
        pt: 8, pr: 8, pb: 8, pl: 8
    };
}

export function byId(id) {
    for (var i = 0; i < st.elements.length; i++) {
        if (st.elements[i].id === id) return st.elements[i];
    }
    return null;
}

export function byGuideId(id) {
    for (var i = 0; i < st.guides.length; i++) {
        if (st.guides[i].id === id) return st.guides[i];
    }
    return null;
}

export function siblingsOf(el) {
    return st.elements.filter(function (e) {
        return e.parentId === el.parentId && e.id !== el.id;
    });
}

export function childrenOf(parentId) {
    return st.elements.filter(function (e) { return e.parentId === parentId; });
}

export function isDescendant(el, ancestorId) {
    var pid = el.parentId;
    while (pid) {
        if (pid === ancestorId) return true;
        var p = byId(pid);
        pid = p ? p.parentId : null;
    }
    return false;
}
