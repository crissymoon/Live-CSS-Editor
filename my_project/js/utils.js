/**
 * utils.js — Shared utility functions
 * Attached to window.LiveCSS.utils
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.utils = (function () {

    /** Safely encode a string for HTML text content */
    function escapeHtml(str) {
        var div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }

    /** Safely encode a string for use inside an HTML attribute value */
    function escapeAttr(str) {
        return str
            .replace(/&/g, '&amp;')
            .replace(/"/g, '&quot;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;');
    }

    /**
     * Returns a debounced version of fn that fires after `delay` ms
     * of inactivity.
     */
    function debounce(fn, delay) {
        var timer;
        return function () {
            clearTimeout(timer);
            timer = setTimeout(fn, delay);
        };
    }

    return { escapeHtml: escapeHtml, escapeAttr: escapeAttr, debounce: debounce };

}());
