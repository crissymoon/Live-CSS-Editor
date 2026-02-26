/**
 * storage.js — localStorage persistence for saved projects
 * Attached to window.LiveCSS.storage
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.storage = (function () {

    var STORAGE_KEY = 'liveCssEditor_projects';

    /** Return all saved projects as a plain object keyed by project name */
    function getSavedProjects() {
        try {
            var raw = localStorage.getItem(STORAGE_KEY);
            if (raw) return JSON.parse(raw);
        } catch (e) { /* corrupt data — fall through */ }
        return {};
    }

    /** Persist a project by name, overwriting any existing entry */
    function saveProject(name, htmlCode, cssCode, jsCode) {
        var projects = getSavedProjects();
        projects[name] = { html: htmlCode, css: cssCode, js: jsCode || '', timestamp: Date.now() };
        localStorage.setItem(STORAGE_KEY, JSON.stringify(projects));
    }

    /** Remove a saved project by name */
    function deleteProject(name) {
        var projects = getSavedProjects();
        delete projects[name];
        localStorage.setItem(STORAGE_KEY, JSON.stringify(projects));
    }

    // ── Auto-save: persists the current working state across reloads ──

    var AUTOSAVE_KEY = 'liveCssEditor_autosave';

    function saveAutosave(htmlCode, cssCode, jsCode) {
        try {
            localStorage.setItem(AUTOSAVE_KEY, JSON.stringify({
                html: htmlCode, css: cssCode, js: jsCode, ts: Date.now()
            }));
        } catch (e) { /* storage full — ignore */ }
    }

    function loadAutosave() {
        try {
            var raw = localStorage.getItem(AUTOSAVE_KEY);
            if (raw) { return JSON.parse(raw); }
        } catch (e) { /* corrupt — fall through */ }
        return null;
    }

    function clearAutosave() {
        localStorage.removeItem(AUTOSAVE_KEY);
    }

    // ── Session history: one snapshot per session, max 8 entries ──────

    var HISTORY_KEY = 'liveCssEditor_history';
    var HISTORY_MAX = 8;

    function pushHistory(html, css, js) {
        var hist = getHistory();
        // Skip if content is identical to the most recent entry
        if (hist.length > 0) {
            var last = hist[0];
            if (last.html === html && last.css === css && last.js === (js || '')) { return; }
        }
        hist.unshift({ html: html || '', css: css || '', js: js || '', ts: Date.now() });
        if (hist.length > HISTORY_MAX) { hist = hist.slice(0, HISTORY_MAX); }
        try {
            localStorage.setItem(HISTORY_KEY, JSON.stringify(hist));
        } catch (e) { /* storage full — ignore */ }
    }

    function getHistory() {
        try {
            var raw = localStorage.getItem(HISTORY_KEY);
            if (raw) { return JSON.parse(raw); }
        } catch (e) { /* corrupt — fall through */ }
        return [];
    }

    // ── UI state: panel layout, tool positions, visibility ───────

    var UI_STATE_KEY = 'livecss-ui-state';

    function saveUIState(state) {
        try { localStorage.setItem(UI_STATE_KEY, JSON.stringify(state)); } catch (e) { /* */ }
    }

    function loadUIState() {
        try {
            var raw = localStorage.getItem(UI_STATE_KEY);
            if (raw) { return JSON.parse(raw); }
        } catch (e) { /* corrupt */ }
        return null;
    }

    return {
        getSavedProjects: getSavedProjects,
        saveProject:      saveProject,
        deleteProject:    deleteProject,
        saveAutosave:     saveAutosave,
        loadAutosave:     loadAutosave,
        clearAutosave:    clearAutosave,
        pushHistory:      pushHistory,
        getHistory:       getHistory,
        saveUIState:      saveUIState,
        loadUIState:      loadUIState
    };

}());
