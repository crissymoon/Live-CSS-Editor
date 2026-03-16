<?php
// agent-flow/index.php
// Drag-and-drop AI agent builder using moon-lang
// EXPERIMENTAL
?><!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Agent Flow -- moon-lang agent builder</title>
    <link rel="stylesheet" href="css/flow.css">
</head>
<body>
<div id="app">

    <!-- sidebar: node palette -->
    <aside id="palette">
        <div class="palette-title">Nodes</div>
        <div class="palette-hint">Drag onto canvas</div>

        <div class="palette-node" draggable="true" data-type="prompt">
            <span class="node-icon">P</span>
            <span class="node-label">Prompt</span>
        </div>
        <div class="palette-node" draggable="true" data-type="ai-call">
            <span class="node-icon">A</span>
            <span class="node-label">AI Call</span>
        </div>
        <div class="palette-node" draggable="true" data-type="condition">
            <span class="node-icon">?</span>
            <span class="node-label">Condition</span>
        </div>
        <div class="palette-node" draggable="true" data-type="loop">
            <span class="node-icon">L</span>
            <span class="node-label">Loop</span>
        </div>
        <div class="palette-node" draggable="true" data-type="memory">
            <span class="node-icon">M</span>
            <span class="node-label">Memory</span>
        </div>
        <div class="palette-node" draggable="true" data-type="tool">
            <span class="node-icon">T</span>
            <span class="node-label">Tool</span>
        </div>
        <div class="palette-node" draggable="true" data-type="output">
            <span class="node-icon">O</span>
            <span class="node-label">Output</span>
        </div>
        <div class="palette-node" draggable="true" data-type="guard">
            <span class="node-icon">G</span>
            <span class="node-label">Guard</span>
        </div>
        <div class="palette-node" draggable="true" data-type="agent-task">
            <span class="node-icon">V</span>
            <span class="node-label">VS Agent Task</span>
        </div>

        <div class="palette-sep"></div>
        <div class="palette-title">Quick Nodes</div>
        <div class="palette-hint">One-click task nodes</div>
        <div class="quick-node-list">
            <button type="button" class="quick-node-btn" data-task-id="code_review_smoke_runner">Code Review Smoke</button>
            <button type="button" class="quick-node-btn" data-task-id="code_review_security_scan">Security Scan</button>
            <button type="button" class="quick-node-btn" data-task-id="push_win_repo">Push (Windows)</button>
            <button type="button" class="quick-node-btn" data-task-id="push_sh_repo">Push (macOS/Linux)</button>
        </div>

        <div class="palette-sep"></div>
        <div class="palette-title">Saved Flows</div>
        <div id="saved-flow-list">
            <span class="muted">Loading...</span>
        </div>
    </aside>

    <!-- center: canvas -->
    <main id="canvas-wrap">
        <div id="toolbar">
            <button id="btn-run"     class="tb-btn tb-primary">Run (moon)</button>
            <button id="btn-run-ai"  class="tb-btn tb-ai">Run AI Direct</button>
            <button id="btn-export"  class="tb-btn">Export .moon</button>
            <button id="btn-save"    class="tb-btn">Save Flow</button>
            <button id="btn-clear"   class="tb-btn tb-danger">Clear</button>
            <span id="toolbar-status" class="tb-status"></span>
        </div>

        <div id="canvas-container">
            <svg id="edge-svg" aria-hidden="true"></svg>
            <div id="canvas"></div>
        </div>
    </main>

    <!-- right: properties -->
    <aside id="properties">
        <div class="palette-title">Properties</div>
        <div id="props-hint" class="muted">Select a node to edit its properties.</div>
        <form id="props-form" style="display:none">
            <input type="hidden" id="prop-id">

            <label for="prop-label">Label</label>
            <input type="text" id="prop-label" autocomplete="off">

            <div id="prop-dynamic"></div>

            <button type="submit" class="tb-btn tb-primary" style="margin-top:12px">Apply</button>
        </form>
    </aside>

</div><!-- #app -->

<!-- output panel -->
<div id="output-panel" class="hidden">
    <div id="output-header">
        <span id="output-title">Output</span>
        <button id="output-close" class="tb-btn" style="margin-left:auto">Close</button>
    </div>
    <pre id="output-stdout"></pre>
    <pre id="output-stderr" class="err-pre hidden"></pre>
    <pre id="output-moon"   class="moon-pre hidden"></pre>
    <div id="output-tabs">
        <button class="out-tab active" data-target="output-stdout">output</button>
        <button class="out-tab"        data-target="output-stderr">steps / stderr</button>
        <button class="out-tab"        data-target="output-moon">.moon source</button>
    </div>
</div>

<!-- edge-in-progress overlay -->
<div id="wire-overlay" style="display:none;position:fixed;inset:0;z-index:200;cursor:crosshair;"></div>

<script src="js/flow.js"></script>
</body>
</html>
