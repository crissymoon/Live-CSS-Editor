/**
 * theme_randomizer/ai-preview.js
 *
 * Request AI to generate a unique modern HTML preview for the active theme.
 * The server saves the result to style-sheets/backups/ with a timestamp.
 * Requires: constants.js, state.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:ai-preview] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    _i.fn.requestAiPreview = function requestAiPreview() {
        var S   = _i.state;
        var C   = _i.constants;
        var dom = _i.dom;

        // Detect slug from the same logic as nudge (reuse state.activeSheet)
        var slug = S.activeSheet ? S.activeSheet.replace(/\.css(\?.*)?$/i, '') : null;
        if (!slug) {
            try {
                var links = document.querySelectorAll('link[rel="stylesheet"]');
                for (var i = 0; i < links.length; i++) {
                    var href  = links[i].getAttribute('href') || '';
                    var match = href.match(/([a-z0-9-]+)\.css(\?|$)/i);
                    if (match && match[1] && match[1] !== 'style') {
                        slug = match[1];
                        break;
                    }
                }
            } catch (e) {
                console.error('[themeRand:ai-preview] slug detection failed:', e);
            }
        }

        if (!slug) {
            console.warn('[themeRand:ai-preview] requestAiPreview() -- no active theme detected.');
            if (dom.aiPreviewStatus) { dom.aiPreviewStatus.textContent = 'No active theme detected.'; }
            return;
        }

        if (dom.aiPreviewStatus) { dom.aiPreviewStatus.textContent = 'Sending to AI...'; }

        var payload;
        try {
            payload = JSON.stringify({
                vars:          (S.activeNudge && S.activeNudge.vars)          || {},
                duration_vars: (S.activeNudge && S.activeNudge.duration_vars) || {},
            });
        } catch (e) {
            console.error('[themeRand:ai-preview] JSON.stringify payload failed:', e);
            payload = '{}';
        }

        fetch(C.NUDGE_API + '?mode=preview&theme=' + encodeURIComponent(slug), {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    payload,
        })
        .then(function (r) {
            if (!r.ok) { throw new Error('HTTP ' + r.status); }
            return r.json();
        })
        .then(function (d) {
            if (d.error) {
                console.error('[themeRand:ai-preview] Server error:', d.error);
                if (dom.aiPreviewStatus) { dom.aiPreviewStatus.textContent = 'Error: ' + d.error; }
                return;
            }
            var msg = 'Saved: ' + (d.backup_file || 'unknown');
            if (dom.aiPreviewStatus) { dom.aiPreviewStatus.textContent = msg; }
            if (dom.nudgeChip)       { dom.nudgeChip.textContent += ' (AI preview saved)'; }

            if (d.html) {
                try {
                    var w = window.open('', '_blank');
                    if (w) {
                        w.document.open();
                        w.document.write(d.html);
                        w.document.close();
                    } else {
                        console.warn('[themeRand:ai-preview] window.open() was blocked.');
                    }
                } catch (e) {
                    console.error('[themeRand:ai-preview] Opening preview window failed:', e);
                }
            }
        })
        .catch(function (e) {
            console.error('[themeRand:ai-preview] requestAiPreview() fetch failed:', e);
            if (dom.aiPreviewStatus) { dom.aiPreviewStatus.textContent = 'Request failed. Check console.'; }
        });
    };

    console.log('[themeRand:ai-preview] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
