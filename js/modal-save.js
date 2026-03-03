/**
 * modal-save.js — Save project modal
 * Attached to window.LiveCSS.modalSave
 *
 * Call LiveCSS.modalSave.init() after storage and editor are ready.
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.modalSave = (function () {

    var modal, nameInput, existingList;

    function init() {
        modal        = document.getElementById('saveModal');
        nameInput    = document.getElementById('saveNameInput');
        existingList = document.getElementById('saveExistingList');

        // Open
        document.getElementById('saveBtn').addEventListener('click', function () {
            nameInput.value = '';
            renderExisting();
            modal.classList.remove('hidden');
            setTimeout(function () { nameInput.focus(); }, 100);
        });

        // Confirm save
        document.getElementById('saveConfirmBtn').addEventListener('click', function () {
            var name = nameInput.value.trim();
            if (!name) { alert('Please enter a project name.'); return; }

            var projects = LiveCSS.storage.getSavedProjects();
            if (projects[name] && !confirm('A save named "' + name + '" already exists. Overwrite it?')) return;

            var html = LiveCSS.editor.getHtmlEditor().getValue();
            var css  = LiveCSS.editor.getCssEditor().getValue();
            var js   = LiveCSS.editor.getJsEditor().getValue();

            // Save to localStorage (legacy)
            LiveCSS.storage.saveProject(name, html, css, js);

            // Also save to SQLite so the bridge and Copilot can access it
            LiveCSS.storage.saveDbProject(name, html, css, js, 'browser')
                .then(function (result) {
                    if (result && result.success) {
                        console.log('[save] Saved "' + name + '" to both localStorage and SQLite');
                    } else {
                        console.error('[save] SQLite save failed for "' + name + '" -- localStorage save was still successful');
                    }
                })
                .catch(function (e) {
                    console.error('[save] SQLite save error for "' + name + '":', e.message);
                });

            close();
        });

        // Close buttons
        document.getElementById('saveCancelBtn').addEventListener('click', close);
        document.getElementById('saveModalClose').addEventListener('click', close);
        modal.addEventListener('click', function (e) { if (e.target === modal) close(); });
    }

    function close() { modal.classList.add('hidden'); }

    function renderExisting() {
        var projects   = LiveCSS.storage.getSavedProjects();
        var keys       = Object.keys(projects).sort();
        var escapeHtml = LiveCSS.utils.escapeHtml;
        var escapeAttr = LiveCSS.utils.escapeAttr;

        if (!keys.length) {
            existingList.innerHTML = '<p class="modal-hint">No existing saves.</p>';
            return;
        }

        var html = '<p class="modal-hint">Existing saves (click to overwrite):</p>';
        for (var i = 0; i < keys.length; i++) {
            var name = keys[i];
            var ts   = new Date(projects[name].timestamp).toLocaleString();
            html += '<div class="save-slot" data-name="' + escapeAttr(name) + '">';
            html += '<span class="save-slot-name">' + escapeHtml(name) + '</span>';
            html += '<span class="save-slot-date">' + escapeHtml(ts) + '</span>';
            html += '</div>';
        }
        existingList.innerHTML = html;

        var slots = existingList.querySelectorAll('.save-slot');
        for (var j = 0; j < slots.length; j++) {
            slots[j].addEventListener('click', function () {
                nameInput.value = this.getAttribute('data-name');
            });
        }
    }

    return { init: init };

}());
