/**
 * AI Chat Module -- LiveCSS.aiChat
 * Manages conversation history, provider selection, streaming SSE responses,
 * and all UI interactions for the AI panel.
 * Depends on: ai/markdown/converter exposed via /ai/markdown-render.php (optional).
 * All rendering is done client-side using a small inline markdown parser.
 */
'use strict';

(function (LiveCSS) {

    // -----------------------------------------------------------------------
    // Config
    // -----------------------------------------------------------------------

    var PROVIDERS = {
        anthropic: { label: 'Anthropic',  endpoint: 'ai/anthropic.php' },
        openai:    { label: 'OpenAI',      endpoint: 'ai/openai.php'    },
        deepseek:  { label: 'Deepseek',    endpoint: 'ai/deepseek.php'  }
    };

    var DEFAULT_PROVIDER = 'anthropic';

    // Base system instruction -- temporal context is appended at send time.
    var SYSTEM_BASE = 'You are a helpful CSS and web development assistant embedded in a live CSS editor. When returning code, always use fenced code blocks with the correct language identifier.';

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    var state = {
        provider:     DEFAULT_PROVIDER,
        model:        '',
        conversations: {},   // keyed by provider: [{role, content}, ...]
        activeStream: null,  // current EventSource or null
        thinking:     false
    };

    function getHistory() {
        var p = state.provider;
        if (!state.conversations[p]) {
            state.conversations[p] = [];
        }
        return state.conversations[p];
    }

    function clearHistory() {
        state.conversations[state.provider] = [];
    }

    // -----------------------------------------------------------------------
    // Temporal and locale context
    // These functions are called at the moment a message is sent so the AI
    // always receives the current real-world moment -- not a stale value.
    // -----------------------------------------------------------------------

    var MONTHS   = ['January','February','March','April','May','June','July','August','September','October','November','December'];
    var WEEKDAYS = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];

    /**
     * Return the user IANA timezone string from the Intl API.
     */
    function resolveTimezone() {
        try {
            return Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC';
        } catch (e) {
            return 'UTC';
        }
    }

    /**
     * Map a BCP 47 language code and region code to a broad cultural area label.
     * Mirrors the server-side logic in ai/context-time.php.
     */
    function resolveLocaleRegion(lang, region) {
        var eastAsian  = ['zh', 'ja', 'ko'];
        var southAsian = ['hi', 'ur', 'bn', 'ta', 'te', 'mr', 'gu', 'kn', 'ml', 'pa'];
        var rtlLangs   = ['ar', 'he', 'fa', 'ps', 'dv', 'yi'];
        var nordic     = ['sv', 'no', 'nb', 'nn', 'da', 'fi', 'is', 'et', 'lv', 'lt'];
        var slavic     = ['ru', 'pl', 'cs', 'sk', 'uk', 'bg', 'sr', 'hr', 'sl', 'mk', 'bs'];
        var latin      = ['es', 'pt', 'fr', 'it', 'ro', 'ca', 'gl'];
        var germanic   = ['de', 'nl', 'af', 'lb'];
        var latamCarib = ['MX','GT','BZ','HN','SV','NI','CR','PA','CU','DO','PR','JM','TT'];
        var latamSouth = ['AR','BO','BR','CL','CO','EC','PE','PY','UY','VE'];

        if (eastAsian.indexOf(lang)  !== -1) { return 'East Asia'; }
        if (southAsian.indexOf(lang) !== -1) { return 'South Asia'; }
        if (rtlLangs.indexOf(lang)   !== -1) { return 'Middle East / North Africa'; }
        if (nordic.indexOf(lang)     !== -1) { return 'Northern Europe'; }
        if (slavic.indexOf(lang)     !== -1) { return 'Eastern Europe'; }
        if (germanic.indexOf(lang)   !== -1) { return 'Western Europe / Germanic'; }
        if (latin.indexOf(lang) !== -1) {
            if (latamCarib.indexOf(region) !== -1) { return 'Latin America / Caribbean'; }
            if (latamSouth.indexOf(region) !== -1) { return 'Latin America / South'; }
            return 'Western Europe / Latin';
        }
        if (region === 'AU' || region === 'NZ') { return 'Australasia'; }
        if (region === 'IN') { return 'South Asia'; }
        return 'North America / English';
    }

    /**
     * Return the current time-of-day band based on local hour.
     */
    function resolveTimeOfDay(hour) {
        if (hour >= 5  && hour < 12) { return 'Morning'; }
        if (hour >= 12 && hour < 17) { return 'Afternoon'; }
        if (hour >= 17 && hour < 21) { return 'Evening'; }
        return 'Night';
    }

    /**
     * Build a multi-line temporal and locale context string to append to the
     * system prompt.  Called at send time so the data is always current.
     */
    function buildTemporalContext() {
        var now       = new Date();
        var locale    = navigator.language || 'en-US';
        var rawParts  = locale.split('-');
        var lang      = rawParts[0].toLowerCase();
        var region    = (rawParts[1] || '').toUpperCase();
        var tz        = resolveTimezone();
        var hour      = now.getHours();
        var min       = now.getMinutes();
        var padMin    = min < 10 ? '0' + min : String(min);
        var timeStr   = hour + ':' + padMin;
        var tod       = resolveTimeOfDay(hour);
        var cultural  = resolveLocaleRegion(lang, region);
        var dateStr   = WEEKDAYS[now.getDay()] + ', ' + MONTHS[now.getMonth()] + ' ' + now.getDate() + ', ' + now.getFullYear();

        return [
            '',
            '--- Context (injected at send time, do not repeat this block to the user) ---',
            'Current date: ' + dateStr,
            'Current time: ' + timeStr + ' (' + tod + ')',
            'Timezone: ' + tz,
            'User locale: ' + locale,
            'Cultural region: ' + cultural,
            '--- End context ---',
        ].join('\n');
    }

    /**
     * Compose the full system prompt by appending the current temporal context
     * to the base instruction string.
     */
    function buildSystemPrompt() {
        return SYSTEM_BASE + buildTemporalContext();
    }

    // -----------------------------------------------------------------------
    // Markdown (inline client-side parser)
    // -----------------------------------------------------------------------

    var MD = (function () {

        function escape(text) {
            return text
                .replace(/&/g, '&amp;')
                .replace(/</g, '&lt;')
                .replace(/>/g, '&gt;')
                .replace(/"/g, '&quot;');
        }

        function inline(text) {
            text = escape(text);
            // Inline code
            text = text.replace(/`([^`]+)`/g, '<code>$1</code>');
            // Bold
            text = text.replace(/\*\*(.+?)\*\*/g, '<strong>$1</strong>');
            text = text.replace(/__(.+?)__/g, '<strong>$1</strong>');
            // Italic
            text = text.replace(/\*(.+?)\*/g, '<em>$1</em>');
            text = text.replace(/_(.+?)_/g, '<em>$1</em>');
            // Strikethrough
            text = text.replace(/~~(.+?)~~/g, '<del>$1</del>');
            // Links
            text = text.replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2" target="_blank" rel="noopener noreferrer">$1</a>');
            return text;
        }

        function toHtml(markdown) {
            var lines  = markdown.split('\n');
            var output = [];
            var i      = 0;
            var total  = lines.length;

            while (i < total) {
                var line = lines[i];

                // Fenced code block
                var fenceMatch = line.match(/^```(\w*)/);
                if (fenceMatch) {
                    var lang = fenceMatch[1] ? escape(fenceMatch[1]) : '';
                    var code = [];
                    i++;
                    while (i < total && !lines[i].match(/^```/)) {
                        code.push(escape(lines[i]));
                        i++;
                    }
                    var langAttr = lang ? ' class="language-' + lang + '"' : '';
                    output.push('<pre><code' + langAttr + '>' + code.join('\n') + '</code></pre>');
                    i++;
                    continue;
                }

                // Heading
                var hMatch = line.match(/^(#{1,6})\s+(.+)/);
                if (hMatch) {
                    var level = hMatch[1].length;
                    output.push('<h' + level + '>' + inline(hMatch[2]) + '</h' + level + '>');
                    i++;
                    continue;
                }

                // Horizontal rule
                if (/^[-*_]{3,}\s*$/.test(line)) {
                    output.push('<hr>');
                    i++;
                    continue;
                }

                // Blockquote
                var bqMatch = line.match(/^>\s?(.*)/);
                if (bqMatch) {
                    var bqLines = [bqMatch[1]];
                    i++;
                    var nextBq;
                    while (i < total && (nextBq = lines[i].match(/^>\s?(.*)/))) {
                        bqLines.push(nextBq[1]);
                        i++;
                    }
                    output.push('<blockquote>' + toHtml(bqLines.join('\n')) + '</blockquote>');
                    continue;
                }

                // Unordered list
                var ulMatch = line.match(/^[-*+]\s+(.+)/);
                if (ulMatch) {
                    var ulItems = [inline(ulMatch[1])];
                    i++;
                    var nextUl;
                    while (i < total && (nextUl = lines[i].match(/^[-*+]\s+(.+)/))) {
                        ulItems.push(inline(nextUl[1]));
                        i++;
                    }
                    output.push('<ul>' + ulItems.map(function (t) { return '<li>' + t + '</li>'; }).join('') + '</ul>');
                    continue;
                }

                // Ordered list
                var olMatch = line.match(/^\d+\.\s+(.+)/);
                if (olMatch) {
                    var olItems = [inline(olMatch[1])];
                    i++;
                    var nextOl;
                    while (i < total && (nextOl = lines[i].match(/^\d+\.\s+(.+)/))) {
                        olItems.push(inline(nextOl[1]));
                        i++;
                    }
                    output.push('<ol>' + olItems.map(function (t) { return '<li>' + t + '</li>'; }).join('') + '</ol>');
                    continue;
                }

                // Empty line
                if (line.trim() === '') {
                    i++;
                    continue;
                }

                // Paragraph
                var pLines = [inline(line)];
                i++;
                while (i < total) {
                    var next = lines[i];
                    if (next.trim() === '' || /^(#{1,6}\s|[-*+]\s|\d+\.\s|>|```|[-*_]{3})/.test(next)) {
                        break;
                    }
                    pLines.push(inline(next));
                    i++;
                }
                output.push('<p>' + pLines.join(' ') + '</p>');
            }
            return output.join('\n');
        }

        return { toHtml: toHtml };
    }());

    // -----------------------------------------------------------------------
    // DOM
    // -----------------------------------------------------------------------

    var dom = {};

    function buildPanel() {
        var panel = document.createElement('div');
        panel.id        = 'aiChatPanel';
        panel.className = 'ai-chat-panel';
        panel.innerHTML = [
            '<div class="ai-chat-header">',
            '  <span class="ai-chat-title">AI Assistant</span>',
            '  <div class="ai-chat-controls">',
            '    <select id="aiProviderSelect" class="ai-select" title="Provider"></select>',
            '    <select id="aiModelSelect"    class="ai-select" title="Model"></select>',
            '    <button id="aiClearBtn"       class="ai-btn ai-btn-ghost" title="Clear conversation">Clear</button>',
            '    <button id="aiCloseBtn"       class="ai-btn ai-btn-ghost ai-close-btn" title="Close">&#215;</button>',
            '  </div>',
            '</div>',
            '<div class="ai-chat-messages" id="aiMessages"></div>',
            '<div class="ai-chat-status"   id="aiStatus"></div>',
            '<div class="ai-chat-input-row">',
            '  <textarea id="aiInput" class="ai-input" placeholder="Ask anything about CSS or code..." rows="3" spellcheck="false"></textarea>',
            '  <button id="aiSendBtn" class="ai-btn ai-btn-primary">Send</button>',
            '</div>'
        ].join('');

        document.body.appendChild(panel);

        dom.panel     = panel;
        dom.messages  = panel.querySelector('#aiMessages');
        dom.input     = panel.querySelector('#aiInput');
        dom.sendBtn   = panel.querySelector('#aiSendBtn');
        dom.clearBtn  = panel.querySelector('#aiClearBtn');
        dom.closeBtn  = panel.querySelector('#aiCloseBtn');
        dom.status    = panel.querySelector('#aiStatus');
        dom.pSelect   = panel.querySelector('#aiProviderSelect');
        dom.mSelect   = panel.querySelector('#aiModelSelect');

        // Trigger button (floating)
        var trigger = document.createElement('button');
        trigger.id        = 'aiTriggerBtn';
        trigger.className = 'ai-trigger-btn';
        trigger.title     = 'Open AI Assistant';
        trigger.textContent = 'AI';
        document.body.appendChild(trigger);
        dom.trigger = trigger;
    }

    function populateProviders() {
        dom.pSelect.innerHTML = '';
        Object.keys(PROVIDERS).forEach(function (slug) {
            var opt       = document.createElement('option');
            opt.value     = slug;
            opt.textContent = PROVIDERS[slug].label;
            if (slug === state.provider) { opt.selected = true; }
            dom.pSelect.appendChild(opt);
        });
        populateModels();
    }

    function populateModels() {
        var config = window.LiveCSSAIConfig && window.LiveCSSAIConfig[state.provider];
        dom.mSelect.innerHTML = '';
        var models = (config && config.models) ? config.models : [];
        var def    = (config && config.default_model) ? config.default_model : '';

        if (models.length === 0) {
            var opt = document.createElement('option');
            opt.value = def || 'default';
            opt.textContent = def || 'Default';
            dom.mSelect.appendChild(opt);
            state.model = def;
            return;
        }

        models.forEach(function (m) {
            var opt       = document.createElement('option');
            opt.value     = m;
            opt.textContent = m;
            if (m === def) { opt.selected = true; }
            dom.mSelect.appendChild(opt);
        });
        state.model = def || models[0];
    }

    // -----------------------------------------------------------------------
    // Message rendering
    // -----------------------------------------------------------------------

    function appendMessage(role, htmlContent, opts) {
        opts = opts || {};
        var wrap = document.createElement('div');
        wrap.className = 'ai-message ai-message-' + role;

        var label = document.createElement('div');
        label.className   = 'ai-message-label';
        label.textContent = role === 'user' ? 'You' : PROVIDERS[state.provider].label;

        var body = document.createElement('div');
        body.className = 'ai-message-body';
        if (opts.streaming) {
            body.setAttribute('data-streaming', 'true');
        }
        body.innerHTML = htmlContent;

        wrap.appendChild(label);
        wrap.appendChild(body);
        dom.messages.appendChild(wrap);
        scrollToBottom();
        return body;
    }

    function appendStreamingPlaceholder() {
        return appendMessage('assistant', '<span class="ai-cursor"></span>', { streaming: true });
    }

    function updateStreamingBody(bodyEl, accumulatedText) {
        bodyEl.innerHTML = MD.toHtml(accumulatedText) + '<span class="ai-cursor"></span>';
        scrollToBottom();
    }

    function finalizeStreamingBody(bodyEl, fullText) {
        bodyEl.removeAttribute('data-streaming');
        bodyEl.innerHTML = MD.toHtml(fullText);
        scrollToBottom();
    }

    function appendReasoningBlock(bodyEl, text) {
        var existing = bodyEl.previousElementSibling;
        if (existing && existing.classList.contains('ai-reasoning')) {
            existing.querySelector('.ai-reasoning-body').textContent += text;
        } else {
            var block = document.createElement('div');
            block.className = 'ai-reasoning';
            block.innerHTML = '<div class="ai-reasoning-label">Reasoning</div><div class="ai-reasoning-body">' + text + '</div>';
            bodyEl.parentNode.insertBefore(block, bodyEl);
        }
        scrollToBottom();
    }

    function scrollToBottom() {
        dom.messages.scrollTop = dom.messages.scrollHeight;
    }

    // -----------------------------------------------------------------------
    // Status / thinking animation
    // -----------------------------------------------------------------------

    var statusInterval = null;
    var statusFrames   = ['Working   ', 'Working.  ', 'Working.. ', 'Working...'];
    var statusIdx      = 0;

    function startStatus() {
        state.thinking   = true;
        statusIdx        = 0;
        dom.status.classList.add('ai-status-active');
        dom.status.textContent = statusFrames[0];
        statusInterval = setInterval(function () {
            statusIdx = (statusIdx + 1) % statusFrames.length;
            dom.status.textContent = statusFrames[statusIdx];
        }, 400);
        dom.sendBtn.disabled = true;
        dom.sendBtn.textContent = 'Stop';
    }

    function stopStatus() {
        state.thinking = false;
        clearInterval(statusInterval);
        statusInterval = null;
        dom.status.classList.remove('ai-status-active');
        dom.status.textContent = '';
        dom.sendBtn.disabled    = false;
        dom.sendBtn.textContent = 'Send';
    }

    // -----------------------------------------------------------------------
    // Streaming
    // -----------------------------------------------------------------------

    function abortStream() {
        if (state.activeStream) {
            state.activeStream.close();
            state.activeStream = null;
        }
        stopStatus();
    }

    function sendMessage(userText) {
        userText = userText.trim();
        if (!userText) { return; }

        // Abort any in-flight stream
        abortStream();

        // Add to history and render
        var history = getHistory();
        history.push({ role: 'user', content: userText });
        appendMessage('user', MD.toHtml(userText));

        // Clear input
        dom.input.value = '';

        startStatus();

        var bodyEl       = appendStreamingPlaceholder();
        var accumulated  = '';
        var reasoningText = '';
        var providerSlug = state.provider;
        var endpoint     = PROVIDERS[providerSlug].endpoint;

        // SSE streams require a GET or use fetch + ReadableStream.
        // We post via fetch (streaming body) since EventSource only supports GET.
        var payload = JSON.stringify({
            messages:  history,
            model:     state.model || '',
            system:    buildSystemPrompt()
        });

        var controller = new AbortController();
        state.activeStream = { close: function () { controller.abort(); } };

        fetch(endpoint, {
            method:  'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-Timezone':   resolveTimezone()
            },
            body:    payload,
            signal:  controller.signal
        })
        .then(function (response) {
            if (!response.ok) {
                throw new Error('HTTP ' + response.status);
            }
            var reader  = response.body.getReader();
            var decoder = new TextDecoder();
            var buffer  = '';

            function pump() {
                return reader.read().then(function (result) {
                    if (result.done) {
                        // Finalize even if done event was missing
                        if (accumulated) {
                            finalizeStreamingBody(bodyEl, accumulated);
                            history.push({ role: 'assistant', content: accumulated });
                        }
                        stopStatus();
                        state.activeStream = null;
                        return;
                    }
                    buffer += decoder.decode(result.value, { stream: true });
                    var parts = buffer.split('\n\n');
                    buffer    = parts.pop(); // Keep incomplete trailing chunk

                    parts.forEach(function (block) {
                        var eventName = '';
                        var dataStr   = '';
                        block.split('\n').forEach(function (line) {
                            if (line.startsWith('event: ')) {
                                eventName = line.slice(7).trim();
                            } else if (line.startsWith('data: ')) {
                                dataStr = line.slice(6).trim();
                            }
                        });
                        if (!dataStr) { return; }

                        var data;
                        try { data = JSON.parse(dataStr); } catch (e) { return; }

                        if (eventName === 'chunk') {
                            accumulated += data.text || '';
                            updateStreamingBody(bodyEl, accumulated);
                        } else if (eventName === 'reasoning') {
                            reasoningText += data.text || '';
                            appendReasoningBlock(bodyEl, data.text || '');
                        } else if (eventName === 'done') {
                            finalizeStreamingBody(bodyEl, accumulated);
                            history.push({ role: 'assistant', content: accumulated });
                            stopStatus();
                            state.activeStream = null;
                        } else if (eventName === 'error') {
                            showError(data.error || 'Unknown error');
                            finalizeStreamingBody(bodyEl, accumulated || '_Error receiving response._');
                            stopStatus();
                            state.activeStream = null;
                        }
                    });

                    return pump();
                });
            }

            return pump();
        })
        .catch(function (err) {
            if (err.name === 'AbortError') { return; }
            showError('Request failed: ' + err.message);
            finalizeStreamingBody(bodyEl, accumulated || '_No response received._');
            stopStatus();
            state.activeStream = null;
        });
    }

    function showError(msg) {
        var el       = document.createElement('div');
        el.className = 'ai-error-toast';
        el.textContent = msg;
        document.body.appendChild(el);
        setTimeout(function () { el.classList.add('ai-error-visible'); }, 10);
        setTimeout(function () {
            el.classList.remove('ai-error-visible');
            setTimeout(function () { el.parentNode && el.parentNode.removeChild(el); }, 300);
        }, 4000);
    }

    // -----------------------------------------------------------------------
    // Panel visibility
    // -----------------------------------------------------------------------

    function openPanel() {
        dom.panel.classList.add('ai-panel-open');
        dom.trigger.classList.add('ai-trigger-active');
        dom.input.focus();
    }

    function closePanel() {
        dom.panel.classList.remove('ai-panel-open');
        dom.trigger.classList.remove('ai-trigger-active');
    }

    function togglePanel() {
        if (dom.panel.classList.contains('ai-panel-open')) {
            closePanel();
        } else {
            openPanel();
        }
    }

    // -----------------------------------------------------------------------
    // Event wiring
    // -----------------------------------------------------------------------

    function bindEvents() {
        dom.trigger.addEventListener('click', togglePanel);
        dom.closeBtn.addEventListener('click', closePanel);

        dom.clearBtn.addEventListener('click', function () {
            clearHistory();
            dom.messages.innerHTML = '';
            abortStream();
        });

        dom.pSelect.addEventListener('change', function () {
            state.provider = dom.pSelect.value;
            populateModels();
        });

        dom.mSelect.addEventListener('change', function () {
            state.model = dom.mSelect.value;
        });

        dom.sendBtn.addEventListener('click', function () {
            if (state.thinking) {
                abortStream();
            } else {
                sendMessage(dom.input.value);
            }
        });

        dom.input.addEventListener('keydown', function (e) {
            // Ctrl+Enter or Cmd+Enter sends; Enter alone is a newline
            if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
                e.preventDefault();
                if (state.thinking) {
                    abortStream();
                } else {
                    sendMessage(dom.input.value);
                }
            }
        });
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    var aiChat = {

        /**
         * Initialise the AI chat panel and inject it into the page.
         * Call once after DOM ready. Optional providerConfig comes from PHP
         * injected inline as window.LiveCSSAIConfig = { anthropic: { models:[], ... } }.
         */
        init: function () {
            buildPanel();
            populateProviders();
            bindEvents();
        },

        open:  openPanel,
        close: closePanel,

        send: function (text) {
            openPanel();
            sendMessage(text);
        },

        clearHistory: clearHistory,

        setProvider: function (slug) {
            if (!PROVIDERS[slug]) { return; }
            state.provider = slug;
            dom.pSelect.value = slug;
            populateModels();
        }
    };

    LiveCSS.aiChat = aiChat;

}(window.LiveCSS = window.LiveCSS || {}));
