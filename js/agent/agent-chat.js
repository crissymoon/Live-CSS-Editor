/**
 * agent-chat.js -- Chat tab + response-area message helpers shared with agent-run.js.
 * Depends on: agent-core.js
 * Registers: C.appendUserMsg, C.appendAssistantPlaceholder, C.updateMsg, C.finalizeMsg,
 *            C.updateReasoning, C.showApplyBar, C.scrollResponse,
 *            C.populateChatProviders, C.populateChatModels, C.chatSend
 * Cross-calls: C.applyAIResult, C.switchTab (registered by agent-run.js / agent.js)
 */
'use strict';

(function (LiveCSS) {

    var C     = LiveCSS._agentCore;
    var state = C.state;
    var dom   = C.dom;

    // -----------------------------------------------------------------------
    // Chat provider / model selects
    // -----------------------------------------------------------------------

    function populateChatProviders() {
        dom.chatProvider.innerHTML = '';
        Object.keys(C.CHAT_PROVIDERS).forEach(function (slug) {
            var opt = document.createElement('option');
            opt.value = slug;
            opt.textContent = C.CHAT_PROVIDERS[slug].label;
            dom.chatProvider.appendChild(opt);
        });
        populateChatModels();
    }

    function populateChatModels() {
        var slug   = dom.chatProvider.value;
        var config = window.LiveCSSAIConfig && window.LiveCSSAIConfig[slug];
        dom.chatModel.innerHTML = '';
        var models = (config && config.models)        ? config.models        : [];
        var def    = (config && config.default_model) ? config.default_model : '';

        if (models.length === 0) {
            var opt = document.createElement('option');
            opt.value = def || 'default';
            opt.textContent = def || 'Default';
            dom.chatModel.appendChild(opt);
            return;
        }
        models.forEach(function (m) {
            var opt = document.createElement('option');
            opt.value = m;
            opt.textContent = m;
            if (m === def) { opt.selected = true; }
            dom.chatModel.appendChild(opt);
        });
    }

    // -----------------------------------------------------------------------
    // Chat send / stream
    // -----------------------------------------------------------------------

    function chatSend() {
        var text = dom.chatInput.value.trim();
        if (!text || state.chatStream) { return; }

        // User message
        var userWrap = C.el('div', 'agent-msg agent-msg-user');
        var userLbl  = C.el('div', 'agent-msg-label'); userLbl.textContent = 'You';
        var userBody = C.el('div', 'agent-msg-body');  userBody.innerHTML  = C.MD.toHtml(text);
        userWrap.appendChild(userLbl); userWrap.appendChild(userBody);
        dom.chatMessages.appendChild(userWrap);
        scrollChat();

        state.chatHistory.push({ role: 'user', content: text });
        dom.chatInput.value = '';
        dom.chatSend.disabled = true;
        dom.chatSend.textContent = 'Stop';

        // Assistant placeholder
        var aWrap = C.el('div', 'agent-msg');
        var aLbl  = C.el('div', 'agent-msg-label'); aLbl.textContent = C.CHAT_PROVIDERS[dom.chatProvider.value].label;
        var aBody = C.el('div', 'agent-msg-body');  aBody.innerHTML  = '<span class="ag-cursor"></span>';
        aWrap.appendChild(aLbl); aWrap.appendChild(aBody);
        dom.chatMessages.appendChild(aWrap);
        scrollChat();

        var accum    = '';
        var ctrl     = new AbortController();
        var endpoint = C.CHAT_PROVIDERS[dom.chatProvider.value].endpoint;

        state.chatStream = { close: function () { ctrl.abort(); } };

        fetch(endpoint, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                messages: state.chatHistory,
                model:    dom.chatModel.value || '',
                system:   C.CHAT_SYSTEM
            }),
            signal: ctrl.signal
        })
        .then(function (resp) {
            if (!resp.ok) { throw new Error('HTTP ' + resp.status); }
            var reader  = resp.body.getReader();
            var decoder = new TextDecoder();
            var buf     = '';

            function pump() {
                return reader.read().then(function (r) {
                    if (r.done) {
                        finalizeChatMsg(aBody, accum);
                        if (accum) { state.chatHistory.push({ role: 'assistant', content: accum }); }
                        chatStreamDone();
                        return;
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
                            aBody.innerHTML = C.MD.toHtml(accum) + '<span class="ag-cursor"></span>';
                            scrollChat();
                        } else if (evName === 'done') {
                            finalizeChatMsg(aBody, accum);
                            if (accum) { state.chatHistory.push({ role: 'assistant', content: accum }); }
                            chatStreamDone();
                        } else if (evName === 'error') {
                            finalizeChatMsg(aBody, accum || '_Error_');
                            chatStreamDone();
                        }
                    });
                    return pump();
                });
            }
            return pump();
        })
        .catch(function (err) {
            if (err.name === 'AbortError') { chatStreamDone(); return; }
            finalizeChatMsg(aBody, accum || '_Request failed: ' + err.message + '_');
            chatStreamDone();
        });
    }

    function finalizeChatMsg(bodyEl, text) {
        bodyEl.innerHTML = text ? C.MD.toHtml(text) : '<em style="color:var(--ag-text-muted)">Empty response</em>';
        scrollChat();
    }

    function chatStreamDone() {
        state.chatStream = null;
        dom.chatSend.disabled = false;
        dom.chatSend.textContent = 'Send';
    }

    function scrollChat() {
        dom.chatMessages.scrollTop = dom.chatMessages.scrollHeight;
    }

    // -----------------------------------------------------------------------
    // Response-area helpers (used by agent-run.js via C.*)
    // -----------------------------------------------------------------------

    function appendUserMsg(text) {
        var wrap = C.el('div', 'agent-msg agent-msg-user');
        var lbl  = C.el('div', 'agent-msg-label'); lbl.textContent = 'You';
        var body = C.el('div', 'agent-msg-body');
        body.innerHTML = C.MD.toHtml(text);
        wrap.appendChild(lbl); wrap.appendChild(body);
        dom.responseArea.appendChild(wrap);
        state.conversation.push({ role: 'user', content: text });
        scrollResponse();
        return body;
    }

    function appendAssistantPlaceholder() {
        var wrap = C.el('div', 'agent-msg');
        var lbl  = C.el('div', 'agent-msg-label');
        lbl.textContent = state.providers[state.provider]
            ? state.providers[state.provider].name : state.provider;
        var body = C.el('div', 'agent-msg-body');
        body.innerHTML = '<span class="ag-cursor"></span>';
        wrap.appendChild(lbl); wrap.appendChild(body);
        dom.responseArea.appendChild(wrap);
        scrollResponse();
        return body;
    }

    function updateMsg(bodyEl, text) {
        // During preview streaming, suppress the code wall -- neural overlay is visible
        if (state.mode === 'new_project' && state.task === 'preview') {
            bodyEl.innerHTML = '<span class="ag-preview-streaming">Building preview<span class="ag-cursor"></span></span>';
            scrollResponse();
            return;
        }
        bodyEl.innerHTML = C.MD.toHtml(text) + '<span class="ag-cursor"></span>';
        scrollResponse();
    }

    function finalizeMsg(bodyEl, text) {
        // For preview, show a compact summary instead of the raw code
        if (state.mode === 'new_project' && state.task === 'preview') {
            var lines = text ? (text.match(/\n/g) || []).length + 1 : 0;
            if (lines > 0) {
                bodyEl.innerHTML = '<span style="color:var(--ag-text-muted);font-size:12px;">Preview ready &mdash; '
                    + lines + ' lines generated. Click <strong>Apply Preview to Editors</strong> below.</span>';
            } else {
                bodyEl.innerHTML = '<em style="color:var(--ag-text-muted)">Empty preview response</em>';
            }
            scrollResponse();
            return;
        }
        bodyEl.innerHTML = text ? C.MD.toHtml(text) : '<em style="color:var(--ag-text-muted)">Empty response</em>';
        scrollResponse();
    }

    function updateReasoning(bodyEl, text) {
        var existing = bodyEl.previousElementSibling;
        if (existing && existing.classList.contains('agent-reasoning-block')) {
            existing.querySelector('.agent-reasoning-body-inner').textContent += text;
        } else {
            var block = C.el('div', 'agent-reasoning-block');
            block.innerHTML = '<div class="agent-reasoning-label">Reasoning</div><pre class="agent-reasoning-body-inner" style="margin:0;white-space:pre-wrap;"></pre>';
            block.querySelector('.agent-reasoning-body-inner').textContent = text;
            bodyEl.parentNode.insertBefore(block, bodyEl);
        }
        scrollResponse();
    }

    function showApplyBar(bodyEl, rawText) {
        var bar = C.el('div', 'agent-apply-bar');

        // Preview task always targets the live editors (not a versioned file)
        var isPreview = (state.mode === 'new_project' && state.task === 'preview');

        if (state.source === 'editors' || isPreview) {
            // Source is the live editors (or preview result) -- push code into editors
            var applyEditorsBtn = C.el('button', 'agent-btn agent-btn-primary');
            applyEditorsBtn.textContent = isPreview ? 'Apply Preview to Editors' : 'Apply to Editors';
            applyEditorsBtn.addEventListener('click', function () { C.applyToEditors(rawText); bar.remove(); });
            bar.appendChild(applyEditorsBtn);
        } else {
            // Source is a loaded file -- save as a version
            var applyBtn = C.el('button', 'agent-btn agent-btn-primary');
            applyBtn.textContent = 'Apply to Version';
            applyBtn.addEventListener('click', function () { C.applyAIResult(rawText); bar.remove(); });

            var diffBtn = C.el('button', 'agent-btn agent-btn-ghost');
            diffBtn.textContent = 'Preview Diff';
            diffBtn.addEventListener('click', function () {
                var code = C.MD.extractCode(rawText);
                C.agentPost({ action: 'diff', file_path: state.filePath, old_text: state.content, new_text: code })
                    .then(function (data) {
                        dom.diffTable.innerHTML = data.html || '';
                        var sum = data.summary || {};
                        dom.diffSummary.style.display = 'flex';
                        dom.diffSummary.innerHTML = '<span class="diff-added">+' + sum.added + '</span><span class="diff-removed">-' + sum.removed + '</span>';
                        C.switchTab('diff');
                    });
            });

            bar.appendChild(applyBtn);
            bar.appendChild(diffBtn);
        }

        // Always offer a dismiss
        var dismissBtn = C.el('button', 'agent-btn agent-btn-ghost');
        dismissBtn.textContent = 'Dismiss';
        dismissBtn.addEventListener('click', function () { bar.remove(); });
        bar.appendChild(dismissBtn);

        bodyEl.parentNode.appendChild(bar);
        scrollResponse();
    }

    function scrollResponse() {
        dom.responseArea.scrollTop = dom.responseArea.scrollHeight;
    }

    // Register
    C.populateChatProviders      = populateChatProviders;
    C.populateChatModels         = populateChatModels;
    C.chatSend                   = chatSend;
    C.appendUserMsg              = appendUserMsg;
    C.appendAssistantPlaceholder = appendAssistantPlaceholder;
    C.updateMsg                  = updateMsg;
    C.finalizeMsg                = finalizeMsg;
    C.updateReasoning            = updateReasoning;
    C.showApplyBar               = showApplyBar;
    C.scrollResponse             = scrollResponse;

}(window.LiveCSS = window.LiveCSS || {}));
