/**
 * modal-load.js — Load project modal
 * Attached to window.LiveCSS.modalLoad
 *
 * Call LiveCSS.modalLoad.init() after storage and editor are ready.
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.modalLoad = (function () {

    var modal, slotList, emptyMsg;

    function init() {
        modal    = document.getElementById('loadModal');
        slotList = document.getElementById('loadSlotList');
        emptyMsg = document.getElementById('loadEmptyMsg');

        // Open
        document.getElementById('loadBtn').addEventListener('click', function () {
            renderSlots();
            modal.classList.remove('hidden');
        });

        // Close buttons
        document.getElementById('loadCancelBtn').addEventListener('click', close);
        document.getElementById('loadModalClose').addEventListener('click', close);
        modal.addEventListener('click', function (e) { if (e.target === modal) close(); });
    }

    function close() { modal.classList.add('hidden'); }

    function renderSlots() {
        var projects   = LiveCSS.storage.getSavedProjects();
        var keys       = Object.keys(projects).sort();
        var escapeHtml = LiveCSS.utils.escapeHtml;
        var escapeAttr = LiveCSS.utils.escapeAttr;
        var autosave   = LiveCSS.storage.loadAutosave();

        // Start with localStorage content
        var hasLocalContent = keys.length > 0 || autosave;

        // Build local slots first, then async-load SQLite slots
        buildLocalSlots(keys, projects, autosave, escapeHtml, escapeAttr);

        // Fetch SQLite projects and append them
        LiveCSS.storage.listDbProjects()
            .then(function (dbProjects) {
                if (!dbProjects || dbProjects.length === 0) {
                    if (!hasLocalContent) {
                        emptyMsg.classList.remove('hidden');
                    }
                    return;
                }
                emptyMsg.classList.add('hidden');
                appendDbSlots(dbProjects, escapeHtml, escapeAttr);
            })
            .catch(function (e) {
                console.error('[load] Failed to fetch SQLite projects:', e.message);
                if (!hasLocalContent) {
                    emptyMsg.classList.remove('hidden');
                }
            });
    }

    function buildLocalSlots(keys, projects, autosave, escapeHtml, escapeAttr) {
        var hasContent = keys.length > 0 || autosave;
        if (!hasContent) {
            slotList.innerHTML = '';
            // Don't show empty yet -- wait for DB check
            return;
        }
        emptyMsg.classList.add('hidden');

        var html = '';

        // ── Autosave entry at top ──────────────────────────────────
        if (autosave) {
            var asDate = autosave.ts ? new Date(autosave.ts).toLocaleString() : '';
            html += '<div class="load-slot load-slot-autosave">';
            html +=   '<div class="load-slot-info">';
            html +=     '<span class="load-slot-name">Autosave</span>';
            html +=     '<span class="load-slot-date">' + escapeHtml(asDate) + '</span>';
            html +=   '</div>';
            html +=   '<div class="load-slot-actions">';
            html +=     '<button class="btn-action load-autosave-btn">Load</button>';
            html +=   '</div>';
            html += '</div>';
        }

        // ── Named project entries ──────────────────────────────────
        for (var i = 0; i < keys.length; i++) {
            var name = keys[i];
            var ts   = new Date(projects[name].timestamp).toLocaleString();
            html += '<div class="load-slot">';
            html +=   '<div class="load-slot-info">';
            html +=     '<span class="load-slot-name">' + escapeHtml(name) + '</span>';
            html +=     '<span class="load-slot-date">' + escapeHtml(ts) + '</span>';
            html +=   '</div>';
            html +=   '<div class="load-slot-actions">';
            html +=     '<button class="btn-action load-slot-load" data-name="' + escapeAttr(name) + '">Load</button>';
            html +=     '<button class="btn-action btn-danger load-slot-delete" data-name="' + escapeAttr(name) + '">Delete</button>';
            html +=   '</div>';
            html += '</div>';
        }
        slotList.innerHTML = html;

        // Autosave load handler
        var asBtn = slotList.querySelector('.load-autosave-btn');
        if (asBtn && autosave) {
            asBtn.addEventListener('click', function () {
                LiveCSS.editor.getHtmlEditor().setValue(autosave.html || '');
                LiveCSS.editor.getCssEditor().setValue(autosave.css  || '');
                LiveCSS.editor.getJsEditor().setValue(autosave.js    || '');
                LiveCSS.editor.updatePreview();
                close();
            });
        }

        // Named project load button handlers
        var loadBtns = slotList.querySelectorAll('.load-slot-load');
        for (var j = 0; j < loadBtns.length; j++) {
            loadBtns[j].addEventListener('click', function () {
                var n = this.getAttribute('data-name');
                var p = LiveCSS.storage.getSavedProjects();
                if (!p[n]) return;
                LiveCSS.editor.getHtmlEditor().setValue(p[n].html);
                LiveCSS.editor.getCssEditor().setValue(p[n].css);
                if (p[n].js !== undefined) { LiveCSS.editor.getJsEditor().setValue(p[n].js); }
                LiveCSS.editor.updatePreview();
                close();
            });
        }

        // Named project delete button handlers
        var delBtns = slotList.querySelectorAll('.load-slot-delete');
        for (var k = 0; k < delBtns.length; k++) {
            delBtns[k].addEventListener('click', function () {
                var n = this.getAttribute('data-name');
                if (confirm('Delete save "' + n + '"?')) {
                    LiveCSS.storage.deleteProject(n);
                    // Also delete from SQLite
                    LiveCSS.storage.deleteDbProject(n)
                        .catch(function (e) { console.error('[load] SQLite delete failed:', e.message); });
                    renderSlots();
                }
            });
        }
    }

    // ── SQLite project slots ───────────────────────────────────────
    function appendDbSlots(dbProjects, escapeHtml, escapeAttr) {
        // Filter out projects that already exist in localStorage to avoid duplicates
        var localProjects = LiveCSS.storage.getSavedProjects();
        var localKeys     = Object.keys(localProjects);
        var filtered      = dbProjects.filter(function (p) {
            return localKeys.indexOf(p.name) === -1;
        });

        if (filtered.length === 0) return;

        var html = '<div class="load-section-header" style="margin-top:12px;padding:6px 0;font-size:11px;color:#8888a0;letter-spacing:0.06em;text-transform:uppercase;border-top:1px solid rgba(255,255,255,0.06);">Saved in Database</div>';
        for (var i = 0; i < filtered.length; i++) {
            var p    = filtered[i];
            var src  = p.source === 'vscode-copilot' ? ' [from Copilot]' : '';
            html += '<div class="load-slot load-slot-db">';
            html +=   '<div class="load-slot-info">';
            html +=     '<span class="load-slot-name">' + escapeHtml(p.name) + escapeHtml(src) + '</span>';
            html +=     '<span class="load-slot-date">' + escapeHtml(p.updated_at || '') + '</span>';
            html +=   '</div>';
            html +=   '<div class="load-slot-actions">';
            html +=     '<button class="btn-action load-db-load" data-name="' + escapeAttr(p.name) + '">Load</button>';
            html +=     '<button class="btn-action btn-danger load-db-delete" data-name="' + escapeAttr(p.name) + '">Delete</button>';
            html +=   '</div>';
            html += '</div>';
        }

        // Append to existing slot list
        var container = document.createElement('div');
        container.innerHTML = html;
        while (container.firstChild) {
            slotList.appendChild(container.firstChild);
        }

        // Wire load handlers for DB slots
        var dbLoadBtns = slotList.querySelectorAll('.load-db-load');
        for (var j = 0; j < dbLoadBtns.length; j++) {
            dbLoadBtns[j].addEventListener('click', function () {
                var n = this.getAttribute('data-name');
                LiveCSS.storage.loadDbProject(n)
                    .then(function (project) {
                        if (!project) {
                            console.error('[load] SQLite project "' + n + '" returned null');
                            alert('Could not load project "' + n + '" from database.');
                            return;
                        }
                        try {
                            LiveCSS.editor.getHtmlEditor().setValue(project.html || '');
                            LiveCSS.editor.getCssEditor().setValue(project.css || '');
                            LiveCSS.editor.getJsEditor().setValue(project.js || '');
                            LiveCSS.editor.updatePreview();
                            console.log('[load] Loaded "' + n + '" from SQLite into all 3 editors');
                        } catch (e) {
                            console.error('[load] Failed to set editor values for "' + n + '":', e.message);
                        }
                        close();
                    })
                    .catch(function (e) {
                        console.error('[load] SQLite load error for "' + n + '":', e.message);
                        alert('Error loading project from database. Check console for details.');
                    });
            });
        }

        // Wire delete handlers for DB slots
        var dbDelBtns = slotList.querySelectorAll('.load-db-delete');
        for (var k = 0; k < dbDelBtns.length; k++) {
            dbDelBtns[k].addEventListener('click', function () {
                var n = this.getAttribute('data-name');
                if (confirm('Delete "' + n + '" from database?')) {
                    LiveCSS.storage.deleteDbProject(n)
                        .then(function (ok) {
                            if (ok) renderSlots();
                            else console.error('[load] SQLite delete failed for "' + n + '"');
                        })
                        .catch(function (e) {
                            console.error('[load] SQLite delete error:', e.message);
                        });
                }
            });
        }
    }

    return { init: init };

}());
