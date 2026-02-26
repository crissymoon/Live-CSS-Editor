/**
 * storage.js — localStorage persistence for saved projects
 * Attached to window.LiveCSS.storage
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

    return {
        getSavedProjects: getSavedProjects,
        saveProject:      saveProject,
        deleteProject:    deleteProject,
        saveAutosave:     saveAutosave,
        loadAutosave:     loadAutosave,
        clearAutosave:    clearAutosave
    };

}());
