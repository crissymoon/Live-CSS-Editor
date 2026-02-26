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
    <title>Live CSS Editor</title>
    <link rel="stylesheet" href="style.css">
    <link rel="stylesheet" href="css/native-bridge.css">

    <!-- CDN fallback loader — handles CodeMirror CSS + JS from multiple sources -->
    <!-- Actual CodeMirror assets are injected at runtime by js/cdn-loader.js    -->
</head>
<body>

    <header class="app-header">
        <div class="header-left">
            <h1 class="app-title">Live CSS Editor</h1>
        </div>
        <div class="header-right">
            <button id="saveBtn" class="btn-action" title="Save to browser storage">Save</button>
            <button id="loadBtn" class="btn-action" title="Load from browser storage">Load</button>
            <button id="resetBtn" class="btn-action">Reset</button>
            <button id="resetLayoutBtn" class="btn-action" title="Restore default panel positions">Reset Layout</button>
            <button id="propertiesBtn" class="btn-action" title="Open properties reference">Properties</button>
            <button id="harmonyBtn" class="btn-action" title="Open color harmony tool">Harmony</button>
        </div>
    </header>

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
                <div class="panel-drag-handle" title="Drag panel"></div>
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
                <div class="panel-drag-handle" title="Drag panel"></div>
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
                <div class="panel-drag-handle" title="Drag panel"></div>
            </div>
            <div class="panel-body">
                <textarea id="cssEditor"><?php echo htmlspecialchars($defaultCss); ?></textarea>
            </div>
        </section>

        <section class="preview-panel" id="previewPanel">
            <div class="panel-header">
                <span class="panel-label">Live Preview</span>
                <div class="panel-drag-handle" title="Drag panel"></div>
            </div>
            <div class="panel-body">
                <iframe id="previewFrame"></iframe>
            </div>
        </section>
    </main>

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

    <!-- Fuzzy autocomplete dropdown for CSS properties -->
    <div class="fuzzy-dropdown hidden" id="fuzzyDropdown"></div>

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
    <script src="js/color-harmony.js"></script>
    <script src="js/modal-save.js"></script>
    <script src="js/modal-load.js"></script>
    <script src="js/gutter.js"></script>
    <script src="js/app.js"></script>
    <!-- Dev/native bridge: file browse, refresh, debug overlay -->
    <script src="js/native-bridge.js"></script>

</body>
</html>
