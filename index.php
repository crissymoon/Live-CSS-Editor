<?php
/**
 * Live CSS Editor
 * A side-by-side HTML and CSS editor with live preview, CSS property reference,
 * save/load functionality, and fuzzy autocomplete for CSS properties.
 */

$cssProperties = [
    'Layout' => [
        'display', 'position', 'top', 'right', 'bottom', 'left', 'float', 'clear',
        'z-index', 'overflow', 'overflow-x', 'overflow-y', 'visibility', 'opacity',
        'box-sizing', 'vertical-align'
    ],
    'Flexbox' => [
        'flex', 'flex-basis', 'flex-direction', 'flex-flow', 'flex-grow', 'flex-shrink',
        'flex-wrap', 'justify-content', 'align-items', 'align-content', 'align-self',
        'order', 'gap', 'row-gap', 'column-gap'
    ],
    'Grid' => [
        'grid', 'grid-template', 'grid-template-columns', 'grid-template-rows',
        'grid-template-areas', 'grid-column', 'grid-row', 'grid-area',
        'grid-auto-columns', 'grid-auto-rows', 'grid-auto-flow',
        'grid-column-start', 'grid-column-end', 'grid-row-start', 'grid-row-end',
        'place-items', 'place-content', 'place-self'
    ],
    'Sizing' => [
        'width', 'height', 'min-width', 'min-height', 'max-width', 'max-height',
        'aspect-ratio'
    ],
    'Spacing' => [
        'margin', 'margin-top', 'margin-right', 'margin-bottom', 'margin-left',
        'padding', 'padding-top', 'padding-right', 'padding-bottom', 'padding-left'
    ],
    'Border' => [
        'border', 'border-top', 'border-right', 'border-bottom', 'border-left',
        'border-width', 'border-style', 'border-color',
        'border-top-width', 'border-right-width', 'border-bottom-width', 'border-left-width',
        'border-top-style', 'border-right-style', 'border-bottom-style', 'border-left-style',
        'border-top-color', 'border-right-color', 'border-bottom-color', 'border-left-color',
        'border-collapse', 'border-spacing', 'border-image',
        'outline', 'outline-width', 'outline-style', 'outline-color', 'outline-offset'
    ],
    'Background' => [
        'background', 'background-color', 'background-image', 'background-position',
        'background-size', 'background-repeat', 'background-attachment',
        'background-origin', 'background-clip', 'background-blend-mode'
    ],
    'Color' => [
        'color', 'opacity', 'filter', 'mix-blend-mode'
    ],
    'Typography' => [
        'font', 'font-family', 'font-size', 'font-weight', 'font-style', 'font-variant',
        'font-stretch', 'line-height', 'letter-spacing', 'word-spacing',
        'text-align', 'text-decoration', 'text-decoration-color', 'text-decoration-line',
        'text-decoration-style', 'text-transform', 'text-indent', 'text-shadow',
        'text-overflow', 'white-space', 'word-break', 'word-wrap', 'overflow-wrap',
        'writing-mode', 'direction', 'tab-size', 'hyphens'
    ],
    'List' => [
        'list-style', 'list-style-type', 'list-style-position', 'list-style-image'
    ],
    'Table' => [
        'table-layout', 'caption-side', 'empty-cells'
    ],
    'Transform' => [
        'transform', 'transform-origin', 'transform-style', 'perspective',
        'perspective-origin', 'backface-visibility', 'translate', 'rotate', 'scale'
    ],
    'Transition and Animation' => [
        'transition', 'transition-property', 'transition-duration',
        'transition-timing-function', 'transition-delay',
        'animation', 'animation-name', 'animation-duration',
        'animation-timing-function', 'animation-delay', 'animation-iteration-count',
        'animation-direction', 'animation-fill-mode', 'animation-play-state'
    ],
    'Misc' => [
        'cursor', 'pointer-events', 'user-select', 'resize', 'scroll-behavior',
        'scroll-snap-type', 'scroll-snap-align', 'object-fit', 'object-position',
        'clip-path', 'mask', 'will-change', 'content', 'counter-reset', 'counter-increment',
        'box-shadow', 'columns', 'column-count', 'column-gap', 'column-rule',
        'page-break-before', 'page-break-after', 'page-break-inside',
        'all', 'appearance', 'accent-color', 'caret-color', 'color-scheme'
    ]
];

$propertyValues = [
    'display' => 'block | inline | inline-block | flex | inline-flex | grid | inline-grid | none | contents | flow-root | table | table-row | table-cell',
    'position' => 'static | relative | absolute | fixed | sticky',
    'float' => 'left | right | none | inline-start | inline-end',
    'clear' => 'left | right | both | none',
    'overflow' => 'visible | hidden | scroll | auto | clip',
    'visibility' => 'visible | hidden | collapse',
    'box-sizing' => 'content-box | border-box',
    'vertical-align' => 'baseline | top | middle | bottom | sub | super | text-top | text-bottom | <length> | <percentage>',
    'flex-direction' => 'row | row-reverse | column | column-reverse',
    'flex-wrap' => 'nowrap | wrap | wrap-reverse',
    'justify-content' => 'flex-start | flex-end | center | space-between | space-around | space-evenly | start | end',
    'align-items' => 'stretch | flex-start | flex-end | center | baseline | start | end',
    'align-content' => 'stretch | flex-start | flex-end | center | space-between | space-around | space-evenly',
    'align-self' => 'auto | stretch | flex-start | flex-end | center | baseline',
    'border-style' => 'none | solid | dashed | dotted | double | groove | ridge | inset | outset | hidden',
    'border-color' => '<color>',
    'border-width' => 'thin | medium | thick | <length>',
    'background-repeat' => 'repeat | repeat-x | repeat-y | no-repeat | space | round',
    'background-size' => 'auto | cover | contain | <length> | <percentage>',
    'background-attachment' => 'scroll | fixed | local',
    'background-position' => 'top | right | bottom | left | center | <length> | <percentage>',
    'background-origin' => 'padding-box | border-box | content-box',
    'background-clip' => 'padding-box | border-box | content-box | text',
    'font-style' => 'normal | italic | oblique',
    'font-weight' => 'normal | bold | bolder | lighter | 100-900',
    'font-variant' => 'normal | small-caps',
    'text-align' => 'left | right | center | justify | start | end',
    'text-decoration-line' => 'none | underline | overline | line-through',
    'text-decoration-style' => 'solid | double | dotted | dashed | wavy',
    'text-transform' => 'none | capitalize | uppercase | lowercase | full-width',
    'white-space' => 'normal | nowrap | pre | pre-wrap | pre-line | break-spaces',
    'word-break' => 'normal | break-all | keep-all | break-word',
    'list-style-type' => 'disc | circle | square | decimal | lower-alpha | upper-alpha | lower-roman | upper-roman | none',
    'list-style-position' => 'inside | outside',
    'cursor' => 'auto | default | pointer | move | text | wait | help | not-allowed | crosshair | grab | grabbing | zoom-in | zoom-out | col-resize | row-resize | none | progress',
    'resize' => 'none | both | horizontal | vertical | block | inline',
    'object-fit' => 'fill | contain | cover | none | scale-down',
    'scroll-behavior' => 'auto | smooth',
    'user-select' => 'auto | none | text | all',
    'pointer-events' => 'auto | none',
    'writing-mode' => 'horizontal-tb | vertical-rl | vertical-lr',
    'direction' => 'ltr | rtl',
    'table-layout' => 'auto | fixed',
    'caption-side' => 'top | bottom',
    'empty-cells' => 'show | hide',
    'animation-direction' => 'normal | reverse | alternate | alternate-reverse',
    'animation-fill-mode' => 'none | forwards | backwards | both',
    'animation-play-state' => 'running | paused',
    'backface-visibility' => 'visible | hidden',
    'color-scheme' => 'normal | light | dark | light dark'
];

// Flat list of all property names for the fuzzy autocomplete
$allPropertyNames = [];
foreach ($cssProperties as $group => $props) {
    foreach ($props as $prop) {
        if (!in_array($prop, $allPropertyNames)) {
            $allPropertyNames[] = $prop;
        }
    }
}
sort($allPropertyNames);

$defaultHtml = '<div class="container">
  <h1>Hello, World!</h1>
  <p>This is a live CSS editor.</p>
  <button class="btn">Click Me</button>
  <ul class="list">
    <li>Item One</li>
    <li>Item Two</li>
    <li>Item Three</li>
  </ul>
</div>';

$defaultCss = '.container {
  padding: 24px;
  font-family: sans-serif;
}

h1 {
  color: #2d1c6e;
  margin-bottom: 12px;
}

p {
  color: #1b1825;
  font-size: 18px;
  line-height: 1.6;
}

.btn {
  background: #2d1c6e;
  color: #eceaf6;
  border: 2px solid #4d31bf;
  padding: 10px 24px;
  font-size: 16px;
  cursor: pointer;
}

.btn:hover {
  background: #4d31bf;
}

.list {
  margin-top: 16px;
  padding-left: 20px;
}

.list li {
  padding: 4px 0;
  color: #1b1825;
}';
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Live CSS Editor</title>
    <link rel="stylesheet" href="style.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/codemirror.min.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/theme/material-darker.min.css">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/codemirror.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/mode/xml/xml.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/mode/htmlmixed/htmlmixed.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/mode/css/css.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/mode/javascript/javascript.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/addon/edit/closetag.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/addon/edit/closebrackets.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.16/addon/edit/matchbrackets.min.js"></script>
</head>
<body>

    <header class="app-header">
        <div class="header-left">
            <h1 class="app-title">Live CSS Editor</h1>
        </div>
        <div class="header-center">
            <div class="css-property-lookup">
                <label for="propertySelect">CSS Properties:</label>
                <select id="propertySelect">
                    <option value="">-- Select a property --</option>
                    <?php foreach ($cssProperties as $group => $props): ?>
                        <optgroup label="<?php echo htmlspecialchars($group); ?>">
                            <?php foreach ($props as $prop): ?>
                                <option value="<?php echo htmlspecialchars($prop); ?>"><?php echo htmlspecialchars($prop); ?></option>
                            <?php endforeach; ?>
                        </optgroup>
                    <?php endforeach; ?>
                </select>
                <button id="insertPropertyBtn" class="btn-insert" title="Insert property at cursor">Insert</button>
            </div>
        </div>
        <div class="header-right">
            <button id="saveBtn" class="btn-action" title="Save to browser storage">Save</button>
            <button id="loadBtn" class="btn-action" title="Load from browser storage">Load</button>
            <button id="resetBtn" class="btn-action">Reset</button>
        </div>
    </header>

    <div id="propertyInfoBar" class="property-info-bar hidden">
        <span class="property-info-name" id="propertyInfoName"></span>
        <span class="property-info-values" id="propertyInfoValues"></span>
        <button class="property-info-close" id="propertyInfoClose" title="Close">&times;</button>
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

        <section class="editor-panel" id="htmlPanel">
            <div class="panel-header">
                <span class="panel-label">HTML</span>
            </div>
            <div class="panel-body">
                <textarea id="htmlEditor"><?php echo htmlspecialchars($defaultHtml); ?></textarea>
            </div>
        </section>

        <div class="gutter" id="gutter1"></div>

        <section class="editor-panel" id="cssPanel">
            <div class="panel-header">
                <span class="panel-label">CSS</span>
            </div>
            <div class="panel-body">
                <textarea id="cssEditor"><?php echo htmlspecialchars($defaultCss); ?></textarea>
            </div>
        </section>

        <div class="gutter" id="gutter2"></div>

        <section class="preview-panel" id="previewPanel">
            <div class="panel-header">
                <span class="panel-label">Live Preview</span>
            </div>
            <div class="panel-body">
                <iframe id="previewFrame"></iframe>
            </div>
        </section>
    </main>

    <!-- Fuzzy autocomplete dropdown for CSS properties -->
    <div class="fuzzy-dropdown hidden" id="fuzzyDropdown"></div>

    <script>
    (function() {
        // =====================================================
        // Property values for info bar
        // =====================================================
        var propertyValues = <?php echo json_encode($propertyValues); ?>;

        // All CSS property names for fuzzy matching
        var allCssProperties = <?php echo json_encode($allPropertyNames); ?>;

        // Default code
        var defaultHtml = <?php echo json_encode($defaultHtml); ?>;
        var defaultCss = <?php echo json_encode($defaultCss); ?>;

        // =====================================================
        // Init CodeMirror for HTML
        // =====================================================
        var htmlEditor = CodeMirror.fromTextArea(document.getElementById('htmlEditor'), {
            mode: 'htmlmixed',
            theme: 'material-darker',
            lineNumbers: true,
            autoCloseTags: true,
            autoCloseBrackets: true,
            matchBrackets: true,
            tabSize: 2,
            indentWithTabs: false,
            lineWrapping: true
        });

        // =====================================================
        // Init CodeMirror for CSS
        // =====================================================
        var cssEditor = CodeMirror.fromTextArea(document.getElementById('cssEditor'), {
            mode: 'css',
            theme: 'material-darker',
            lineNumbers: true,
            autoCloseBrackets: true,
            matchBrackets: true,
            tabSize: 2,
            indentWithTabs: false,
            lineWrapping: true
        });

        // =====================================================
        // Live preview update
        // =====================================================
        function updatePreview() {
            var htmlCode = htmlEditor.getValue();
            var cssCode = cssEditor.getValue();
            var frame = document.getElementById('previewFrame');
            var content = '<!DOCTYPE html><html><head><meta charset="UTF-8"><style>' +
                cssCode + '<\/style></head><body>' + htmlCode + '</body></html>';

            // Use srcdoc for reliable rendering without sandbox restrictions
            frame.srcdoc = content;
        }

        // Debounce helper
        var debounceTimer;
        function debounceUpdate() {
            clearTimeout(debounceTimer);
            debounceTimer = setTimeout(updatePreview, 150);
        }

        htmlEditor.on('change', debounceUpdate);
        cssEditor.on('change', debounceUpdate);

        // Initial render after CodeMirror is ready
        setTimeout(function() {
            htmlEditor.refresh();
            cssEditor.refresh();
            updatePreview();
        }, 200);

        // =====================================================
        // CSS Property dropdown in header
        // =====================================================
        var propertySelect = document.getElementById('propertySelect');
        var insertBtn = document.getElementById('insertPropertyBtn');
        var infoBar = document.getElementById('propertyInfoBar');
        var infoName = document.getElementById('propertyInfoName');
        var infoValues = document.getElementById('propertyInfoValues');
        var infoClose = document.getElementById('propertyInfoClose');

        propertySelect.addEventListener('change', function() {
            var prop = this.value;
            if (prop && propertyValues[prop]) {
                infoName.textContent = prop + ':';
                infoValues.textContent = propertyValues[prop];
                infoBar.classList.remove('hidden');
            } else if (prop) {
                infoName.textContent = prop + ':';
                infoValues.textContent = '(see MDN for values)';
                infoBar.classList.remove('hidden');
            } else {
                infoBar.classList.add('hidden');
            }
        });

        infoClose.addEventListener('click', function() {
            infoBar.classList.add('hidden');
        });

        insertBtn.addEventListener('click', function() {
            var prop = propertySelect.value;
            if (!prop) return;
            var snippet = '  ' + prop + ': ;';
            var cursor = cssEditor.getCursor();
            cssEditor.replaceRange(snippet + '\n', cursor);
            var newLine = cursor.line;
            var newCh = cursor.ch + snippet.length - 1;
            cssEditor.setCursor({ line: newLine, ch: newCh });
            cssEditor.focus();
        });

        // =====================================================
        // Reset button
        // =====================================================
        document.getElementById('resetBtn').addEventListener('click', function() {
            if (confirm('Reset both editors to default code?')) {
                htmlEditor.setValue(defaultHtml);
                cssEditor.setValue(defaultCss);
                updatePreview();
            }
        });

        // =====================================================
        // Save / Load system using localStorage
        // =====================================================
        var STORAGE_KEY = 'liveCssEditor_projects';

        function getSavedProjects() {
            try {
                var data = localStorage.getItem(STORAGE_KEY);
                if (data) return JSON.parse(data);
            } catch (e) {}
            return {};
        }

        function saveProject(name, htmlCode, cssCode) {
            var projects = getSavedProjects();
            projects[name] = {
                html: htmlCode,
                css: cssCode,
                timestamp: Date.now()
            };
            localStorage.setItem(STORAGE_KEY, JSON.stringify(projects));
        }

        function deleteProject(name) {
            var projects = getSavedProjects();
            delete projects[name];
            localStorage.setItem(STORAGE_KEY, JSON.stringify(projects));
        }

        // Save modal
        var saveModal = document.getElementById('saveModal');
        var saveNameInput = document.getElementById('saveNameInput');
        var saveExistingList = document.getElementById('saveExistingList');

        document.getElementById('saveBtn').addEventListener('click', function() {
            saveNameInput.value = '';
            renderSaveExisting();
            saveModal.classList.remove('hidden');
            setTimeout(function() { saveNameInput.focus(); }, 100);
        });

        function renderSaveExisting() {
            var projects = getSavedProjects();
            var keys = Object.keys(projects).sort();
            if (keys.length === 0) {
                saveExistingList.innerHTML = '<p class="modal-hint">No existing saves.</p>';
                return;
            }
            var html = '<p class="modal-hint">Existing saves (click to overwrite):</p>';
            for (var i = 0; i < keys.length; i++) {
                var name = keys[i];
                var ts = new Date(projects[name].timestamp).toLocaleString();
                html += '<div class="save-slot" data-name="' + escapeAttr(name) + '">';
                html += '<span class="save-slot-name">' + escapeHtml(name) + '</span>';
                html += '<span class="save-slot-date">' + escapeHtml(ts) + '</span>';
                html += '</div>';
            }
            saveExistingList.innerHTML = html;

            var slots = saveExistingList.querySelectorAll('.save-slot');
            for (var j = 0; j < slots.length; j++) {
                slots[j].addEventListener('click', function() {
                    saveNameInput.value = this.getAttribute('data-name');
                });
            }
        }

        document.getElementById('saveConfirmBtn').addEventListener('click', function() {
            var name = saveNameInput.value.trim();
            if (!name) {
                alert('Please enter a project name.');
                return;
            }
            var projects = getSavedProjects();
            if (projects[name]) {
                if (!confirm('A save named "' + name + '" already exists. Overwrite it?')) return;
            }
            saveProject(name, htmlEditor.getValue(), cssEditor.getValue());
            saveModal.classList.add('hidden');
        });

        document.getElementById('saveCancelBtn').addEventListener('click', function() {
            saveModal.classList.add('hidden');
        });
        document.getElementById('saveModalClose').addEventListener('click', function() {
            saveModal.classList.add('hidden');
        });

        // Load modal
        var loadModal = document.getElementById('loadModal');
        var loadSlotList = document.getElementById('loadSlotList');
        var loadEmptyMsg = document.getElementById('loadEmptyMsg');

        document.getElementById('loadBtn').addEventListener('click', function() {
            renderLoadSlots();
            loadModal.classList.remove('hidden');
        });

        function renderLoadSlots() {
            var projects = getSavedProjects();
            var keys = Object.keys(projects).sort();
            if (keys.length === 0) {
                loadSlotList.innerHTML = '';
                loadEmptyMsg.classList.remove('hidden');
                return;
            }
            loadEmptyMsg.classList.add('hidden');
            var html = '';
            for (var i = 0; i < keys.length; i++) {
                var name = keys[i];
                var ts = new Date(projects[name].timestamp).toLocaleString();
                html += '<div class="load-slot">';
                html += '<div class="load-slot-info">';
                html += '<span class="load-slot-name">' + escapeHtml(name) + '</span>';
                html += '<span class="load-slot-date">' + escapeHtml(ts) + '</span>';
                html += '</div>';
                html += '<div class="load-slot-actions">';
                html += '<button class="btn-action load-slot-load" data-name="' + escapeAttr(name) + '">Load</button>';
                html += '<button class="btn-action btn-danger load-slot-delete" data-name="' + escapeAttr(name) + '">Delete</button>';
                html += '</div>';
                html += '</div>';
            }
            loadSlotList.innerHTML = html;

            var loadBtns = loadSlotList.querySelectorAll('.load-slot-load');
            for (var j = 0; j < loadBtns.length; j++) {
                loadBtns[j].addEventListener('click', function() {
                    var n = this.getAttribute('data-name');
                    var p = getSavedProjects();
                    if (p[n]) {
                        htmlEditor.setValue(p[n].html);
                        cssEditor.setValue(p[n].css);
                        updatePreview();
                        loadModal.classList.add('hidden');
                    }
                });
            }

            var delBtns = loadSlotList.querySelectorAll('.load-slot-delete');
            for (var k = 0; k < delBtns.length; k++) {
                delBtns[k].addEventListener('click', function() {
                    var n = this.getAttribute('data-name');
                    if (confirm('Delete save "' + n + '"?')) {
                        deleteProject(n);
                        renderLoadSlots();
                    }
                });
            }
        }

        document.getElementById('loadCancelBtn').addEventListener('click', function() {
            loadModal.classList.add('hidden');
        });
        document.getElementById('loadModalClose').addEventListener('click', function() {
            loadModal.classList.add('hidden');
        });

        // Close modals on overlay click
        saveModal.addEventListener('click', function(e) {
            if (e.target === saveModal) saveModal.classList.add('hidden');
        });
        loadModal.addEventListener('click', function(e) {
            if (e.target === loadModal) loadModal.classList.add('hidden');
        });

        // HTML escape helpers
        function escapeHtml(str) {
            var div = document.createElement('div');
            div.textContent = str;
            return div.innerHTML;
        }
        function escapeAttr(str) {
            return str.replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
        }

        // =====================================================
        // Fuzzy autocomplete for CSS properties in CSS editor
        // =====================================================
        var fuzzyDropdown = document.getElementById('fuzzyDropdown');
        var fuzzySelectedIndex = -1;
        var fuzzyMatches = [];
        var fuzzyActive = false;
        var fuzzyWordStart = null;

        function fuzzyMatch(query, target) {
            // Simple subsequence fuzzy match
            query = query.toLowerCase();
            target = target.toLowerCase();
            if (query.length === 0) return false;

            // First check if it is a substring (higher priority)
            if (target.indexOf(query) !== -1) return true;

            // Then check subsequence
            var qi = 0;
            for (var ti = 0; ti < target.length && qi < query.length; ti++) {
                if (target[ti] === query[qi]) qi++;
            }
            return qi === query.length;
        }

        function fuzzyScore(query, target) {
            // Lower score = better match
            query = query.toLowerCase();
            target = target.toLowerCase();
            // Exact start match is best
            if (target.indexOf(query) === 0) return 0;
            // Contains as substring
            var idx = target.indexOf(query);
            if (idx !== -1) return 1 + idx;
            // Subsequence match
            return 100 + target.length;
        }

        function getWordBeforeCursor(cm) {
            var cursor = cm.getCursor();
            var line = cm.getLine(cursor.line);
            // Walk backwards from cursor to find the start of a CSS property name
            var end = cursor.ch;
            var start = end;
            while (start > 0 && /[a-zA-Z\-]/.test(line[start - 1])) {
                start--;
            }
            // Only trigger after colon-free context (property name position)
            // Check if there is a colon between start and some earlier { or beginning of line
            var before = line.substring(0, start).trim();
            // If the last non-whitespace char before our word is : then the user is typing a value, not a property
            if (before.length > 0 && before[before.length - 1] === ':') return null;

            var word = line.substring(start, end);
            if (word.length < 1) return null;
            return { word: word, start: start, end: end, line: cursor.line };
        }

        function showFuzzyDropdown(cm) {
            var info = getWordBeforeCursor(cm);
            if (!info || info.word.length < 1) {
                hideFuzzyDropdown();
                return;
            }

            fuzzyWordStart = info;
            var query = info.word;

            // Filter and sort matches
            fuzzyMatches = [];
            for (var i = 0; i < allCssProperties.length; i++) {
                if (fuzzyMatch(query, allCssProperties[i])) {
                    fuzzyMatches.push(allCssProperties[i]);
                }
            }
            fuzzyMatches.sort(function(a, b) {
                return fuzzyScore(query, a) - fuzzyScore(query, b);
            });

            // Limit to 12 results
            if (fuzzyMatches.length > 12) fuzzyMatches = fuzzyMatches.slice(0, 12);

            if (fuzzyMatches.length === 0) {
                hideFuzzyDropdown();
                return;
            }

            // If there is exactly one match and it equals the typed word, hide
            if (fuzzyMatches.length === 1 && fuzzyMatches[0] === query) {
                hideFuzzyDropdown();
                return;
            }

            fuzzySelectedIndex = 0;
            renderFuzzyDropdown(cm);
            fuzzyActive = true;
        }

        function renderFuzzyDropdown(cm) {
            var html = '';
            for (var i = 0; i < fuzzyMatches.length; i++) {
                var cls = 'fuzzy-item';
                if (i === fuzzySelectedIndex) cls += ' fuzzy-item-active';
                html += '<div class="' + cls + '" data-index="' + i + '">' + escapeHtml(fuzzyMatches[i]) + '</div>';
            }
            fuzzyDropdown.innerHTML = html;

            // Position the dropdown near the cursor
            var cursorCoords = cm.cursorCoords(true, 'page');
            fuzzyDropdown.style.left = cursorCoords.left + 'px';
            fuzzyDropdown.style.top = (cursorCoords.bottom + 2) + 'px';
            fuzzyDropdown.classList.remove('hidden');

            // Bind click on items
            var items = fuzzyDropdown.querySelectorAll('.fuzzy-item');
            for (var j = 0; j < items.length; j++) {
                items[j].addEventListener('mousedown', function(e) {
                    e.preventDefault();
                    var idx = parseInt(this.getAttribute('data-index'), 10);
                    acceptFuzzyMatch(cm, idx);
                });
            }
        }

        function hideFuzzyDropdown() {
            fuzzyDropdown.classList.add('hidden');
            fuzzyDropdown.innerHTML = '';
            fuzzyActive = false;
            fuzzySelectedIndex = -1;
            fuzzyMatches = [];
            fuzzyWordStart = null;
        }

        function acceptFuzzyMatch(cm, index) {
            if (index < 0 || index >= fuzzyMatches.length) return;
            var match = fuzzyMatches[index];
            if (!fuzzyWordStart) return;

            var from = { line: fuzzyWordStart.line, ch: fuzzyWordStart.start };
            var to = { line: fuzzyWordStart.line, ch: fuzzyWordStart.end };
            cm.replaceRange(match, from, to);
            hideFuzzyDropdown();
            cm.focus();
        }

        // Listen for changes in the CSS editor to trigger fuzzy dropdown
        cssEditor.on('inputRead', function(cm, changeObj) {
            // Only trigger on character input, not paste or delete
            if (changeObj.origin === '+input' || changeObj.origin === '+delete') {
                showFuzzyDropdown(cm);
            }
        });

        // Also update on cursor activity (e.g. after delete key)
        cssEditor.on('cursorActivity', function(cm) {
            if (fuzzyActive) {
                // Re-evaluate with current word
                var info = getWordBeforeCursor(cm);
                if (!info || info.word.length < 1) {
                    hideFuzzyDropdown();
                } else {
                    showFuzzyDropdown(cm);
                }
            }
        });

        // Intercept keyboard events on the CSS editor for arrow navigation and Tab
        cssEditor.on('keydown', function(cm, e) {
            if (!fuzzyActive) return;

            if (e.keyCode === 40) {
                // Arrow Down
                e.preventDefault();
                fuzzySelectedIndex = Math.min(fuzzySelectedIndex + 1, fuzzyMatches.length - 1);
                renderFuzzyDropdown(cm);
            } else if (e.keyCode === 38) {
                // Arrow Up
                e.preventDefault();
                fuzzySelectedIndex = Math.max(fuzzySelectedIndex - 1, 0);
                renderFuzzyDropdown(cm);
            } else if (e.keyCode === 9 || e.keyCode === 13) {
                // Tab or Enter
                e.preventDefault();
                if (fuzzySelectedIndex >= 0) {
                    acceptFuzzyMatch(cm, fuzzySelectedIndex);
                }
            } else if (e.keyCode === 27) {
                // Escape
                hideFuzzyDropdown();
            }
        });

        // Hide fuzzy dropdown when CSS editor loses focus
        cssEditor.on('blur', function() {
            setTimeout(hideFuzzyDropdown, 200);
        });

        // =====================================================
        // Resizable gutters with overlay
        // =====================================================
        var dragOverlay = document.getElementById('dragOverlay');
        var layoutContainer = document.querySelector('.editor-layout');

        var dragState = {
            active: false,
            gutter: null,
            leftPanel: null,
            rightPanel: null
        };

        function startDrag(e, gutterEl, leftPanelEl, rightPanelEl) {
            e.preventDefault();
            dragState.active = true;
            dragState.gutter = gutterEl;
            dragState.leftPanel = leftPanelEl;
            dragState.rightPanel = rightPanelEl;

            dragOverlay.classList.add('active');
            document.body.classList.add('is-dragging');
            gutterEl.classList.add('active');
        }

        function stopDrag() {
            if (!dragState.active) return;

            dragOverlay.classList.remove('active');
            document.body.classList.remove('is-dragging');
            if (dragState.gutter) {
                dragState.gutter.classList.remove('active');
            }

            dragState.active = false;
            dragState.gutter = null;
            dragState.leftPanel = null;
            dragState.rightPanel = null;

            htmlEditor.refresh();
            cssEditor.refresh();
        }

        function onDragMove(e) {
            if (!dragState.active) return;
            e.preventDefault();

            var containerRect = layoutContainer.getBoundingClientRect();
            var totalWidth = containerRect.width;
            var offsetX = e.clientX - containerRect.left;

            var leftPanel = dragState.leftPanel;
            var rightPanel = dragState.rightPanel;

            var leftRect = leftPanel.getBoundingClientRect();
            var leftStart = leftRect.left - containerRect.left;
            var rightRect = rightPanel.getBoundingClientRect();
            var rightEnd = rightRect.right - containerRect.left;

            var newLeftWidth = offsetX - leftStart - 3;
            var newRightWidth = rightEnd - offsetX - 3;

            if (newLeftWidth < 150 || newRightWidth < 150) return;

            var leftPct = (newLeftWidth / totalWidth * 100);
            var rightPct = (newRightWidth / totalWidth * 100);

            leftPanel.style.flex = '0 0 ' + leftPct + '%';
            rightPanel.style.flex = '0 0 ' + rightPct + '%';
        }

        document.getElementById('gutter1').addEventListener('mousedown', function(e) {
            startDrag(e, this, document.getElementById('htmlPanel'), document.getElementById('cssPanel'));
        });

        document.getElementById('gutter2').addEventListener('mousedown', function(e) {
            startDrag(e, this, document.getElementById('cssPanel'), document.getElementById('previewPanel'));
        });

        document.addEventListener('mousemove', onDragMove);
        document.addEventListener('mouseup', stopDrag);
        window.addEventListener('blur', stopDrag);
        document.addEventListener('mouseleave', stopDrag);

    })();
    </script>

</body>
</html>