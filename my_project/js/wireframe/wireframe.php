<?php
/*
 * Crissy's Style Tool -- Wireframe Tool (standalone page)
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * MIT License -- see LICENSE file for full text.
 *
 * Served at:  js/wireframe/wireframe.php
 * Opened by:  File menu -> Wireframes in index.php
 * Close btn:  navigates back to the calling page (history.back())
 *
 * This page is fully self-contained. It has no dependency on index.php,
 * app.js, cdn-loader, or CodeMirror. The ES module sub-files
 * (init.js, state.js, etc.) are imported directly via a module script.
 * Works on any server (PHP built-in, Node WASM server, nginx).
 */

// Resolve the base URL so CSS/font paths work from any server root.
// index.php lives two directories up from this file.
$base = rtrim(dirname(dirname(dirname($_SERVER['SCRIPT_NAME']))), '/');
?><!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wireframes -- Crissy's Style Tool</title>

    <!-- Page-level reset and fonts matching the main app -->
    <style>
        *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
        html, body { height: 100%; overflow: hidden; }
        body {
            background: #080412;
            color: #eceaf6;
            font-family: 'Segoe UI', system-ui, sans-serif;
            font-size: 13px;
        }

        /* In standalone mode the overlay IS the page.
           Use an explicit height so that .wf-modal { height:100% } resolves
           correctly and the flex layout fills the full viewport. */
        .wf-overlay {
            position: static !important;
            background: none !important;
            height: 100vh !important;
            z-index: auto !important;
        }

        /* "Close" becomes "Back to Editor" in the toolbar */
        #wfCloseBtn::before { content: '\2190  '; font-size: 11px; }
    </style>

    <!-- Wireframe component styles -->
    <link rel="stylesheet" href="<?= $base ?>/css/wireframe.css">

    <!-- Dev event logger: forwards clicks, JS errors, and module load events
         to /dev-log on the server so they appear in server.log.
         Remove this tag for production builds. -->
    <script src="../wf-debug.js"></script>
</head>
<body data-wf-standalone="1">

    <!-- Exact same DOM structure and IDs as the overlay in index.php.
         init.js finds elements by ID -- nothing changes on the JS side. -->
    <div class="wf-overlay" id="wireframeOverlay">
        <div class="wf-modal">
            <div class="wf-toolbar">
                <span class="wf-toolbar-title">Wireframes</span>
                <button class="wf-btn" id="wfAddBtn">+ Add Element</button>
                <button class="wf-btn" id="wfClearBtn">Clear All</button>
                <button class="wf-btn" id="wfSaveBtn">Save JSON</button>
                <button class="wf-btn" id="wfLoadBtn">Load JSON</button>
                <button class="wf-btn" id="wfContextBtn">Copy Context</button>
                <input type="file" id="wfFileInput" accept=".json,.wf.json" style="display:none">
                <button class="wf-btn wf-btn-close" id="wfCloseBtn">Back to Editor</button>
            </div>
            <div class="wf-body">
                <div class="wf-canvas-area" id="wfCanvasArea">
                    <div class="wf-ruler-corner"></div>
                    <div class="wf-ruler-h" id="wfRulerH"></div>
                    <div class="wf-ruler-v" id="wfRulerV"></div>
                    <div id="wfCanvas">
                        <span class="wf-canvas-info">1200 &times; 900</span>
                    </div>
                </div>
                <div class="wf-props-panel" id="wfProps">
                    <div class="wf-props-empty">Click an element to select it</div>
                </div>
            </div>
        </div>
    </div>

    <!--
        Inline module script: imports directly from sibling files.
        Because the script is inline, the module base URL is this page's URL,
        so './init.js' resolves to js/wireframe/init.js correctly on any server.
    -->

    <script type="module">
        import { init, getState, loadState } from './init.js';

        // Expose API for any external tooling (e.g. vscode-bridge)
        window.LiveCSS = window.LiveCSS || {};
        window.LiveCSS.wireframe = { init: init, getState: getState, loadState: loadState };

        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', init);
        } else {
            init();
        }
    </script>
</body>
</html>
