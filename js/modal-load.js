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

        var hasContent = keys.length > 0 || autosave;
        if (!hasContent) {
            slotList.innerHTML = '';
            emptyMsg.classList.remove('hidden');
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
                    renderSlots();
                }
            });
        }
    }

    return { init: init };

}());
