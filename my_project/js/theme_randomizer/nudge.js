/**
 * theme_randomizer/nudge.js
 *
 * Harmony Nudge: fetch small color-offset vars from the PHP endpoint and
 * apply them to :root.  Exposes clearNudge() to restore original values.
 * Requires: constants.js, state.js, persistence.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:nudge] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    // -----------------------------------------------------------------------
    // Internal: detect active theme slug
    // -----------------------------------------------------------------------

    function detectActiveThemeSlug() {
        var S = _i.state;
        try {
            if (S.activeSheet) {
                return S.activeSheet.replace(/\.css(\?.*)?$/i, '');
            }
            var links = document.querySelectorAll('link[rel="stylesheet"]');
            for (var i = 0; i < links.length; i++) {
                var href  = links[i].getAttribute('href') || '';
                var match = href.match(/([a-z0-9-]+)\.css(\?|$)/i);
                if (match && match[1] && match[1] !== 'style') {
                    return match[1];
                }
            }
        } catch (e) {
            console.error('[themeRand:nudge] detectActiveThemeSlug() failed:', e);
        }
        return null;
    }

    // -----------------------------------------------------------------------
    // Internal: write nudged vars to :root
    // -----------------------------------------------------------------------

    function applyNudgedVars(vars, durationVars) {
        try {
            var root = document.documentElement;
            if (vars) {
                Object.keys(vars).forEach(function (k) {
                    root.style.setProperty(k, vars[k]);
                });
            }
            if (durationVars) {
                Object.keys(durationVars).forEach(function (k) {
                    root.style.setProperty(k, durationVars[k]);
                });
            }
        } catch (e) {
            console.error('[themeRand:nudge] applyNudgedVars() failed:', e);
        }
    }

    // -----------------------------------------------------------------------
    // Public: nudge
    // -----------------------------------------------------------------------

    _i.fn.nudgeActiveTheme = function nudgeActiveTheme() {
        var S   = _i.state;
        var C   = _i.constants;
        var dom = _i.dom;
        var fn  = _i.fn;

        var slug = detectActiveThemeSlug();
        if (!slug) {
            console.warn('[themeRand:nudge] nudgeActiveTheme() -- no active theme detected.');
            if (dom.nudgeStatus) { dom.nudgeStatus.textContent = 'No active theme detected.'; }
            return;
        }

        if (dom.nudgeStatus) { dom.nudgeStatus.textContent = 'Nudging...'; }
        if (dom.nudgeChip)   { dom.nudgeChip.textContent   = 'Nudging...'; }

        fetch(C.NUDGE_API + '?mode=nudge&theme=' + encodeURIComponent(slug))
            .then(function (r) {
                if (!r.ok) { throw new Error('HTTP ' + r.status); }
                return r.json();
            })
            .then(function (d) {
                if (d.error) {
                    console.error('[themeRand:nudge] Server error:', d.error);
                    if (dom.nudgeStatus) { dom.nudgeStatus.textContent  = 'Error: ' + d.error; }
                    if (dom.nudgeChip)   { dom.nudgeChip.textContent    = ''; }
                    return;
                }
                applyNudgedVars(d.vars, d.duration_vars);
                S.activeNudge     = { vars: d.vars || {}, duration_vars: d.duration_vars || {} };
                S.activeNudgeSeed = d.seed || null;

                var label = slug + ' #' + (d.seed || '?');
                if (dom.nudgeStatus) { dom.nudgeStatus.textContent = 'Applied: ' + label; }
                if (dom.nudgeChip)   { dom.nudgeChip.textContent   = 'Nudge #' + (d.seed || '?'); }

                try { fn.savePref(C.SK_NUDGE, JSON.stringify({ seed: d.seed, theme: slug })); } catch (e) {
                    console.warn('[themeRand:nudge] Could not persist nudge pref:', e);
                }
            })
            .catch(function (e) {
                console.error('[themeRand:nudge] nudgeActiveTheme() fetch failed:', e);
                if (dom.nudgeStatus) { dom.nudgeStatus.textContent = 'Nudge endpoint unavailable.'; }
                if (dom.nudgeChip)   { dom.nudgeChip.textContent   = ''; }
            });
    };

    // -----------------------------------------------------------------------
    // Public: clear
    // -----------------------------------------------------------------------

    _i.fn.clearNudge = function clearNudge() {
        var S   = _i.state;
        var C   = _i.constants;
        var dom = _i.dom;
        var fn  = _i.fn;
        try {
            var root = document.documentElement;
            if (S.activeNudge) {
                Object.keys(S.activeNudge.vars || {}).forEach(function (k) {
                    root.style.removeProperty(k);
                });
                Object.keys(S.activeNudge.duration_vars || {}).forEach(function (k) {
                    root.style.removeProperty(k);
                });
            }
            S.activeNudge     = null;
            S.activeNudgeSeed = null;
            fn.clearPref(C.SK_NUDGE);
            if (dom.nudgeStatus) { dom.nudgeStatus.textContent = 'Nudge cleared.'; }
            if (dom.nudgeChip)   { dom.nudgeChip.textContent   = ''; }
        } catch (e) {
            console.error('[themeRand:nudge] clearNudge() failed:', e);
        }
    };

    console.log('[themeRand:nudge] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
