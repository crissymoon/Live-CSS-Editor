/**
 * cdn-loader.js — Loads CodeMirror from the first available CDN.
 *
 * Tries sources in order:
 *   1. cdnjs.cloudflare.com  (primary)
 *   2. cdn.jsdelivr.net       (fallback 1)
 *   3. unpkg.com              (fallback 2)
 *
 * Usage: LiveCSS.cdnLoader.load(callback)
 *   callback is invoked once all scripts are loaded and ready,
 *   or not at all if every CDN fails (a fatal error UI is shown instead).
 *
 * Attached to window.LiveCSS.cdnLoader
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.cdnLoader = (function () {

    var CM_VER = '5.65.16';

    /**
     * CDN source definitions.
     * core  — the main codemirror.min.js URL
     * css   — codemirror.min.css URL
     * theme — material-darker.min.css URL
     * base  — prefix used to build all mode/addon URLs below
     */
    var CDNS = [
        {
            label : 'cdnjs',
            core  : 'https://cdnjs.cloudflare.com/ajax/libs/codemirror/' + CM_VER + '/codemirror.min.js',
            css   : 'https://cdnjs.cloudflare.com/ajax/libs/codemirror/' + CM_VER + '/codemirror.min.css',
            theme : 'https://cdnjs.cloudflare.com/ajax/libs/codemirror/' + CM_VER + '/theme/material-darker.min.css',
            base  : 'https://cdnjs.cloudflare.com/ajax/libs/codemirror/' + CM_VER
        },
        {
            label : 'jsdelivr',
            core  : 'https://cdn.jsdelivr.net/npm/codemirror@' + CM_VER + '/lib/codemirror.min.js',
            css   : 'https://cdn.jsdelivr.net/npm/codemirror@' + CM_VER + '/lib/codemirror.min.css',
            theme : 'https://cdn.jsdelivr.net/npm/codemirror@' + CM_VER + '/theme/material-darker.min.css',
            base  : 'https://cdn.jsdelivr.net/npm/codemirror@' + CM_VER
        },
        {
            label : 'unpkg',
            core  : 'https://unpkg.com/codemirror@' + CM_VER + '/lib/codemirror.min.js',
            css   : 'https://unpkg.com/codemirror@' + CM_VER + '/lib/codemirror.min.css',
            theme : 'https://unpkg.com/codemirror@' + CM_VER + '/theme/material-darker.min.css',
            base  : 'https://unpkg.com/codemirror@' + CM_VER
        }
    ];

    /**
     * Module paths, relative to each CDN base above.
     * All three CDNs share the same relative paths for modes and addons.
     */
    var MODULES = [
        '/mode/xml/xml.min.js',
        '/mode/htmlmixed/htmlmixed.min.js',
        '/mode/css/css.min.js',
        '/mode/javascript/javascript.min.js',
        '/addon/edit/closetag.min.js',
        '/addon/edit/closebrackets.min.js',
        '/addon/edit/matchbrackets.min.js'
    ];

    // ── Internal helpers ────────────────────────────────────────

    /** Inject a <link rel="stylesheet"> and mark existing ones for removal */
    function injectCss(href) {
        var link = document.createElement('link');
        link.rel  = 'stylesheet';
        link.href = href;
        document.head.appendChild(link);
    }

    /** Remove any previously injected CDN stylesheets (on CDN switch) */
    function removePreviousCss() {
        var links = document.head.querySelectorAll('link[data-cm-cdn]');
        for (var i = 0; i < links.length; i++) {
            document.head.removeChild(links[i]);
        }
    }

    function injectCdnCss(cdn) {
        removePreviousCss();
        var linkCore  = document.createElement('link');
        linkCore.rel  = 'stylesheet';
        linkCore.href = cdn.css;
        linkCore.setAttribute('data-cm-cdn', '1');
        document.head.appendChild(linkCore);

        var linkTheme  = document.createElement('link');
        linkTheme.rel  = 'stylesheet';
        linkTheme.href = cdn.theme;
        linkTheme.setAttribute('data-cm-cdn', '1');
        document.head.appendChild(linkTheme);
    }

    /**
     * Load a single script, fire onSuccess or onError when done.
     * Sets async=false so browser execution order is preserved.
     */
    function loadScript(src, onSuccess, onError) {
        var el    = document.createElement('script');
        el.async  = false;
        el.src    = src;
        el.onload = onSuccess;
        el.onerror = function () {
            document.head.removeChild(el);
            onError();
        };
        document.head.appendChild(el);
    }

    /**
     * Load an ordered list of scripts sequentially.
     * Calls onAllDone when every script has loaded.
     * Calls onAnyError as soon as any one fails.
     */
    function loadModulesSequentially(base, modules, index, onAllDone, onAnyError) {
        if (index >= modules.length) {
            onAllDone();
            return;
        }
        loadScript(
            base + modules[index],
            function () {
                loadModulesSequentially(base, modules, index + 1, onAllDone, onAnyError);
            },
            onAnyError
        );
    }

    /** Show a fatal error banner replacing the whole page */
    function showFatalError() {
        document.body.style.cssText = 'margin:0;background:#0c071c;font-family:sans-serif;display:flex;align-items:center;justify-content:center;height:100vh;';
        document.body.innerHTML =
            '<div style="color:#eceaf6;text-align:center;padding:40px;">' +
            '<p style="font-size:20px;font-weight:700;margin-bottom:12px;">Failed to load CodeMirror</p>' +
            '<p style="font-size:14px;color:#9e93c0;">All CDN sources failed to respond.</p>' +
            '<p style="font-size:14px;color:#9e93c0;margin-top:8px;">Check your network connection and reload the page.</p>' +
            '</div>';
    }

    // ── Public API ──────────────────────────────────────────────

    /**
     * Attempt to load CodeMirror (CSS + core + all modules) from each CDN
     * in sequence. Calls onReady() once everything is loaded successfully.
     */
    function load(onReady, cdnIndex) {
        cdnIndex = cdnIndex || 0;

        if (cdnIndex >= CDNS.length) {
            showFatalError();
            return;
        }

        var cdn = CDNS[cdnIndex];

        // Inject stylesheets for this CDN attempt (swap on retry)
        injectCdnCss(cdn);

        // Load core first, then modules
        loadScript(
            cdn.core,
            function () {
                loadModulesSequentially(
                    cdn.base,
                    MODULES,
                    0,
                    function () {
                        // All scripts loaded from this CDN
                        console.info('[cdn-loader] Loaded from ' + cdn.label);
                        onReady();
                    },
                    function () {
                        // A module failed — try next CDN from scratch
                        console.warn('[cdn-loader] Module failed on ' + cdn.label + ', trying next CDN...');
                        load(onReady, cdnIndex + 1);
                    }
                );
            },
            function () {
                // Core failed — try next CDN
                console.warn('[cdn-loader] Core failed on ' + cdn.label + ', trying next CDN...');
                load(onReady, cdnIndex + 1);
            }
        );
    }

    return { load: load };

}());
