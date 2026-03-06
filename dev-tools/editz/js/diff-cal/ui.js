/**
 * diff-cal/ui.js
 * UI event handlers: theme switching, font controls, reset, AutoSaveManager.
 */

import { generateCalendar } from './calendar.js';
import { saveCalendarData, clearCalendarData } from './storage.js';

/* ── Debounce ────────────────────────────────────────────── */

export function debounce(fn, wait = 1000) {
    let t;
    return function (...args) {
        clearTimeout(t);
        t = setTimeout(() => fn.apply(this, args), wait);
    };
}

/* ── Theme ───────────────────────────────────────────────── */

export function applyTheme(theme) {
    document.body.className = document.body.className.replace(/theme-\w+/g, '').trim();
    document.body.classList.add('theme-' + theme);
}

/* ── Reset ───────────────────────────────────────────────── */

export function resetCalendar() {
    if (!confirm('Reset the calendar? All content will be cleared.')) return;

    clearCalendarData();

    // Reset title
    const h1 = document.querySelector('#the-cal h1');
    if (h1) h1.textContent = 'Write Your Calendar Title Here';

    // Reset theme
    document.getElementById('theme-select').value = 'classic';
    applyTheme('classic');

    // Reset font family
    document.getElementById('font-family-select').value = 'Arial, sans-serif';
    document.getElementById('the-cal').style.fontFamily = 'Arial, sans-serif';

    // Reset font size
    document.getElementById('font-size-select').value = '14px';
    document.getElementById('the-cal').style.fontSize = '14px';

    // Re-render calendar
    const month = parseInt(document.getElementById('month-select').value);
    const year  = parseInt(document.getElementById('year-select').value);
    generateCalendar(month, year);
}

/* ── Wire-up all UI event listeners ─────────────────────── */

export function bindUIEvents() {
    const debouncedSave = debounce(saveCalendarData, 1000);

    // "Create" / "Go" button
    document.getElementById('go-btn').addEventListener('click', () => {
        const month = parseInt(document.getElementById('month-select').value);
        const year  = parseInt(document.getElementById('year-select').value);
        generateCalendar(month, year);
    });

    // Theme selector
    document.getElementById('theme-select').addEventListener('change', function () {
        applyTheme(this.value);
        saveCalendarData();
    });

    // Font family
    document.getElementById('font-family-select').addEventListener('change', function () {
        document.getElementById('the-cal').style.fontFamily = this.value;
        saveCalendarData();
    });

    // Font size
    document.getElementById('font-size-select').addEventListener('change', function () {
        document.getElementById('the-cal').style.fontSize = this.value;
        saveCalendarData();
    });

    // Reset button
    document.getElementById('reset-btn').addEventListener('click', resetCalendar);

    // Contenteditable areas → debounced save
    const editableDiv = document.getElementById('diff-cal-editit');
    const titleH1     = document.querySelector('#the-cal h1');

    if (editableDiv) editableDiv.addEventListener('input', debouncedSave);
    if (titleH1)     titleH1.addEventListener('input', debouncedSave);
}

/* ── AutoSaveManager ─────────────────────────────────────── */

/**
 * Watches all [contenteditable][data-save-id] elements and
 * debounces saves through the storage module.
 */
export class AutoSaveManager {
    constructor(options = {}) {
        this.options = {
            debounceTime : 1000,
            statusElement: null,
            ...options,
        };

        this._timeout = null;
        this._elements = [];
        this._init();
    }

    _init() {
        this._elements = Array.from(
            document.querySelectorAll('[contenteditable][data-save-id]')
        );
        this._elements.forEach(el =>
            el.addEventListener('input', this._onInput.bind(this))
        );
    }

    _onInput() {
        clearTimeout(this._timeout);
        this._showStatus('Saving…');
        this._timeout = setTimeout(async () => {
            await saveCalendarData();
            this._showStatus('All changes saved', true);
            setTimeout(() => this._showStatus(''), 2000);
        }, this.options.debounceTime);
    }

    _showStatus(msg, ok = false) {
        if (!this.options.statusElement) return;
        const el = typeof this.options.statusElement === 'string'
            ? document.querySelector(this.options.statusElement)
            : this.options.statusElement;
        if (el) {
            el.textContent = msg;
            el.style.color  = ok ? '#4CAF50' : '#ff9800';
        }
    }
}

/* ── Color Picker Utility ────────────────────────────────── */

/**
 * Attach a color picker to change a CSS property on an element.
 * @param {string} targetSelector   CSS selector for the target element
 * @param {string} [inputSelector]  CSS selector for an existing <input type="color">
 * @param {'color'|'background-color'|string} [property='color']
 */
export function setupColorPicker(targetSelector, inputSelector, property = 'color') {
    let input;

    if (!inputSelector) {
        input = document.createElement('input');
        input.type = 'color';
        Object.assign(input.style, {
            position: 'fixed',
            top: '10px',
            right: '10px',
            zIndex: '1000',
        });
        document.body.appendChild(input);
    } else {
        input = document.querySelector(inputSelector);
        if (!input) {
            console.error(`setupColorPicker: selector '${inputSelector}' not found`);
            return;
        }
        input.type = 'color';
    }

    const target = document.querySelector(targetSelector);
    if (!target) {
        console.error(`setupColorPicker: target '${targetSelector}' not found`);
        return;
    }

    // Seed the picker with the element's current colour
    const currentColor = getComputedStyle(target)[property];
    input.value = _rgbToHex(currentColor) || '#000000';

    input.addEventListener('input', () => {
        target.style[property] = input.value;
    });
}

function _rgbToHex(rgb) {
    if (!rgb) return null;
    const m = rgb.match(/\d+/g);
    if (!m || m.length < 3) return null;
    const [r, g, b] = m.map(Number);
    return `#${((1 << 24) | (r << 16) | (g << 8) | b).toString(16).slice(1)}`;
}
