/**
 * diff-cal/storage.js
 * Persist calendar data in SQLite via the local Flask backend.
 * Falls back to localStorage when the server is unreachable.
 *
 * Backend: server/cal_server.py  (default http://localhost:5050)
 */

const API_BASE     = 'http://localhost:5050';
const LS_KEY       = 'calendarData';
const SAVE_STATUS_SELECTOR = '#save-status';

/* ── Helpers ─────────────────────────────────────────────── */

function setStatus(msg, ok = false) {
    const el = document.querySelector(SAVE_STATUS_SELECTOR);
    if (!el) return;
    el.textContent = msg;
    el.style.color  = ok ? '#4CAF50' : '#ff9800';
}

/** Collect the current calendar state into a plain object. */
export function collectData() {
    return {
        content   : document.getElementById('diff-cal-editit').innerHTML,
        title     : document.querySelector('#the-cal h1').textContent,
        theme     : document.getElementById('theme-select').value,
        fontFamily: document.getElementById('font-family-select').value,
        fontSize  : document.getElementById('font-size-select').value,
    };
}

/* ── Save ────────────────────────────────────────────────── */

/**
 * Save current state to the SQLite backend.
 * On network error, silently mirrors the save to localStorage.
 */
export async function saveCalendarData() {
    const data = collectData();
    setStatus('Saving…');

    try {
        const res = await fetch(`${API_BASE}/api/calendar`, {
            method : 'POST',
            headers: { 'Content-Type': 'application/json' },
            body   : JSON.stringify(data),
        });
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        localStorage.setItem(LS_KEY, JSON.stringify(data)); // keep LS in sync
        setStatus('Saved', true);
        setTimeout(() => setStatus(''), 2500);
    } catch (err) {
        console.warn('Backend unreachable, using localStorage fallback:', err.message);
        localStorage.setItem(LS_KEY, JSON.stringify(data));
        setStatus('Saved locally', true);
        setTimeout(() => setStatus(''), 2500);
    }
}

/* ── Load ────────────────────────────────────────────────── */

/**
 * Apply a data object to the DOM.
 * @param {object} data
 */
function applyData(data) {
    if (data.content) {
        document.getElementById('diff-cal-editit').innerHTML = data.content;
    }
    if (data.title) {
        document.querySelector('#the-cal h1').textContent = data.title;
    }
    if (data.theme) {
        document.getElementById('theme-select').value = data.theme;
        document.body.className = document.body.className.replace(/theme-\w+/g, '').trim();
        document.body.classList.add('theme-' + data.theme);
    }
    if (data.fontFamily) {
        document.getElementById('font-family-select').value = data.fontFamily;
        document.getElementById('the-cal').style.fontFamily = data.fontFamily;
    }
    if (data.fontSize) {
        document.getElementById('font-size-select').value = data.fontSize;
        document.getElementById('the-cal').style.fontSize = data.fontSize;
    }
}

/**
 * Load saved state from the SQLite backend, falling back to localStorage.
 */
export async function loadCalendarData() {
    // 1. Try the backend first
    try {
        const res = await fetch(`${API_BASE}/api/calendar`);
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const data = await res.json();
        if (data && Object.keys(data).length > 0) {
            applyData(data);
            setStatus('Loaded from DB', true);
            setTimeout(() => setStatus(''), 2000);
            return;
        }
    } catch (err) {
        console.warn('Backend unreachable, loading from localStorage:', err.message);
    }

    // 2. Fallback: localStorage
    const raw = localStorage.getItem(LS_KEY);
    if (raw) {
        try {
            applyData(JSON.parse(raw));
            setStatus('Loaded from local storage', true);
            setTimeout(() => setStatus(''), 2000);
        } catch (e) {
            console.error('localStorage parse error:', e);
        }
    }
}

/* ── Clear ───────────────────────────────────────────────── */

/**
 * Delete saved data from both backend and localStorage.
 */
export async function clearCalendarData() {
    localStorage.removeItem(LS_KEY);
    try {
        await fetch(`${API_BASE}/api/calendar`, { method: 'DELETE' });
    } catch (_) { /* ignore if offline */ }
}
