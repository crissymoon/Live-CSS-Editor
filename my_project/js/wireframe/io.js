/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* JSON save/load and context block generator. */

import { CANVAS_W, CANVAS_H } from './constants.js';
import { st, saveToStorage } from './state.js';
import { childrenOf } from './elements.js';

export function saveJSON(btn) {
    var payload = JSON.stringify({
        version:     1,
        nextId:      st.nextId,
        nextGuideId: st.nextGuideId,
        elements:    st.elements,
        guides:      st.guides
    }, null, 2);

    function blobDownload() {
        var blob = new Blob([payload], { type: 'application/json' });
        var url  = URL.createObjectURL(blob);
        var a    = document.createElement('a');
        a.href          = url;
        a.download      = 'wireframe.wf.json';
        a.style.display = 'none';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        setTimeout(function () { URL.revokeObjectURL(url); }, 2000);
    }

    function flashBtn(label) {
        if (!btn) return;
        var orig = btn.textContent;
        btn.textContent = label;
        setTimeout(function () { btn.textContent = orig; }, 1800);
    }

    /* Try Tauri native save dialog first */
    var tauri = window.__TAURI_INTERNALS__ || (window.__TAURI__ && window.__TAURI__.core);
    if (tauri) {
        var invoker = (window.__TAURI__ && window.__TAURI__.core)
            ? window.__TAURI__.core.invoke
            : window.__TAURI_INTERNALS__.invoke;

        Promise.resolve(
            invoker('save_text_file', {
                content:     payload,
                defaultName: 'wireframe.wf.json'
            })
        ).then(function (savedPath) {
            if (savedPath) {
                flashBtn('Saved!');
            }
        }).catch(function (err) {
            console.warn('[wireframe] Tauri save failed, falling back to download:', err);
            blobDownload();
        });
    } else {
        blobDownload();
    }
}

/* onLoad callback is called after state is populated; should trigger render() */
export function loadJSON(file, onLoad) {
    var reader = new FileReader();
    reader.onload = function (e) {
        try {
            var data = JSON.parse(e.target.result);
            if (!data || !Array.isArray(data.elements)) {
                alert('Invalid wireframe JSON file.');
                return;
            }
            st.elements    = data.elements;
            st.nextId      = data.nextId      || (st.elements.length + 1);
            st.guides      = Array.isArray(data.guides) ? data.guides : [];
            st.nextGuideId = data.nextGuideId || (st.guides.length + 1);
            st.selId       = null;
            saveToStorage();
            if (onLoad) onLoad();
        } catch (ex) {
            alert('Could not parse JSON: ' + ex.message);
        }
    };
    reader.readAsText(file);
}

export function buildContext() {
    if (!st.elements.length) return '/* No wireframe elements */';

    var lines = [];
    lines.push('/* =============================================');
    lines.push('   Wireframe Layout -- ' + st.elements.length + ' element(s)');
    lines.push('   Canvas: ' + CANVAS_W + 'px x ' + CANVAS_H + 'px');
    lines.push('   ============================================= */');
    lines.push('');

    function describe(el, indent) {
        var pfx = indent;
        lines.push(pfx + '/* ' + el.label + ' */');
        lines.push(pfx + '.el-' + el.id + ' {');
        lines.push(pfx + '    position: absolute;');
        lines.push(pfx + '    left: '    + el.x + 'px;');
        lines.push(pfx + '    top: '     + el.y + 'px;');
        lines.push(pfx + '    width: '   + el.w + 'px;');
        lines.push(pfx + '    height: '  + el.h + 'px;');
        lines.push(pfx + '    margin: '  + el.mt + 'px ' + el.mr + 'px ' + el.mb + 'px ' + el.ml + 'px;');
        lines.push(pfx + '    padding: ' + el.pt + 'px ' + el.pr + 'px ' + el.pb + 'px ' + el.pl + 'px;');
        lines.push(pfx + '    background: ' + el.bgColor + ';');
        lines.push(pfx + '    border: ' + el.borderWidth + 'px solid ' + el.borderColor + ';');
        if (el.borderRadius) {
            lines.push(pfx + '    border-radius: ' + el.borderRadius + 'px;');
        }
        lines.push(pfx + '}');
        childrenOf(el.id).forEach(function (k) { describe(k, indent + '    '); });
    }

    var roots = st.elements.filter(function (e) { return !e.parentId; });
    roots.forEach(function (el) {
        describe(el, '');
        lines.push('');
    });

    if (st.guides.length) {
        lines.push('/* Guides:');
        st.guides.forEach(function (g) {
            lines.push('   ' + (g.axis === 'h' ? 'horizontal' : 'vertical') +
                       ' guide at ' + g.pos + 'px');
        });
        lines.push(' */');
        lines.push('');
    }

    lines.push('/* --- Raw wireframe JSON ---');
    lines.push(JSON.stringify({ version: 1, nextId: st.nextId, elements: st.elements }, null, 2));
    lines.push('--- end wireframe JSON --- */');

    return lines.join('\n');
}
