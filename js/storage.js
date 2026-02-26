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

    return { getSavedProjects: getSavedProjects, saveProject: saveProject, deleteProject: deleteProject };

}());
