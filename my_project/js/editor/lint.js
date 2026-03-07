/**
 * editor/lint.js -- CodeMirror lint configuration and CSS lint validator
 * Attached to window.LiveCSS.editorLint
 *
 * Public API
 *   LiveCSS.editorLint.JSHINT_OPTS        -- options object for JSHint
 *   LiveCSS.editorLint.CSSLINT_OPTS       -- rules object for CSSLint (all noisy rules disabled)
 *   LiveCSS.editorLint.cssLintValidator   -- custom annotation function for CodeMirror CSS lint
 *   LiveCSS.editorLint.lintAvailable(mode)-- returns bool, checks that lint addons and engines loaded
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editorLint = (function () {

    /** JSHint options -- relaxed for quick prototyping */
    var JSHINT_OPTS = {
        esversion: 11,
        asi:       true,
        boss:      true,
        evil:      true,
        laxbreak:  true,
        laxcomma:  true,
        loopfunc:  true,
        sub:       true,
        supernew:  true,
        undef:     false,
        unused:    false,
        browser:   true,
        devel:     true
    };

    /** CSSLint rules to disable (too noisy or incompatible with modern CSS) */
    var CSSLINT_OPTS = {
        'known-properties':                              false,
        'vendor-prefix':                                 false,
        'compatible-vendor-prefixes-and-properties':     false,
        'star-property-hack':                            false,
        'underscore-property-hack':                      false,
        'important':                                     false,
        'box-sizing':                                    false,
        'adjoining-classes':                             false,
        'qualified-headings':                            false,
        'unique-headings':                               false,
        'universal-selector':                            false,
        'unqualified-attributes':                        false,
        'overqualified-elements':                        false,
        'floats':                                        false,
        'font-sizes':                                    false,
        'ids':                                           false,
        'regex-selectors':                               false,
        'outline-none':                                  false,
        'shorthand':                                     false,
        'display-property-grouping':                     false,
        'fallback-colors':                               false,
        'duplicate-properties':                          false,
        'order-alphabetical':                            false,
        'zero-units':                                    false,
        'bulletproof-font-face':                         false,
        'font-faces':                                    false,
        'gradients':                                     false
    };

    /**
     * Custom CSS lint annotation function. Wraps CSSLint but pre-processes
     * modern CSS features that CSSLint cannot parse (custom props, var()).
     */
    function cssLintValidator(text) {
        // 1. Strip custom property declarations (--foo: value;), preserving line count
        var cleaned = text.replace(/(--[\w-]+)\s*:\s*([^;}{]*)/g, function (m) {
            var lines = m.split('\n');
            var out = 'color: red';
            for (var i = 1; i < lines.length; i++) { out += '\n'; }
            return out;
        });

        // 2. Replace var(...) references -- handle nested var() like var(--a, var(--b, #fff))
        var maxIter = 10;
        while (maxIter-- > 0 && cleaned.indexOf('var(') !== -1) {
            cleaned = cleaned.replace(/var\s*\([^()]*\)/g, '#000');
        }

        // 3. Replace modern CSS functions CSSLint cannot handle
        cleaned = cleaned.replace(/env\s*\([^)]*\)/g, '0px');
        cleaned = cleaned.replace(/clamp\s*\([^)]*\)/g, '16px');
        cleaned = cleaned.replace(/min\s*\([^)]*\)/g, '0px');
        cleaned = cleaned.replace(/max\s*\([^)]*\)/g, '0px');

        // 4. Replace modern CSS units CSSLint does not know
        //    fr (grid), dvh/dvw/svh/svw/lvh/lvw, cqi/cqb, etc.
        cleaned = cleaned.replace(/(\d+(?:\.\d+)?)\s*fr\b/g, '$1px');
        cleaned = cleaned.replace(/(\d+(?:\.\d+)?)\s*(dvh|dvw|svh|svw|lvh|lvw|cqi|cqb|cqw|cqh|cqmin|cqmax)\b/g, '$1px');

        // 5. Replace modern CSS layout keywords CSSLint cannot parse
        cleaned = cleaned.replace(/repeat\s*\([^)]*\)/g, '1px');
        cleaned = cleaned.replace(/minmax\s*\([^)]*\)/g, '1px');
        cleaned = cleaned.replace(/fit-content\s*\([^)]*\)/g, '1px');
        cleaned = cleaned.replace(/\bauto-fill\b/g, '1');
        cleaned = cleaned.replace(/\bauto-fit\b/g, '1');

        var errors   = CSSLint.verify(cleaned, CSSLINT_OPTS);
        var found    = [];
        var messages = errors.messages || [];
        for (var i = 0; i < messages.length; i++) {
            var msg = messages[i];
            if (msg.message && msg.message.indexOf('Unknown property') !== -1) continue;
            if (msg.message && msg.message.indexOf('Expected') !== -1 && msg.message.indexOf('--') !== -1) continue;
            if (msg.message && msg.message.indexOf('fallback') !== -1) continue;
            found.push({
                from:     CodeMirror.Pos((msg.line || 1) - 1, (msg.col || 1) - 1),
                to:       CodeMirror.Pos((msg.line || 1) - 1, (msg.col || 1)),
                message:  msg.message,
                severity: msg.type === 'error' ? 'error' : 'warning'
            });
        }
        return found;
    }

    /** Check whether lint addons and engine libraries loaded successfully */
    function lintAvailable(mode) {
        if (!CodeMirror.lint) return false;
        if (mode === 'javascript' && typeof JSHINT    === 'undefined') return false;
        if (mode === 'css'        && typeof CSSLint   === 'undefined') return false;
        if (mode === 'htmlmixed'  && typeof HTMLHint  === 'undefined') return false;
        return true;
    }

    return {
        JSHINT_OPTS:      JSHINT_OPTS,
        CSSLINT_OPTS:     CSSLINT_OPTS,
        cssLintValidator: cssLintValidator,
        lintAvailable:    lintAvailable
    };

}());
