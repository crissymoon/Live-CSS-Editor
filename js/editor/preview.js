/**
 * editor/preview.js -- Live preview iframe management
 * Attached to window.LiveCSS.editorPreview
 *
 * Public API
 *   LiveCSS.editorPreview.updatePreview(htmlEditor, cssEditor, jsEditor)
 *     Rebuilds the full iframe srcdoc from all three editor values.
 *
 *   LiveCSS.editorPreview.setPreviewCss(fullCss)
 *     Patches only the user CSS style tag in the live preview without
 *     rebuilding the entire srcdoc. Returns true on success, false on
 *     failure (caller should fall back to updatePreview).
 *
 *   LiveCSS.editorPreview.buildContextMenuScript()
 *     Returns an inline <script> string for the right-click inspector
 *     injected into the preview iframe.
 *
 *   LiveCSS.editorPreview.buildNavFixScript()
 *     Returns an inline <script> string that intercepts anchor clicks
 *     inside the preview iframe to keep it self-contained.
 */
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 * MIT License -- see LICENSE file for full text.
 */
window.LiveCSS = window.LiveCSS || {};

window.LiveCSS.editorPreview = (function () {

    /**
     * Base CSS reset injected before user CSS.
     * Strips native OS form-control appearance (macOS forces its own styling
     * on select/input/button/textarea without this).
     */
    var PREVIEW_BASE_CSS =
        'select,input,button,textarea{' +
            '-webkit-appearance:none;' +
            '-moz-appearance:none;' +
            'appearance:none;' +
        '}' +
        'select{background-image:none;}';

    /**
     * Build the right-click context-menu inspector script.
     * Always injected into the preview iframe.
     * Right-clicking any element shows selector, dimensions, and key styles.
     */
    function buildContextMenuScript() {
        function ctxMenuInit() {
            var menu = document.createElement('div');
            menu.setAttribute('data-ctx-menu', '1');
            menu.style.position  = 'fixed';
            menu.style.zIndex    = '999999';
            menu.style.background = '#0c071c';
            menu.style.color     = '#eceaf6';
            menu.style.font      = '12px/1.6 Consolas,Monaco,monospace';
            menu.style.padding   = '6px 0';
            menu.style.border    = '1px solid #4d31bf';
            menu.style.display   = 'none';
            menu.style.minWidth  = '220px';
            menu.style.maxWidth  = '360px';
            menu.style.boxShadow = '0 4px 16px rgba(0,0,0,.5)';
            menu.style.overflow  = 'hidden';
            document.body.appendChild(menu);

            var lastEl = null;
            var lastSelector = '';

            function hideMenu() {
                menu.style.display = 'none';
                if (lastEl) { lastEl.style.outline = ''; lastEl = null; }
            }

            function esc(str) {
                var d = document.createElement('span');
                d.textContent = str;
                return d.innerHTML;
            }

            document.addEventListener('contextmenu', function (e) {
                var el = e.target;
                if (!el || !el.tagName) return;
                if (el.getAttribute('data-ctx-menu') === '1') return;
                var p = el;
                while (p) { if (p === menu) return; p = p.parentNode; }
                e.preventDefault();

                if (lastEl) lastEl.style.outline = '';
                lastEl = el;
                el.style.outline = '2px solid #4d31bf';

                var tag    = el.tagName.toLowerCase();
                var idStr  = el.id ? '#' + el.id : '';
                var clsStr = '';
                if (el.className && typeof el.className === 'string' && el.className.trim()) {
                    clsStr = '.' + el.className.trim().replace(/\s+/g, '.');
                }
                var selector = tag + idStr + clsStr;
                lastSelector = selector;

                var rect = el.getBoundingClientRect();
                var dims = Math.round(rect.width) + ' \u00D7 ' + Math.round(rect.height);

                var cs    = window.getComputedStyle(el);
                var props = [
                    ['display',     cs.display],
                    ['position',    cs.position],
                    ['color',       cs.color],
                    ['background',  cs.backgroundColor],
                    ['font-size',   cs.fontSize],
                    ['margin',      cs.margin],
                    ['padding',     cs.padding]
                ];

                var h = '';
                h += '<div style="padding:5px 10px;color:#b39ddb;font-weight:bold;border-bottom:1px solid #2a1f4e;margin-bottom:2px;word-break:break-all;">' + esc(selector) + '</div>';
                h += '<div style="padding:2px 10px;color:#888;">' + esc(dims) + '</div>';
                for (var i = 0; i < props.length; i++) {
                    h += '<div style="padding:2px 10px;"><span style="color:#7c6fb0;">' + esc(props[i][0]) + '</span><span style="color:#555;"> : </span><span style="color:#eceaf6;">' + esc(props[i][1]) + '</span></div>';
                }
                h += '<div data-copy="1" style="padding:5px 10px;margin-top:2px;border-top:1px solid #2a1f4e;cursor:pointer;color:#4d31bf;">Copy selector</div>';
                h += '<div data-goto-css="1" style="padding:5px 10px;cursor:pointer;color:#7c6fb0;">Go to in CSS</div>';
                menu.innerHTML = h;

                menu.style.display = 'block';
                var x = e.clientX, y = e.clientY;
                var mw = menu.offsetWidth, mh = menu.offsetHeight;
                if (x + mw > window.innerWidth)  x = window.innerWidth  - mw - 4;
                if (y + mh > window.innerHeight) y = window.innerHeight - mh - 4;
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                menu.style.left = x + 'px';
                menu.style.top  = y + 'px';
            }, true);

            menu.addEventListener('mouseover', function (e) {
                var t = e.target;
                while (t && t !== menu) {
                    if (t.getAttribute && (t.getAttribute('data-copy') === '1' || t.getAttribute('data-goto-css') === '1')) {
                        t.style.background = '#1a1130'; return;
                    }
                    t = t.parentNode;
                }
            });

            menu.addEventListener('mouseout', function (e) {
                var t = e.target;
                while (t && t !== menu) {
                    if (t.getAttribute && (t.getAttribute('data-copy') === '1' || t.getAttribute('data-goto-css') === '1')) {
                        t.style.background = 'transparent'; return;
                    }
                    t = t.parentNode;
                }
            });

            menu.addEventListener('click', function (e) {
                var t = e.target;
                while (t && t !== menu) {
                    if (t.getAttribute && t.getAttribute('data-copy') === '1') {
                        var selDiv = menu.firstChild;
                        if (selDiv) {
                            var txt = selDiv.textContent;
                            var ta  = document.createElement('textarea');
                            ta.value = txt;
                            ta.style.position = 'fixed';
                            ta.style.left = '-9999px';
                            document.body.appendChild(ta);
                            ta.select();
                            document.execCommand('copy');
                            document.body.removeChild(ta);
                            t.textContent = 'Copied!';
                            setTimeout(function () { t.textContent = 'Copy selector'; }, 1200);
                        }
                        return;
                    }
                    if (t.getAttribute && t.getAttribute('data-goto-css') === '1') {
                        console.log('[GoToCSS] sending postMessage for selector:', lastSelector);
                        window.parent.postMessage({ type: 'livecss-goto-css', selector: lastSelector }, '*');
                        hideMenu();
                        return;
                    }
                    t = t.parentNode;
                }
            });

            document.addEventListener('click', function (e) {
                if (menu.style.display !== 'none') {
                    var p = e.target;
                    while (p) { if (p === menu) return; p = p.parentNode; }
                    hideMenu();
                }
            }, true);

            document.addEventListener('scroll', hideMenu, true);
        }

        return '<script>(' + ctxMenuInit.toString() + ')();<\/script>';
    }

    /**
     * Build the nav-fix script injected into the preview iframe.
     * Intercepts anchor clicks so that:
     *  - hash-only links (#section) smooth-scroll within the preview
     *  - external / full-URL links are suppressed to keep the preview self-contained
     */
    function buildNavFixScript() {
        var fn = function () {
            document.addEventListener('click', function (e) {
                var a = e.target.closest ? e.target.closest('a[href]') : null;
                if (!a) {
                    var el = e.target;
                    while (el && el.tagName !== 'A') { el = el.parentElement; }
                    if (el && el.hasAttribute('href')) { a = el; }
                }
                if (!a) { return; }
                var href = a.getAttribute('href') || '';
                e.preventDefault();
                if (href.charAt(0) === '#' && href.length > 1) {
                    var target = document.querySelector(href);
                    if (target) { target.scrollIntoView({ behavior: 'smooth' }); }
                }
                // All other links (external, relative) are intentionally blocked
            });
        };
        return '<script>(' + fn.toString() + ')();<\/script>';
    }

    /** Rebuild the iframe contents from all three editor values */
    function updatePreview(htmlEditor, cssEditor, jsEditor) {
        if (!htmlEditor || !cssEditor || !jsEditor) { return; }
        var frame   = document.getElementById('previewFrame');
        var htmlVal = htmlEditor.getValue();
        var jsCode  = jsEditor.getValue();
        var safeJs  = jsCode
            ? '<script>\ntry {\n' + jsCode + '\n} catch (e) { console.error("[preview]", e); }\n<\/script>'
            : '';
        var userCss = cssEditor.getValue();
        var styleBlocks =
            '<style>' + PREVIEW_BASE_CSS + '<\/style>' +
            '<style data-livecss-user="1">' + (userCss || '') + '<\/style>';
        var scriptBlocks = buildNavFixScript() + buildContextMenuScript() + safeJs;

        // Build a <base> tag so srcdoc iframes can resolve absolute paths
        // (images, fonts, etc.) back to the app server origin.
        var baseTag = '';
        try {
            baseTag = '<base href="' + window.location.origin + '/">';
        } catch (e) {
            console.error('[editor] Could not build base tag:', e);
        }

        // If the HTML editor contains a full document, inject our extras into
        // its <head> rather than double-wrapping it in another HTML shell.
        var isFullDoc = /^\s*<!doctype\s|^\s*<html[\s>]/i.test(htmlVal);
        if (isFullDoc) {
            var injected = htmlVal;
            if (/<head[^>]*>/i.test(injected)) {
                injected = injected.replace(/<head([^>]*)>/i, '<head$1>' + baseTag);
            }
            if (/<\/head>/i.test(injected)) {
                injected = injected.replace(/<\/head>/i, styleBlocks + '<\/head>');
            } else {
                injected = baseTag + styleBlocks + injected;
            }
            if (/<\/body>/i.test(injected)) {
                injected = injected.replace(/<\/body>/i, scriptBlocks + '<\/body>');
            } else {
                injected = injected + scriptBlocks;
            }
            frame.srcdoc = injected;
        } else {
            frame.srcdoc =
                '<!DOCTYPE html><html><head><meta charset="UTF-8">' +
                baseTag +
                styleBlocks +
                '<\/head><body>' +
                htmlVal +
                scriptBlocks +
                '</body></html>';
        }
    }

    /**
     * Patch only the user CSS style tag in the live preview without
     * rebuilding the entire srcdoc. Used by color-swatch, size-slider,
     * and the CSS editor change handler to avoid resetting scroll.
     * Returns true on success, false if the patch could not be applied.
     */
    function setPreviewCss(fullCss) {
        try {
            var frame = document.getElementById('previewFrame');
            if (!frame) {
                console.warn('[editor] setPreviewCss: previewFrame not found');
                return false;
            }
            var fdoc = frame.contentDocument || (frame.contentWindow && frame.contentWindow.document);
            if (!fdoc) {
                console.warn('[editor] setPreviewCss: cannot access iframe document');
                return false;
            }
            // Iframe may be mid-load (about:blank or blank srcdoc) -- check readyState
            if (fdoc.readyState === 'loading' || !fdoc.body) {
                console.warn('[editor] setPreviewCss: iframe not ready (readyState=' + fdoc.readyState + '), falling back to full rebuild');
                return false;
            }
            var styleEl = fdoc.querySelector('style[data-livecss-user]');
            if (!styleEl) {
                var all = fdoc.querySelectorAll('style');
                styleEl = all.length > 1 ? all[all.length - 1] : (all[0] || null);
                if (!styleEl) {
                    console.warn('[editor] setPreviewCss: no style tag found in preview, falling back to full rebuild');
                    return false;
                }
                console.warn('[editor] setPreviewCss: falling back to last style tag -- preview may need refresh');
            }
            styleEl.textContent = fullCss;
            return true;
        } catch (e) {
            console.error('[editor] setPreviewCss failed:', e);
            return false;
        }
    }

    return {
        PREVIEW_BASE_CSS:       PREVIEW_BASE_CSS,
        buildContextMenuScript: buildContextMenuScript,
        buildNavFixScript:      buildNavFixScript,
        updatePreview:          updatePreview,
        setPreviewCss:          setPreviewCss
    };

}());
