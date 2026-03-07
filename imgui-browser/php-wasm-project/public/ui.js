/**
 * ui.js – Browser UI controller for the PHP WASM demo.
 *
 * Handles all DOM interaction: URL bar, method select, run button,
 * tab switching, and rendering the PHP response into the output pane.
 *
 * Imports phpExec from php-wasm.js (the WASM bridge).
 */

import { phpExec } from './php-wasm.js';

// ── DOM references ─────────────────────────────────────────────────────────

const urlInput       = /** @type {HTMLInputElement}  */ (document.getElementById('url-input'));
const methodSelect   = /** @type {HTMLSelectElement} */ (document.getElementById('method-select'));
const runBtn         = /** @type {HTMLButtonElement} */ (document.getElementById('run-btn'));
const statusText     = document.getElementById('status-text');
const responseCode   = document.getElementById('response-code');
const outputFrame    = /** @type {HTMLIFrameElement} */ (document.getElementById('output-frame'));
const rawOutput      = document.getElementById('raw-output');
const headersOutput  = document.getElementById('headers-output');
const postBodySec    = document.getElementById('post-body-section');
const postBodyInput  = /** @type {HTMLTextAreaElement} */ (document.getElementById('post-body'));

// ── Initialisation ─────────────────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', () => {
    bindEvents();
    setStatus('Waiting for PHP WASM to load…');
    autoRunOnReady();
});

function bindEvents() {
    runBtn.addEventListener('click', handleRun);
    urlInput.addEventListener('keydown', e => { if (e.key === 'Enter') handleRun(); });
    methodSelect.addEventListener('change', togglePostBody);
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', () => switchTab(btn.dataset.tab));
    });
}

async function autoRunOnReady() {
    // Trigger the default route once the module is ready
    try {
        await handleRun();
    } catch {
        setStatus('PHP WASM not loaded — run build.sh first', 'error');
    }
}

// ── Event handlers ─────────────────────────────────────────────────────────

async function handleRun() {
    const uri    = urlInput.value.trim() || '/';
    const method = methodSelect.value;
    const body   = method === 'POST' ? postBodyInput.value : '';

    setStatus('Running…');
    runBtn.disabled = true;

    try {
        const response = await phpExec({ uri, method, body });
        displayResponse(response);
    } catch (err) {
        displayError(err);
    } finally {
        runBtn.disabled = false;
    }
}

function togglePostBody() {
    postBodySec.classList.toggle('hidden', methodSelect.value !== 'POST');
}

// ── Response rendering ─────────────────────────────────────────────────────

function displayResponse(response) {
    const { status, headers, body } = response;

    // Status badge
    responseCode.textContent = String(status);
    responseCode.className = 'code-badge ' + (status < 400 ? 'ok' : 'err');
    setStatus(`Completed in — ${new Date().toLocaleTimeString()}`);

    // Rendered HTML pane
    const contentType = headers['Content-Type'] || headers['content-type'] || '';
    if (contentType.includes('html') || !contentType) {
        outputFrame.srcdoc = body;
    } else {
        outputFrame.srcdoc = `<pre style="white-space:pre-wrap;font-family:monospace">${escapeHtml(body)}</pre>`;
    }

    // Raw output pane
    rawOutput.textContent = body;

    // Headers pane
    headersOutput.textContent = formatHeaders(headers);
}

function displayError(err) {
    responseCode.textContent = '500';
    responseCode.className = 'code-badge err';
    setStatus('Error: ' + err.message, 'error');
    rawOutput.textContent = err.stack || err.message;
    outputFrame.srcdoc = `<pre style="color:red;padding:1rem">${escapeHtml(String(err))}</pre>`;
}

// ── Tab switching ──────────────────────────────────────────────────────────

function switchTab(name) {
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.tab === name);
    });
    document.querySelectorAll('.tab-pane').forEach(pane => {
        pane.classList.toggle('active', pane.id === `tab-${name}`);
        pane.classList.toggle('hidden', pane.id !== `tab-${name}`);
    });
}

// ── Utility helpers ────────────────────────────────────────────────────────

function setStatus(msg) {
    statusText.textContent = msg;
}

function escapeHtml(str) {
    return str
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
}

function formatHeaders(headers) {
    return Object.entries(headers)
        .map(([k, v]) => `${k}: ${v}`)
        .join('\n') || '(no headers)';
}
