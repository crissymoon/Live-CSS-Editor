/**
 * agent-run.js -- File loading, version history, run/stream logic.
 * Depends on: agent-core.js, agent-ui.js
 * Cross-calls (registered by other modules): C.appendUserMsg, C.appendAssistantPlaceholder,
 *   C.updateMsg, C.finalizeMsg, C.updateReasoning, C.showApplyBar, C.scrollResponse
 * Cross-calls (registered by agent.js): C.switchTab
 */
'use strict';

(function (LiveCSS) {

    var C     = LiveCSS._agentCore;
    var state = C.state;
    var dom   = C.dom;

    // -----------------------------------------------------------------------
    // Load file / versions
    // -----------------------------------------------------------------------

    function loadFile() {
        var path = dom.filePath.value.trim();
        if (!path) { C.toast('Enter a file path.', 'error'); return; }
        state.filePath = path;
        C.setStatus('busy', 'Loading...');

        C.agentPost({ action: 'get_versions', file_path: path }).then(function (data) {
            if (data.error) { C.setStatus('error', data.error); return; }
            state.versions = data.versions || [];
            state.history  = data.history  || {};
            updateHistorySlots();
            setActiveCurrent();
            dom.historyBar.style.display = '';
            C.setStatus('ok', 'Loaded ' + state.versions.length + ' version(s)');
            if (state.versions.length > 0) {
                state.content = state.versions[0].content;
                loadOutline();
            }
        }).catch(function (e) { C.setStatus('error', e.message); });
    }

    function setActiveCurrent() {
        var cur = state.history.current_version;
        if (cur === undefined) { return; }
        var count   = state.versions.length;
        var offset  = (count - 1) - cur;
        var version = state.versions[offset];
        if (version) { state.content = version.content; }
        renderHistorySlots();
        dom.backBtn.disabled = (cur <= 0 || (state.history.consecutive_back >= 1));
        dom.fwdBtn.disabled  = (cur >= count - 1);
    }

    function renderHistorySlots() {
        var count = state.versions.length;
        var cur   = parseInt(state.history.current_version) || 0;
        var cBack = parseInt(state.history.consecutive_back) || 0;
        dom.historySlots.innerHTML = '';

        for (var i = 0; i < 3; i++) {
            var slot = C.el('span', 'agent-history-slot');
            if (i < count) { slot.classList.add('slot-occupied'); }
            if (i === cur)  { slot.classList.add('slot-current'); }
            if (i > 0 && cBack >= 1 && i < cur) { slot.classList.add('slot-blocked'); }
            slot.title = i < count ? 'Version ' + (i + 1) : 'Empty slot';
            dom.historySlots.appendChild(slot);
        }
    }

    function updateHistorySlots() { renderHistorySlots(); }

    // -----------------------------------------------------------------------
    // Navigate (back / forward)
    // -----------------------------------------------------------------------

    function navigate(direction) {
        if (!state.filePath) { return; }
        C.setStatus('busy', 'Navigating ' + direction + '...');
        C.agentPost({ action: 'navigate', file_path: state.filePath, direction: direction })
            .then(function (data) {
                if (data.blocked) {
                    C.setStatus('error', data.reason);
                    C.toast(data.reason, 'error');
                    return;
                }
                if (data.error) { C.setStatus('error', data.error); return; }
                state.history = data.history || {};
                if (data.version) {
                    state.content = data.version.content;
                    loadOutline();
                }
                C.agentPost({ action: 'get_versions', file_path: state.filePath }).then(function (vd) {
                    state.versions = vd.versions || [];
                    setActiveCurrent();
                    C.setStatus('ok', 'At version ' + (parseInt(state.history.current_version || 0) + 1));
                });
            }).catch(function (e) { C.setStatus('error', e.message); });
    }

    // -----------------------------------------------------------------------
    // Outline
    // -----------------------------------------------------------------------

    function loadOutline() {
        if (!state.filePath || !state.content) { return; }
        C.agentPost({ action: 'outline', file_path: state.filePath, content: state.content })
            .then(function (data) {
                if (data.html) { dom.outline.innerHTML = data.html; }
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

    function loadOutlineFromEditors() {
        var cssContent = '';
        try {
            if (LiveCSS && LiveCSS.editor && LiveCSS.editor.getCssEditor) {
                cssContent = LiveCSS.editor.getCssEditor().getValue();
            }
        } catch(e) {}

        if (!cssContent || !cssContent.trim()) {
            dom.outline.innerHTML = '<p class="outline-empty">No CSS in editor yet</p>';
            return;
        }

        C.agentPost({ action: 'outline', file_path: '_editors.css', content: cssContent })
            .then(function (data) {
                if (data.html) {
                    dom.outline.innerHTML = data.html;
                    dom.outline.querySelectorAll('.outline-node').forEach(function (node) {
                        node.addEventListener('click', function () {
                            var line = parseInt(node.dataset.line, 10);
                            if (line && LiveCSS.editor && LiveCSS.editor.getCssEditor) {
                                LiveCSS.editor.getCssEditor().setCursor({ line: line - 1, ch: 0 });
                                LiveCSS.editor.getCssEditor().focus();
                            }
                        });
                    });
                }
            }).catch(function () {});
    }

    function loadThemeList() {
        C.agentPost({ action: 'list_themes' }).then(function (data) {
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
    // Run agent (streaming or blocking)
    // -----------------------------------------------------------------------

    function runAgent() {
        if (state.busy) { return; }

        var taskVal = dom.task.value;

        if (state.mode === 'new_project' && taskVal === 'preview') {
            var themeFile = dom.themeSelect ? dom.themeSelect.value : '';
            if (!themeFile) { C.toast('Select a theme first.', 'error'); return; }
            runPreviewGeneration(themeFile);
            return;
        }

        var filePath = '';
        var content  = '';

        if (state.source === 'editors') {
            try {
                var htmlVal = LiveCSS.editor.getHtmlEditor ? LiveCSS.editor.getHtmlEditor().getValue() : '';
                var cssVal  = LiveCSS.editor.getCssEditor  ? LiveCSS.editor.getCssEditor().getValue()  : '';
                var jsVal   = LiveCSS.editor.getJsEditor   ? LiveCSS.editor.getJsEditor().getValue()   : '';
                content = '/* === HTML === */\n' + htmlVal
                        + '\n\n/* === CSS === */\n' + cssVal
                        + '\n\n/* === JS === */\n'  + jsVal;
            } catch(e) { content = ''; }
            filePath = '_editors';
            if (!content.replace(/\/\*[^*]*\*\//g, '').trim()) {
                C.toast('The editors are empty.', 'error');
                return;
            }
        } else {
            if (!state.filePath) { C.toast('Load a file first.', 'error'); return; }
            filePath = state.filePath;
            content  = state.content;
        }

        var userInstruction = dom.instruction.value.trim();

        // Request Change: require an instruction and build an action-oriented prompt
        if (taskVal === 'request_change') {
            if (!userInstruction) { C.toast('Describe the change you want to make.', 'error'); return; }
            userInstruction = 'Make this change to the code: ' + userInstruction + '\n\n'
                + 'Rules:\n'
                + '- Do not ask clarifying questions. Make your best interpretation and apply it.\n'
                + '- If the requested element or thing does not exist yet, add it.\n'
                + '- Return the COMPLETE updated code using these exact three fenced code blocks (include all original code with changes applied):\n'
                + '```html\n(full HTML here)\n```\n'
                + '```css\n(full CSS here)\n```\n'
                + '```javascript\n(full JS here)\n```';
        }

        var payload = {
            provider:    state.provider,
            model:       state.model,
            task:        taskVal,
            file_path:   filePath,
            content:     content,
            instruction: userInstruction,
            messages:    state.conversation,
            mode:        state.mode,
        };

        var isStream = (state.providers[state.provider] || {}).supports_streaming !== false;

        C.appendUserMsg(payload.task + (dom.instruction.value.trim() ? ': ' + dom.instruction.value.trim() : ''));
        C.setBusy(true);

        // For editors source, go directly to the AI provider endpoint (not run.php).
        // run.php is intended for file-based version management; editors content
        // is in memory and needs a simple chat-style API call.
        if (state.source === 'editors') {
            streamRunDirect(payload);
        } else if (isStream) {
            streamRun(payload);
        } else {
            blockingRun(payload);
        }
    }

    function runPreviewGeneration(themeFile) {
        C.setBusy(true);
        C.setStatus('busy', 'Generating preview for ' + themeFile + '...');
        state.previewCss = '';

        // Fetch actual theme CSS in parallel -- applied to CSS editor on "Apply"
        fetch('style-sheets/' + themeFile)
            .then(function (r) { return r.ok ? r.text() : ''; })
            .catch(function () { return ''; })
            .then(function (css) { state.previewCss = css; });

        C.agentPost({ action: 'backup', target: 'all' }).then(function () {
            return C.agentPost({ action: 'css_outline', file_path: themeFile, format: 'text' });
        }).then(function (outlineData) {
            var outlineText = outlineData.outline || '';
            var themeName   = themeFile.replace('.css', '');
            return C.agentPost({ action: 'fuzzy_search', query: 'component preview ' + themeName, format: 'text' }).then(function (searchData) {
                return { outline: outlineText, search: searchData.results || '' };
            });
        }).then(function (context) {
            var instruction     = dom.instruction.value.trim();
            var fullInstruction = 'Generate a visually rich, complete preview HTML page for this CSS theme.\n\n'
                + 'CSS Outline (available classes and variables):\n' + context.outline + '\n\n'
                + 'Related context:\n' + context.search + '\n\n'
                + (instruction ? 'Additional instructions: ' + instruction + '\n\n' : '')
                + 'Rules:\n'
                + '- Do NOT include <link> tags -- the theme CSS is injected externally\n'
                + '- In the <head> include exactly this reset block and nothing else:\n'
                + '  <style>\n'
                + '    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }\n'
                + '    body { padding: 0; }\n'
                + '    section, .section { padding: 3rem 2rem; }\n'
                + '    .container, .inner, .content { padding: 2rem; max-width: 1100px; margin: 0 auto; }\n'
                + '    .card, .panel, .box { padding: 1.5rem; margin-bottom: 1.5rem; }\n'
                + '    .hero, .hero-section { padding: 4rem 2rem; }\n'
                + '    nav, .nav, .navbar { padding: 0.75rem 2rem; }\n'
                + '    footer, .footer { padding: 2rem; margin-top: 2rem; }\n'
                + '    form, .form { padding: 1.5rem; }\n'
                + '    .form-group, .field { margin-bottom: 1rem; }\n'
                + '    h1, h2, h3, h4 { margin-bottom: 0.75rem; }\n'
                + '    p { margin-bottom: 1rem; line-height: 1.6; }\n'
                + '  </style>\n'
                + '- Use the theme body class and ALL component classes from the outline\n'
                + '- Build a full one-page layout: nav, hero, cards, buttons, forms, footer\n'
                + '- Every section and card must have explicit padding via its class\n'
                + '- Reference only CSS variables from the outline, no hardcoded colors\n\n'
                + 'Return ONLY a single fenced html code block. No explanation, no other blocks.\n';

            var payload = {
                provider:    state.provider,
                model:       state.model,
                task:        'preview',
                file_path:   themeFile,
                content:     '',
                instruction: fullInstruction,
                messages:    state.conversation,
                mode:        'new_project',
            };

            var label = instruction ? instruction.slice(0, 50) : themeFile;
            C.appendUserMsg('Generate preview: ' + label);

            var isStream = (state.providers[state.provider] || {}).supports_streaming !== false;
            if (isStream) { streamRun(payload); }
            else          { blockingRun(payload); }
        }).catch(function (e) {
            C.setStatus('error', e.message);
            C.setBusy(false);
        });
    }

    function streamRun(payload) {
        var bodyEl    = C.appendAssistantPlaceholder();
        var accum     = '';
        var finalized = false;
        var ctrl      = new AbortController();

        state.activeStream = { close: function () { ctrl.abort(); } };

        function finish(text) {
            if (finalized) { return; }
            finalized = true;
            C.finalizeMsg(bodyEl, text);
            if (text) {
                state.conversation.push({ role: 'assistant', content: text });
                C.showApplyBar(bodyEl, text);
            }
            C.setBusy(false);
            state.activeStream = null;
        }

        fetch(C.RUN_URL, {
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
                    if (r.done) { finish(accum); return; }
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
                            C.updateMsg(bodyEl, accum);
                        } else if (evName === 'reasoning') {
                            C.updateReasoning(bodyEl, data.text || '');
                        } else if (evName === 'done') {
                            finish(accum);
                        } else if (evName === 'error') {
                            if (!finalized) {
                                finalized = true;
                                C.finalizeMsg(bodyEl, accum || '_Error_');
                                C.setStatus('error', data.error || 'Error');
                                C.setBusy(false);
                                state.activeStream = null;
                            }
                        }
                    });
                    return pump();
                });
            }
            return pump();
        })
        .catch(function (err) {
            if (err.name === 'AbortError') { C.setBusy(false); return; }
            C.setStatus('error', err.message);
            if (!finalized) {
                finalized = true;
                C.finalizeMsg(bodyEl, accum || '_Request failed_');
                C.setBusy(false);
                state.activeStream = null;
            }
        });
    }

    // -----------------------------------------------------------------------
    // streamRunDirect -- posts content + instruction straight to the AI
    // provider endpoint (ai/anthropic.php etc.).  Used when source=editors so
    // we get real streaming without needing run.php / version management.
    // -----------------------------------------------------------------------

    function streamRunDirect(payload) {
        var providerSlug = payload.provider || state.provider || 'anthropic';
        var provCfg      = C.CHAT_PROVIDERS[providerSlug] || C.CHAT_PROVIDERS.anthropic;
        var endpoint     = provCfg.endpoint;

        var system = 'You are a web design editing assistant embedded in a live CSS editor. '
            + 'The user will give you their current HTML, CSS, and JavaScript followed by an instruction. '
            + 'Apply the instruction and respond with ONLY the three complete fenced code blocks below '
            + '(no other text, no explanations, no questions):\n'
            + '```html\n(full HTML here)\n```\n'
            + '```css\n(full CSS here)\n```\n'
            + '```javascript\n(full JS here)\n```';

        var userMsg = payload.content + '\n\n---\n' + payload.instruction;

        var messages = [{ role: 'user', content: userMsg }];

        var bodyEl    = C.appendAssistantPlaceholder();
        var accum     = '';
        var finalized = false;
        var ctrl      = new AbortController();

        state.activeStream = { close: function () { ctrl.abort(); } };

        function finish(text) {
            if (finalized) { return; }
            finalized = true;
            C.finalizeMsg(bodyEl, text);
            if (text) {
                state.conversation.push({ role: 'assistant', content: text });
                C.showApplyBar(bodyEl, text);
            }
            C.setBusy(false);
            state.activeStream = null;
        }

        fetch(endpoint, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({ messages: messages, model: payload.model || '', system: system }),
            signal:  ctrl.signal
        })
        .then(function (resp) {
            if (!resp.ok) { throw new Error('HTTP ' + resp.status); }
            var reader  = resp.body.getReader();
            var decoder = new TextDecoder();
            var buf     = '';

            function pump() {
                return reader.read().then(function (r) {
                    if (r.done) { finish(accum); return; }
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
                            C.updateMsg(bodyEl, accum);
                        } else if (evName === 'done') {
                            finish(accum);
                        } else if (evName === 'error') {
                            if (!finalized) {
                                finalized = true;
                                C.finalizeMsg(bodyEl, accum || '_Error: ' + (data.error || 'Unknown') + '_');
                                C.setStatus('error', data.error || 'Error');
                                C.setBusy(false);
                                state.activeStream = null;
                            }
                        }
                    });
                    return pump();
                });
            }
            return pump();
        })
        .catch(function (err) {
            if (err.name === 'AbortError') { C.setBusy(false); return; }
            C.setStatus('error', err.message);
            if (!finalized) {
                finalized = true;
                C.finalizeMsg(bodyEl, accum || '_Request failed: ' + err.message + '_');
                C.setBusy(false);
                state.activeStream = null;
            }
        });
    }

    function blockingRun(payload) {
        var bodyEl = C.appendAssistantPlaceholder();
        fetch(C.RUN_URL, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(payload),
        })
        .then(function (r) { return r.json(); })
        .then(function (data) {
            if (data.error) {
                C.finalizeMsg(bodyEl, '_Error: ' + data.error + '_');
                C.setStatus('error', data.error);
            } else {
                C.finalizeMsg(bodyEl, data.text || '');
                if (data.text) {
                    state.conversation.push({ role: 'assistant', content: data.text });
                    C.showApplyBar(bodyEl, data.text);
                }
                C.setStatus('ok', 'Done');
            }
        })
        .catch(function (err) {
            C.finalizeMsg(bodyEl, '_Request failed_');
            C.setStatus('error', err.message);
        })
        .finally(function () { C.setBusy(false); });
    }

    function abortStream() {
        if (state.activeStream) {
            state.activeStream.close();
            state.activeStream = null;
        }
        C.setBusy(false);
        C.setStatus('ok', 'Aborted');
    }

    // -----------------------------------------------------------------------
    // Apply AI result
    // -----------------------------------------------------------------------

    function applyAIResult(rawText) {
        var code = C.MD.extractCode(rawText);
        if (!code.trim()) { C.toast('No code block found in response.', 'error'); return; }

        var summary = 'AI: ' + dom.task.value + (dom.instruction.value ? ' -- ' + dom.instruction.value.slice(0, 60) : '');
        var payload = { action: 'apply', file_path: state.filePath, content: code, summary: summary, model: state.provider + ':' + state.model };

        C.agentPost(payload).then(function (data) {
            if (data.error) { C.toast(data.error, 'error'); return; }
            state.versions = data.versions || [];
            state.history  = data.history  || {};
            state.content  = code;
            setActiveCurrent();
            loadOutline();
            C.toast('Applied and saved as version ' + data.version_id, 'success');
            C.setStatus('ok', 'Version ' + data.version_id + ' saved');
        }).catch(function (e) { C.toast(e.message, 'error'); });
    }

    // -----------------------------------------------------------------------
    // Apply AI result to the three live editors
    // -----------------------------------------------------------------------

    function applyToEditors(rawText) {
        function extractLang(text, langs) {
            for (var i = 0; i < langs.length; i++) {
                var re = new RegExp('```' + langs[i] + '\\s*\\n([\\s\\S]*?)```', 'i');
                var m  = text.match(re);
                if (m) { return m[1]; }
            }
            return null;
        }

        var isPreview = (state.mode === 'new_project' && state.task === 'preview');

        var html = extractLang(rawText, ['html']);
        // For preview: use the fetched theme CSS (not AI output); skip JS entirely
        var css  = isPreview ? (state.previewCss || null) : extractLang(rawText, ['css']);
        var js   = isPreview ? null : extractLang(rawText, ['javascript', 'js']);

        var applied = [];
        try {
            if (html !== null && LiveCSS.editor && LiveCSS.editor.getHtmlEditor) {
                LiveCSS.editor.getHtmlEditor().setValue(html);
                applied.push('HTML');
            }
            if (css !== null && css !== '' && LiveCSS.editor && LiveCSS.editor.getCssEditor) {
                LiveCSS.editor.getCssEditor().setValue(css);
                applied.push('CSS');
            }
            if (js !== null && js !== '' && LiveCSS.editor && LiveCSS.editor.getJsEditor) {
                LiveCSS.editor.getJsEditor().setValue(js);
                applied.push('JS');
            }
            if (applied.length && LiveCSS.editor && LiveCSS.editor.updatePreview) {
                LiveCSS.editor.updatePreview();
            }
        } catch(e) {}

        if (applied.length) {
            C.toast('Applied to editors: ' + applied.join(', '), 'success');
        } else {
            // Fallback: push the first code block to the HTML editor
            var anyCode = C.MD.extractCode(rawText);
            if (anyCode && LiveCSS.editor && LiveCSS.editor.getHtmlEditor) {
                LiveCSS.editor.getHtmlEditor().setValue(anyCode);
                if (LiveCSS.editor.updatePreview) { LiveCSS.editor.updatePreview(); }
                C.toast('Applied to HTML editor', 'success');
            } else {
                C.toast('No code blocks found in response.', 'error');
            }
        }
    }

    // -----------------------------------------------------------------------
    // Run command (lint/check)
    // -----------------------------------------------------------------------

    function runCommand() {
        if (!state.filePath) { C.toast('Load a file first.', 'error'); return; }
        dom.runCmdOutput.style.display = 'block';
        dom.runCmdOutput.textContent   = 'Running...';
        C.agentPost({ action: 'run_command', file_path: state.filePath })
            .then(function (data) {
                dom.runCmdOutput.textContent = data.output || '(no output)';
                C.setStatus(data.exit_code === 0 ? 'ok' : 'error', 'Exit ' + data.exit_code);
            }).catch(function (e) { dom.runCmdOutput.textContent = 'Error: ' + e.message; });
    }

    // -----------------------------------------------------------------------
    // Mode / source switching
    // -----------------------------------------------------------------------

    function switchMode(mode) {
        state.mode = mode;
        var modeInfo = C.AGENT_MODES[mode];
        if (!modeInfo) { return; }

        var tasks = modeInfo.tasks;
        dom.task.innerHTML = '';
        tasks.forEach(function (o) {
            var opt = document.createElement('option');
            opt.value = o.value;
            opt.textContent = o.label;
            dom.task.appendChild(opt);
        });
        state.task = tasks[0].value;

        var isNewProject = mode === 'new_project';
        dom.fuzzyRow.style.display = isNewProject ? 'flex' : 'none';
        dom.themeRow.style.display = isNewProject ? 'flex' : 'none';

        if (isNewProject && dom.themeSelect.children.length === 0) {
            loadThemeList();
        }

        C.setStatus('ok', 'Mode: ' + modeInfo.name);
    }

    function switchSource(src) {
        state.source = src;
        var isFile = (src === 'file');

        dom.fileRow.style.display    = isFile ? '' : 'none';
        dom.historyBar.style.display = (isFile && state.filePath) ? '' : 'none';

        dom.srcEditorsBtn.classList.toggle('agent-source-btn-active', src === 'editors');
        dom.srcFileBtn.classList.toggle('agent-source-btn-active',    src === 'file');

        if (src === 'editors') {
            loadOutlineFromEditors();
        } else {
            if (state.filePath && state.content) {
                loadOutline();
            } else {
                dom.outline.innerHTML = '<p class="outline-empty">Load a file to see outline</p>';
            }
        }
    }

    // -----------------------------------------------------------------------
    // Fuzzy / CSS Outline / Backup
    // -----------------------------------------------------------------------

    function runFuzzySearch() {
        var query = dom.fuzzyInput.value.trim();
        if (!query) { C.toast('Enter a search query.', 'error'); return; }
        C.setStatus('busy', 'Searching...');
        C.agentPost({ action: 'fuzzy_search', query: query, format: 'text' })
            .then(function (data) {
                if (data.error) { C.setStatus('error', data.error); return; }
                dom.runCmdOutput.style.display = 'block';
                dom.runCmdOutput.textContent = data.results || '(no results)';
                C.setStatus('ok', 'Search complete');
            }).catch(function (e) { C.setStatus('error', e.message); });
    }

    function runCSSOutline() {
        var themeFile = dom.themeSelect.value;
        if (!themeFile) { C.toast('Select a theme first.', 'error'); return; }
        C.setStatus('busy', 'Extracting outline...');
        C.agentPost({ action: 'css_outline', file_path: themeFile, format: 'text' })
            .then(function (data) {
                if (data.error) { C.setStatus('error', data.error); return; }
                dom.runCmdOutput.style.display = 'block';
                dom.runCmdOutput.textContent = data.outline || '(no outline)';
                C.setStatus('ok', 'Outline extracted');
            }).catch(function (e) { C.setStatus('error', e.message); });
    }

    function runBackup() {
        C.setStatus('busy', 'Creating backups...');
        C.agentPost({ action: 'backup', target: 'all' })
            .then(function (data) {
                if (data.error) { C.setStatus('error', data.error); return; }
                var backed = (data.backups || []).map(function (b) { return b.file + ' -> ' + b.backup; });
                C.toast('Backed up: ' + backed.join(', '), 'success');
                C.setStatus('ok', 'Backup complete (' + data.timestamp + ')');
            }).catch(function (e) { C.setStatus('error', e.message); });
    }

    // -----------------------------------------------------------------------
    // Register on shared core
    // -----------------------------------------------------------------------
    C.loadFile           = loadFile;
    C.setActiveCurrent   = setActiveCurrent;
    C.renderHistorySlots = renderHistorySlots;
    C.updateHistorySlots = updateHistorySlots;
    C.navigate           = navigate;
    C.loadOutline        = loadOutline;
    C.loadOutlineFromEditors = loadOutlineFromEditors;
    C.loadThemeList      = loadThemeList;
    C.runAgent           = runAgent;
    C.runPreviewGeneration = runPreviewGeneration;
    C.streamRun          = streamRun;
    C.streamRunDirect    = streamRunDirect;
    C.blockingRun        = blockingRun;
    C.abortStream        = abortStream;
    C.applyAIResult      = applyAIResult;
    C.applyToEditors     = applyToEditors;
    C.runCommand         = runCommand;
    C.switchMode         = switchMode;
    C.switchSource       = switchSource;
    C.runFuzzySearch     = runFuzzySearch;
    C.runCSSOutline      = runCSSOutline;
    C.runBackup          = runBackup;

}(window.LiveCSS = window.LiveCSS || {}));
