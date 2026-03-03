/**
 * page-builder/sections/chatbot/chatbot-widget.js
 *
 * Self-contained chatbot widget that mounts into any element carrying the
 * class  pb-chatbot-wrap.  Config is read from data-* attributes on the
 * container element.
 *
 * Data attributes:
 *   data-flow-id      -- agent-flow flow name (default: "chatbot-company-context")
 *   data-title        -- widget header label   (default: "Chat")
 *   data-placeholder  -- input placeholder     (default: "Type a message...")
 *   data-height       -- messages area height  (default: "400px")
 *   data-api-url      -- chat API endpoint     (default: "/page-builder/sections/chatbot/api/chat.php")
 *   data-theme        -- "dark" | "light"       (default: "dark")
 *   data-accent       -- accent hex color       (default: "#6366f1")
 *   data-show-avatar  -- "true" | "false"       (default: "true")
 *   data-bot-name     -- bot display name       (default: "Assistant")
 *
 * No external dependencies. IE11 not supported.
 */

(function () {
  'use strict';

  var DEFAULTS = {
    flowId:      'chatbot-company-context',
    title:       'Chat',
    placeholder: 'Type a message...',
    height:      '400px',
    apiUrl:      '/page-builder/sections/chatbot/api/chat.php',
    theme:       'dark',
    accent:      '#6366f1',
    showAvatar:  true,
    botName:     'Assistant',
  };

  // ---- CSS injection (runs once) -------------------------------------------

  var CSS_INJECTED = false;

  function injectCSS() {
    if (CSS_INJECTED) return;
    CSS_INJECTED = true;

    // Prefer external stylesheet if already on page (added by renderBlock)
    var existing = document.querySelector('link[href*="chatbot-widget.css"]');
    if (existing) return;

    // Try to load relative to this script's location
    var scripts = document.querySelectorAll('script[src*="chatbot-widget.js"]');
    if (scripts.length > 0) {
      var src  = scripts[scripts.length - 1].src;
      var base = src.substring(0, src.lastIndexOf('/') + 1);
      var link = document.createElement('link');
      link.rel  = 'stylesheet';
      link.href = base + 'chatbot-widget.css';
      document.head.appendChild(link);
      return;
    }

    console.warn('[chatbot-widget] Could not resolve stylesheet path -- widget may be unstyled.');
  }

  // ---- Helpers ---------------------------------------------------------------

  function attr(el, name, fallback) {
    var v = el.getAttribute('data-' + name);
    if (v === null || v === '') return fallback;
    return v;
  }

  function boolAttr(el, name, fallback) {
    var v = el.getAttribute('data-' + name);
    if (v === null) return fallback;
    return v !== 'false' && v !== '0';
  }

  /**
   * Minimal safe HTML renderer: escapes all user text, then converts
   * newlines to <br> and wraps bare URLs in clickable links.
   */
  function safeHtml(text) {
    if (typeof text !== 'string') return '';
    // Escape HTML entities
    var escaped = text
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
    // Linkify URLs (http/https only)
    escaped = escaped.replace(
      /(https?:\/\/[^\s<>"'`()]+)/g,
      '<a href="$1" target="_blank" rel="noopener noreferrer" class="cw-link">$1</a>'
    );
    // Newlines to <br>
    escaped = escaped.replace(/\n/g, '<br>');
    return escaped;
  }

  function scrollToBottom(el) {
    try {
      el.scrollTop = el.scrollHeight;
    } catch (e) {
      console.error('[chatbot-widget] scrollToBottom failed:', e);
    }
  }

  function timestamp() {
    var d = new Date();
    var h = String(d.getHours()).padStart(2, '0');
    var m = String(d.getMinutes()).padStart(2, '0');
    return h + ':' + m;
  }

  // ---- Widget factory --------------------------------------------------------

  function mountWidget(container) {
    try {
      var cfg = {
        flowId:     attr(container, 'flow-id',     DEFAULTS.flowId),
        title:      attr(container, 'title',        DEFAULTS.title),
        placeholder:attr(container, 'placeholder',  DEFAULTS.placeholder),
        height:     attr(container, 'height',       DEFAULTS.height),
        apiUrl:     attr(container, 'api-url',      DEFAULTS.apiUrl),
        theme:      attr(container, 'theme',        DEFAULTS.theme),
        accent:     attr(container, 'accent',       DEFAULTS.accent),
        showAvatar: boolAttr(container, 'show-avatar', DEFAULTS.showAvatar),
        botName:    attr(container, 'bot-name',     DEFAULTS.botName),
      };

      console.log('[chatbot-widget] mounting widget flow_id=' + cfg.flowId + ' api=' + cfg.apiUrl);

      var history  = [];   // [{role, content}]
      var sending  = false;

      // ---- DOM build ---------------------------------------------------------

      container.setAttribute('data-cw-theme', cfg.theme);
      container.style.setProperty('--cw-accent', cfg.accent);
      container.classList.add('cw-root');

      // Header
      var header = document.createElement('div');
      header.className = 'cw-header';

      var headerTitle = document.createElement('span');
      headerTitle.className = 'cw-header-title';
      headerTitle.textContent = cfg.title;

      var headerStatus = document.createElement('span');
      headerStatus.className = 'cw-header-status';
      headerStatus.textContent = 'online';

      header.appendChild(headerTitle);
      header.appendChild(headerStatus);

      // Messages area
      var messages = document.createElement('div');
      messages.className = 'cw-messages';
      messages.style.height = cfg.height;
      messages.setAttribute('aria-live', 'polite');
      messages.setAttribute('aria-label', 'Conversation');
      messages.setAttribute('role', 'log');

      // Input row
      var inputRow = document.createElement('div');
      inputRow.className = 'cw-input-row';

      var input = document.createElement('textarea');
      input.className   = 'cw-input';
      input.placeholder = cfg.placeholder;
      input.rows        = 1;
      input.setAttribute('aria-label', 'Message input');
      input.setAttribute('autocomplete', 'off');
      input.setAttribute('spellcheck', 'true');

      var sendBtn = document.createElement('button');
      sendBtn.className = 'cw-send-btn';
      sendBtn.type      = 'button';
      sendBtn.setAttribute('aria-label', 'Send message');
      sendBtn.innerHTML =
        '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><line x1="22" y1="2" x2="11" y2="13"/><polygon points="22 2 15 22 11 13 2 9 22 2"/></svg>';

      inputRow.appendChild(input);
      inputRow.appendChild(sendBtn);

      // Footer note
      var footer = document.createElement('div');
      footer.className = 'cw-footer';
      footer.textContent = 'AI can make mistakes. Verify important information.';

      container.appendChild(header);
      container.appendChild(messages);
      container.appendChild(inputRow);
      container.appendChild(footer);

      // ---- Welcome message ---------------------------------------------------

      appendMessage('assistant', 'Hi! I\'m ' + cfg.botName + '. How can I help you today?');

      // ---- Message rendering -------------------------------------------------

      function appendMessage(role, text, opts) {
        opts = opts || {};
        var wrap = document.createElement('div');
        wrap.className = 'cw-msg cw-msg-' + role;

        if (cfg.showAvatar) {
          var avatar = document.createElement('div');
          avatar.className = 'cw-avatar cw-avatar-' + role;
          avatar.textContent = role === 'assistant' ? cfg.botName.charAt(0).toUpperCase() : 'Y';
          avatar.setAttribute('aria-hidden', 'true');
          wrap.appendChild(avatar);
        }

        var bubble = document.createElement('div');
        bubble.className = 'cw-bubble';

        if (opts.isError) {
          bubble.classList.add('cw-bubble-error');
        }

        bubble.innerHTML = safeHtml(text);

        var ts = document.createElement('span');
        ts.className   = 'cw-ts';
        ts.textContent = timestamp();
        ts.setAttribute('aria-hidden', 'true');

        var inner = document.createElement('div');
        inner.className = 'cw-bubble-wrap';
        inner.appendChild(bubble);
        inner.appendChild(ts);

        wrap.appendChild(inner);
        messages.appendChild(wrap);
        scrollToBottom(messages);
        return wrap;
      }

      function appendTypingIndicator() {
        var wrap = document.createElement('div');
        wrap.className = 'cw-msg cw-msg-assistant cw-typing-row';
        wrap.id = 'cw-typing-' + Date.now();

        if (cfg.showAvatar) {
          var avatar = document.createElement('div');
          avatar.className = 'cw-avatar cw-avatar-assistant';
          avatar.textContent = cfg.botName.charAt(0).toUpperCase();
          avatar.setAttribute('aria-hidden', 'true');
          wrap.appendChild(avatar);
        }

        var inner = document.createElement('div');
        inner.className = 'cw-bubble-wrap';

        var bubble = document.createElement('div');
        bubble.className = 'cw-bubble cw-typing';
        bubble.setAttribute('aria-label', cfg.botName + ' is typing');
        bubble.innerHTML = '<span></span><span></span><span></span>';

        inner.appendChild(bubble);
        wrap.appendChild(inner);
        messages.appendChild(wrap);
        scrollToBottom(messages);
        return wrap;
      }

      // ---- Send logic --------------------------------------------------------

      function sendMessage() {
        if (sending) return;

        var text = input.value.trim();
        if (!text) return;

        sending = true;
        input.value   = '';
        input.style.height = '';
        sendBtn.disabled  = true;
        sendBtn.classList.add('cw-sending');

        appendMessage('user', text);
        var typingEl = appendTypingIndicator();

        var payload = {
          message: text,
          flow_id: cfg.flowId,
          history: history.slice(),
        };

        console.log('[chatbot-widget] sending message len=' + text.length + ' history_len=' + history.length + ' flow=' + cfg.flowId);

        fetch(cfg.apiUrl, {
          method:  'POST',
          headers: { 'Content-Type': 'application/json' },
          body:    JSON.stringify(payload),
        })
          .then(function (res) {
            var status = res.status;
            return res.json().then(function (data) {
              return { status: status, data: data };
            });
          })
          .then(function (result) {
            var status = result.status;
            var data   = result.data;

            // Remove typing indicator
            if (typingEl && typingEl.parentNode) {
              typingEl.parentNode.removeChild(typingEl);
            }

            if (!data.ok || status >= 400) {
              var errText = data.error || ('Server returned HTTP ' + status);
              console.error('[chatbot-widget] API error:', errText, 'flagged=' + data.flagged, 'guard=' + data.guard_status, 'status=' + status);

              if (status === 429) {
                var retry = data.retry_after || 60;
                appendMessage('assistant', 'You have sent too many messages. Please wait ' + retry + ' seconds before trying again.', { isError: true });
              } else if (data.flagged) {
                appendMessage('assistant', 'Your message could not be processed due to a security check. Please rephrase and try again.', { isError: true });
              } else {
                appendMessage('assistant', 'Something went wrong. Please try again. (' + errText + ')', { isError: true });
              }
            } else {
              var reply = data.reply || '';
              console.log('[chatbot-widget] received reply len=' + reply.length + ' guard=' + data.guard_status);

              // Push both turns into history for context
              history.push({ role: 'user',      content: text  });
              history.push({ role: 'assistant', content: reply });

              // Trim history to last 20 turns (10 exchanges)
              if (history.length > 20) {
                history = history.slice(history.length - 20);
              }

              appendMessage('assistant', reply);
            }
          })
          .catch(function (err) {
            // Remove typing indicator on network failure
            if (typingEl && typingEl.parentNode) {
              typingEl.parentNode.removeChild(typingEl);
            }
            console.error('[chatbot-widget] fetch error:', err);
            appendMessage('assistant', 'Network error -- could not reach the server. Check your connection and try again.', { isError: true });
          })
          .finally(function () {
            sending = false;
            sendBtn.disabled = false;
            sendBtn.classList.remove('cw-sending');
            input.focus();
          });
      }

      // ---- Events ------------------------------------------------------------

      sendBtn.addEventListener('click', function () {
        sendMessage();
      });

      input.addEventListener('keydown', function (e) {
        if (e.key === 'Enter' && !e.shiftKey) {
          e.preventDefault();
          sendMessage();
        }
      });

      // Auto-resize textarea up to 5 lines
      input.addEventListener('input', function () {
        input.style.height = '';
        var maxH = parseInt(window.getComputedStyle(input).lineHeight, 10) * 5 || 100;
        input.style.height = Math.min(input.scrollHeight, maxH) + 'px';
      });

      console.log('[chatbot-widget] mounted successfully flow_id=' + cfg.flowId);

    } catch (err) {
      console.error('[chatbot-widget] mount error on container:', container, err);
      container.innerHTML = '<p style="color:#f87171;padding:16px;font-size:13px;">[chatbot-widget] Failed to initialize. See console for details.</p>';
    }
  }

  // ---- Init ------------------------------------------------------------------

  function init() {
    try {
      injectCSS();
      var containers = document.querySelectorAll('.pb-chatbot-wrap');
      if (!containers.length) {
        console.log('[chatbot-widget] no .pb-chatbot-wrap elements found on page');
        return;
      }
      console.log('[chatbot-widget] found ' + containers.length + ' widget container(s)');
      for (var i = 0; i < containers.length; i++) {
        mountWidget(containers[i]);
      }
    } catch (err) {
      console.error('[chatbot-widget] init error:', err);
    }
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();
