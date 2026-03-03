<?php
/**
 * dom-inspector/index.php
 *
 * Header: URL bar + Inspect Mode toggle
 * Left pane: iframe loading proxy.php (the target page with inspector script)
 * Right pane: live element inspector panel fed by postMessage
 */
$PHP_SRV   = 'http://127.0.0.1:9879';
$PROXY_URL = $PHP_SRV . '/dom-inspector/proxy.php';
?><!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>DOM Inspector</title>
<style>
/* ── reset / base ──────────────────────────────────────────── */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    font-size: 13px;
    background: #0f0f13;
    color: #e2e2ea;
    height: 100vh;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}

/* ── toolbar ───────────────────────────────────────────────── */
#toolbar {
    flex: 0 0 auto;
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 12px;
    background: #1a1a23;
    border-bottom: 1px solid #2e2e3e;
}
#toolbar .label {
    font-size: 12px;
    font-weight: 600;
    color: #6366f1;
    white-space: nowrap;
    letter-spacing: .04em;
}
#url-input {
    flex: 1;
    padding: 6px 10px;
    background: #0f0f17;
    border: 1px solid #2e2e3e;
    border-radius: 6px;
    color: #e2e2ea;
    font-size: 13px;
    outline: none;
    transition: border-color .15s;
}
#url-input:focus { border-color: #6366f1; }
#url-input::placeholder { color: #555; }
#load-btn {
    padding: 6px 14px;
    background: #6366f1;
    color: #fff;
    border: none;
    border-radius: 6px;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    transition: background .15s;
    white-space: nowrap;
}
#load-btn:hover { background: #4f51d8; }
#inspect-btn {
    padding: 6px 14px;
    background: #1e2030;
    color: #a0a0b4;
    border: 1px solid #2e2e3e;
    border-radius: 6px;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    white-space: nowrap;
    transition: all .15s;
    user-select: none;
}
#inspect-btn.active {
    background: rgba(250, 176, 5, 0.15);
    border-color: #fab005;
    color: #fab005;
}
#status-dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: #333;
    flex-shrink: 0;
    transition: background .2s;
}
#status-dot.loading { background: #fab005; box-shadow: 0 0 6px #fab00580; }
#status-dot.ready   { background: #40c057; box-shadow: 0 0 6px #40c05780; }
#status-dot.error   { background: #f03e3e; }

/* ── main split ────────────────────────────────────────────── */
#main {
    flex: 1 1 0;
    min-height: 0;
    display: flex;
    overflow: hidden;
}
#frame-wrap {
    flex: 1 1 0;
    min-height: 0;
    position: relative;
    background: #fff;
    overflow: hidden;
}
#page-frame {
    width: 100%;
    height: 100%;
    border: none;
    display: block;
    background: #fff;
}

/* ── right side panel ──────────────────────────────────────── */
#panel {
    flex: 0 0 340px;
    background: #121218;
    border-left: 1px solid #2e2e3e;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}
#panel-header {
    flex: 0 0 auto;
    padding: 10px 14px;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: .08em;
    text-transform: uppercase;
    color: #555;
    border-bottom: 1px solid #2e2e3e;
    display: flex;
    align-items: center;
    justify-content: space-between;
}
#panel-header span { color: #6366f1; }
#clear-btn {
    font-size: 11px;
    background: none;
    border: none;
    color: #555;
    cursor: pointer;
    padding: 2px 6px;
    border-radius: 4px;
}
#clear-btn:hover { color: #e2e2ea; background: #1e1e2e; }
#panel-body {
    flex: 1 1 0;
    overflow-y: auto;
    padding: 10px 12px;
    scrollbar-width: thin;
    scrollbar-color: #2e2e3e transparent;
}
#panel-body::-webkit-scrollbar { width: 5px; }
#panel-body::-webkit-scrollbar-thumb { background: #2e2e3e; border-radius: 3px; }

/* ── inspector sections ────────────────────────────────────── */
.empty-msg {
    color: #444;
    text-align: center;
    margin-top: 60px;
    line-height: 1.8;
    font-size: 12px;
}
.insp-section {
    margin-bottom: 14px;
}
.insp-section-title {
    font-size: 10px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: .07em;
    color: #555;
    margin-bottom: 5px;
}
.insp-tag {
    display: inline-block;
    background: #6366f1;
    color: #fff;
    font-size: 14px;
    font-weight: 700;
    padding: 3px 10px;
    border-radius: 5px;
    font-family: monospace;
    letter-spacing: .02em;
}
.insp-id {
    font-family: monospace;
    font-size: 13px;
    color: #e879f9;
    word-break: break-all;
}
.insp-id::before { content: '#'; color: #888; }
.insp-id-none { color: #444; font-style: italic; font-size: 12px; }
.classes-wrap { display: flex; flex-wrap: wrap; gap: 4px; margin-top: 2px; }
.class-pill {
    display: inline-block;
    background: #1e2030;
    border: 1px solid #2e3060;
    color: #93c5fd;
    font-family: monospace;
    font-size: 11px;
    padding: 2px 7px;
    border-radius: 12px;
}
.class-pill::before { content: '.'; color: #666; }
.no-classes { color: #444; font-style: italic; font-size: 12px; }

.attr-table {
    width: 100%;
    border-collapse: collapse;
    font-family: monospace;
    font-size: 11px;
}
.attr-table tr:nth-child(even) td { background: #16161e; }
.attr-table td {
    padding: 3px 6px;
    vertical-align: top;
    border-bottom: 1px solid #1e1e2a;
}
.attr-table td:first-child { color: #6ee7b7; white-space: nowrap; padding-right: 8px; }
.attr-table td:last-child  { color: #fcd34d; word-break: break-all; }

.mono-box {
    font-family: monospace;
    font-size: 11px;
    background: #0a0a10;
    border: 1px solid #2e2e3e;
    border-radius: 5px;
    padding: 6px 8px;
    word-break: break-all;
    color: #bfdbfe;
    position: relative;
}
.copy-btn {
    position: absolute;
    top: 4px;
    right: 4px;
    font-size: 10px;
    background: #1e2030;
    border: 1px solid #2e2e3e;
    color: #888;
    border-radius: 3px;
    padding: 1px 5px;
    cursor: pointer;
    opacity: 0;
    transition: opacity .15s;
}
.mono-box:hover .copy-btn { opacity: 1; }
.copy-btn:hover { color: #e2e2ea; }

.css-table {
    width: 100%;
    border-collapse: collapse;
    font-family: monospace;
    font-size: 11px;
}
.css-table tr:nth-child(even) td { background: #16161e; }
.css-table td {
    padding: 2px 6px;
    border-bottom: 1px solid #1e1e2a;
    vertical-align: top;
}
.css-table td:first-child { color: #a78bfa; white-space: nowrap; padding-right: 8px; }
.css-table td:last-child  { color: #e2e2ea; word-break: break-all; }
.css-table tr.dim td       { color: #444; }

.children-list {
    display: flex; flex-wrap: wrap; gap: 4px;
}
.child-tag {
    background: #1a1a28;
    border: 1px solid #2e2e3e;
    color: #94a3b8;
    font-family: monospace;
    font-size: 11px;
    padding: 2px 6px;
    border-radius: 4px;
}
.rect-row { display: flex; gap: 8px; flex-wrap: wrap; }
.rect-box {
    flex: 1 1 60px;
    background: #1a1a28;
    border: 1px solid #2e2e3e;
    border-radius: 4px;
    padding: 4px 6px;
    text-align: center;
}
.rect-box .rk { font-size: 10px; color: #666; }
.rect-box .rv { font-size: 13px; color: #e2e2ea; font-weight: 600; }
</style>
</head>
<body>

<!-- ── toolbar ─────────────────────────────────────────────── -->
<div id="toolbar">
    <span class="label">DOM Inspector</span>
    <div id="status-dot" title="Frame status"></div>
    <input id="url-input" type="text" placeholder="Enter URL to inspect (e.g. https://example.com)" spellcheck="false" />
    <button id="load-btn">Load</button>
    <button id="inspect-btn">Inspect Mode: OFF</button>
</div>

<!-- ── main split ──────────────────────────────────────────── -->
<div id="main">
    <div id="frame-wrap">
        <iframe id="page-frame" scrolling="yes"></iframe>
    </div>

    <div id="panel">
        <div id="panel-header">
            <span>Element Inspector</span>
            <button id="clear-btn" title="Clear panel">Clear</button>
        </div>
        <div id="panel-body">
            <div class="empty-msg" id="empty-msg">
                Load a page,<br>enable Inspect Mode,<br>then click any element.
            </div>
        </div>
    </div>
</div>

<script>
/* ── refs ───────────────────────────────────────────────────── */
const urlInput    = document.getElementById('url-input');
const loadBtn     = document.getElementById('load-btn');
const inspectBtn  = document.getElementById('inspect-btn');
const frame       = document.getElementById('page-frame');
const panelBody   = document.getElementById('panel-body');
const statusDot   = document.getElementById('status-dot');
const clearBtn    = document.getElementById('clear-btn');
const PROXY       = '<?= $PROXY_URL ?>';

let inspectActive = false;
let frameReady    = false;

/* ── Load page ─────────────────────────────────────────────── */
function loadPage() {
    let raw = urlInput.value.trim();
    if (!raw) return;
    if (!/^https?:\/\//i.test(raw)) raw = 'https://' + raw;
    statusDot.className = 'loading';
    frameReady = false;
    frame.src = PROXY + '?url=' + encodeURIComponent(raw);
}

loadBtn.addEventListener('click', loadPage);
urlInput.addEventListener('keydown', e => { if (e.key === 'Enter') loadPage(); });

/* ── Inspect Mode toggle ────────────────────────────────────── */
inspectBtn.addEventListener('click', () => {
    inspectActive = !inspectActive;
    inspectBtn.textContent = 'Inspect Mode: ' + (inspectActive ? 'ON' : 'OFF');
    inspectBtn.classList.toggle('active', inspectActive);
    // Tell the injected script inside the iframe directly -- no overlay needed
    try {
        frame.contentWindow.postMessage({type: 'xcm_set_active', active: inspectActive}, '*');
    } catch(ex) {}
});

/* ── Listen to postMessages from proxied page ───────────────── */
window.addEventListener('message', e => {
    if (!e.data) return;

    if (e.data.type === 'xcm_ready') {
        frameReady    = true;
        statusDot.className = 'ready';
        // If inspect mode was already ON, tell the new page
        if (inspectActive) {
            frame.contentWindow.postMessage({type: 'xcm_set_active', active: true}, '*');
        }
    }

    if (e.data.type === 'xcm_inspect') {
        renderPanel(e.data.data);
    }
});

frame.addEventListener('error', () => { statusDot.className = 'error'; });
frame.addEventListener('load',  () => {
    // Fallback status -- script sends xcm_ready but just in case
    statusDot.className = 'ready';
    frameReady = true;
    if (inspectActive) {
        try {
            frame.contentWindow.postMessage({type: 'xcm_set_active', active: true}, '*');
        } catch(ex) {}
    }
});

/* ── Panel renderer ─────────────────────────────────────────── */
function esc(str) {
    return String(str)
        .replace(/&/g,'&amp;')
        .replace(/</g,'&lt;')
        .replace(/>/g,'&gt;')
        .replace(/"/g,'&quot;');
}

function monoBox(text, id) {
    return `<div class="mono-box">${esc(text)}<button class="copy-btn" data-copy="${esc(text)}">copy</button></div>`;
}

function renderPanel(d) {
    const emptyMsg = document.getElementById('empty-msg');
    if (emptyMsg) emptyMsg.remove();

    let html = '';

    /* Tag */
    html += `<div class="insp-section">
        <div class="insp-section-title">Element</div>
        <span class="insp-tag">&lt;${esc(d.tagName)}&gt;</span>
    </div>`;

    /* Rect */
    html += `<div class="insp-section">
        <div class="insp-section-title">Bounding Rect</div>
        <div class="rect-row">
            <div class="rect-box"><div class="rk">x</div><div class="rv">${d.rect.left}</div></div>
            <div class="rect-box"><div class="rk">y</div><div class="rv">${d.rect.top}</div></div>
            <div class="rect-box"><div class="rk">w</div><div class="rv">${d.rect.width}</div></div>
            <div class="rect-box"><div class="rk">h</div><div class="rv">${d.rect.height}</div></div>
        </div>
    </div>`;

    /* ID */
    html += `<div class="insp-section">
        <div class="insp-section-title">ID</div>
        ${d.id
            ? `<span class="insp-id">${esc(d.id)}</span>`
            : '<span class="insp-id-none">no id</span>'}
    </div>`;

    /* Classes */
    html += `<div class="insp-section">
        <div class="insp-section-title">Classes</div>
        ${d.classes.length
            ? '<div class="classes-wrap">' + d.classes.map(c=>`<span class="class-pill">${esc(c)}</span>`).join('') + '</div>'
            : '<span class="no-classes">no classes</span>'}
    </div>`;

    /* Attributes */
    const attrKeys = Object.keys(d.attrs).filter(k => k !== 'id' && k !== 'class');
    if (attrKeys.length) {
        html += `<div class="insp-section">
            <div class="insp-section-title">Attributes (${attrKeys.length})</div>
            <table class="attr-table">
            ${attrKeys.map(k=>`<tr><td>${esc(k)}</td><td>${esc(d.attrs[k])}</td></tr>`).join('')}
            </table>
        </div>`;
    }

    /* CSS Selector */
    html += `<div class="insp-section">
        <div class="insp-section-title">CSS Selector</div>
        ${monoBox(d.cssSelector, 'css-sel')}
    </div>`;

    /* XPath */
    html += `<div class="insp-section">
        <div class="insp-section-title">XPath</div>
        ${monoBox(d.xpath, 'xpath')}
    </div>`;

    /* Children */
    if (d.childCount > 0) {
        html += `<div class="insp-section">
            <div class="insp-section-title">Children (${d.childCount})</div>
            <div class="children-list">
                ${d.children.map(c=>`<span class="child-tag">${esc(c)}</span>`).join('')}
                ${d.childCount > d.children.length ? `<span class="child-tag">+${d.childCount - d.children.length} more</span>` : ''}
            </div>
        </div>`;
    }

    /* Computed styles */
    const cssProps = Object.keys(d.computed);
    if (cssProps.length) {
        html += `<div class="insp-section">
            <div class="insp-section-title">Computed Styles</div>
            <table class="css-table">
            ${cssProps.map(k=>{
                const v = d.computed[k];
                const dim = !v || v === 'none' || v === 'normal' || v === 'auto' || v === '0px';
                return `<tr class="${dim?'dim':''}"><td>${esc(k)}</td><td>${esc(v||'')}</td></tr>`;
            }).join('')}
            </table>
        </div>`;
    }

    /* Text content */
    if (d.text) {
        html += `<div class="insp-section">
            <div class="insp-section-title">Text Content</div>
            <div class="mono-box" style="color:#fcd34d; white-space: pre-wrap;">${esc(d.text)}</div>
        </div>`;
    }

    /* Outer HTML */
    html += `<div class="insp-section">
        <div class="insp-section-title">Outer HTML</div>
        ${monoBox(d.outerHtml, 'outer-html')}
    </div>`;

    panelBody.innerHTML = html;

    /* copy buttons */
    panelBody.querySelectorAll('.copy-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const text = btn.dataset.copy;
            navigator.clipboard.writeText(text).catch(() => {
                const ta = document.createElement('textarea');
                ta.value = text;
                document.body.appendChild(ta);
                ta.select();
                document.execCommand('copy');
                ta.remove();
            });
            btn.textContent = 'copied!';
            setTimeout(() => btn.textContent = 'copy', 1200);
        });
    });
}

/* ── Clear ──────────────────────────────────────────────────── */
clearBtn.addEventListener('click', () => {
    panelBody.innerHTML = '<div class="empty-msg" id="empty-msg">Load a page,<br>enable Inspect Mode,<br>then click any element.</div>';
});
</script>
</body>
</html>
