/**
 * property-lookup.js — Floating draggable properties reference panel
 * Attached to window.LiveCSS.propertyLookup
 *
 * Tabs: CSS | JS | HTML
 * Each tab shows a fuzzy-searchable list of properties/snippets.
 * Click to select, double-click or press Enter/Insert to insert at cursor.
 *
 * Call LiveCSS.propertyLookup.init(propertyValues) after editor.init().
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.propertyLookup = (function () {
    'use strict';

    // ── Static data ──────────────────────────────────────────────

    var JS_PROPS = [
        'console.log', 'console.error', 'console.warn', 'console.table',
        'document.getElementById', 'document.querySelector', 'document.querySelectorAll',
        'document.createElement', 'document.createTextNode', 'document.body',
        'document.head', 'document.title', 'document.cookie',
        'addEventListener', 'removeEventListener', 'dispatchEvent',
        'setTimeout', 'clearTimeout', 'setInterval', 'clearInterval',
        'requestAnimationFrame', 'cancelAnimationFrame',
        'fetch', 'Promise', 'Promise.all', 'Promise.race', 'async', 'await',
        'Array.from', 'Array.isArray', 'Array.of',
        'Object.keys', 'Object.values', 'Object.entries', 'Object.assign',
        'Object.freeze', 'Object.create', 'Object.defineProperty',
        'JSON.parse', 'JSON.stringify',
        'localStorage.getItem', 'localStorage.setItem', 'localStorage.removeItem',
        'sessionStorage.getItem', 'sessionStorage.setItem',
        'window.location', 'window.location.href', 'window.location.reload',
        'window.history.pushState', 'window.history.back',
        'window.innerWidth', 'window.innerHeight', 'window.scrollX', 'window.scrollY',
        'element.classList.add', 'element.classList.remove', 'element.classList.toggle',
        'element.classList.contains', 'element.setAttribute', 'element.getAttribute',
        'element.removeAttribute', 'element.style', 'element.dataset',
        'element.textContent', 'element.innerHTML', 'element.outerHTML',
        'element.appendChild', 'element.removeChild', 'element.insertBefore',
        'element.closest', 'element.matches', 'element.getBoundingClientRect',
        'element.offsetWidth', 'element.offsetHeight', 'element.offsetTop',
        'element.offsetLeft', 'element.scrollTop', 'element.scrollLeft',
        'event.preventDefault', 'event.stopPropagation', 'event.target',
        'event.currentTarget', 'event.clientX', 'event.clientY', 'event.key',
        'typeof', 'instanceof', 'hasOwnProperty', 'prototype',
        'Math.floor', 'Math.ceil', 'Math.round', 'Math.abs',
        'Math.max', 'Math.min', 'Math.random', 'Math.sqrt', 'Math.pow', 'Math.PI',
        'Array.prototype.map', 'Array.prototype.filter', 'Array.prototype.reduce',
        'Array.prototype.forEach', 'Array.prototype.find', 'Array.prototype.findIndex',
        'Array.prototype.some', 'Array.prototype.every', 'Array.prototype.includes',
        'Array.prototype.push', 'Array.prototype.pop', 'Array.prototype.shift',
        'Array.prototype.unshift', 'Array.prototype.splice', 'Array.prototype.slice',
        'Array.prototype.sort', 'Array.prototype.reverse', 'Array.prototype.concat',
        'Array.prototype.flat', 'Array.prototype.flatMap',
        'String.prototype.split', 'String.prototype.trim', 'String.prototype.replace',
        'String.prototype.indexOf', 'String.prototype.includes',
        'String.prototype.startsWith', 'String.prototype.endsWith',
        'String.prototype.slice', 'String.prototype.substring', 'String.prototype.toUpperCase',
        'String.prototype.toLowerCase', 'String.prototype.padStart', 'String.prototype.padEnd'
    ];

    var HTML_PROPS = [
        'id', 'class', 'style', 'title', 'lang', 'dir', 'tabindex', 'hidden',
        'href', 'src', 'alt', 'width', 'height', 'target', 'rel', 'download',
        'type', 'name', 'value', 'placeholder', 'disabled', 'readonly', 'required',
        'checked', 'selected', 'multiple', 'min', 'max', 'step', 'maxlength',
        'pattern', 'autofocus', 'autocomplete', 'form', 'for', 'action', 'method',
        'enctype', 'novalidate', 'accept', 'capture',
        'colspan', 'rowspan', 'scope', 'headers',
        'data-*', 'role', 'aria-label', 'aria-hidden', 'aria-expanded',
        'aria-controls', 'aria-live', 'aria-atomic', 'aria-describedby',
        'aria-labelledby', 'aria-selected', 'aria-disabled', 'aria-checked',
        'draggable', 'contenteditable', 'spellcheck', 'translate',
        'crossorigin', 'loading', 'decoding', 'fetchpriority', 'sizes', 'srcset',
        'media', 'async', 'defer', 'integrity', 'nonce',
        'http-equiv', 'charset', 'content', 'viewport'
    ];

    var HTML_VALUE_ATTRS = [
        'id','class','style','title','href','src','alt','data-*',
        'placeholder','name','value','type','action','method','for','role',
        'aria-label','aria-describedby','aria-labelledby','aria-controls',
        'pattern','accept','media','sizes','srcset','content','http-equiv',
        'charset','viewport','rel','target','lang','dir','loading','decoding',
        'enctype','fetchpriority','integrity','nonce','crossorigin','maxlength',
        'form','tabindex'
    ];

    // ── State ─────────────────────────────────────────────────────

    var propertyValues  = {};
    var panel, searchInput, propList, insertBtn, valueHint, tabBtns;
    var currentTab   = 'css';
    var selectedProp = null;
    var drag = { active: false, sx: 0, sy: 0, ox: 0, oy: 0 };

    // ── Helpers ───────────────────────────────────────────────────

    function getTabData() {
        if (currentTab === 'js')   { return JS_PROPS; }
        if (currentTab === 'html') { return HTML_PROPS; }
        return (window.LiveCSSData && window.LiveCSSData.allCssProperties) || [];
    }

    function fuzzyMatch(query, target) {
        if (!query) { return true; }
        var q = query.toLowerCase();
        var t = target.toLowerCase();
        if (t.indexOf(q) !== -1) { return true; }
        var qi = 0;
        for (var ti = 0; ti < t.length && qi < q.length; ti++) {
            if (t[ti] === q[qi]) { qi++; }
        }
        return qi === q.length;
    }

    // ── Render ────────────────────────────────────────────────────

    function renderList() {
        var query    = searchInput ? searchInput.value.trim() : '';
        var items    = getTabData();
        var prev     = selectedProp;
        var filtered = items.filter(function (p) { return fuzzyMatch(query, p); });

        propList.innerHTML = '';
        selectedProp = null;

        filtered.forEach(function (prop) {
            var li = document.createElement('li');
            li.className = 'prop-item';
            li.textContent = prop;

            if (prop === prev) {
                li.classList.add('selected');
                selectedProp = prop;
                updateHint(prop);
            }

            li.addEventListener('mousedown', function (e) {
                e.preventDefault();
                selectItem(prop, li);
            });

            li.addEventListener('dblclick', function (e) {
                e.preventDefault();
                selectItem(prop, li);
                insertSelected();
            });

            propList.appendChild(li);
        });

        if (!selectedProp) { valueHint.textContent = ''; }
    }

    function selectItem(prop, li) {
        propList.querySelectorAll('.prop-item').forEach(function (el) {
            el.classList.remove('selected');
        });
        li.classList.add('selected');
        selectedProp = prop;
        updateHint(prop);
    }

    function updateHint(prop) {
        if (currentTab === 'css' && propertyValues[prop]) {
            valueHint.textContent = propertyValues[prop];
        } else {
            valueHint.textContent = '';
        }
    }

    // ── Insert ────────────────────────────────────────────────────

    function insertSelected() {
        if (!selectedProp) { return; }
        var cm, snippet, cursor;

        if (currentTab === 'css') {
            cm      = LiveCSS.editor.getCssEditor();
            snippet = '  ' + selectedProp + ': ;';
            cursor  = cm.getCursor();
            cm.replaceRange(snippet + '\n', cursor);
            cm.setCursor({ line: cursor.line, ch: cursor.ch + snippet.length - 1 });
        } else if (currentTab === 'js') {
            cm     = LiveCSS.editor.getJsEditor();
            cursor = cm.getCursor();
            cm.replaceRange(selectedProp, cursor);
        } else {
            cm     = LiveCSS.editor.getHtmlEditor();
            cursor = cm.getCursor();
            var hasVal = HTML_VALUE_ATTRS.indexOf(selectedProp) !== -1;
            snippet = hasVal ? selectedProp + '=""' : selectedProp;
            cm.replaceRange(snippet, cursor);
            if (hasVal) {
                cm.setCursor({ line: cursor.line, ch: cursor.ch + snippet.length - 1 });
            }
        }
        cm.focus();
    }

    // ── Drag ──────────────────────────────────────────────────────

    function onHeaderMousedown(e) {
        if (e.target.classList.contains('prop-tool-close')) { return; }
        drag.active = true;
        drag.sx = e.clientX;
        drag.sy = e.clientY;
        drag.ox = panel.offsetLeft;
        drag.oy = panel.offsetTop;
        e.preventDefault();
    }

    function onDocMousemove(e) {
        if (!drag.active) { return; }
        var nx = drag.ox + (e.clientX - drag.sx);
        var ny = drag.oy + (e.clientY - drag.sy);
        panel.style.left = nx + 'px';
        panel.style.top  = ny + 'px';
    }

    function onDocMouseup() {
        if (drag.active) {
            drag.active = false;
            saveToolState();
        }
    }

    function saveToolState() {
        var state = LiveCSS.storage.loadUIState() || {};
        state.properties = {
            visible: !panel.classList.contains('hidden'),
            left:    panel.style.left,
            top:     panel.style.top,
            tab:     currentTab
        };
        LiveCSS.storage.saveUIState(state);
    }

    function positionNearBtn(btn) {
        var rect = btn.getBoundingClientRect();
        var pw   = 300;
        var ph   = 420;
        var left = Math.max(4, Math.min(rect.right - pw, window.innerWidth - pw - 4));
        var top  = Math.min(rect.bottom + 6, window.innerHeight - ph - 4);
        panel.style.left = left + 'px';
        panel.style.top  = top  + 'px';
    }

    // ── Init ──────────────────────────────────────────────────────

    function init(propValues) {
        propertyValues = propValues || {};
        panel       = document.getElementById('propertiesToolPanel');
        searchInput = document.getElementById('propSearchInput');
        propList    = document.getElementById('propList');
        insertBtn   = document.getElementById('propInsertBtn');
        valueHint   = document.getElementById('propValueHint');
        tabBtns     = panel.querySelectorAll('.prop-tab');

        // Toggle panel
        document.getElementById('propertiesBtn').addEventListener('click', function () {
            if (panel.classList.contains('hidden')) {
                panel.classList.remove('hidden');
                var uiState = LiveCSS.storage.loadUIState() || {};
                if (uiState.properties && uiState.properties.left) {
                    panel.style.left = uiState.properties.left;
                    panel.style.top  = uiState.properties.top;
                } else {
                    positionNearBtn(this);
                }
                searchInput.focus();
                renderList();
            } else {
                panel.classList.add('hidden');
            }
            saveToolState();
        });

        document.getElementById('propToolClose').addEventListener('click', function () {
            panel.classList.add('hidden');
            saveToolState();
        });

        // Tabs
        tabBtns.forEach(function (btn) {
            btn.addEventListener('click', function () {
                tabBtns.forEach(function (b) { b.classList.remove('active'); });
                this.classList.add('active');
                currentTab   = this.dataset.tab;
                selectedProp = null;
                valueHint.textContent = '';
                searchInput.value = '';
                renderList();
            });
        });

        // Search / keyboard nav
        searchInput.addEventListener('input', function () {
            selectedProp = null;
            renderList();
        });

        searchInput.addEventListener('keydown', function (e) {
            var items = propList.querySelectorAll('.prop-item');
            if (!items.length) { return; }
            var cur = propList.querySelector('.prop-item.selected');

            if (e.keyCode === 40) {                                 // Down
                e.preventDefault();
                var next = cur ? cur.nextElementSibling : items[0];
                if (!next) { next = items[0]; }
                selectItem(next.textContent, next);
                next.scrollIntoView({ block: 'nearest' });

            } else if (e.keyCode === 38) {                          // Up
                e.preventDefault();
                var prev = cur ? cur.previousElementSibling : items[items.length - 1];
                if (!prev) { prev = items[items.length - 1]; }
                selectItem(prev.textContent, prev);
                prev.scrollIntoView({ block: 'nearest' });

            } else if (e.keyCode === 13) {                          // Enter
                e.preventDefault();
                insertSelected();

            } else if (e.keyCode === 27) {                          // Escape
                panel.classList.add('hidden');
                saveToolState();
            }
        });

        // Insert button
        insertBtn.addEventListener('click', insertSelected);

        // Drag
        panel.querySelector('.prop-tool-header').addEventListener('mousedown', onHeaderMousedown);
        document.addEventListener('mousemove', onDocMousemove);
        document.addEventListener('mouseup', onDocMouseup);

        // Restore saved visibility and position
        var savedUI = LiveCSS.storage.loadUIState() || {};
        if (savedUI.properties) {
            if (savedUI.properties.visible) {
                panel.classList.remove('hidden');
                if (savedUI.properties.left) {
                    panel.style.left = savedUI.properties.left;
                    panel.style.top  = savedUI.properties.top;
                }
                renderList();
            }
            if (savedUI.properties.tab) {
                currentTab = savedUI.properties.tab;
                tabBtns.forEach(function (b) {
                    b.classList.toggle('active', b.dataset.tab === currentTab);
                });
                if (!panel.classList.contains('hidden')) { renderList(); }
            }
        }
    }

    return { init: init };

}());
