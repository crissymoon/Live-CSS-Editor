/**
 * agent-prompts.js -- Prompts editor tab.
 * Depends on: agent-core.js
 */
'use strict';

(function (LiveCSS) {

    var C   = LiveCSS._agentCore;
    var dom = C.dom;

    var promptsData = null;

    function loadPromptsEditor() {
        if (promptsData) { renderPromptsEditor(); return; }

        var stored = null;
        try { stored = localStorage.getItem('agent-prompts'); } catch(e) {}
        if (stored) {
            try { promptsData = JSON.parse(stored); } catch(e) {}
            if (promptsData) { renderPromptsEditor(); return; }
        }

        fetch('ai/agent/prompts.json')
            .then(function (r) { return r.json(); })
            .then(function (d) { promptsData = d; renderPromptsEditor(); })
            .catch(function () { dom.promptsEditor.textContent = 'Failed to load prompts.json'; });
    }

    function renderPromptsEditor() {
        if (!promptsData) { return; }
        var html = [];
        var editableKeys = ['system', 'persona', 'reasoning_prefix', 'no_stream_instruction', 'diff_review'];
        editableKeys.forEach(function (key) {
            if (!promptsData[key]) { return; }
            html.push('<div class="agent-prompts-field">');
            html.push('<label>' + key.replace(/_/g, ' ') + '</label>');
            html.push('<textarea class="agent-prompts-textarea" data-key="' + key + '">' + C.esc(promptsData[key]) + '</textarea>');
            html.push('</div>');
        });

        if (promptsData.tasks) {
            html.push('<div class="agent-context-section-title" style="border-top:1px solid var(--ag-border);padding-top:10px;margin-top:4px;">Task Prompts</div>');
            Object.keys(promptsData.tasks).forEach(function (task) {
                html.push('<div class="agent-prompts-field">');
                html.push('<label>task: ' + task + '</label>');
                html.push('<textarea class="agent-prompts-textarea" data-key="tasks.' + task + '">' + C.esc(promptsData.tasks[task]) + '</textarea>');
                html.push('</div>');
            });
        }
        dom.promptsEditor.innerHTML = html.join('');
    }

    function savePrompts() {
        if (!promptsData) { return; }
        dom.promptsEditor.querySelectorAll('.agent-prompts-textarea').forEach(function (ta) {
            var key = ta.dataset.key;
            if (key.startsWith('tasks.')) {
                var tname = key.slice(6);
                if (promptsData.tasks) { promptsData.tasks[tname] = ta.value; }
            } else {
                promptsData[key] = ta.value;
            }
        });
        try {
            localStorage.setItem('agent-prompts', JSON.stringify(promptsData));
            C.toast('Prompts saved to local storage.', 'success');
        } catch(e) { C.toast('Could not save: ' + e.message, 'error'); }
    }

    C.loadPromptsEditor   = loadPromptsEditor;
    C.renderPromptsEditor = renderPromptsEditor;
    C.savePrompts         = savePrompts;

}(window.LiveCSS = window.LiveCSS || {}));
