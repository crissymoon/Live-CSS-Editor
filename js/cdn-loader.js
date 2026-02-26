/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */

/**
 * cdn-loader.js -- Loads CodeMirror, local-first with CDN fallback.
 *
 * Tries sources in order:
 *   0. vendor/codemirror/   (local — fastest, works offline)
 *   1. cdnjs.cloudflare.com  (fallback 1)
 *   2. cdn.jsdelivr.net       (fallback 2)
 *   3. unpkg.com              (fallback 3)
 *
 * Usage: LiveCSS.cdnLoader.load(callback)
 *   callback is invoked once all scripts are loaded and ready,
 *   or not at all if every source fails (a fatal error UI is shown instead).
 *
 * Attached to window.LiveCSS.cdnLoader
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.cdnLoader = (function () {

    var CM_VER = '5.65.16';

    /**
     * Source definitions (index 0 = local vendor, 1-3 = CDN fallbacks).
     * core  — the main codemirror.min.js URL
     * css   — codemirror.min.css URL
     * theme — material-darker.min.css URL
     * base  — prefix used to build all mode/addon URLs below
     */
    var SOURCES = [
        {
            label : 'local',
            core  : 'vendor/codemirror/lib/codemirror.min.js',
            css   : 'vendor/codemirror/lib/codemirror.min.css',
            theme : 'vendor/codemirror/theme/material-darker.min.css',
            base  : 'vendor/codemirror'
        },
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
     * Module paths, relative to each source base above.
     * Rulers addon removed — custom indent guides used instead.
     */
    var MODULES = [
        '/mode/xml/xml.min.js',
        '/mode/htmlmixed/htmlmixed.min.js',
        '/mode/css/css.min.js',
        '/mode/javascript/javascript.min.js',
        '/addon/edit/closetag.min.js',
        '/addon/edit/closebrackets.min.js',
        '/addon/edit/matchbrackets.min.js',
        '/addon/fold/foldcode.min.js',
        '/addon/fold/foldgutter.min.js',
        '/addon/fold/brace-fold.min.js',
        '/addon/fold/xml-fold.min.js',
        '/addon/fold/comment-fold.min.js',
        '/addon/lint/lint.min.js'
    ];

    /**
     * External linter libraries that must load BEFORE the CM lint bridges.
     * Paths are relative to the same base as SOURCES[0].base parent.
     * For CDN sources these are loaded from fixed CDN URLs.
     */
    var LINTER_LIBS_LOCAL = [
        'vendor/linters/jshint.min.js',
        'vendor/linters/csslint.min.js',
        'vendor/linters/htmlhint.min.js'
    ];

    var LINTER_LIBS_CDN = [
        'https://cdnjs.cloudflare.com/ajax/libs/jshint/2.13.6/jshint.min.js',
        'https://cdnjs.cloudflare.com/ajax/libs/csslint/1.0.5/csslint.min.js',
        'https://cdn.jsdelivr.net/npm/htmlhint@0.16.3/dist/htmlhint.min.js'
    ];

    /** CM lint bridge addons — loaded after external linter libs */
    var LINT_BRIDGES = [
        '/addon/lint/javascript-lint.min.js',
        '/addon/lint/css-lint.min.js',
        '/addon/lint/html-lint.min.js'
    ];

    // ── Internal helpers ────────────────────────────────────────

    /** Remove any previously injected CM stylesheets (on source switch) */
    function removePreviousCss() {
        var links = document.head.querySelectorAll('link[data-cm-cdn]');
        for (var i = 0; i < links.length; i++) {
            document.head.removeChild(links[i]);
        }
    }

    function injectSourceCss(src) {
        removePreviousCss();
        var linkCore  = document.createElement('link');
        linkCore.rel  = 'stylesheet';
        linkCore.href = src.css;
        linkCore.setAttribute('data-cm-cdn', '1');
        document.head.appendChild(linkCore);

        var linkTheme  = document.createElement('link');
        linkTheme.rel  = 'stylesheet';
        linkTheme.href = src.theme;
        linkTheme.setAttribute('data-cm-cdn', '1');
        document.head.appendChild(linkTheme);

        var linkFold  = document.createElement('link');
        linkFold.rel  = 'stylesheet';
        linkFold.href = src.base + '/addon/fold/foldgutter.min.css';
        linkFold.setAttribute('data-cm-cdn', '1');
        document.head.appendChild(linkFold);

        var linkLint  = document.createElement('link');
        linkLint.rel  = 'stylesheet';
        linkLint.href = src.base + '/addon/lint/lint.min.css';
        linkLint.setAttribute('data-cm-cdn', '1');
        document.head.appendChild(linkLint);
    }

    /**
     * Load a single script, fire onSuccess or onError when done.
     * Sets async=false so browser execution order is preserved.
     */
    function loadScript(url, onSuccess, onError) {
        var el    = document.createElement('script');
        el.async  = false;
        el.src    = url;
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
            '<p style="font-size:14px;color:#9e93c0;">All sources (local + CDN) failed to respond.</p>' +
            '<p style="font-size:14px;color:#9e93c0;margin-top:8px;">Check your network connection and reload the page.</p>' +
            '</div>';
    }

    // ── Public API ──────────────────────────────────────────────

    /**
     * Attempt to load CodeMirror (CSS + core + all modules) from each
     * source in sequence (local first, then CDNs).
     * Calls onReady() once everything is loaded successfully.
     */
    function load(onReady, srcIndex) {
        srcIndex = srcIndex || 0;

        if (srcIndex >= SOURCES.length) {
            showFatalError();
            return;
        }

        var src = SOURCES[srcIndex];

        // Inject stylesheets for this source attempt (swap on retry)
        injectSourceCss(src);

        // Load core first, then modules, then external linters, then lint bridges
        loadScript(
            src.core,
            function () {
                loadModulesSequentially(
                    src.base,
                    MODULES,
                    0,
                    function () {
                        // Now load external linter libs (absolute paths)
                        var linterUrls = (srcIndex === 0) ? LINTER_LIBS_LOCAL : LINTER_LIBS_CDN;
                        loadModulesSequentially(
                            '',
                            linterUrls,
                            0,
                            function () {
                                // Finally load CM lint bridge addons
                                loadModulesSequentially(
                                    src.base,
                                    LINT_BRIDGES,
                                    0,
                                    function () {
                                        console.info('[cdn-loader] Loaded from ' + src.label);
                                        onReady();
                                    },
                                    function () {
                                        // Lint bridges failed — still usable without lint
                                        console.warn('[cdn-loader] Lint bridges failed on ' + src.label + ', continuing without lint');
                                        onReady();
                                    }
                                );
                            },
                            function () {
                                // External linters failed — still usable without lint
                                console.warn('[cdn-loader] Linter libs failed on ' + src.label + ', continuing without lint');
                                onReady();
                            }
                        );
                    },
                    function () {
                        console.warn('[cdn-loader] Module failed on ' + src.label + ', trying next source...');
                        load(onReady, srcIndex + 1);
                    }
                );
            },
            function () {
                console.warn('[cdn-loader] Core failed on ' + src.label + ', trying next source...');
                load(onReady, srcIndex + 1);
            }
        );
    }

    return { load: load };

}());
