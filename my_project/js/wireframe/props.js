/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
/* Properties panel HTML builder and input binding helpers.
   No render dependencies -- pure UI utilities. */

export function escHtml(s) {
    return String(s)
        .replace(/&/g,  '&amp;')
        .replace(/</g,  '&lt;')
        .replace(/>/g,  '&gt;')
        .replace(/"/g,  '&quot;');
}

/* Build the inner HTML for the properties panel.
   eligibleParents is pre-filtered: excludes el itself and its descendants. */
export function buildPropsHTML(el, eligibleParents) {
    var parentOpts = '<option value="">-- canvas (no parent) --</option>';
    eligibleParents.forEach(function (e) {
        parentOpts += '<option value="' + e.id + '"' +
            (el.parentId === e.id ? ' selected' : '') +
            '>' + escHtml(e.label) + '</option>';
    });

    return (
        '<div class="wf-props-section">' +
          '<div class="wf-props-section-title">Element</div>' +
          '<div class="wf-props-row"><label>Label</label>' +
            '<input class="wf-pi" type="text" id="wfpLabel" value="' + escHtml(el.label) + '">' +
          '</div>' +
          '<div class="wf-props-row"><label>Parent</label>' +
            '<select class="wf-pi" id="wfpParent">' + parentOpts + '</select>' +
          '</div>' +
        '</div>' +

        '<div class="wf-props-section">' +
          '<div class="wf-props-section-title">Position &amp; Size</div>' +
          '<div class="wf-props-row wf-props-row-half">' +
            '<label>X</label><input class="wf-pi" type="number" id="wfpX" value="' + el.x + '">' +
            '<label>Y</label><input class="wf-pi" type="number" id="wfpY" value="' + el.y + '">' +
          '</div>' +
          '<div class="wf-props-row wf-props-row-half">' +
            '<label>W</label><input class="wf-pi" type="number" id="wfpW" value="' + el.w + '">' +
            '<label>H</label><input class="wf-pi" type="number" id="wfpH" value="' + el.h + '">' +
          '</div>' +
        '</div>' +

        '<div class="wf-props-section">' +
          '<div class="wf-props-section-title">Margin</div>' +
          '<div class="wf-props-sublabel">' +
            '<span>top</span><span>right</span><span>bottom</span><span>left</span>' +
          '</div>' +
          '<div class="wf-props-row wf-props-row-quad">' +
            '<input class="wf-pi" type="number" id="wfpMt" min="0" value="' + el.mt + '">' +
            '<input class="wf-pi" type="number" id="wfpMr" min="0" value="' + el.mr + '">' +
            '<input class="wf-pi" type="number" id="wfpMb" min="0" value="' + el.mb + '">' +
            '<input class="wf-pi" type="number" id="wfpMl" min="0" value="' + el.ml + '">' +
          '</div>' +
        '</div>' +

        '<div class="wf-props-section">' +
          '<div class="wf-props-section-title">Padding</div>' +
          '<div class="wf-props-sublabel">' +
            '<span>top</span><span>right</span><span>bottom</span><span>left</span>' +
          '</div>' +
          '<div class="wf-props-row wf-props-row-quad">' +
            '<input class="wf-pi" type="number" id="wfpPt" min="0" value="' + el.pt + '">' +
            '<input class="wf-pi" type="number" id="wfpPr" min="0" value="' + el.pr + '">' +
            '<input class="wf-pi" type="number" id="wfpPb" min="0" value="' + el.pb + '">' +
            '<input class="wf-pi" type="number" id="wfpPl" min="0" value="' + el.pl + '">' +
          '</div>' +
        '</div>' +

        '<div class="wf-props-section">' +
          '<div class="wf-props-section-title">Appearance</div>' +
          '<div class="wf-props-row">' +
            '<label>BG color</label>' +
            '<input class="wf-pi wf-pi-color" type="color" id="wfpBg" value="' + el.bgColor + '">' +
            '<span class="wf-pi-hex" id="wfpBgHex">' + el.bgColor + '</span>' +
          '</div>' +
          '<div class="wf-props-row">' +
            '<label>Border</label>' +
            '<input class="wf-pi wf-pi-color" type="color" id="wfpBorder" value="' + el.borderColor + '">' +
            '<span class="wf-pi-hex" id="wfpBorderHex">' + el.borderColor + '</span>' +
          '</div>' +
          '<div class="wf-props-row">' +
            '<label>Border W</label>' +
            '<input class="wf-pi" type="number" id="wfpBW" min="0" max="20" value="' + el.borderWidth + '">' +
          '</div>' +
          '<div class="wf-props-row">' +
            '<label>Radius</label>' +
            '<input class="wf-pi" type="number" id="wfpBR" min="0" max="500" value="' + (el.borderRadius || 0) + '">' +
          '</div>' +
        '</div>' +

        '<div class="wf-props-section">' +
          '<button class="wf-btn-del" id="wfpDelete">Delete Element</button>' +
        '</div>'
    );
}

export function bindStr(id, fn) {
    var inp = document.getElementById(id);
    if (inp) inp.addEventListener('input', function () { fn(this.value); });
}

export function bindNum(id, fn) {
    var inp = document.getElementById(id);
    if (inp) {
        inp.addEventListener('input', function () {
            var v = parseInt(this.value, 10);
            if (!isNaN(v)) fn(v);
        });
    }
}
