/**
 * Code Agent UI Module -- LiveCSS.agent
 * Reusable drop-in: attach to any page by calling LiveCSS.agent.init({ ... }).
 * Talks to ai/agent/agent.php and ai/agent/run.php.
 * Handles: version history, diff view, outline viewer, context panel,
 *          streaming AI runs, prompts editor, and theme switching.
 */
'use strict';

(function (LiveCSS) {

    // -----------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------

    var AGENT_URL = 'ai/agent/agent.php';
    var RUN_URL   = 'ai/agent/run.php';

    var THEMES = ['dark-neu', 'morphism', 'glassmorphic', 'keyboard-ui', 'ada'];

    var THEME_LABELS = {
        'dark-neu':       'Dark Neu',
        'morphism':       'Morphism',
        'glassmorphic':   'Glassmorphic',
        'keyboard-ui':    'Keyboard',
        'ada':            'Ada (auto)'
    };

    var TASK_OPTIONS = [
        { value: 'fix',        label: 'Fix Bug'       },
        { value: 'refactor',   label: 'Refactor'      },
        { value: 'modernize',  label: 'Modernize'     },
        { value: 'add_feature',label: 'Add Feature'   },
        { value: 'explain',    label: 'Explain'       },
        { value: 'review',     label: 'Review'        },
        { value: 'test',       label: 'Write Tests'   },
        { value: 'document',   label: 'Document'      },
        { value: 'optimize',   label: 'Optimize'      },
        { value: 'security',   label: 'Security Audit'}
    ];

    var NEW_PROJECT_TASKS = [
        { value: 'preview',    label: 'Generate Preview' },
        { value: 'add_feature',label: 'Add Component'    },
        { value: 'refactor',   label: 'Refactor Theme'   },
        { value: 'review',     label: 'Review Theme'     }
    ];

    var AGENT_MODES = {
        repair:      { name: 'Code Repair / Edit', tasks: TASK_OPTIONS },
        new_project: { name: 'New Project',         tasks: NEW_PROJECT_TASKS }
    };

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    var state = {
        theme:      'ada',
        mode:       'repair',
        filePath:   '',
        content:    '',
        provider:   'anthropic',
        model:      '',
        task:       'fix',
        providers:  {},
        versions:   [],
        history:    {},
        conversation: [],
        activeStream: null,
        busy:       false,
        activeTab:  'run'
    };

    // -----------------------------------------------------------------------
    // Markdown (inline mini parser, same as ai-chat.js)
    // -----------------------------------------------------------------------

    var MD = (function () {
        function esc(t) {
            return t.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
        }
        function inline(t) {
            t = esc(t);
            t = t.replace(/`([^`]+)`/g,        '<code>$1</code>');
            t = t.replace(/\*\*(.+?)\*\*/g,     '<strong>$1</strong>');
            t = t.replace(/__(.+?)__/g,          '<strong>$1</strong>');
            t = t.replace(/\*(.+?)\*/g,          '<em>$1</em>');
            t = t.replace(/_(.+?)_/g,            '<em>$1</em>');
            t = t.replace(/~~(.+?)~~/g,          '<del>$1</del>');
            t = t.replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2" target="_blank" rel="noopener">$1</a>');
            return t;
        }
        function toHtml(md) {
            var lines  = md.split('\n');
            var out    = [];
            var i      = 0;
            var n      = lines.length;
            while (i < n) {
                var l = lines[i];
                var fm = l.match(/^```(\w*)/);
                if (fm) {
                    var lang = fm[1] ? esc(fm[1]) : '';
                    var code = [];
                    i++;
                    while (i < n && !lines[i].match(/^```/)) { code.push(esc(lines[i])); i++; }
                    out.push('<pre><code' + (lang ? ' class="language-' + lang + '"' : '') + '>' + code.join('\n') + '</code></pre>');
                    i++; continue;
                }
                var hm = l.match(/^(#{1,6})\s+(.+)/);
                if (hm) {
                    out.push('<h' + hm[1].length + '>' + inline(hm[2]) + '</h' + hm[1].length + '>');
                    i++; continue;
                }
                if (/^[-*_]{3,}\s*$/.test(l)) { out.push('<hr>'); i++; continue; }
                var bm = l.match(/^>\s?(.*)/);
                if (bm) {
                    var bl = [bm[1]]; i++;
                    var nb;
                    while (i < n && (nb = lines[i].match(/^>\s?(.*)/))) { bl.push(nb[1]); i++; }
                    out.push('<blockquote>' + toHtml(bl.join('\n')) + '</blockquote>'); continue;
                }
                var um = l.match(/^[-*+]\s+(.+)/);
                if (um) {
                    var ui = [inline(um[1])]; i++;
                    var nu;
                    while (i < n && (nu = lines[i].match(/^[-*+]\s+(.+)/))) { ui.push(inline(nu[1])); i++; }
                    out.push('<ul>' + ui.map(function(t){return '<li>' + t + '</li>';}).join('') + '</ul>'); continue;
                }
                var om = l.match(/^\d+\.\s+(.+)/);
                if (om) {
                    var oi = [inline(om[1])]; i++;
                    var no;
                    while (i < n && (no = lines[i].match(/^\d+\.\s+(.+)/))) { oi.push(inline(no[1])); i++; }
                    out.push('<ol>' + oi.map(function(t){return '<li>' + t + '</li>';}).join('') + '</ol>'); continue;
                }
                if (l.trim() === '') { i++; continue; }
                var pl = [inline(l)]; i++;
                while (i < n) {
                    var nx = lines[i];
                    if (nx.trim() === '' || /^(#{1,6}\s|[-*+]\s|\d+\.\s|>|```|[-*_]{3})/.test(nx)) { break; }
                    pl.push(inline(nx)); i++;
                }
                out.push('<p>' + pl.join(' ') + '</p>');
            }
            return out.join('\n');
        }

        /** Extract first fenced code block content from a string. */
        function extractCode(text) {
            var m = text.match(/```[\w]*\n([\s\S]*?)```/);
            return m ? m[1] : text;
        }

        return { toHtml: toHtml, extractCode: extractCode };
    }());

    // -----------------------------------------------------------------------
    // DOM cache
    // -----------------------------------------------------------------------

    var dom = {};

    // -----------------------------------------------------------------------
    // Build UI
    // -----------------------------------------------------------------------

    function buildUI() {
        // Overlay
        var overlay = el('div', 'agent-overlay');
        overlay.id  = 'agentOverlay';
        document.body.appendChild(overlay);

        // Modal
        var modal = el('div', 'agent-modal theme-' + state.theme);
        modal.id  = 'agentModal';
        modal.innerHTML = buildModalHTML();
        document.body.appendChild(modal);

        // Trigger
        var trigger = el('button', 'agent-trigger');
        trigger.id          = 'agentTrigger';
        trigger.textContent = 'AGT';
        trigger.title       = 'Open Code Agent';
        document.body.appendChild(trigger);

        // Toast container
        dom.overlay  = overlay;
        dom.modal    = modal;
        dom.trigger  = trigger;

        cacheDOM();
        bindEvents();
    }

    function buildModalHTML() {
        return [
            '<div class="agent-header">',
            '  <span class="agent-title">Code Agent</span>',
            '  <div class="agent-header-controls">',
            '    <div class="agent-theme-dots" id="agentThemeDots">' + buildThemeDots() + '</div>',
            '    <button class="agent-btn agent-btn-ghost agent-btn-icon" id="agentCloseBtn" title="Close">&#215;</button>',
            '  </div>',
            '</div>',
            '<div class="agent-tabs" id="agentTabs">',
            '  <button class="agent-tab agent-tab-active" data-tab="run">Run</button>',
            '  <button class="agent-tab" data-tab="diff">Diff</button>',
            '  <button class="agent-tab" data-tab="versions">Versions</button>',
            '  <button class="agent-tab" data-tab="context">Context</button>',
            '  <button class="agent-tab" data-tab="prompts">Prompts</button>',
            '</div>',
            '<div class="agent-body">',
            buildRunPane(),
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
        return THEMES.map(function (t) {
            var active = t === state.theme ? ' dot-active' : '';
            return '<span class="agent-theme-dot dot-' + t + active + '" data-theme="' + t + '" title="' + THEME_LABELS[t] + '"></span>';
        }).join('');
    }

    function buildRunPane() {
        return [
            '<div class="agent-pane agent-pane-active" data-pane="run">',
            '  <div class="agent-split">',
            '    <div class="agent-outline-col" id="agentOutlineCol">',
            '      <div class="outline-header">Outline</div>',
            '      <div id="agentOutline"><p class="outline-empty">No file loaded</p></div>',
            '    </div>',
            '    <div class="agent-main-col">',
            '      <div class="agent-toolbar">',
            '        <span class="agent-toolbar-label">File</span>',
            '        <input class="agent-input" id="agentFilePath" placeholder="relative/path/to/file.php" style="flex:1;min-width:120px;" autocomplete="off">',
            '        <button class="agent-btn agent-btn-ghost" id="agentLoadBtn">Load</button>',
            '      </div>',
            '      <div class="agent-history-bar" id="agentHistoryBar">',
            '        <span class="agent-history-label">History</span>',
            '        <div class="agent-history-slots" id="agentHistorySlots"></div>',
            '        <button class="agent-btn agent-btn-ghost agent-btn-icon" id="agentBackBtn" title="Go back" disabled>&#8592;</button>',
            '        <button class="agent-btn agent-btn-ghost agent-btn-icon" id="agentFwdBtn"  title="Go forward" disabled>&#8594;</button>',
            '      </div>',
            '      <div class="agent-task-form">',
            '        <div class="agent-task-row">',
            '          <span class="agent-task-label">Mode</span>',
            '          <select class="agent-select" id="agentMode"><option value="repair" selected>Code Repair / Edit</option><option value="new_project">New Project</option></select>',
            '        </div>',
            '        <div class="agent-task-row">',
            '          <span class="agent-task-label">Provider</span>',
            '          <select class="agent-select" id="agentProvider"></select>',
            '          <select class="agent-select" id="agentModel"></select>',
            '          <span id="agentStreamBadge" class="agent-badge agent-badge-stream">stream</span>',
            '        </div>',
            '        <div class="agent-task-row">',
            '          <span class="agent-task-label">Task</span>',
            '          <select class="agent-select" id="agentTask">' + TASK_OPTIONS.map(function(o){return '<option value="'+o.value+'">'+o.label+'</option>';}).join('') + '</select>',
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
            '          <textarea class="agent-input" id="agentInstruction" rows="2" placeholder="Optional extra instruction..."></textarea>',
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

    function cacheDOM() {
        var m = dom.modal;
        dom.closeBtn       = m.querySelector('#agentCloseBtn');
        dom.tabs           = m.querySelectorAll('.agent-tab');
        dom.panes          = m.querySelectorAll('.agent-pane');
        dom.themeDots      = m.querySelector('#agentThemeDots');
        dom.statusBar      = m.querySelector('#agentStatusBar');
        dom.statusDot      = m.querySelector('#agentStatusDot');
        dom.statusText     = m.querySelector('#agentStatusText');
        dom.filePath       = m.querySelector('#agentFilePath');
        dom.loadBtn        = m.querySelector('#agentLoadBtn');
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
    }

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    function bindEvents() {
        dom.trigger.addEventListener('click', toggleOpen);
        dom.overlay.addEventListener('click', close);
        dom.closeBtn.addEventListener('click', close);

        dom.tabs.forEach(function (tab) {
            tab.addEventListener('click', function () { switchTab(tab.dataset.tab); });
        });

        dom.themeDots.addEventListener('click', function (e) {
            var dot = e.target.closest('.agent-theme-dot');
            if (dot) { applyTheme(dot.dataset.theme); }
        });

        dom.loadBtn.addEventListener('click', loadFile);
        dom.filePath.addEventListener('keydown', function (e) { if (e.key === 'Enter') { loadFile(); } });

        dom.backBtn.addEventListener('click', function () { navigate('back'); });
        dom.fwdBtn.addEventListener('click',  function () { navigate('forward'); });

        dom.provider.addEventListener('change', function () {
            state.provider = dom.provider.value;
            populateModels();
            updateStreamBadge();
        });
        dom.model.addEventListener('change', function () { state.model = dom.model.value; });

        dom.runBtn.addEventListener('click', runAgent);
        dom.abortBtn.addEventListener('click', abortStream);
        dom.runCmdBtn.addEventListener('click', runCommand);

        dom.modeSelect.addEventListener('change', function () { switchMode(dom.modeSelect.value); });
        dom.fuzzyBtn.addEventListener('click', runFuzzySearch);
        dom.fuzzyInput.addEventListener('keydown', function (e) { if (e.key === 'Enter') { runFuzzySearch(); } });
        dom.outlineBtn.addEventListener('click', runCSSOutline);
        dom.backupBtn.addEventListener('click', runBackup);

        dom.diffBtn.addEventListener('click', showDiff);
        dom.refreshContext.addEventListener('click', loadContext);
        dom.savePrompts.addEventListener('click', savePrompts);
    }

    // -----------------------------------------------------------------------
    // Open / close
    // -----------------------------------------------------------------------

    function toggleOpen() {
        if (dom.modal.classList.contains('agent-open')) { close(); }
        else { open(); }
    }

    function open() {
        dom.overlay.classList.add('agent-open');
        dom.modal.classList.add('agent-open');
        dom.trigger.classList.add('agent-trigger-active');
        if (!state.providers || !Object.keys(state.providers).length) {
            loadProviders();
        }
        if (state.activeTab === 'prompts') { loadPromptsEditor(); }
    }

    function close() {
        dom.overlay.classList.remove('agent-open');
        dom.modal.classList.remove('agent-open');
        dom.trigger.classList.remove('agent-trigger-active');
    }

    // -----------------------------------------------------------------------
    // Tabs
    // -----------------------------------------------------------------------

    function switchTab(name) {
        state.activeTab = name;
        dom.tabs.forEach(function (t) {
            t.classList.toggle('agent-tab-active', t.dataset.tab === name);
        });
        dom.panes.forEach(function (p) {
            p.classList.toggle('agent-pane-active', p.dataset.pane === name);
        });
        if (name === 'prompts') { loadPromptsEditor(); }
        if (name === 'context' && state.filePath) { loadContext(); }
        if (name === 'versions' && state.filePath) { renderVersionList(); }
        if (name === 'diff' && state.filePath)     { refreshDiffSelectors(); }
    }

    // -----------------------------------------------------------------------
    // Theme
    // -----------------------------------------------------------------------

    function applyTheme(name) {
        if (!THEMES.includes(name)) { name = 'ada'; }
        THEMES.forEach(function (t) {
            dom.modal.classList.remove('theme-' + t);
        });
        dom.modal.classList.add('theme-' + name);
        state.theme = name;
        dom.themeDots.querySelectorAll('.agent-theme-dot').forEach(function (d) {
            d.classList.toggle('dot-active', d.dataset.theme === name);
        });
        try { localStorage.setItem('agent-theme', name); } catch(e) {}
    }

    // -----------------------------------------------------------------------
    // Providers
    // -----------------------------------------------------------------------

    function loadProviders() {
        agentPost({ action: 'providers' }).then(function (data) {
            if (data.error) { setStatus('error', data.error); return; }
            state.providers = data.providers || {};
            populateProviders();
        }).catch(function (err) { setStatus('error', 'Cannot load providers: ' + err.message); });
    }

    function populateProviders() {
        dom.provider.innerHTML = '';
        Object.keys(state.providers).forEach(function (slug) {
            var o = document.createElement('option');
            o.value       = slug;
            o.textContent = state.providers[slug].name;
            if (slug === state.provider) { o.selected = true; }
            dom.provider.appendChild(o);
        });
        state.provider = dom.provider.value;
        populateModels();
        updateStreamBadge();
    }

    function populateModels() {
        var p  = state.providers[state.provider] || {};
        var ms = p.models || (p.default_model ? [p.default_model] : []);
        var def = p.default_model || (ms[0] || '');
        dom.model.innerHTML = '';
        ms.forEach(function (m) {
            var o = document.createElement('option');
            o.value = m; o.textContent = m;
            if (m === def) { o.selected = true; }
            dom.model.appendChild(o);
        });
        state.model = def;
    }

    function updateStreamBadge() {
        var p  = state.providers[state.provider] || {};
        var ok = p.supports_streaming !== false;
        dom.streamBadge.textContent = ok ? 'stream' : 'no stream';
        dom.streamBadge.className   = 'agent-badge ' + (ok ? 'agent-badge-stream' : 'agent-badge-nostream');
    }

    // -----------------------------------------------------------------------
    // Load file / versions
    // -----------------------------------------------------------------------

    function loadFile() {
        var path = dom.filePath.value.trim();
        if (!path) { toast('Enter a file path.', 'error'); return; }
        state.filePath = path;
        setStatus('busy', 'Loading...');

        agentPost({ action: 'get_versions', file_path: path }).then(function (data) {
            if (data.error) { setStatus('error', data.error); return; }
            state.versions = data.versions || [];
            state.history  = data.history  || {};
            updateHistorySlots();
            setActiveCurrent();
            setStatus('ok', 'Loaded ' + state.versions.length + ' version(s)');
            if (state.versions.length > 0) {
                state.content = state.versions[0].content;
                loadOutline();
            }
        }).catch(function (e) { setStatus('error', e.message); });
    }

    function setActiveCurrent() {
        var cur = state.history.current_version;
        if (cur === undefined) { return; }
        var count    = state.versions.length;
        var offset   = (count - 1) - cur;
        var version  = state.versions[offset];
        if (version) {
            state.content = version.content;
        }
        renderHistorySlots();
        dom.backBtn.disabled = (cur <= 0 || (state.history.consecutive_back >= 1));
        dom.fwdBtn.disabled  = (cur >= count - 1);
    }

    function renderHistorySlots() {
        var count   = state.versions.length;
        var cur     = parseInt(state.history.current_version) || 0;
        var cBack   = parseInt(state.history.consecutive_back) || 0;
        dom.historySlots.innerHTML = '';

        for (var i = 0; i < 3; i++) {
            var slot = el('span', 'agent-history-slot');
            if (i < count)   { slot.classList.add('slot-occupied'); }
            if (i === cur)   { slot.classList.add('slot-current'); }
            if (i > 0 && cBack >= 1 && i < cur) { slot.classList.add('slot-blocked'); }
            slot.title = i < count ? 'Version ' + (i + 1) : 'Empty slot';
            dom.historySlots.appendChild(slot);
        }
    }

    function updateHistorySlots() {
        renderHistorySlots();
    }

    // -----------------------------------------------------------------------
    // Navigate (back / forward)
    // -----------------------------------------------------------------------

    function navigate(direction) {
        if (!state.filePath) { return; }
        setStatus('busy', 'Navigating ' + direction + '...');
        agentPost({ action: 'navigate', file_path: state.filePath, direction: direction })
            .then(function (data) {
                if (data.blocked) {
                    setStatus('error', data.reason);
                    toast(data.reason, 'error');
                    return;
                }
                if (data.error) { setStatus('error', data.error); return; }
                state.history = data.history || {};
                if (data.version) {
                    state.content = data.version.content;
                    loadOutline();
                }
                // Reload version list
                agentPost({ action: 'get_versions', file_path: state.filePath }).then(function (vd) {
                    state.versions = vd.versions || [];
                    setActiveCurrent();
                    setStatus('ok', 'At version ' + (parseInt(state.history.current_version || 0) + 1));
                });
            }).catch(function (e) { setStatus('error', e.message); });
    }

    // -----------------------------------------------------------------------
    // Outline
    // -----------------------------------------------------------------------

    function loadOutline() {
        if (!state.filePath || !state.content) { return; }
        agentPost({ action: 'outline', file_path: state.filePath, content: state.content })
            .then(function (data) {
                if (data.html) { dom.outline.innerHTML = data.html; }
                // Wire click to jump to line in CodeMirror if available
                dom.outline.querySelectorAll('.outline-node').forEach(function (node) {
                    node.addEventListener('click', function () {
                        var line = parseInt(node.dataset.line, 10);
                        if (line && LiveCSS.editor && LiveCSS.editor.goToLine) {
                            LiveCSS.editor.goToLine(line);
                        }
                    });
                });
            }).catch(function () {});
    }

    // -----------------------------------------------------------------------
    // Run agent (streaming or blocking)
    // -----------------------------------------------------------------------

    function runAgent() {
        if (state.busy) { return; }

        // In new_project mode with preview task, use theme file instead of requiring loaded file
        var taskVal = dom.task.value;
        if (state.mode === 'new_project' && taskVal === 'preview') {
            var themeFile = dom.themeSelect ? dom.themeSelect.value : '';
            if (!themeFile) { toast('Select a theme first.', 'error'); return; }
            runPreviewGeneration(themeFile);
            return;
        }

        if (!state.filePath) { toast('Load a file first.', 'error'); return; }

        var payload = {
            provider:    state.provider,
            model:       state.model,
            task:        taskVal,
            file_path:   state.filePath,
            content:     state.content,
            instruction: dom.instruction.value.trim(),
            messages:    state.conversation,
            mode:        state.mode,
        };

        var isStream = (state.providers[state.provider] || {}).supports_streaming !== false;

        appendUserMsg(payload.task + (payload.instruction ? ': ' + payload.instruction : ''));

        setBusy(true);

        if (isStream) {
            streamRun(payload);
        } else {
            blockingRun(payload);
        }
    }

    /**
     * Generate a preview for a CSS theme using Haiku 4.5.
     * Fetches the CSS outline and fuzzy search context, then sends to the model.
     */
    function runPreviewGeneration(themeFile) {
        setBusy(true);
        setStatus('busy', 'Generating preview for ' + themeFile + '...');

        // Step 1: Backup first
        agentPost({ action: 'backup', target: 'all' }).then(function () {
            // Step 2: Get CSS outline
            return agentPost({ action: 'css_outline', file_path: themeFile, format: 'text' });
        }).then(function (outlineData) {
            var outlineText = outlineData.outline || '';

            // Step 3: Fuzzy search for related info
            var themeName = themeFile.replace('.css', '');
            return agentPost({ action: 'fuzzy_search', query: 'component preview ' + themeName, format: 'text' }).then(function (searchData) {
                return { outline: outlineText, search: searchData.results || '' };
            });
        }).then(function (context) {
            // Step 4: Send to model with outline + search as instruction context
            var instruction = dom.instruction.value.trim();
            var fullInstruction = 'Generate a preview HTML page for this CSS theme.\n\n'
                + 'CSS Outline:\n' + context.outline + '\n\n'
                + 'Related context:\n' + context.search + '\n\n'
                + (instruction ? 'Additional instructions: ' + instruction + '\n' : '');

            var payload = {
                provider:    'anthropic',
                model:       'claude-haiku-4-5-20251001',
                task:        'preview',
                file_path:   themeFile,
                content:     '',
                instruction: fullInstruction,
                messages:    state.conversation,
                mode:        'new_project',
            };

            appendUserMsg('preview: Generate preview for ' + themeFile);

            var isStream = (state.providers['anthropic'] || {}).supports_streaming !== false;
            if (isStream) {
                streamRun(payload);
            } else {
                blockingRun(payload);
            }
        }).catch(function (e) {
            setStatus('error', e.message);
            setBusy(false);
        });
    }

    function streamRun(payload) {
        var bodyEl  = appendAssistantPlaceholder();
        var accum   = '';
        var reasoning = '';
        var ctrl    = new AbortController();

        state.activeStream = { close: function () { ctrl.abort(); } };

        fetch(RUN_URL, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(payload),
            signal:  ctrl.signal
        })
        .then(function (resp) {
            if (!resp.ok) { throw new Error('HTTP ' + resp.status); }
            var reader  = resp.body.getReader();
            var decoder = new TextDecoder();
            var buf     = '';

            function pump() {
                return reader.read().then(function (r) {
                    if (r.done) {
                        finalizeMsg(bodyEl, accum);
                        if (accum) {
                            state.conversation.push({ role: 'assistant', content: accum });
                            showApplyBar(bodyEl, accum);
                        }
                        setBusy(false); state.activeStream = null; return;
                    }
                    buf += decoder.decode(r.value, { stream: true });
                    var parts = buf.split('\n\n'); buf = parts.pop();
                    parts.forEach(function (block) {
                        var evName = ''; var dataStr = '';
                        block.split('\n').forEach(function (line) {
                            if (line.startsWith('event: ')) { evName  = line.slice(7).trim(); }
                            if (line.startsWith('data: '))  { dataStr = line.slice(6).trim(); }
                        });
                        if (!dataStr) { return; }
                        var data;
                        try { data = JSON.parse(dataStr); } catch(e) { return; }
                        if (evName === 'chunk') {
                            accum += data.text || '';
                            updateMsg(bodyEl, accum);
                        } else if (evName === 'reasoning') {
                            reasoning += data.text || '';
                            updateReasoning(bodyEl, data.text || '');
                        } else if (evName === 'done') {
                            finalizeMsg(bodyEl, accum);
                            if (accum) {
                                state.conversation.push({ role: 'assistant', content: accum });
                                showApplyBar(bodyEl, accum);
                            }
                            setBusy(false); state.activeStream = null;
                        } else if (evName === 'error') {
                            finalizeMsg(bodyEl, accum || '_Error_');
                            setStatus('error', data.error || 'Error');
                            setBusy(false); state.activeStream = null;
                        }
                    });
                    return pump();
                });
            }
            return pump();
        })
        .catch(function (err) {
            if (err.name === 'AbortError') { setBusy(false); return; }
            setStatus('error', err.message);
            finalizeMsg(bodyEl, accum || '_Request failed_');
            setBusy(false); state.activeStream = null;
        });
    }

    function blockingRun(payload) {
        var bodyEl = appendAssistantPlaceholder();
        fetch(RUN_URL, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(payload),
        })
        .then(function (r) { return r.json(); })
        .then(function (data) {
            if (data.error) {
                finalizeMsg(bodyEl, '_Error: ' + data.error + '_');
                setStatus('error', data.error);
            } else {
                finalizeMsg(bodyEl, data.text || '');
                if (data.text) {
                    state.conversation.push({ role: 'assistant', content: data.text });
                    showApplyBar(bodyEl, data.text);
                }
                setStatus('ok', 'Done');
            }
        })
        .catch(function (err) {
            finalizeMsg(bodyEl, '_Request failed_');
            setStatus('error', err.message);
        })
        .finally(function () { setBusy(false); });
    }

    function abortStream() {
        if (state.activeStream) {
            state.activeStream.close();
            state.activeStream = null;
        }
        setBusy(false);
        setStatus('ok', 'Aborted');
    }

    // -----------------------------------------------------------------------
    // Apply AI result
    // -----------------------------------------------------------------------

    function applyAIResult(rawText) {
        var code = MD.extractCode(rawText);
        if (!code.trim()) { toast('No code block found in response.', 'error'); return; }

        var summary  = 'AI: ' + dom.task.value + (dom.instruction.value ? ' -- ' + dom.instruction.value.slice(0, 60) : '');
        var payload  = { action: 'apply', file_path: state.filePath, content: code, summary: summary, model: state.provider + ':' + state.model };

        agentPost(payload).then(function (data) {
            if (data.error) { toast(data.error, 'error'); return; }
            state.versions = data.versions || [];
            state.history  = data.history  || {};
            state.content  = code;
            setActiveCurrent();
            loadOutline();
            toast('Applied and saved as version ' + data.version_id, 'success');
            setStatus('ok', 'Version ' + data.version_id + ' saved');
        }).catch(function (e) { toast(e.message, 'error'); });
    }

    // -----------------------------------------------------------------------
    // Run command (lint/check)
    // -----------------------------------------------------------------------

    function runCommand() {
        if (!state.filePath) { toast('Load a file first.', 'error'); return; }
        dom.runCmdOutput.style.display = 'block';
        dom.runCmdOutput.textContent   = 'Running...';
        agentPost({ action: 'run_command', file_path: state.filePath })
            .then(function (data) {
                dom.runCmdOutput.textContent = data.output || '(no output)';
                setStatus(data.exit_code === 0 ? 'ok' : 'error', 'Exit ' + data.exit_code);
            }).catch(function (e) { dom.runCmdOutput.textContent = 'Error: ' + e.message; });
    }

    // -----------------------------------------------------------------------
    // Agent Mode Switching
    // -----------------------------------------------------------------------

    function switchMode(mode) {
        state.mode = mode;
        var modeInfo = AGENT_MODES[mode];
        if (!modeInfo) { return; }

        // Update task dropdown
        var tasks = modeInfo.tasks;
        dom.task.innerHTML = '';
        tasks.forEach(function (o) {
            var opt = document.createElement('option');
            opt.value = o.value;
            opt.textContent = o.label;
            dom.task.appendChild(opt);
        });
        state.task = tasks[0].value;

        // Show/hide new_project-specific rows
        var isNewProject = mode === 'new_project';
        dom.fuzzyRow.style.display = isNewProject ? 'flex' : 'none';
        dom.themeRow.style.display = isNewProject ? 'flex' : 'none';

        // Load theme list for new_project mode
        if (isNewProject && dom.themeSelect.children.length === 0) {
            loadThemeList();
        }

        setStatus('ok', 'Mode: ' + modeInfo.name);
    }

    function loadThemeList() {
        agentPost({ action: 'list_themes' }).then(function (data) {
            if (data.error) { return; }
            dom.themeSelect.innerHTML = '';
            (data.themes || []).forEach(function (t) {
                var opt = document.createElement('option');
                opt.value = t.file;
                opt.textContent = t.name + ' (' + t.prefix + ') -- ' + t.stats.variables + ' vars, ' + t.stats.classes + ' classes';
                dom.themeSelect.appendChild(opt);
            });
        }).catch(function () {});
    }

    // -----------------------------------------------------------------------
    // Fuzzy Search
    // -----------------------------------------------------------------------

    function runFuzzySearch() {
        var query = dom.fuzzyInput.value.trim();
        if (!query) { toast('Enter a search query.', 'error'); return; }
        setStatus('busy', 'Searching...');
        agentPost({ action: 'fuzzy_search', query: query, format: 'text' })
            .then(function (data) {
                if (data.error) { setStatus('error', data.error); return; }
                dom.runCmdOutput.style.display = 'block';
                dom.runCmdOutput.textContent = data.results || '(no results)';
                setStatus('ok', 'Search complete');
            }).catch(function (e) { setStatus('error', e.message); });
    }

    // -----------------------------------------------------------------------
    // CSS Outline
    // -----------------------------------------------------------------------

    function runCSSOutline() {
        var themeFile = dom.themeSelect.value;
        if (!themeFile) { toast('Select a theme first.', 'error'); return; }
        setStatus('busy', 'Extracting outline...');
        agentPost({ action: 'css_outline', file_path: themeFile, format: 'text' })
            .then(function (data) {
                if (data.error) { setStatus('error', data.error); return; }
                dom.runCmdOutput.style.display = 'block';
                dom.runCmdOutput.textContent = data.outline || '(no outline)';
                setStatus('ok', 'Outline extracted');
            }).catch(function (e) { setStatus('error', e.message); });
    }

    // -----------------------------------------------------------------------
    // Backup
    // -----------------------------------------------------------------------

    function runBackup() {
        setStatus('busy', 'Creating backups...');
        agentPost({ action: 'backup', target: 'all' })
            .then(function (data) {
                if (data.error) { setStatus('error', data.error); return; }
                var backed = (data.backups || []).map(function (b) { return b.file + ' -> ' + b.backup; });
                toast('Backed up: ' + backed.join(', '), 'success');
                setStatus('ok', 'Backup complete (' + data.timestamp + ')');
            }).catch(function (e) { setStatus('error', e.message); });
    }

    // -----------------------------------------------------------------------
    // Diff
    // -----------------------------------------------------------------------

    function refreshDiffSelectors() {}

    function showDiff() {
        if (!state.filePath) { toast('Load a file first.', 'error'); return; }
        setStatus('busy', 'Computing diff...');
        agentPost({ action: 'diff', file_path: state.filePath })
            .then(function (data) {
                if (data.error) { setStatus('error', data.error); return; }
                dom.diffTable.innerHTML = data.html || '';
                var sum = data.summary || {};
                dom.diffSummary.style.display = 'flex';
                dom.diffSummary.innerHTML = [
                    '<span class="diff-added">+' + (sum.added   || 0) + ' added</span>',
                    '<span class="diff-removed">-' + (sum.removed || 0) + ' removed</span>',
                ].join('');
                if (data.message) { dom.diffSummary.innerHTML += '<span style="color:var(--ag-text-muted)">' + data.message + '</span>'; }
                setStatus('ok', 'Diff ready');
                switchTab('diff');
            }).catch(function (e) { setStatus('error', e.message); });
    }

    function renderVersionList() {
        agentPost({ action: 'get_versions', file_path: state.filePath }).then(function (data) {
            state.versions = data.versions || [];
            state.history  = data.history  || {};
            dom.versionPath.textContent = state.filePath;
            dom.versionList.innerHTML   = '';
            if (!state.versions.length) {
                dom.versionList.innerHTML = '<li style="padding:14px;color:var(--ag-text-muted);font-size:12px;">No versions stored.</li>';
                return;
            }
            state.versions.forEach(function (v, idx) {
                var cur     = state.history.current_version;
                var count   = state.versions.length;
                var posIdx  = (count - 1) - idx;
                var isCur   = posIdx === parseInt(cur || 0);
                var li      = el('li', 'agent-version-item' + (isCur ? ' version-current' : ''));
                var date    = new Date(parseInt(v.created_at) * 1000);
                li.innerHTML = [
                    '<div class="agent-version-meta">',
                    '  <div class="agent-version-label">' + (v.label || 'Version ' + (idx + 1)) + '</div>',
                    '  <div class="agent-version-time">' + date.toLocaleString() + '</div>',
                    '</div>',
                    '<button class="agent-btn agent-btn-ghost" style="font-size:10px;" data-vid="' + v.id + '">View Diff</button>',
                ].join('');
                li.querySelector('button').addEventListener('click', function () {
                    showDiffForVersion(v.id, state.versions);
                });
                dom.versionList.appendChild(li);
            });
        });
    }

    function showDiffForVersion(vid, versions) {
        var idx     = versions.findIndex(function (v) { return v.id === vid; });
        if (idx < 0 || idx >= versions.length - 1) { toast('No previous version to diff against.'); return; }
        var newer   = versions[idx].content;
        var older   = versions[idx + 1].content;
        agentPost({ action: 'diff', file_path: state.filePath, old_text: older, new_text: newer })
            .then(function (data) {
                dom.diffTable.innerHTML = data.html || '';
                var sum = data.summary || {};
                dom.diffSummary.style.display = 'flex';
                dom.diffSummary.innerHTML = '<span class="diff-added">+' + sum.added + '</span><span class="diff-removed">-' + sum.removed + '</span>';
                switchTab('diff');
            });
    }

    // -----------------------------------------------------------------------
    // Context
    // -----------------------------------------------------------------------

    function loadContext() {
        if (!state.filePath) { return; }
        agentPost({ action: 'context_get', file_path: state.filePath }).then(function (data) {
            if (data.error) { dom.contextBody.textContent = data.error; return; }
            dom.contextBody.innerHTML = [
                '<div class="agent-context-section-title">Requests (' + (data.requests || []).length + ')</div>',
                '<div class="agent-context-json">' + esc(JSON.stringify(data.requests || [], null, 2)) + '</div>',
                '<div class="agent-context-section-title">Connections</div>',
                '<div class="agent-context-json">' + esc(JSON.stringify(data.connections || [], null, 2)) + '</div>',
                '<div class="agent-context-section-title">Learned</div>',
                '<div class="agent-context-json">' + esc(JSON.stringify(data.learned || {}, null, 2)) + '</div>',
                '<div class="agent-context-section-title">Change Log (' + (data.change_log || []).length + ')</div>',
                '<div class="agent-context-json">' + esc(JSON.stringify(data.change_log || [], null, 2)) + '</div>',
            ].join('');
        });
    }

    function esc(s) {
        return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
    }

    // -----------------------------------------------------------------------
    // Prompts editor
    // -----------------------------------------------------------------------

    var promptsData = null;

    function loadPromptsEditor() {
        if (promptsData) { renderPromptsEditor(); return; }
        // Load from localStorage override or defaults via fetch
        var stored = null;
        try { stored = localStorage.getItem('agent-prompts'); } catch(e) {}
        if (stored) { promptsData = JSON.parse(stored); renderPromptsEditor(); return; }

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
            html.push('<textarea class="agent-prompts-textarea" data-key="' + key + '">' + esc(promptsData[key]) + '</textarea>');
            html.push('</div>');
        });
        // Tasks
        if (promptsData.tasks) {
            html.push('<div class="agent-context-section-title" style="border-top:1px solid var(--ag-border);padding-top:10px;margin-top:4px;">Task Prompts</div>');
            Object.keys(promptsData.tasks).forEach(function (task) {
                html.push('<div class="agent-prompts-field">');
                html.push('<label>task: ' + task + '</label>');
                html.push('<textarea class="agent-prompts-textarea" data-key="tasks.' + task + '">' + esc(promptsData.tasks[task]) + '</textarea>');
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
            toast('Prompts saved to local storage.', 'success');
        } catch(e) { toast('Could not save: ' + e.message, 'error'); }
    }

    // -----------------------------------------------------------------------
    // Response area helpers
    // -----------------------------------------------------------------------

    function appendUserMsg(text) {
        var wrap   = el('div', 'agent-msg agent-msg-user');
        var lbl    = el('div', 'agent-msg-label'); lbl.textContent = 'You';
        var body   = el('div', 'agent-msg-body');
        body.innerHTML = MD.toHtml(text);
        wrap.appendChild(lbl); wrap.appendChild(body);
        dom.responseArea.appendChild(wrap);
        state.conversation.push({ role: 'user', content: text });
        scrollResponse();
        return body;
    }

    function appendAssistantPlaceholder() {
        var wrap   = el('div', 'agent-msg');
        var lbl    = el('div', 'agent-msg-label'); lbl.textContent = state.providers[state.provider]
            ? state.providers[state.provider].name : state.provider;
        var body   = el('div', 'agent-msg-body');
        body.innerHTML = '<span class="ag-cursor"></span>';
        wrap.appendChild(lbl); wrap.appendChild(body);
        dom.responseArea.appendChild(wrap);
        scrollResponse();
        return body;
    }

    function updateMsg(bodyEl, text) {
        bodyEl.innerHTML = MD.toHtml(text) + '<span class="ag-cursor"></span>';
        scrollResponse();
    }

    function finalizeMsg(bodyEl, text) {
        bodyEl.innerHTML = text ? MD.toHtml(text) : '<em style="color:var(--ag-text-muted)">Empty response</em>';
        scrollResponse();
    }

    function updateReasoning(bodyEl, text) {
        var existing = bodyEl.previousElementSibling;
        if (existing && existing.classList.contains('agent-reasoning-block')) {
            existing.querySelector('.agent-reasoning-body-inner').textContent += text;
        } else {
            var block = el('div', 'agent-reasoning-block');
            block.innerHTML = '<div class="agent-reasoning-label">Reasoning</div><pre class="agent-reasoning-body-inner" style="margin:0;white-space:pre-wrap;"></pre>';
            block.querySelector('.agent-reasoning-body-inner').textContent = text;
            bodyEl.parentNode.insertBefore(block, bodyEl);
        }
        scrollResponse();
    }

    function showApplyBar(bodyEl, rawText) {
        var bar  = el('div', 'agent-apply-bar');
        var applyBtn = el('button', 'agent-btn agent-btn-primary');
        applyBtn.textContent = 'Apply to Version';
        applyBtn.addEventListener('click', function () { applyAIResult(rawText); bar.remove(); });

        var diffBtn = el('button', 'agent-btn agent-btn-ghost');
        diffBtn.textContent = 'Preview Diff';
        diffBtn.addEventListener('click', function () {
            var code = MD.extractCode(rawText);
            agentPost({ action: 'diff', file_path: state.filePath, old_text: state.content, new_text: code })
                .then(function (data) {
                    dom.diffTable.innerHTML = data.html || '';
                    var sum = data.summary || {};
                    dom.diffSummary.style.display = 'flex';
                    dom.diffSummary.innerHTML = '<span class="diff-added">+' + sum.added + '</span><span class="diff-removed">-' + sum.removed + '</span>';
                    switchTab('diff');
                });
        });

        bar.appendChild(applyBtn);
        bar.appendChild(diffBtn);
        bodyEl.parentNode.appendChild(bar);
        scrollResponse();
    }

    function scrollResponse() {
        dom.responseArea.scrollTop = dom.responseArea.scrollHeight;
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    function setStatus(state_name, text) {
        dom.statusBar.className = 'agent-status-bar agent-' + state_name;
        dom.statusText.textContent = text || '';
    }

    function setBusy(busy) {
        state.busy = busy;
        dom.runBtn.disabled   = busy;
        dom.abortBtn.disabled = !busy;
        dom.loadBtn.disabled  = busy;
        if (busy) { setStatus('busy', 'Working...'); }
    }

    // -----------------------------------------------------------------------
    // Toast
    // -----------------------------------------------------------------------

    function toast(msg, type) {
        var t = el('div', 'agent-toast agent-toast-' + (type || 'info'));
        t.textContent = msg;
        document.body.appendChild(t);
        setTimeout(function () { t.classList.add('toast-visible'); }, 10);
        setTimeout(function () {
            t.classList.remove('toast-visible');
            setTimeout(function () { t.parentNode && t.parentNode.removeChild(t); }, 250);
        }, 3500);
    }

    // -----------------------------------------------------------------------
    // HTTP helpers
    // -----------------------------------------------------------------------

    function agentPost(data) {
        return fetch(AGENT_URL, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(data),
        }).then(function (r) { return r.json(); });
    }

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    function el(tag, className) {
        var e = document.createElement(tag);
        if (className) { e.className = className; }
        return e;
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    var agent = {

        /**
         * Initialise the agent panel. Options:
         *   theme:       'dark-neu' | 'morphism' | 'glassmorphic' | 'keyboard-ui' | 'ada'
         *   agentUrl:    path to agent.php   (default 'ai/agent/agent.php')
         *   runUrl:      path to run.php     (default 'ai/agent/run.php')
         */
        init: function (opts) {
            opts = opts || {};
            if (opts.agentUrl) { AGENT_URL = opts.agentUrl; }
            if (opts.runUrl)   { RUN_URL   = opts.runUrl;   }

            // Restore theme from storage or use option or default
            var stored = null;
            try { stored = localStorage.getItem('agent-theme'); } catch(e) {}
            state.theme = stored || opts.theme || 'ada';

            buildUI();

            // If a theme was persisted, apply it after build
            applyTheme(state.theme);

            // Load providers immediately if modal is already open somehow
            loadProviders();
        },

        open:  open,
        close: close,

        /**
         * Programmatically load a file into the agent.
         * path: relative path string
         * content: file content string
         */
        loadFile: function (path, content) {
            if (!dom.filePath) { return; }
            dom.filePath.value = path;
            state.filePath = path;
            state.content  = content || '';
            if (content) {
                agentPost({ action: 'save_version', file_path: path, content: content, label: 'initial' })
                    .then(function (data) {
                        state.versions = data.versions || [];
                        state.history  = data.history  || {};
                        setActiveCurrent();
                        loadOutline();
                    });
            } else {
                loadFile();
            }
        },

        applyTheme: applyTheme,

        getDiff: showDiff,
    };

    LiveCSS.agent = agent;

}(window.LiveCSS = window.LiveCSS || {}));
