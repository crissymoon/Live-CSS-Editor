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

    // ── SQLite-backed project storage (via vscode-bridge API) ──────

    // Use env-detect helper so the path is correct whether the project is served
    // from the root (PHP built-in: /vscode-bridge/...) or a subdirectory
    // (WASM node server: /my_project/vscode-bridge/...).
    var _env         = (global.LiveCSS && global.LiveCSS.env) || { resolve: function (p) { return p; } };
    var PROJECTS_API = _env.resolve('/vscode-bridge/api/projects.php');

    /**
     * Fetch all projects from the SQLite database.
     * Returns a Promise that resolves to an array of {name, source, updated_at, html_len, css_len, js_len}.
     */
    function listDbProjects() {
        return fetch(PROJECTS_API + '?action=list', { cache: 'no-store' })
            .then(function (res) {
                if (!res.ok) {
                    console.error('[storage] listDbProjects HTTP ' + res.status);
                    return [];
                }
                return res.json();
            })
            .then(function (data) {
                if (data && data.success) return data.projects || [];
                console.error('[storage] listDbProjects failed:', data ? data.error : 'no data');
                return [];
            })
            .catch(function (e) {
                console.error('[storage] listDbProjects network error:', e.message);
                return [];
            });
    }

    /**
     * Load a single project from SQLite by name.
     * Returns a Promise that resolves to {name, html, css, js, source, updated_at} or null.
     */
    function loadDbProject(name) {
        return fetch(PROJECTS_API + '?action=get&name=' + encodeURIComponent(name), { cache: 'no-store' })
            .then(function (res) {
                if (!res.ok) {
                    console.error('[storage] loadDbProject HTTP ' + res.status + ' for "' + name + '"');
                    return null;
                }
                return res.json();
            })
            .then(function (data) {
                if (data && data.success) return data.project;
                console.error('[storage] loadDbProject failed for "' + name + '":', data ? data.error : 'no data');
                return null;
            })
            .catch(function (e) {
                console.error('[storage] loadDbProject network error:', e.message);
                return null;
            });
    }

    /**
     * Save a project to SQLite. Auto-backs up previous version.
     * Returns a Promise that resolves to {success, name, updatedAt, backedUp} or null.
     */
    function saveDbProject(name, html, css, js, source) {
        return fetch(PROJECTS_API + '?action=save', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({
                name:   name,
                html:   html || '',
                css:    css  || '',
                js:     js   || '',
                source: source || 'browser',
            }),
        })
        .then(function (res) {
            if (!res.ok) {
                console.error('[storage] saveDbProject HTTP ' + res.status);
                return null;
            }
            return res.json();
        })
        .then(function (data) {
            if (data && data.success) {
                console.log('[storage] Saved "' + name + '" to SQLite at ' + data.updatedAt + (data.backedUp ? ' (backup created)' : ''));
                return data;
            }
            console.error('[storage] saveDbProject failed:', data ? data.error : 'no data');
            return null;
        })
        .catch(function (e) {
            console.error('[storage] saveDbProject network error:', e.message);
            return null;
        });
    }

    /**
     * Delete a project from SQLite.
     */
    function deleteDbProject(name) {
        return fetch(PROJECTS_API + '?action=delete&name=' + encodeURIComponent(name), {
            method: 'POST',
        })
        .then(function (res) {
            if (!res.ok) {
                console.error('[storage] deleteDbProject HTTP ' + res.status);
                return false;
            }
            return res.json();
        })
        .then(function (data) {
            if (data && data.success) {
                console.log('[storage] Deleted "' + name + '" from SQLite');
                return true;
            }
            console.error('[storage] deleteDbProject failed:', data ? data.error : 'no data');
            return false;
        })
        .catch(function (e) {
            console.error('[storage] deleteDbProject network error:', e.message);
            return false;
        });
    }

    /**
     * List backups for a project from SQLite.
     */
    function listDbBackups(name) {
        return fetch(PROJECTS_API + '?action=backups&name=' + encodeURIComponent(name), { cache: 'no-store' })
            .then(function (res) {
                if (!res.ok) return [];
                return res.json();
            })
            .then(function (data) {
                if (data && data.success) return data.backups || [];
                console.error('[storage] listDbBackups failed:', data ? data.error : 'no data');
                return [];
            })
            .catch(function (e) {
                console.error('[storage] listDbBackups network error:', e.message);
                return [];
            });
    }

    /**
     * Poll for a project update signal (from Copilot).
     * Returns a Promise that resolves to {hasUpdate, name, source, updatedAt} or {hasUpdate: false}.
     */
    function pollProjectUpdate() {
        return fetch(PROJECTS_API + '?action=poll_update', { cache: 'no-store' })
            .then(function (res) {
                if (!res.ok) return { hasUpdate: false };
                return res.json();
            })
            .then(function (data) {
                if (data && data.success) return data;
                return { hasUpdate: false };
            })
            .catch(function (e) {
                console.error('[storage] pollProjectUpdate network error:', e.message);
                return { hasUpdate: false };
            });
    }

    /**
     * Acknowledge a project update signal.
     */
    function ackProjectUpdate() {
        return fetch(PROJECTS_API + '?action=ack_update', {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({ acked: true }),
        })
        .then(function (res) { return res.json(); })
        .catch(function (e) {
            console.error('[storage] ackProjectUpdate network error:', e.message);
        });
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
        loadUIState:      loadUIState,
        // SQLite-backed methods
        listDbProjects:    listDbProjects,
        loadDbProject:     loadDbProject,
        saveDbProject:     saveDbProject,
        deleteDbProject:   deleteDbProject,
        listDbBackups:     listDbBackups,
        pollProjectUpdate: pollProjectUpdate,
        ackProjectUpdate:  ackProjectUpdate,
    };

}());
