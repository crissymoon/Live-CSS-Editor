/**
 * agent.js -- Entry point for the Code Agent UI.
 * Loaded LAST. Registers switchTab, applyTheme, loadProviders and wires all events.
 * All logic lives in agent-core / agent-ui / agent-run / agent-diff /
 * agent-context / agent-prompts / agent-chat / agent-window.
 */
'use strict';

(function (LiveCSS) {

    var C     = LiveCSS._agentCore;
    var state = C.state;
    var dom   = C.dom;

    // -----------------------------------------------------------------------
    // Tab switching  (registered on C so other modules can call C.switchTab)
    // -----------------------------------------------------------------------

    function switchTab(name) {
        state.activeTab = name;
        dom.tabs.forEach(function (t) {
            t.classList.toggle('agent-tab-active', t.dataset.tab === name);
        });
        dom.panes.forEach(function (p) {
            p.classList.toggle('agent-pane-active', p.dataset.pane === name);
        });
        if (name === 'prompts')                    { C.loadPromptsEditor(); }
        if (name === 'context'  && state.filePath) { C.loadContext(); }
        if (name === 'versions' && state.filePath) { C.renderVersionList(); }
        if (name === 'diff'     && state.filePath) { C.refreshDiffSelectors(); }
    }
    C.switchTab = switchTab;

    // -----------------------------------------------------------------------
    // Theme
    // -----------------------------------------------------------------------

    function applyTheme(name) {
        if (!C.THEMES.includes(name)) { name = 'ada'; }
        C.THEMES.forEach(function (t) { dom.modal.classList.remove('theme-' + t); });
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
        C.agentPost({ action: 'providers' }).then(function (data) {
            if (data.error) { C.setStatus('error', data.error); return; }
            state.providers = data.providers || {};
            populateProviders();
        }).catch(function (err) { C.setStatus('error', 'Cannot load providers: ' + err.message); });
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
        var p   = state.providers[state.provider] || {};
        var ms  = p.models || (p.default_model ? [p.default_model] : []);
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
    // Open / close / toggle
    // -----------------------------------------------------------------------

    function toggleOpen() {
        if (dom.modal.classList.contains('agent-open')) { close(); }
        else { open(); }
    }

    function open() {
        var chip = document.getElementById('agent-taskbar-chip');
        if (chip) { chip.remove(); }
        state.minimized = false;

        if (!dom.modal.style.left) {
            var vw = window.innerWidth;
            var vh = window.innerHeight;
            var mw = dom.modal.offsetWidth  || 860;
            var mh = dom.modal.offsetHeight || 580;
            dom.modal.style.left   = Math.max(0, Math.round((vw - mw) / 2)) + 'px';
            dom.modal.style.top    = Math.max(0, Math.round((vh - mh) / 2)) + 'px';
            dom.modal.style.right  = 'auto';
            dom.modal.style.bottom = 'auto';
        }

        dom.modal.classList.add('agent-open');
        dom.trigger.classList.add('agent-trigger-active');

        if (!state.providers || !Object.keys(state.providers).length) {
            loadProviders();
        }
        if (state.activeTab === 'prompts') { C.loadPromptsEditor(); }
        if (state.source    === 'editors') { C.loadOutlineFromEditors(); }
    }

    function close() {
        dom.modal.classList.remove('agent-open');
        dom.trigger.classList.remove('agent-trigger-active');
        var chip = document.getElementById('agent-taskbar-chip');
        if (chip) { chip.remove(); }
        state.minimized = false;
    }

    // -----------------------------------------------------------------------
    // Event binding
    // -----------------------------------------------------------------------

    function bindEvents() {
        dom.trigger.addEventListener('click', toggleOpen);
        dom.closeBtn.addEventListener('click', close);

        dom.tabs.forEach(function (tab) {
            tab.addEventListener('click', function () { switchTab(tab.dataset.tab); });
        });

        dom.themeDots.addEventListener('click', function (e) {
            var dot = e.target.closest('.agent-theme-dot');
            if (dot) { applyTheme(dot.dataset.theme); }
        });

        dom.loadBtn.addEventListener('click', C.loadFile);
        dom.filePath.addEventListener('keydown', function (e) { if (e.key === 'Enter') { C.loadFile(); } });

        dom.srcEditorsBtn.addEventListener('click', function () { C.switchSource('editors'); });
        dom.srcFileBtn.addEventListener('click',    function () { C.switchSource('file'); });

        dom.backBtn.addEventListener('click', function () { C.navigate('back'); });
        dom.fwdBtn.addEventListener('click',  function () { C.navigate('forward'); });

        dom.provider.addEventListener('change', function () {
            state.provider = dom.provider.value;
            populateModels();
            updateStreamBadge();
        });
        dom.model.addEventListener('change', function () { state.model = dom.model.value; });

        dom.runBtn.addEventListener('click',    C.runAgent);
        dom.abortBtn.addEventListener('click',  C.abortStream);
        dom.runCmdBtn.addEventListener('click', C.runCommand);

        dom.modeSelect.addEventListener('change', function () { C.switchMode(dom.modeSelect.value); });
        dom.fuzzyBtn.addEventListener('click', C.runFuzzySearch);
        dom.fuzzyInput.addEventListener('keydown', function (e) { if (e.key === 'Enter') { C.runFuzzySearch(); } });
        dom.outlineBtn.addEventListener('click', C.runCSSOutline);
        dom.backupBtn.addEventListener('click',  C.runBackup);

        dom.diffBtn.addEventListener('click', C.showDiff);
        dom.refreshContext.addEventListener('click', C.loadContext);
        dom.savePrompts.addEventListener('click', C.savePrompts);

        dom.chatSend.addEventListener('click', C.chatSend);
        dom.chatInput.addEventListener('keydown', function (e) {
            if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); C.chatSend(); }
        });
        dom.chatClear.addEventListener('click', function () {
            state.chatHistory = [];
            dom.chatMessages.innerHTML = '';
        });
        dom.chatProvider.addEventListener('change', function () { C.populateChatModels(); });

        dom.minBtn.addEventListener('click', C.minimize);
        C.initDrag();
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    LiveCSS.agent = {

        /**
         * Initialise the agent.
         * @param {object} opts  { agentUrl, runUrl, theme }
         */
        init: function (opts) {
            opts = opts || {};
            if (opts.agentUrl) { C.AGENT_URL = opts.agentUrl; }
            if (opts.runUrl)   { C.RUN_URL   = opts.runUrl;   }

            var stored = null;
            try { stored = localStorage.getItem('agent-theme'); } catch(e) {}
            state.theme = stored || opts.theme || 'ada';

            C.buildUI();
            bindEvents();
            applyTheme(state.theme);
            loadProviders();
            C.populateChatProviders();
        },

        open:  open,
        close: close,

        /**
         * Programmatically load a file into the agent.
         * @param {string} path
         * @param {string} [content]
         */
        loadFile: function (path, content) {
            if (!dom.filePath) { return; }
            dom.filePath.value = path;
            state.filePath = path;
            state.content  = content || '';
            if (content) {
                C.agentPost({ action: 'save_version', file_path: path, content: content, label: 'initial' })
                    .then(function (data) {
                        state.versions = data.versions || [];
                        state.history  = data.history  || {};
                        C.setActiveCurrent();
                        C.loadOutline();
                    });
            } else {
                C.loadFile();
            }
        },

        applyTheme: applyTheme,
        getDiff:    function () { C.showDiff(); },
    };

}(window.LiveCSS = window.LiveCSS || {}));
