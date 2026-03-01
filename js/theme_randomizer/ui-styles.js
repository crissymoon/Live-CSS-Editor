/**
 * theme_randomizer/ui-styles.js
 *
 * Injects the lc-* component CSS directly into <head>.
 * No external CSS file dependency -- all color references use active palette vars.
 * Requires: constants.js
 */

'use strict';

(function (LiveCSS) {

    var _i = LiveCSS._trInternal;
    if (!_i) {
        console.error('[themeRand:ui-styles] _trInternal missing -- ensure constants.js is loaded first.');
        return;
    }

    _i.fn.injectStyles = function injectStyles() {
        var id = 'lcRandStyles';
        if (document.getElementById(id)) { return; }

        try {
            var css = [
                /* Context bar */
                '.lc-ctx-bar{',
                '  position:fixed;bottom:0;left:0;',
                '  display:flex;align-items:center;gap:4px;',
                '  padding:0 8px;height:26px;',
                '  background:var(--clr-bg-alt,#141620);',
                '  border-top:1px solid var(--clr-border,#2a2d3e);',
                '  border-right:1px solid var(--clr-border,#2a2d3e);',
                '  z-index:8000;',
                '  font-family:system-ui,-apple-system,sans-serif;',
                '}',
                '.lc-ctx-chip{',
                '  font-size:10px;',
                '  color:var(--clr-text-dim,#50536a);',
                '  padding:0 5px;',
                '  border-right:1px solid var(--clr-border,#2a2d3e);',
                '  white-space:nowrap;',
                '}',
                '.lc-ctx-btn{',
                '  height:20px;',
                '  padding:0 7px;',
                '  font-size:10px;',
                '  font-weight:600;',
                '  letter-spacing:0.04em;',
                '  text-transform:uppercase;',
                '  background:transparent;',
                '  border:1px solid var(--clr-border,#2a2d3e);',
                '  color:var(--clr-text-muted,#8890b0);',
                '  cursor:pointer;',
                '  font-family:inherit;',
                '  transition:color 100ms,border-color 100ms;',
                '}',
                '.lc-ctx-btn:hover{',
                '  color:var(--clr-accent,#3da8ff);',
                '  border-color:var(--clr-accent,#3da8ff);',
                '}',
                '.lc-rand-btn{',
                '  background:var(--clr-surface-alt,#22253a);',
                '}',
                '.lc-nudge-btn{',
                '  background:var(--clr-surface-alt,#22253a);',
                '  border-color:var(--clr-border,#2a2d3e);',
                '}',
                '.lc-nudge-btn:hover{',
                '  color:var(--clr-accent,#3da8ff);',
                '  border-color:var(--clr-accent,#3da8ff);',
                '}',
                '.lc-ai-preview-btn{',
                '  background:var(--clr-surface,#1a1c28);',
                '  border-color:var(--clr-border,#2a2d3e);',
                '}',
                '.lc-ai-preview-btn:hover{',
                '  color:var(--clr-accent-light,#70c4ff);',
                '  border-color:var(--clr-accent-light,#70c4ff);',
                '}',
                '.lc-ctx-nudge{',
                '  font-size:10px;',
                '  color:var(--clr-accent,#3da8ff);',
                '  padding:0 5px;',
                '  border-right:1px solid var(--clr-border,#2a2d3e);',
                '  white-space:nowrap;',
                '}',
                '.lc-nudge-status,.lc-ai-status{',
                '  font-size:10px;',
                '  color:var(--clr-accent,#3da8ff);',
                '  min-height:14px;',
                '}',
                '.lc-drawer-btn-sm{',
                '  height:22px;font-size:10px;',
                '}',
                '.lc-drawer-overlay{',
                '  position:fixed;inset:0;',
                '  background:var(--clr-overlay,rgba(0,0,0,0.60));',
                '  z-index:8001;',
                '  opacity:0;pointer-events:none;',
                '  transition:opacity 180ms;',
                '}',
                '.lc-overlay-open{opacity:1!important;pointer-events:auto!important;}',

                /* Drawer */
                '.lc-drawer{',
                '  position:fixed;top:0;right:0;bottom:0;',
                '  width:320px;',
                '  background:var(--clr-surface,#1a1c28);',
                '  border-left:1px solid var(--clr-border,#2a2d3e);',
                '  z-index:8002;',
                '  transform:translateX(100%);',
                '  transition:transform 240ms cubic-bezier(0.0,0.0,0.2,1);',
                '  display:flex;flex-direction:column;',
                '  font-family:system-ui,-apple-system,sans-serif;',
                '}',
                '.lc-drawer-open{transform:translateX(0)!important;}',
                '.lc-drawer-header{',
                '  display:flex;align-items:center;justify-content:space-between;',
                '  padding:0 14px;',
                '  height:40px;',
                '  border-bottom:1px solid var(--clr-border,#2a2d3e);',
                '  background:var(--clr-bg-alt,#141620);',
                '  flex-shrink:0;',
                '}',
                '.lc-drawer-title{',
                '  font-size:11px;font-weight:600;letter-spacing:0.06em;text-transform:uppercase;',
                '  color:var(--clr-text-muted,#8890b0);',
                '}',
                '.lc-drawer-body{',
                '  flex:1;overflow-y:auto;padding:14px;',
                '  display:flex;flex-direction:column;gap:16px;',
                '}',
                '.lc-drawer-section{display:flex;flex-direction:column;gap:6px;}',
                '.lc-field-label{',
                '  font-size:10px;font-weight:600;letter-spacing:0.06em;text-transform:uppercase;',
                '  color:var(--clr-text-dim,#50536a);',
                '}',
                '.lc-field-note{',
                '  font-size:10px;color:var(--clr-text-dim,#50536a);',
                '}',
                '.lc-select{',
                '  width:100%;height:30px;',
                '  background:var(--clr-surface-alt,#22253a);',
                '  border:1px solid var(--clr-border,#2a2d3e);',
                '  color:var(--clr-text,#e0e2f0);',
                '  font-size:12px;font-family:inherit;',
                '  padding:0 8px;',
                '  appearance:none;-webkit-appearance:none;',
                '  cursor:pointer;',
                '}',
                '.lc-select:focus{outline:2px solid var(--clr-accent,#3da8ff);outline-offset:0;}',
                '.lc-drawer-btn{',
                '  display:inline-flex;align-items:center;justify-content:center;',
                '  height:28px;padding:0 10px;',
                '  background:var(--clr-surface-alt,#22253a);',
                '  border:1px solid var(--clr-border,#2a2d3e);',
                '  color:var(--clr-text-muted,#8890b0);',
                '  font-size:11px;font-weight:600;letter-spacing:0.04em;',
                '  font-family:inherit;text-transform:uppercase;',
                '  cursor:pointer;',
                '  transition:border-color 100ms,color 100ms;',
                '}',
                '.lc-drawer-btn:hover{',
                '  border-color:var(--clr-accent,#3da8ff);color:var(--clr-accent,#3da8ff);',
                '}',

                /* Context detail rows */
                '.lc-ctx-detail{display:flex;flex-direction:column;gap:3px;}',
                '.lc-cd-row{display:flex;gap:6px;font-size:11px;}',
                '.lc-cd-key{',
                '  width:72px;flex-shrink:0;',
                '  color:var(--clr-text-dim,#50536a);',
                '  font-size:10px;letter-spacing:0.04em;text-transform:uppercase;',
                '}',
                '.lc-cd-val{color:var(--clr-text-muted,#8890b0);word-break:break-word;}',

                /* UX guidance */
                '.lc-ux-info{display:flex;flex-direction:column;gap:4px;}',
                '.lc-ux-section{',
                '  font-size:10px;font-weight:600;letter-spacing:0.05em;text-transform:uppercase;',
                '  color:var(--clr-accent,#3da8ff);margin-top:6px;',
                '}',
                '.lc-ux-rule{',
                '  font-size:11px;color:var(--clr-text-dim,#50536a);line-height:1.5;',
                '}',
            ].join('\n');

            var style         = document.createElement('style');
            style.id          = id;
            style.textContent = css;
            document.head.appendChild(style);
        } catch (e) {
            console.error('[themeRand:ui-styles] injectStyles() failed:', e);
        }
    };

    console.log('[themeRand:ui-styles] loaded');

}(window.LiveCSS = window.LiveCSS || {}));
