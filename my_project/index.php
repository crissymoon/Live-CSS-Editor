<?php
/*
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * https://xcaliburmoon.net/
 *
 * MIT License -- Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
/**
 * Live CSS Editor -- index.php
 * Thin orchestrator: loads data from data/, renders the HTML shell,
 * then hands off to modular CSS (css/) and JS (js/) files.
 */

require_once 'data/css-properties.php';   // $cssProperties
require_once 'data/property-values.php';  // $propertyValues
require_once 'data/default-content.php';  // $defaultHtml, $defaultCss, $defaultJs

// Flat sorted list of all property names for the JS fuzzy autocomplete
$allPropertyNames = [];
foreach ($cssProperties as $props) {
    foreach ($props as $prop) {
        if (!in_array($prop, $allPropertyNames)) {
            $allPropertyNames[] = $prop;
        }
    }
}
sort($allPropertyNames);
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="format-detection" content="telephone=no, date=no, email=no, address=no">
    <title>Crissy's Style Tool</title>
    <link rel="stylesheet" href="css/app-loader.css">
    <link rel="stylesheet" href="style.css">
    <link rel="stylesheet" href="css/wireframe.css">
    <link rel="stylesheet" href="css/agent.css">
    <link rel="stylesheet" href="css/ai-chat.css">

    <!-- CDN fallback loader — handles CodeMirror CSS + JS from multiple sources -->
    <!-- Actual CodeMirror assets are injected at runtime by js/cdn-loader.js    -->
</head>
<body>

    <header class="app-header">
        <nav class="app-menubar" id="appMenubar">
            <div class="menu-item">
                <span class="menu-label">File</span>
                <ul class="menu-dropdown">
                    <li><button id="saveBtn">Save</button></li>
                    <li><button id="loadBtn">Load</button></li>
                    <li class="menu-sep"></li>
                    <li><button id="syncToBridgeBtn">Sync to Bridge</button></li>
                    <li><button id="pullFromVscodeBtn">Pull from VSCode</button></li>
                    <li><button id="shareBtn">Share Design Link</button></li>
                    <li class="menu-sep"></li>
                    <li><button id="resetBtn">Reset</button></li>
                    <li><button id="resetLayoutBtn">Reset Layout</button></li>
                    <li class="menu-sep"></li>
                    <li><button id="propertiesBtn">Properties</button></li>
                    <li class="menu-sep"></li>
                    <li><button onclick="window.location.href='js/wireframe/wireframe.php'">Wireframes</button></li>
                </ul>
            </div>
            <div class="menu-item">
                <span class="menu-label">Edit</span>
                <ul class="menu-dropdown">
                    <li><button id="harmonyBtn">Harmony</button></li>
                    <li><button id="guidesBtn">Guides</button></li>
                    <li><button id="wireframeBtn" onclick="window.location.href='js/wireframe/wireframe.php'">Wireframes</button></li>
                    <li><button id="widgetsBtn" class="menu-btn-active">Widgets</button></li>
                </ul>
            </div>
        </nav>
        <div class="header-center">
            <h1 class="app-title">Crissy's Style Tool</h1>
            <span id="autosaveStatus" class="autosave-status"></span>
        </div>
        <div class="header-right">
            <button id="helpBtn" class="btn-action btn-help" title="Copilot design help" style="display:none">? Help</button>
            <button id="vscodeBridgeToggle" class="btn-action btn-vscode-bridge" title="Toggle VSCode Copilot Bridge" aria-pressed="false">VSCode Bridge</button>
            <button id="agentBtn" class="btn-action btn-agent" title="Open Code Agent">Agent</button>
        </div>
    </header>

    <!-- Session history restore bar -->
    <div id="sessionRestoreBar" class="session-restore-bar hidden">
        <span class="srb-label">Restore session:</span>
        <select id="sessionHistorySelect" class="srb-select"></select>
        <button id="sessionRestoreBtn" class="srb-btn srb-restore">Restore</button>
        <button id="sessionDismissBtn" class="srb-btn srb-dismiss">Dismiss</button>
    </div>

    <!-- Save Modal -->
    <div class="modal-overlay hidden" id="saveModal">
        <div class="modal-box">
            <div class="modal-header">
                <span class="modal-title">Save Project</span>
                <button class="modal-close" id="saveModalClose">&times;</button>
            </div>
            <div class="modal-body">
                <label for="saveNameInput" class="modal-label">Project Name:</label>
                <input type="text" id="saveNameInput" class="modal-input" placeholder="my-project" maxlength="80">
                <div class="modal-existing" id="saveExistingList"></div>
            </div>
            <div class="modal-footer">
                <button class="btn-action" id="saveConfirmBtn">Save</button>
                <button class="btn-action" id="saveCancelBtn">Cancel</button>
            </div>
        </div>
    </div>

    <!-- Load Modal -->
    <div class="modal-overlay hidden" id="loadModal">
        <div class="modal-box">
            <div class="modal-header">
                <span class="modal-title">Load Project</span>
                <button class="modal-close" id="loadModalClose">&times;</button>
            </div>
            <div class="modal-body">
                <div class="modal-slots" id="loadSlotList"></div>
                <p class="modal-empty hidden" id="loadEmptyMsg">No saved projects found.</p>
            </div>
            <div class="modal-footer">
                <button class="btn-action" id="loadCancelBtn">Cancel</button>
            </div>
        </div>
    </div>

    <main class="editor-layout">
        <!-- Transparent overlay to capture mouse events during gutter drag -->
        <div class="drag-overlay" id="dragOverlay"></div>

        <section class="editor-panel" id="jsPanel">
            <div class="panel-header">
                <span class="panel-label">JS</span>
                <button class="panel-hist-btn" id="jsUndoBtn" title="Undo">&#8592;</button>
                <button class="panel-hist-btn" id="jsRedoBtn" title="Redo">&#8594;</button>
                <button class="panel-min-btn" title="Minimize">&#8722;</button>
                <div class="panel-drag-handle" title="Drag panel"></div>
            </div>
            <div class="panel-search" id="jsSearch">
                <input class="ps-input" type="text" placeholder="Find..." autocomplete="off" spellcheck="false">
                <span class="ps-count"></span>
                <button class="ps-btn ps-prev" title="Previous">&#8593;</button>
                <button class="ps-btn ps-next" title="Next">&#8595;</button>
                <button class="ps-btn ps-close" title="Close">&#215;</button>
            </div>
            <div class="panel-body">
                <textarea id="jsEditor"></textarea>
            </div>
        </section>

        <section class="editor-panel" id="htmlPanel">
            <div class="panel-header">
                <span class="panel-label">HTML</span>
                <button class="panel-hist-btn" id="htmlUndoBtn" title="Undo">&#8592;</button>
                <button class="panel-hist-btn" id="htmlRedoBtn" title="Redo">&#8594;</button>
                <button class="panel-min-btn" title="Minimize">&#8722;</button>
                <div class="panel-drag-handle" title="Drag panel"></div>
            </div>
            <div class="panel-search" id="htmlSearch">
                <input class="ps-input" type="text" placeholder="Find..." autocomplete="off" spellcheck="false">
                <span class="ps-count"></span>
                <button class="ps-btn ps-prev" title="Previous">&#8593;</button>
                <button class="ps-btn ps-next" title="Next">&#8595;</button>
                <button class="ps-btn ps-close" title="Close">&#215;</button>
            </div>
            <div class="panel-body">
                <textarea id="htmlEditor"><?php echo htmlspecialchars($defaultHtml); ?></textarea>
            </div>
        </section>

        <section class="editor-panel" id="cssPanel">
            <div class="panel-header">
                <span class="panel-label">CSS</span>
                <button class="panel-hist-btn" id="cssUndoBtn" title="Undo">&#8592;</button>
                <button class="panel-hist-btn" id="cssRedoBtn" title="Redo">&#8594;</button>
                <button class="panel-min-btn" title="Minimize">&#8722;</button>
                <div class="panel-drag-handle" title="Drag panel"></div>
            </div>
            <div class="panel-search" id="cssSearch">
                <input class="ps-input" type="text" placeholder="Find..." autocomplete="off" spellcheck="false">
                <span class="ps-count"></span>
                <button class="ps-btn ps-prev" title="Previous">&#8593;</button>
                <button class="ps-btn ps-next" title="Next">&#8595;</button>
                <button class="ps-btn ps-close" title="Close">&#215;</button>
            </div>
            <div class="panel-body">
                <textarea id="cssEditor"><?php echo htmlspecialchars($defaultCss); ?></textarea>
            </div>
        </section>

        <section class="preview-panel" id="previewPanel">
            <div class="panel-header">
                <span class="panel-label">Live Preview</span>
                <button class="panel-min-btn" title="Minimize">&#8722;</button>
                <div class="panel-drag-handle" title="Drag panel"></div>
            </div>
            <div class="panel-body">
                <iframe id="previewFrame"></iframe>
            </div>
        </section>
    </main>

    <!-- Minimized panel taskbar (hidden when empty) -->
    <div id="panel-taskbar"></div>

    <!-- Floating Color Harmony Tool -->
    <div class="harmony-tool hidden" id="harmonyTool">
        <div class="harmony-header">
            <span class="harmony-title">Color Harmony</span>
            <button class="harmony-close" id="harmonyClose">&times;</button>
        </div>
        <div class="harmony-modes">
            <button class="harmony-mode-btn active" data-mode="complementary">Complementary</button>
            <button class="harmony-mode-btn" data-mode="analogous">Analogous</button>
            <button class="harmony-mode-btn" data-mode="triadic">Triadic</button>
            <button class="harmony-mode-btn" data-mode="split-comp">Split-Comp</button>
            <button class="harmony-mode-btn" data-mode="tetradic">Tetradic</button>
            <button class="harmony-mode-btn" data-mode="square">Square</button>
            <button class="harmony-mode-btn" data-mode="monochromatic">Mono</button>
        </div>
        <div class="harmony-swatches" id="harmonySwatches"></div>
        <div class="harmony-picker-row">
            <span class="harmony-picker-label">Base Color</span>
            <input type="color" class="harmony-picker-input" id="harmonyPicker" value="#4d31bf">
            <span class="harmony-hex-display" id="harmonyHexDisplay">#4d31bf</span>
        </div>
    </div>



    <!-- Indent Guide Settings Panel -->
    <div class="guide-tool hidden" id="guideTool">
        <div class="guide-header">
            <span class="guide-title">Indent Guides</span>
            <button class="guide-close" id="guideClose">&times;</button>
        </div>
        <div class="guide-body">
            <label class="guide-row">
                <span class="guide-label">Visible</span>
                <input type="checkbox" id="guideToggle" class="guide-check" checked>
            </label>
            <label class="guide-row">
                <span class="guide-label">Color</span>
                <input type="color" id="guideColor" class="guide-color-input" value="#5a41b4">
            </label>
            <label class="guide-row">
                <span class="guide-label">Opacity</span>
                <input type="range" id="guideOpacity" class="guide-slider" min="0" max="100" value="18">
                <span class="guide-slider-val" id="guideOpacityVal">18%</span>
            </label>
            <label class="guide-row">
                <span class="guide-label">Thickness</span>
                <input type="range" id="guideThickness" class="guide-slider" min="1" max="4" value="1">
                <span class="guide-slider-val" id="guideThicknessVal">1px</span>
            </label>
            <label class="guide-row">
                <span class="guide-label">Style</span>
                <select id="guideStyle" class="guide-select">
                    <option value="solid">Solid</option>
                    <option value="dashed">Dashed</option>
                    <option value="dotted">Dotted</option>
                </select>
            </label>
            <label class="guide-row">
                <span class="guide-label">Step (cols)</span>
                <select id="guideStep" class="guide-select">
                    <option value="2" selected>2</option>
                    <option value="4">4</option>
                    <option value="8">8</option>
                </select>
            </label>
            <div style="border-top:1px solid #2a1a55;margin:4px 0 2px;"></div>
            <label class="guide-row">
                <span class="guide-label">Col ruler</span>
                <input type="checkbox" id="guideRulerToggle" class="guide-check">
            </label>
            <label class="guide-row">
                <span class="guide-label">Col length</span>
                <input type="number" id="guideRulerCol" class="guide-number-input" min="1" max="999" value="96">
            </label>
            <label class="guide-row">
                <span class="guide-label">Ruler color</span>
                <input type="color" id="guideRulerColor" class="guide-color-input" value="#4d31bf">
            </label>
            <label class="guide-row">
                <span class="guide-label">Ruler opacity</span>
                <input type="range" id="guideRulerOpacity" class="guide-slider" min="0" max="100" value="30">
                <span class="guide-slider-val" id="guideRulerOpacityVal">30%</span>
            </label>
            <div style="border-top:1px solid #2a1a55;margin:4px 0 2px;"></div>
            <label class="guide-row">
                <span class="guide-label">Search outline</span>
                <input type="checkbox" id="guideSearchOutline" class="guide-check">
            </label>
        </div>
    </div>

    <!-- Help Modal: Prompt Builder -->
    <div class="modal-overlay hidden" id="helpModal">
        <div class="modal-box help-modal-box">
            <div class="modal-header">
                <span class="modal-title">Create a Copilot Prompt</span>
                <button class="modal-close" id="helpModalClose">&times;</button>
            </div>
            <div class="modal-body help-modal-body">

                <p class="help-intro">
                    Describe the design change you want and click Create Prompt. Copy the result and paste it into Copilot. Copilot will edit the files in vscode-bridge/projects/ and push them to the tool for you.
                </p>

                <div class="help-input-row">
                    <label for="helpRequestInput" class="help-label">What do you want to do?</label>
                    <textarea id="helpRequestInput" class="help-textarea" rows="3" placeholder="e.g. create a pricing section with three tiers, or fix the header layout on mobile"></textarea>
                </div>
                <div class="help-actions-row">
                    <button id="helpCreateBtn" class="help-create-btn">Create Prompt</button>
                </div>
                <div class="help-output-wrap hidden" id="helpOutputWrap">
                    <div class="help-output-header">
                        <span class="help-output-label">Generated prompt</span>
                        <button id="helpCopyBtn" class="help-copy-btn">Copy</button>
                    </div>
                    <pre class="help-prompt-code" id="helpOutputPre"></pre>
                </div>
            </div>
        </div>
    </div>
    <!-- End Help Modal -->

    <!-- Fuzzy autocomplete dropdown for CSS properties -->
    <div class="fuzzy-dropdown hidden" id="fuzzyDropdown"></div>

    <!-- Wireframe Tool Modal -->
    <!-- Wireframe tool lives at js/wireframe/wireframe.php (standalone page) -->

    <!-- Floating properties reference tool -->
    <div class="prop-tool hidden" id="propertiesToolPanel">
        <div class="prop-tool-header">
            <span class="prop-tool-title">Properties</span>
            <button class="prop-tool-close" id="propToolClose">&times;</button>
        </div>
        <div class="prop-tool-tabs">
            <button class="prop-tab active" data-tab="css">CSS</button>
            <button class="prop-tab" data-tab="js">JS</button>
            <button class="prop-tab" data-tab="html">HTML</button>
        </div>
        <div class="prop-tool-search">
            <input type="text" class="prop-search-input" id="propSearchInput" placeholder="Search..." autocomplete="off" spellcheck="false">
        </div>
        <ul class="prop-list" id="propList"></ul>
        <div class="prop-tool-footer">
            <span class="prop-value-hint" id="propValueHint"></span>
            <button class="prop-insert-btn" id="propInsertBtn">Insert</button>
        </div>
    </div>

    <!-- =====================================================
         PHP-to-JS data bridge — consumed by js/app.js
         ===================================================== -->
    <script>
    window.LiveCSSData = {
        propertyValues:   <?= json_encode($propertyValues,   JSON_UNESCAPED_UNICODE) ?>,
        allCssProperties: <?= json_encode($allPropertyNames, JSON_UNESCAPED_UNICODE) ?>,
        defaultHtml:      <?= json_encode($defaultHtml,      JSON_UNESCAPED_UNICODE) ?>,
        defaultCss:       <?= json_encode($defaultCss,       JSON_UNESCAPED_UNICODE) ?>,
        defaultJs:        <?= json_encode($defaultJs,        JSON_UNESCAPED_UNICODE) ?>
    };
    </script>

    <style>
    /* ---- Help Modal ---- */
    .help-modal-box {
        max-width: 580px;
        width: 96vw;
    }
    .help-modal-body {
        padding: 22px 24px;
        display: flex;
        flex-direction: column;
        gap: 16px;
    }
    .help-intro {
        font-size: 13px;
        color: #c7c7f0;
        line-height: 1.55;
        margin: 0;
    }
    .help-input-row {
        display: flex;
        flex-direction: column;
        gap: 6px;
    }
    .help-label {
        font-size: 12px;
        color: #8888a0;
        letter-spacing: 0.03em;
    }
    .help-textarea {
        background: #0c071c;
        border: 1px solid #2d1c6e;
        color: #eceaf6;
        font-family: inherit;
        font-size: 13px;
        padding: 10px 12px;
        resize: vertical;
        outline: none;
        line-height: 1.5;
        width: 100%;
        box-sizing: border-box;
    }
    .help-textarea:focus {
        border-color: #4d31bf;
    }
    .help-actions-row {
        display: flex;
        justify-content: flex-end;
    }
    .help-create-btn {
        background: #4d31bf;
        border: 1px solid #6d51df;
        color: #fff;
        font-size: 13px;
        font-family: inherit;
        padding: 7px 20px;
        cursor: pointer;
        letter-spacing: 0.04em;
        transition: background 0.12s;
    }
    .help-create-btn:hover {
        background: #6d51df;
    }
    .help-output-wrap {
        display: flex;
        flex-direction: column;
        gap: 8px;
    }
    .help-output-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
    }
    .help-output-label {
        font-size: 12px;
        color: #8888a0;
        letter-spacing: 0.03em;
    }
    .help-prompt-code {
        background: #1b1825;
        border: 1px solid #2d1c6e;
        color: #c7c7f0;
        font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
        font-size: 11px;
        line-height: 1.6;
        padding: 12px 14px;
        margin: 0;
        white-space: pre-wrap;
        word-break: break-word;
        max-height: 300px;
        overflow-y: auto;
        user-select: text;
    }
    .help-copy-btn {
        background: transparent;
        border: 1px solid #4d31bf;
        color: #a5a5c0;
        font-size: 11px;
        font-family: inherit;
        padding: 3px 12px;
        cursor: pointer;
        letter-spacing: 0.04em;
        white-space: nowrap;
        flex-shrink: 0;
        transition: background 0.12s, color 0.12s;
    }
    .help-copy-btn:hover {
        background: #4d31bf;
        color: #fff;
    }
    .help-copy-btn.copied {
        background: #065f46;
        border-color: #10b981;
        color: #10b981;
    }
    .btn-help {
        border-color: #2d1c6e;
        color: #8888a0;
        font-size: 13px;
        padding: 6px 12px;
        margin-right: 8px;
    }
    .btn-help:hover {
        background: #2d1c6e;
        color: #eceaf6;
    }
    </style>

    <!-- JS modules (load order: utilities first, app last) -->
    <!-- app-loader injects the loading overlay before anything else renders -->
    <script src="js/app-loader.js"></script>
    <!-- env-detect must be first: sets LiveCSS.env.basePath and LiveCSS.env.resolve() -->
    <script src="js/env-detect.js"></script>
    <script src="js/cdn-loader.js"></script>
    <script src="js/utils.js"></script>
    <script src="js/storage.js"></script>
    <script src="js/editor/goto-css.js"></script>
    <script src="js/editor/lint.js"></script>
    <script src="js/editor/preview.js"></script>
    <script src="js/editor.js"></script>
    <script src="js/property-lookup.js"></script>
    <script src="js/fuzzy.js"></script>
    <script src="js/color-swatch.js"></script>
    <script src="js/size-slider.js"></script>
    <script src="js/color-harmony.js"></script>
    <script src="js/modal-save.js"></script>
    <script src="js/modal-load.js"></script>
    <script src="js/gutter.js"></script>
    <script src="js/editor-search.js"></script>
    <script src="js/indent-guide.js"></script>
    <script src="js/app.js" defer></script>
    <script src="js/agent/agent-core.js"></script>
    <script src="js/agent/agent-ui.js"></script>
    <script src="js/agent/agent-run.js"></script>
    <script src="js/agent/agent-diff.js"></script>
    <script src="js/agent/agent-context.js"></script>
    <script src="js/agent/agent-prompts.js"></script>
    <script src="js/agent/agent-chat.js"></script>
    <script src="js/agent/agent-window.js"></script>
    <script src="js/agent/agent-neural.js"></script>
    <script src="js/agent.js"></script>
    <script src="js/ai-chat.js"></script>
    <!-- VSCode Copilot bridge: two-way sync with MCP server -->
    <script src="vscode-bridge/js/bridge-sync.js"></script>

    <!-- NOTE: bridge-sync.js already wires vscodeBridgeToggle; do NOT
         double-wire it here or BridgeSync.toggle() would fire twice per click. -->

    <script>
    // Menu bar dropdown toggle
    (function () {
        var menubar = document.getElementById('appMenubar');
        if (!menubar) { return; }

        function closeAll() {
            menubar.querySelectorAll('.menu-item.menu-open').forEach(function (m) {
                m.classList.remove('menu-open');
            });
        }

        menubar.querySelectorAll('.menu-item').forEach(function (item) {
            item.addEventListener('click', function (e) {
                var wasOpen = item.classList.contains('menu-open');
                closeAll();
                if (!wasOpen) { item.classList.add('menu-open'); }
                e.stopPropagation();
            });
            // Close when a dropdown button is clicked
            item.querySelectorAll('.menu-dropdown button').forEach(function (btn) {
                btn.addEventListener('click', function () { closeAll(); });
            });
        });

        document.addEventListener('click', closeAll);
        document.addEventListener('keydown', function (e) {
            if (e.key === 'Escape') { closeAll(); }
        });
    }());

    // Help modal - prompt builder
    (function () {
        var btn        = document.getElementById('helpBtn');
        var modal      = document.getElementById('helpModal');
        var close      = document.getElementById('helpModalClose');
        var input      = document.getElementById('helpRequestInput');
        var createBtn  = document.getElementById('helpCreateBtn');
        var outputWrap = document.getElementById('helpOutputWrap');
        var outputPre  = document.getElementById('helpOutputPre');
        var copyBtn    = document.getElementById('helpCopyBtn');

        if (!btn || !modal || !close) {
            console.error('[Help] Missing helpBtn, helpModal, or helpModalClose element');
            return;
        }

        function openHelp()  { modal.classList.remove('hidden'); }
        function closeHelp() { modal.classList.add('hidden'); }

        btn.addEventListener('click', openHelp);
        close.addEventListener('click', closeHelp);
        modal.addEventListener('click', function (e) {
            if (e.target === modal) { closeHelp(); }
        });
        document.addEventListener('keydown', function (e) {
            if (e.key === 'Escape' && !modal.classList.contains('hidden')) {
                closeHelp();
            }
        });

        function buildPrompt(request) {
            var req = (request || '').trim() || '[describe what you want to do]';
            return [
                "I am working in Crissy's Style Tool, a live HTML/CSS/JS editor running at http://localhost:8080.",
                '',
                'PROJECT ROOT: /Users/mac/Documents/live-css',
                '',
                'The design lives in three files inside vscode-bridge/projects/:',
                '  vscode-bridge/projects/html-editor.html   -- the HTML markup',
                '  vscode-bridge/projects/css-editor.css     -- styles for the design',
                '  vscode-bridge/projects/js-editor.js       -- optional JS (leave empty if not needed)',
                '',
                'Image assets are in vscode-bridge/projects/custom_design_assets/ and should be',
                'referenced with absolute paths like /vscode-bridge/projects/custom_design_assets/...',
                '',
                'After editing the files, run this command to push them to the tool:',
                '  php push-pull.php push',
                '',
                'The tool will auto-detect the update and load it into all three editors.',
                'To pull the current state from the tool back to files:',
                '  php push-pull.php pull',
                '',
                'To see all saved projects:',
                '  php push-pull.php list',
                '',
                'DESIGN REQUEST: ' + req,
            ].join('\n');
        }

        createBtn.addEventListener('click', function () {
            var prompt = buildPrompt(input.value);
            outputPre.textContent = prompt;
            outputWrap.classList.remove('hidden');
            outputPre.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        });

        input.addEventListener('keydown', function (e) {
            if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') { createBtn.click(); }
        });

        copyBtn.addEventListener('click', function () {
            var text = outputPre.textContent || '';
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(text).then(function () {
                    copyBtn.textContent = 'Copied!';
                    copyBtn.classList.add('copied');
                    setTimeout(function () {
                        copyBtn.textContent = 'Copy';
                        copyBtn.classList.remove('copied');
                    }, 2000);
                }).catch(function (err) {
                    console.error('[Help] clipboard.writeText failed:', err);
                    fallbackCopy(text);
                });
            } else {
                fallbackCopy(text);
            }
        });

        function fallbackCopy(text) {
            try {
                var ta = document.createElement('textarea');
                ta.value = text;
                ta.style.position = 'fixed';
                ta.style.opacity  = '0';
                document.body.appendChild(ta);
                ta.focus();
                ta.select();
                var ok = document.execCommand('copy');
                document.body.removeChild(ta);
                if (ok) {
                    copyBtn.textContent = 'Copied!';
                    copyBtn.classList.add('copied');
                    setTimeout(function () {
                        copyBtn.textContent = 'Copy';
                        copyBtn.classList.remove('copied');
                    }, 2000);
                } else {
                    console.error('[Help] fallbackCopy execCommand returned false');
                }
            } catch (e) {
                console.error('[Help] fallbackCopy threw:', e);
            }
        }
    }());

    // AI Chat init - runs after DOM is ready and all scripts are loaded
    (function () {
        try {
            if (window.LiveCSS && window.LiveCSS.aiChat && typeof window.LiveCSS.aiChat.init === 'function') {
                window.LiveCSS.aiChat.init();
                console.log('[App] LiveCSS.aiChat initialized');
            } else {
                console.error('[App] LiveCSS.aiChat not available - check js/ai-chat.js loaded correctly');
            }
        } catch (e) {
            console.error('[App] LiveCSS.aiChat.init() threw:', e.message, e);
        }
    }());
    </script>

</body>
</html>
