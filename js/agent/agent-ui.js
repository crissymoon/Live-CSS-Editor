/**
 * agent-ui.js -- HTML builders for the agent modal and cacheDOM.
 * Depends on: agent-core.js
 */
'use strict';

(function (LiveCSS) {

    var C     = LiveCSS._agentCore;
    var state = C.state;
    var dom   = C.dom;

    // -----------------------------------------------------------------------
    // Build + mount the whole modal
    // -----------------------------------------------------------------------

    C.buildUI = function () {
        var overlay = C.el('div', 'agent-overlay');
        overlay.id  = 'agentOverlay';
        document.body.appendChild(overlay);

        var modal = C.el('div', 'agent-modal theme-' + state.theme);
        modal.id  = 'agentModal';
        modal.innerHTML = buildModalHTML();
        document.body.appendChild(modal);

        var trigger = C.el('button', 'agent-trigger');
        trigger.id          = 'agentTrigger';
        trigger.textContent = 'AGT';
        trigger.title       = 'Open Code Agent';
        document.body.appendChild(trigger);

        dom.overlay = overlay;
        dom.modal   = modal;
        dom.trigger = trigger;

        C.cacheDOM();
    };

    // -----------------------------------------------------------------------
    // Modal HTML assembly
    // -----------------------------------------------------------------------

    function buildModalHTML() {
        return [
            '<div class="agent-header" id="agentHeader">',
            '  <span class="agent-title">Code Agent</span>',
            '  <div class="agent-header-controls">',
            '    <div class="agent-theme-dots" id="agentThemeDots">' + buildThemeDots() + '</div>',
            '    <button class="agent-btn agent-btn-ghost agent-btn-icon" id="agentMinBtn" title="Minimize">&#8722;</button>',
            '    <button class="agent-btn agent-btn-ghost agent-btn-icon" id="agentCloseBtn" title="Close">&#215;</button>',
            '  </div>',
            '</div>',
            '<div class="agent-tabs" id="agentTabs">',
            '  <button class="agent-tab agent-tab-active" data-tab="run">Run</button>',
            '  <button class="agent-tab" data-tab="chat">Chat</button>',
            '  <button class="agent-tab" data-tab="diff">Diff</button>',
            '  <button class="agent-tab" data-tab="versions">Versions</button>',
            '  <button class="agent-tab" data-tab="context">Context</button>',
            '  <button class="agent-tab" data-tab="prompts">Prompts</button>',
            '</div>',
            '<div class="agent-body">',
            buildRunPane(),
            buildChatPane(),
            buildDiffPane(),
            buildVersionsPane(),
            buildContextPane(),
            buildPromptsPane(),
            '</div>',
            '<div class="agent-status-bar" id="agentStatusBar">',
            '  <span class="agent-status-dot" id="agentStatusDot"></span>',
            '  <span id="agentStatusText">Ready</span>',
            '</div>',
        ].join('');
    }

    function buildThemeDots() {
        return C.THEMES.map(function (t) {
            var active = t === state.theme ? ' dot-active' : '';
            return '<span class="agent-theme-dot dot-' + t + active + '" data-theme="' + t + '" title="' + C.THEME_LABELS[t] + '"></span>';
        }).join('');
    }

    function buildRunPane() {
        return [
            '<div class="agent-pane agent-pane-active" data-pane="run">',
            '  <div class="agent-split">',
            '    <div class="agent-outline-col" id="agentOutlineCol">',
            '      <div class="outline-header">Outline</div>',
            '      <div id="agentOutline"><p class="outline-empty">Select a source above</p></div>',
            '    </div>',
            '    <div class="agent-main-col">',
            '      <div class="agent-source-toggle">',
            '        <span class="agent-task-label">Source</span>',
            '        <button class="agent-source-btn agent-source-btn-active" id="agentSrcEditors">Crissy\'s Editors</button>',
            '        <button class="agent-source-btn" id="agentSrcFile">Load a File</button>',
            '      </div>',
            '      <div class="agent-toolbar" id="agentFileRow" style="display:none;">',
            '        <span class="agent-toolbar-label">File</span>',
            '        <input class="agent-input" id="agentFilePath" placeholder="relative/path/to/file.php" style="flex:1;min-width:120px;" autocomplete="off">',
            '        <button class="agent-btn agent-btn-ghost" id="agentLoadBtn">Load</button>',
            '      </div>',
            '      <div class="agent-history-bar" id="agentHistoryBar" style="display:none;">',
            '        <span class="agent-history-label">History</span>',
            '        <div class="agent-history-slots" id="agentHistorySlots"></div>',
            '        <button class="agent-btn agent-btn-ghost agent-btn-icon" id="agentBackBtn" title="Go back" disabled>&#8592;</button>',
            '        <button class="agent-btn agent-btn-ghost agent-btn-icon" id="agentFwdBtn"  title="Go forward" disabled>&#8594;</button>',
            '      </div>',
            '      <div class="agent-task-form">',
            '        <div class="agent-task-row">',
            '          <span class="agent-task-label">Mode</span>',
            '          <select class="agent-select" id="agentMode"><option value="repair" selected>Fix / Edit</option><option value="new_project">New Project</option></select>',
            '        </div>',
            '        <div class="agent-task-row">',
            '          <span class="agent-task-label">AI Provider</span>',
            '          <select class="agent-select" id="agentProvider"></select>',
            '          <select class="agent-select" id="agentModel"></select>',
            '          <span id="agentStreamBadge" class="agent-badge agent-badge-stream">stream</span>',
            '        </div>',
            '        <div class="agent-task-row">',
            '          <span class="agent-task-label">Task</span>',
            '          <select class="agent-select" id="agentTask">' + C.TASK_OPTIONS.map(function(o){ return '<option value="'+o.value+'">'+o.label+'</option>'; }).join('') + '</select>',
            '        </div>',
            '        <div class="agent-task-row" id="agentFuzzyRow" style="display:none;">',
            '          <span class="agent-task-label">Search</span>',
            '          <input class="agent-input" id="agentFuzzyInput" placeholder="fuzzy search themes, variables, components..." style="flex:1;">',
            '          <button class="agent-btn agent-btn-ghost" id="agentFuzzyBtn">Search</button>',
            '        </div>',
            '        <div class="agent-task-row" id="agentThemeRow" style="display:none;">',
            '          <span class="agent-task-label">Theme</span>',
            '          <select class="agent-select" id="agentThemeSelect"></select>',
            '          <button class="agent-btn agent-btn-ghost" id="agentOutlineBtn">CSS Outline</button>',
            '          <button class="agent-btn agent-btn-ghost" id="agentBackupBtn">Backup</button>',
            '        </div>',
            '        <div class="agent-task-row" style="flex-direction:column;align-items:flex-start;gap:4px;">',
            '          <span class="agent-task-label">Instruction</span>',
            '          <textarea class="agent-input" id="agentInstruction" rows="2" placeholder="Describe what you want the agent to do..."></textarea>',
            '        </div>',
            '        <div class="agent-task-row">',
            '          <button class="agent-btn agent-btn-primary" id="agentRunBtn" style="min-width:80px;">Run</button>',
            '          <button class="agent-btn agent-btn-ghost"   id="agentRunCmdBtn">Lint / Check</button>',
            '          <button class="agent-btn agent-btn-ghost agent-btn-danger" id="agentAbortBtn" disabled>Abort</button>',
            '        </div>',
            '      </div>',
            '      <div class="agent-response-area" id="agentResponseArea"></div>',
            '      <div class="agent-run-output" id="agentRunCmdOutput" style="display:none;"></div>',
            '    </div>',
            '  </div>',
            '</div>',
        ].join('');
    }

    function buildChatPane() {
        return [
            '<div class="agent-pane" data-pane="chat">',
            '  <div class="agent-toolbar">',
            '    <span class="agent-toolbar-label">Provider</span>',
            '    <select class="agent-select" id="chatProvider"></select>',
            '    <select class="agent-select" id="chatModel"></select>',
            '    <button class="agent-btn agent-btn-ghost" id="chatClearBtn">Clear</button>',
            '  </div>',
            '  <div class="agent-chat-messages" id="agentChatMessages"></div>',
            '  <div class="agent-chat-input-row">',
            '    <div class="agent-chat-input-wrap">',
            '      <textarea class="agent-input agent-chat-input" id="agentChatInput" rows="3" placeholder="Ask anything about CSS or code..." spellcheck="false"></textarea>',
            '      <button class="agent-btn agent-btn-primary agent-chat-send-btn" id="agentChatSend">Send</button>',
            '    </div>',
            '  </div>',
            '</div>',
        ].join('');
    }

    function buildDiffPane() {
        return [
            '<div class="agent-pane" data-pane="diff">',
            '  <div class="agent-toolbar">',
            '    <span class="agent-toolbar-label">Compare</span>',
            '    <select class="agent-select" id="diffFromSel"><option value="1">Version -1 (oldest)</option><option value="0" selected>Version -0 (2nd)</option></select>',
            '    <span style="color:var(--ag-text-muted);font-size:11px;">vs</span>',
            '    <select class="agent-select" id="diffToSel"><option value="0" selected>Current</option></select>',
            '    <button class="agent-btn agent-btn-ghost" id="agentDiffBtn">Diff</button>',
            '  </div>',
            '  <div class="agent-diff-summary" id="agentDiffSummary" style="display:none;"></div>',
            '  <div class="agent-diff-wrap">',
            '    <table class="agent-diff-table" id="agentDiffTable"><tbody></tbody></table>',
            '  </div>',
            '</div>',
        ].join('');
    }

    function buildVersionsPane() {
        return [
            '<div class="agent-pane" data-pane="versions">',
            '  <div class="agent-toolbar">',
            '    <span class="agent-toolbar-label">File versions</span>',
            '    <span style="color:var(--ag-text-muted);font-size:11px;" id="agentVersionFilePath"></span>',
            '  </div>',
            '  <ul class="agent-version-list" id="agentVersionList"></ul>',
            '</div>',
        ].join('');
    }

    function buildContextPane() {
        return [
            '<div class="agent-pane" data-pane="context">',
            '  <div class="agent-toolbar">',
            '    <span class="agent-toolbar-label">Context</span>',
            '    <button class="agent-btn agent-btn-ghost" id="agentRefreshContext">Refresh</button>',
            '  </div>',
            '  <div class="agent-context-file" id="agentContextBody">',
            '    <p style="color:var(--ag-text-dim);font-size:12px;padding:14px 0;">Load a file first.</p>',
            '  </div>',
            '</div>',
        ].join('');
    }

    function buildPromptsPane() {
        return [
            '<div class="agent-pane" data-pane="prompts">',
            '  <div class="agent-toolbar">',
            '    <span class="agent-toolbar-label">Prompts</span>',
            '    <button class="agent-btn agent-btn-primary" id="agentSavePrompts">Save to Storage</button>',
            '  </div>',
            '  <div class="agent-prompts-editor" id="agentPromptsEditor">',
            '    <p style="color:var(--ag-text-muted);font-size:12px;">Loading...</p>',
            '  </div>',
            '</div>',
        ].join('');
    }

    // -----------------------------------------------------------------------
    // Cache DOM references into dom object
    // -----------------------------------------------------------------------

    C.cacheDOM = function () {
        var m = dom.modal;
        dom.closeBtn       = m.querySelector('#agentCloseBtn');
        dom.minBtn         = m.querySelector('#agentMinBtn');
        dom.header         = m.querySelector('#agentHeader');
        dom.tabs           = m.querySelectorAll('.agent-tab');
        dom.panes          = m.querySelectorAll('.agent-pane');
        dom.themeDots      = m.querySelector('#agentThemeDots');
        dom.statusBar      = m.querySelector('#agentStatusBar');
        dom.statusDot      = m.querySelector('#agentStatusDot');
        dom.statusText     = m.querySelector('#agentStatusText');
        dom.filePath       = m.querySelector('#agentFilePath');
        dom.loadBtn        = m.querySelector('#agentLoadBtn');
        dom.fileRow        = m.querySelector('#agentFileRow');
        dom.srcEditorsBtn  = m.querySelector('#agentSrcEditors');
        dom.srcFileBtn     = m.querySelector('#agentSrcFile');
        dom.historyBar     = m.querySelector('#agentHistoryBar');
        dom.historySlots   = m.querySelector('#agentHistorySlots');
        dom.backBtn        = m.querySelector('#agentBackBtn');
        dom.fwdBtn         = m.querySelector('#agentFwdBtn');
        dom.provider       = m.querySelector('#agentProvider');
        dom.model          = m.querySelector('#agentModel');
        dom.streamBadge    = m.querySelector('#agentStreamBadge');
        dom.task           = m.querySelector('#agentTask');
        dom.instruction    = m.querySelector('#agentInstruction');
        dom.runBtn         = m.querySelector('#agentRunBtn');
        dom.runCmdBtn      = m.querySelector('#agentRunCmdBtn');
        dom.abortBtn       = m.querySelector('#agentAbortBtn');
        dom.responseArea   = m.querySelector('#agentResponseArea');
        dom.runCmdOutput   = m.querySelector('#agentRunCmdOutput');
        dom.outline        = m.querySelector('#agentOutline');
        dom.modeSelect     = m.querySelector('#agentMode');
        dom.fuzzyRow       = m.querySelector('#agentFuzzyRow');
        dom.fuzzyInput     = m.querySelector('#agentFuzzyInput');
        dom.fuzzyBtn       = m.querySelector('#agentFuzzyBtn');
        dom.themeRow       = m.querySelector('#agentThemeRow');
        dom.themeSelect    = m.querySelector('#agentThemeSelect');
        dom.outlineBtn     = m.querySelector('#agentOutlineBtn');
        dom.backupBtn      = m.querySelector('#agentBackupBtn');
        dom.diffBtn        = m.querySelector('#agentDiffBtn');
        dom.diffSummary    = m.querySelector('#agentDiffSummary');
        dom.diffTable      = m.querySelector('#agentDiffTable tbody');
        dom.versionList    = m.querySelector('#agentVersionList');
        dom.versionPath    = m.querySelector('#agentVersionFilePath');
        dom.contextBody    = m.querySelector('#agentContextBody');
        dom.refreshContext = m.querySelector('#agentRefreshContext');
        dom.promptsEditor  = m.querySelector('#agentPromptsEditor');
        dom.savePrompts    = m.querySelector('#agentSavePrompts');
        dom.chatMessages   = m.querySelector('#agentChatMessages');
        dom.chatInput      = m.querySelector('#agentChatInput');
        dom.chatSend       = m.querySelector('#agentChatSend');
        dom.chatClear      = m.querySelector('#chatClearBtn');
        dom.chatProvider   = m.querySelector('#chatProvider');
        dom.chatModel      = m.querySelector('#chatModel');
    };

}(window.LiveCSS = window.LiveCSS || {}));
