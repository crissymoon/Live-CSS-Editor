/**
 * agent-core.js -- shared constants, state, dom ref, and utility functions.
 * Must be loaded before all other agent-*.js modules.
 * Exposes LiveCSS._agentCore for the rest of the modules.
 */
'use strict';

(function (LiveCSS) {

    var C = LiveCSS._agentCore = {};

    // -----------------------------------------------------------------------
    // URLs (mutable so init() can override them)
    // -----------------------------------------------------------------------

    C.AGENT_URL = 'ai/agent/agent.php';
    C.RUN_URL   = 'ai/agent/run.php';

    // -----------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------

    C.CHAT_PROVIDERS = {
        anthropic: { label: 'Anthropic', endpoint: 'ai/anthropic.php' },
        openai:    { label: 'OpenAI',    endpoint: 'ai/openai.php'    },
        deepseek:  { label: 'Deepseek',  endpoint: 'ai/deepseek.php'  }
    };

    C.CHAT_SYSTEM = 'You are a helpful CSS and web development assistant embedded in a live CSS editor. When returning code, always use fenced code blocks with the correct language identifier.';

    C.THEMES = ['dark-neu', 'morphism', 'glassmorphic', 'keyboard-ui', 'ada'];

    C.THEME_LABELS = {
        'dark-neu':     'Dark Neu',
        'morphism':     'Morphism',
        'glassmorphic': 'Glassmorphic',
        'keyboard-ui':  'Keyboard',
        'ada':          'Ada (auto)'
    };

    C.TASK_OPTIONS = [
        { value: 'fix',         label: 'Fix Bug'        },
        { value: 'refactor',    label: 'Refactor'       },
        { value: 'modernize',   label: 'Modernize'      },
        { value: 'add_feature', label: 'Add Feature'    },
        { value: 'explain',     label: 'Explain'        },
        { value: 'review',      label: 'Review'         },
        { value: 'test',        label: 'Write Tests'    },
        { value: 'document',    label: 'Document'       },
        { value: 'optimize',    label: 'Optimize'       },
        { value: 'security',    label: 'Security Audit' }
    ];

    C.NEW_PROJECT_TASKS = [
        { value: 'preview',     label: 'Generate Preview' },
        { value: 'add_feature', label: 'Add Component'    },
        { value: 'refactor',    label: 'Refactor Theme'   },
        { value: 'review',      label: 'Review Theme'     }
    ];

    C.AGENT_MODES = {
        repair:      { name: 'Code Repair / Edit', tasks: null },
        new_project: { name: 'New Project',         tasks: null }
    };
    // Assign after arrays are defined to avoid forward-ref issues
    C.AGENT_MODES.repair.tasks       = C.TASK_OPTIONS;
    C.AGENT_MODES.new_project.tasks  = C.NEW_PROJECT_TASKS;

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    C.state = {
        theme:        'ada',
        mode:         'repair',
        source:       'editors',
        filePath:     '',
        content:      '',
        provider:     'anthropic',
        model:        '',
        task:         'fix',
        providers:    {},
        versions:     [],
        history:      {},
        conversation: [],
        activeStream: null,
        busy:         false,
        activeTab:    'run',
        chatHistory:  [],
        chatStream:   null,
        minimized:    false
    };

    // -----------------------------------------------------------------------
    // Shared DOM reference object
    // -----------------------------------------------------------------------

    C.dom = {};

    // -----------------------------------------------------------------------
    // Markdown (inline mini parser)
    // -----------------------------------------------------------------------

    C.MD = (function () {
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
            var lines = md.split('\n'), out = [], i = 0, n = lines.length;
            while (i < n) {
                var l = lines[i];
                var fm = l.match(/^```(\w*)/);
                if (fm) {
                    var lang = fm[1] ? esc(fm[1]) : '', code = [];
                    i++;
                    while (i < n && !lines[i].match(/^```/)) { code.push(esc(lines[i])); i++; }
                    out.push('<pre><code' + (lang ? ' class="language-' + lang + '"' : '') + '>' + code.join('\n') + '</code></pre>');
                    i++; continue;
                }
                var hm = l.match(/^(#{1,6})\s+(.+)/);
                if (hm) { out.push('<h' + hm[1].length + '>' + inline(hm[2]) + '</h' + hm[1].length + '>'); i++; continue; }
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
                    out.push('<ul>' + ui.map(function(t){ return '<li>' + t + '</li>'; }).join('') + '</ul>'); continue;
                }
                var om = l.match(/^\d+\.\s+(.+)/);
                if (om) {
                    var oi = [inline(om[1])]; i++;
                    var no;
                    while (i < n && (no = lines[i].match(/^\d+\.\s+(.+)/))) { oi.push(inline(no[1])); i++; }
                    out.push('<ol>' + oi.map(function(t){ return '<li>' + t + '</li>'; }).join('') + '</ol>'); continue;
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
        function extractCode(text) {
            var m = text.match(/```[\w]*\n([\s\S]*?)```/);
            return m ? m[1] : text;
        }
        return { toHtml: toHtml, extractCode: extractCode };
    }());

    // -----------------------------------------------------------------------
    // HTML escape (used by context + prompts modules)
    // -----------------------------------------------------------------------

    C.esc = function (s) {
        return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
    };

    // -----------------------------------------------------------------------
    // HTTP helper
    // -----------------------------------------------------------------------

    C.agentPost = function (data) {
        return fetch(C.AGENT_URL, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify(data)
        }).then(function (r) { return r.json(); });
    };

    // -----------------------------------------------------------------------
    // DOM element factory
    // -----------------------------------------------------------------------

    C.el = function (tag, className) {
        var e = document.createElement(tag);
        if (className) { e.className = className; }
        return e;
    };

    // -----------------------------------------------------------------------
    // Status bar
    // -----------------------------------------------------------------------

    C.setStatus = function (stateName, text) {
        C.dom.statusBar.className  = 'agent-status-bar agent-' + stateName;
        C.dom.statusText.textContent = text || '';
    };

    C.setBusy = function (busy) {
        C.state.busy = busy;
        C.dom.runBtn.disabled   = busy;
        C.dom.abortBtn.disabled = !busy;
        C.dom.loadBtn.disabled  = busy;
        if (busy) { C.setStatus('busy', 'Working...'); }
    };

    // -----------------------------------------------------------------------
    // Toast
    // -----------------------------------------------------------------------

    C.toast = function (msg, type) {
        var t = C.el('div', 'agent-toast agent-toast-' + (type || 'info'));
        t.textContent = msg;
        document.body.appendChild(t);
        setTimeout(function () { t.classList.add('toast-visible'); }, 10);
        setTimeout(function () {
            t.classList.remove('toast-visible');
            setTimeout(function () { if (t.parentNode) { t.parentNode.removeChild(t); } }, 250);
        }, 3500);
    };

}(window.LiveCSS = window.LiveCSS || {}));
