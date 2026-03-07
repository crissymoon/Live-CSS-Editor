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
    // Preview model lock
    // When the task is set to "preview" the provider and model selects are
    // forced to Anthropic + claude-haiku-4-5-20251001, which is the only
    // validated model for preview generation. The selects are disabled so the
    // user cannot accidentally switch to a model that breaks the workflow.
    // The previous provider and model are restored when leaving the task.
    // -----------------------------------------------------------------------

    var _lockProv       = '';
    var _lockModel      = '';
    var _lockPickValue  = '';
    var _isLocked       = false;

    var PREVIEW_PROVIDER = 'anthropic';
    var PREVIEW_MODEL    = 'claude-haiku-4-5-20251001';

    // Apply lock immediately if providers are populated, otherwise poll.
    function applyLockWhenReady() {
        if (_isLocked) { return; }
        if (Object.keys(state.providers).length > 0 &&
            dom.provider.options.length > 0) {
            lockForPreview();
        } else {
            var t = setInterval(function () {
                if (Object.keys(state.providers).length > 0 &&
                    dom.provider.options.length > 0) {
                    clearInterval(t);
                    if (state.task === 'preview') { lockForPreview(); }
                }
            }, 80);
        }
    }

    function lockForPreview() {
        if (_isLocked) { return; }
        _isLocked  = true;
        _lockProv  = state.provider;
        _lockModel = state.model;

        // Force the provider select to show Anthropic.
        // Create the option if the server did not return it.
        var provOpt = dom.provider.querySelector('option[value="' + PREVIEW_PROVIDER + '"]');
        if (!provOpt) {
            provOpt             = document.createElement('option');
            provOpt.value       = PREVIEW_PROVIDER;
            provOpt.textContent = 'Anthropic';
            dom.provider.appendChild(provOpt);
        }
        dom.provider.value = PREVIEW_PROVIDER;
        state.provider     = PREVIEW_PROVIDER;

        // Force the model select to show haiku.
        // Add the option if it is not already present.
        var modelOpt = dom.model.querySelector('option[value="' + PREVIEW_MODEL + '"]');
        if (!modelOpt) {
            modelOpt             = document.createElement('option');
            modelOpt.value       = PREVIEW_MODEL;
            modelOpt.textContent = PREVIEW_MODEL;
            dom.model.appendChild(modelOpt);
        }
        dom.model.value = PREVIEW_MODEL;
        state.model     = PREVIEW_MODEL;

        updateStreamBadge();

        // Visually disable the selects and mark the label row.
        dom.provider.classList.add('preview-locked');
        dom.provider.disabled = true;
        dom.model.classList.add('preview-locked');
        dom.model.disabled = true;

        var row = dom.provider.closest('.agent-task-row');
        if (row) { row.classList.add('agent-provider-row-locked'); }
    }

    function unlockFromPreview() {
        if (!_isLocked) { return; }
        _isLocked = false;

        dom.provider.disabled = false;
        dom.provider.classList.remove('preview-locked');
        dom.model.disabled = false;
        dom.model.classList.remove('preview-locked');

        var row = dom.provider.closest('.agent-task-row');
        if (row) { row.classList.remove('agent-provider-row-locked'); }

        // Restore state flags so populateProviders re-selects the right items.
        state.provider = _lockProv || state.provider;
        state.model    = _lockModel || state.model;
        _lockPickValue = '';

        // Fully rebuild both selects from the server provider list.
        populateProviders();
    }

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
        C.saveSettings();
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
        C.saveSettings();
    }

    // -----------------------------------------------------------------------
    // Providers
    // -----------------------------------------------------------------------

    // Sync state.provider + state.model to the hardcoded workflow values.
    // These two models are fixed -- the run tab has no selectable picker.
    function syncStateFromModelPick() {
        try {
            state.provider = 'anthropic';
            state.model    = 'claude-haiku-4-5-20251001';
            if (dom.provider) { dom.provider.value = state.provider; }
            if (dom.model)    { dom.model.value    = state.model;    }
            console.log('[agent] syncStateFromModelPick: hardcoded workflow -- provider=anthropic model=claude-haiku-4-5-20251001 (GPT-4 Mini used in parallel by backend)');
        } catch (e) {
            console.error('[agent] syncStateFromModelPick failed:', e);
        }
    }

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
            // Skip providers flagged as not available for the agent run tab.
            if (state.providers[slug].agent_run === false) {
                console.log('[agent] provider "' + slug + '" excluded from run tab (agent_run: false)');
                return;
            }
            var o = document.createElement('option');
            o.value       = slug;
            o.textContent = state.providers[slug].name;
            if (slug === state.provider) { o.selected = true; }
            dom.provider.appendChild(o);
        });
        // If the previously saved provider was excluded, fall back to the first available.
        if (!dom.provider.value && dom.provider.options.length > 0) {
            dom.provider.value = dom.provider.options[0].value;
            console.warn('[agent] saved provider was excluded from run tab -- defaulting to "' + dom.provider.value + '"');
        }
        state.provider = dom.provider.value;
        populateModels();
        updateStreamBadge();

        // state.providers is now populated -- refresh the chat tab model list so it
        // has real model names instead of the placeholder set before the async fetch.
        if (C.populateChatModels) {
            try {
                C.populateChatModels();
                console.log('[agent] chat model list refreshed from state.providers');
            } catch (e) {
                console.warn('[agent] populateChatModels refresh failed:', e);
            }
        }
        // The visible model picker has hardcoded options and should win over the
        // hidden select state set by populateModels() above.
        syncStateFromModelPick();
        updateStreamBadge();
    }

    function populateModels() {
        var p   = state.providers[state.provider] || {};
        var ms  = p.models || (p.default_model ? [p.default_model] : []);
        var def = p.default_model || (ms[0] || '');
        // Prefer the previously-saved model for this provider if it is still available.
        var preferred = (state.model && ms.indexOf(state.model) !== -1) ? state.model : def;
        dom.model.innerHTML = '';
        ms.forEach(function (m) {
            var o = document.createElement('option');
            o.value = m; o.textContent = m;
            if (m === preferred) { o.selected = true; }
            dom.model.appendChild(o);
        });
        state.model = preferred || def;
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

        dom.srcEditorsBtn.addEventListener('click', function () { C.switchSource('editors'); C.saveSettings(); });

        dom.backBtn.addEventListener('click', function () { C.navigate('back'); });
        dom.fwdBtn.addEventListener('click',  function () { C.navigate('forward'); });

        // No model picker change listener needed -- the run tab uses a fixed workflow.

        dom.runBtn.addEventListener('click',    C.runAgent);
        dom.abortBtn.addEventListener('click',  C.abortStream);
        dom.runCmdBtn.addEventListener('click', C.runCommand);

        dom.modeSelect.addEventListener('change', function () {
            // Leaving a mode unlocks any model override that was active.
            unlockFromPreview();
            C.switchMode(dom.modeSelect.value);
            // switchMode rebuilds the task dropdown and sets state.task internally
            // but does not fire dom.task change, so apply the lock here if needed.
            if (state.task === 'preview') {
                applyLockWhenReady();
            }
            C.saveSettings();
        });
        dom.task.addEventListener('change', function () {
            state.task = dom.task.value;
            if (state.task === 'preview') {
                applyLockWhenReady();
            } else {
                unlockFromPreview();
            }
            updateInstructionPlaceholder();
            C.saveSettings();
        });
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

        // ---- Restore persisted UI state -----------------------------------
        // Mode: if saved mode differs from default, rebuild the task dropdown.
        if (state.mode && state.mode !== 'repair') {
            dom.modeSelect.value = state.mode;
            C.switchMode(state.mode);  // rebuilds task dropdown for that mode
        }
        // Task: set the saved task in the (now-correct) dropdown.
        if (state.task) {
            dom.task.value = state.task;
            state.task = dom.task.value; // snap to actual available value
        }
        // Source toggle.
        C.switchSource(state.source || 'editors');
        // Active tab.
        if (state.activeTab && state.activeTab !== 'run') {
            switchTab(state.activeTab);
        }
        // If the restored task is preview, apply the model lock.
        // Providers may still be loading so use applyLockWhenReady.
        if (state.task === 'preview') {
            applyLockWhenReady();
        }
        // ------------------------------------------------------------------

        updateInstructionPlaceholder();
    }

    // -----------------------------------------------------------------------
    // Instruction placeholder
    // -----------------------------------------------------------------------

    var INSTRUCTION_PLACEHOLDERS = {
        'request_change': 'e.g. make the cat image bigger, change the nav color to purple, add a border to the card...',
        'fix':            'Describe the bug you want fixed...',
        'add_feature':    'Describe the feature you want added...',
        'explain':        'What part of the code should be explained?',
        'review':         'Anything specific to focus the review on?',
        'optimize':       'Any specific performance goals?',
        'security':       'Any areas of particular concern?',
        'preview':        'Optional hint for the preview layout or mood. Leave blank for a full auto-generated preview.'
    };
    var DEFAULT_PLACEHOLDER = 'Describe what you want the agent to do...';

    function updateInstructionPlaceholder() {
        var task = dom.task.value;
        dom.instruction.placeholder = INSTRUCTION_PLACEHOLDERS[task] || DEFAULT_PLACEHOLDER;
        dom.runBtn.textContent = (task === 'request_change') ? 'Apply Change' : 'Run';
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
            syncStateFromModelPick();  // set state.provider + state.model from hardcoded picker
            loadProviders();
            C.populateChatProviders();
            if (C.neuralPreload) { C.neuralPreload(); }
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
