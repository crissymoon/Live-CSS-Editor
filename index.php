<?php
/**
 * Live CSS Editor — index.php
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
    <title>Crissy's Style Tool</title>
    <link rel="stylesheet" href="style.css">
    <link rel="stylesheet" href="css/native-bridge.css">
    <link rel="stylesheet" href="css/wireframe.css">

    <!-- CDN fallback loader — handles CodeMirror CSS + JS from multiple sources -->
    <!-- Actual CodeMirror assets are injected at runtime by js/cdn-loader.js    -->
</head>
<body>

    <header class="app-header">
        <div class="header-left">
            <h1 class="app-title">Crissy's Style Tool</h1>
            <span id="autosaveStatus" class="autosave-status"></span>
        </div>
        <div class="header-right">
            <button id="saveBtn" class="btn-action" title="Save to browser storage">Save</button>
            <button id="loadBtn" class="btn-action" title="Load from browser storage">Load</button>
            <button id="resetBtn" class="btn-action">Reset</button>
            <button id="resetLayoutBtn" class="btn-action" title="Restore default panel positions">Reset Layout</button>
            <button id="propertiesBtn" class="btn-action" title="Open properties reference">Properties</button>
            <button id="harmonyBtn" class="btn-action" title="Open color harmony tool">Harmony</button>
            <button id="guidesBtn" class="btn-action" title="Indent guide settings">Guides</button>
            <button id="wireframeBtn" class="btn-action" title="Open wireframe tool">Wireframes</button>
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
        </div>
    </div>

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
                <div class="wf-canvas-wrap">
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
    <!-- Dev/native bridge: file browse, refresh, debug overlay -->
    <script src="js/native-bridge.js"></script>

</body>
</html>
