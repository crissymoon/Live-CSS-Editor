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
    <link rel="stylesheet" href="style.css">
    <link rel="stylesheet" href="css/native-bridge.css">
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
                    <li><button id="resetBtn">Reset</button></li>
                    <li><button id="resetLayoutBtn">Reset Layout</button></li>
                    <li class="menu-sep"></li>
                    <li><button id="propertiesBtn">Properties</button></li>
                </ul>
            </div>
            <div class="menu-item">
                <span class="menu-label">Edit</span>
                <ul class="menu-dropdown">
                    <li><button id="harmonyBtn">Harmony</button></li>
                    <li><button id="guidesBtn">Guides</button></li>
                    <li><button id="wireframeBtn">Wireframes</button></li>
                    <li><button id="widgetsBtn" class="menu-btn-active">Widgets</button></li>
                </ul>
            </div>
        </nav>
        <div class="header-center">
            <h1 class="app-title">Crissy's Style Tool</h1>
            <span id="autosaveStatus" class="autosave-status"></span>
        </div>
        <div class="header-right">
            <button id="helpBtn" class="btn-action btn-help" title="Copilot section help">? Help</button>
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

    <!-- Minimized panel taskbar -->
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

    <!-- Help Modal: Copilot + Page Builder Sections -->
    <div class="modal-overlay hidden" id="helpModal">
        <div class="modal-box help-modal-box">
            <div class="modal-header">
                <span class="modal-title">Copilot + Page Builder Sections</span>
                <button class="modal-close" id="helpModalClose">&times;</button>
            </div>
            <div class="modal-body help-modal-body">

                <p class="help-intro">
                    Use VSCode Copilot (Agent mode) to create, edit, audit, and preview page builder sections.
                    Copy a prompt below, fill in the bracketed parts, and paste it into Copilot Chat.
                    Copilot will read the schema file automatically and produce valid JSON.
                </p>

                <p class="help-tip">
                    Tip: Copilot Agent mode can read and write files on disk directly.
                    New template files appear in the section library the next time you open the composer.
                </p>

                <div class="help-prompt-list">

                    <div class="help-prompt-card">
                        <div class="help-prompt-card-header">
                            <span class="help-prompt-title">Create a new section template</span>
                            <button class="help-copy-btn" data-target="prompt-create">Copy</button>
                        </div>
                        <p class="help-prompt-desc">Creates a reusable template in the section library. Replace the MY REQUEST line with what you want.</p>
                        <pre class="help-prompt-code" id="prompt-create">I am working on a PHP page builder called Crissy's Style Tool.
PROJECT ROOT: /Users/mac/Documents/live-css

Read vscode-bridge/context/section-schema.md before doing anything else.

MY REQUEST: [describe the section, e.g. "a pricing table with three tiers: Free, Pro, and Enterprise"]
SECTION TYPE: [header / footer / section / panel]
SAVE LOCATION: page-builder/sections/[headers|footers|sections|panels]/[name].json

Requirements:
- Valid JSON only, no comments
- All numeric values as quoted strings with units ("16px" not 16)
- Every block has a unique id
- Every heading block has a tag key
- Every button block has an href key
- Include _meta at the top
- Use dark palette from schema unless told otherwise
- All JS fetch calls need .catch(err => console.error('[section] error:', err))

After saving: list the file path, block ids, and any notes for overrides.json.</pre>
                    </div>

                    <div class="help-prompt-card">
                        <div class="help-prompt-card-header">
                            <span class="help-prompt-title">Add a section to a specific page</span>
                            <button class="help-copy-btn" data-target="prompt-add">Copy</button>
                        </div>
                        <p class="help-prompt-desc">Adds a new section directly into a live page and updates the page.json manifest. Replace PAGE NAME and the section description.</p>
                        <pre class="help-prompt-code" id="prompt-add">I am working on the Crissy Style Tool page builder.
PROJECT ROOT: /Users/mac/Documents/live-css

Read vscode-bridge/context/section-schema.md first.

PAGE NAME: [page-folder-name, e.g. "demo"]
PAGE DIRECTORY: page-builder/pages/[page-name]/

TASK:
1. Read page-builder/pages/[page-name]/page.json to see the current manifest.
2. Create a new section file: section-[short-slug].json in that page directory.
3. Add it to page.json sections array before the footer entry:
   { "id": "pb-[short-slug]", "file": "section-[short-slug].json", "type": "section", "label": "[Label]" }
4. Do NOT include _meta in page-specific files.

THE SECTION I WANT: [describe it]
Use dark palette (bg #0a0a14, text #e8e8f0, accent #6366f1).
All block ids must be unique and not conflict with existing ids in the page.

After finishing: list the new file path, updated page.json sections array, and all block ids used.</pre>
                    </div>

                    <div class="help-prompt-card">
                        <div class="help-prompt-card-header">
                            <span class="help-prompt-title">Preview a section in the browser</span>
                            <button class="help-copy-btn" data-target="prompt-preview">Copy</button>
                        </div>
                        <p class="help-prompt-desc">Generates a standalone HTML preview file for a single section. Replace the file path.</p>
                        <pre class="help-prompt-code" id="prompt-preview">I am working on the Crissy Style Tool page builder.
PROJECT ROOT: /Users/mac/Documents/live-css

Read vscode-bridge/context/section-schema.md first.
Also read page-builder/build.php so you understand how sections are rendered.

SECTION FILE: page-builder/[sections/type/name.json OR pages/page-name/file.json]

TASK:
Generate a standalone HTML preview file for this single section.
Save it to page-builder/pages/_preview/index.html

Requirements:
- Inline all CSS as a style block (no external files)
- Render only this one section, centered on a dark background (#08080f)
- Add a small label at the top showing the section file path
- Include a browser console.error fallback if any dynamic rendering fails

After saving, tell me the URL to open:
http://localhost:8080/page-builder/pages/_preview/index.html</pre>
                    </div>

                    <div class="help-prompt-card">
                        <div class="help-prompt-card-header">
                            <span class="help-prompt-title">Audit a section file for errors</span>
                            <button class="help-copy-btn" data-target="prompt-audit">Copy</button>
                        </div>
                        <p class="help-prompt-desc">Checks a section JSON file against the full schema and reports every issue with the fix needed.</p>
                        <pre class="help-prompt-code" id="prompt-audit">I am working on the Crissy Style Tool page builder.
PROJECT ROOT: /Users/mac/Documents/live-css

Read vscode-bridge/context/section-schema.md first.

FILE TO AUDIT: page-builder/[sections/type/name.json OR pages/page-name/file.json]

Check for:
1. Valid JSON (no trailing commas, all strings quoted)
2. Required top-level keys for the section type
3. All blocks have unique non-empty id values
4. Every heading block has a tag key
5. Every button block has an href key
6. Every card block has a non-empty children array
7. All numeric values are quoted strings with units ("16px" not 16)
8. _meta present if template, absent if page-specific

Report each issue as:
  PATH: [json path to the field]
  PROBLEM: [what is wrong]
  FIX: [the corrected value or structure]

If no issues found, confirm the file is valid and list all block ids.</pre>
                    </div>

                    <div class="help-prompt-card">
                        <div class="help-prompt-card-header">
                            <span class="help-prompt-title">Convert active stylesheet into section defaults</span>
                            <button class="help-copy-btn" data-target="prompt-convert">Copy</button>
                        </div>
                        <p class="help-prompt-desc">Reads the active stylesheet and style-context.txt to pull colors and typography into a new section template. Replace the stylesheet name and section description.</p>
                        <pre class="help-prompt-code" id="prompt-convert">I am working on the Crissy Style Tool page builder.
PROJECT ROOT: /Users/mac/Documents/live-css

Read vscode-bridge/context/section-schema.md first.
Also read:
- style-sheets/[stylesheet-name].css
- style-context.txt

EXTRACT these values from the stylesheet and map them:
  Background  ->  section settings.bg
  Primary text  ->  heading settings.color
  Muted text  ->  text block settings.color
  Accent color  ->  button settings.bg, heading accent
  Card bg  ->  card settings.bg
  Border  ->  card settings.border
  Border radius  ->  button and card settings.borderRadius
  Body font  ->  do not set (inherited)

SECTION TO CREATE: [describe the section]
SECTION TYPE: [section / header / footer / panel]
SAVE TO: page-builder/sections/[type-folder]/[name].json

Output the JSON with _meta included.
After saving, list which CSS rule or variable each extracted value came from.</pre>
                    </div>

                </div>
            </div>
        </div>
    </div>
    <!-- End Help Modal -->

    <!-- Fuzzy autocomplete dropdown for CSS properties -->
    <div class="fuzzy-dropdown hidden" id="fuzzyDropdown"></div>

    <!-- Wireframe Tool Modal -->
    <div class="wf-overlay hidden" id="wireframeOverlay">
        <div class="wf-modal">
            <div class="wf-toolbar">
                <span class="wf-toolbar-title">Wireframes</span>
                <button class="wf-btn" id="wfAddBtn">+ Add Element</button>
                <button class="wf-btn" id="wfClearBtn">Clear All</button>
                <button class="wf-btn" id="wfSaveBtn">Save JSON</button>
                <button class="wf-btn" id="wfLoadBtn">Load JSON</button>
                <button class="wf-btn" id="wfContextBtn">Copy Context</button>
                <input type="file" id="wfFileInput" accept=".json,.wf.json" style="display:none">
                <button class="wf-btn wf-btn-close" id="wfCloseBtn">Close</button>
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
        max-width: 820px;
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
    .help-tip {
        font-size: 12px;
        color: #8888a0;
        background: rgba(77,49,191,0.12);
        border-left: 3px solid #4d31bf;
        padding: 8px 12px;
        margin: 0;
        line-height: 1.5;
    }
    .help-prompt-list {
        display: flex;
        flex-direction: column;
        gap: 14px;
    }
    .help-prompt-card {
        background: #0c071c;
        border: 1px solid #2d1c6e;
        padding: 14px 16px;
        display: flex;
        flex-direction: column;
        gap: 8px;
    }
    .help-prompt-card-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 10px;
    }
    .help-prompt-title {
        font-size: 13px;
        font-weight: 700;
        color: #eceaf6;
        letter-spacing: 0.02em;
    }
    .help-prompt-desc {
        font-size: 12px;
        color: #8888a0;
        line-height: 1.5;
        margin: 0;
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
        max-height: 200px;
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
    }
    .btn-help:hover {
        background: #2d1c6e;
        color: #eceaf6;
    }
    </style>

    <!-- JS modules (load order: utilities first, app last) -->
    <script src="js/cdn-loader.js"></script>
    <script src="js/utils.js"></script>
    <script src="js/storage.js"></script>
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
    <script src="js/wireframe.js"></script>
    <script src="js/app.js"></script>
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
    <!-- Dev/native bridge: file browse, refresh, debug overlay -->
    <script src="js/native-bridge.js"></script>
    <!-- VSCode Copilot bridge: two-way sync with MCP server -->
    <script src="vscode-bridge/js/bridge-sync.js"></script>
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

    // Help modal
    (function () {
        var btn   = document.getElementById('helpBtn');
        var modal = document.getElementById('helpModal');
        var close = document.getElementById('helpModalClose');

        if (!btn || !modal || !close) {
            console.error('[Help] Missing helpBtn, helpModal, or helpModalClose element');
            return;
        }

        function openHelp() {
            modal.classList.remove('hidden');
        }

        function closeHelp() {
            modal.classList.add('hidden');
        }

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

        // Copy prompt buttons
        modal.querySelectorAll('.help-copy-btn').forEach(function (copyBtn) {
            copyBtn.addEventListener('click', function () {
                var targetId = copyBtn.getAttribute('data-target');
                if (!targetId) {
                    console.error('[Help] help-copy-btn missing data-target');
                    return;
                }
                var pre = document.getElementById(targetId);
                if (!pre) {
                    console.error('[Help] copy target not found: #' + targetId);
                    return;
                }
                var text = pre.textContent || '';
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
                        fallbackCopy(text, copyBtn);
                    });
                } else {
                    fallbackCopy(text, copyBtn);
                }
            });
        });

        function fallbackCopy(text, copyBtn) {
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
