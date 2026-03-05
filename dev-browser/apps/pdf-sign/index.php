<?php
/**
 * pdf-sign/index.php
 *
 * Fill and sign PDF tool.
 * Renders each page with PDF.js, overlays interactive annotations
 * (text, checkmark, date, signature), then bakes them into the
 * final PDF with pdf-lib for download.
 */
?><!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PDF Fill &amp; Sign</title>

<!-- PDF.js 4.x (Mozilla CDN) -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.min.js"></script>
<!-- pdf-lib for writing back to PDF -->
<script src="https://unpkg.com/pdf-lib@1.17.1/dist/pdf-lib.min.js"></script>
<!-- fontkit for embedded fonts inside pdf-lib -->
<script src="https://unpkg.com/@pdf-lib/fontkit@1.1.1/dist/fontkit.umd.min.js"></script>

<style>
/* ── base ────────────────────────────────────────────────────── */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
:root {
    --bg:      #0f0f13;
    --surface: #1a1a24;
    --raised:  #22222e;
    --border:  rgba(100,100,200,.18);
    --accent:  #6366f1;
    --accent2: #818cf8;
    --text:    #e2e2ea;
    --dim:     #888899;
    --ok:      #34d399;
    --warn:    #fbbf24;
    --bad:     #f87171;
    --toolbar: 52px;
    --sidebar: 260px;
}
html, body {
    height: 100%;
    background: var(--bg);
    color: var(--text);
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    font-size: 13px;
    overflow: hidden;
}

/* ── layout ──────────────────────────────────────────────────── */
#app { display: flex; flex-direction: column; height: 100%; }

/* toolbar */
#toolbar {
    height: var(--toolbar);
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 0 12px;
    flex-shrink: 0;
    user-select: none;
}
#toolbar .sep {
    width: 1px;
    height: 24px;
    background: var(--border);
    margin: 0 4px;
}
.tb-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 6px 11px;
    border-radius: 6px;
    border: 1px solid transparent;
    background: transparent;
    color: var(--dim);
    font-size: 12.5px;
    cursor: pointer;
    transition: background .12s, color .12s, border-color .12s;
    white-space: nowrap;
}
.tb-btn:hover { background: rgba(99,102,241,.12); color: var(--text); }
.tb-btn.active {
    background: rgba(99,102,241,.22);
    color: var(--accent2);
    border-color: rgba(99,102,241,.35);
}
.tb-btn svg { flex-shrink: 0; }
.tb-btn.danger:hover { background: rgba(248,113,113,.15); color: var(--bad); }
.tb-btn.download { background: var(--accent); color: #fff; border-color: transparent; }
.tb-btn.download:hover { background: var(--accent2); }
.tb-btn.download:disabled { background: var(--raised); color: var(--dim); cursor: default; }

#page-info { margin-left: auto; font-size: 12px; color: var(--dim); min-width: 80px; text-align: right; }

/* body split */
#body { display: flex; flex: 1; overflow: hidden; }

/* sidebar */
#sidebar {
    width: var(--sidebar);
    background: var(--surface);
    border-right: 1px solid var(--border);
    display: flex;
    flex-direction: column;
    flex-shrink: 0;
    overflow-y: auto;
}
.sb-section { padding: 14px 14px 6px; }
.sb-label {
    font-size: 10px;
    font-weight: 600;
    letter-spacing: .08em;
    text-transform: uppercase;
    color: var(--dim);
    margin-bottom: 8px;
}
.tool-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 6px;
}
.tool-btn {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 5px;
    padding: 10px 6px;
    border-radius: 8px;
    border: 1px solid var(--border);
    background: var(--raised);
    color: var(--dim);
    font-size: 11px;
    cursor: pointer;
    transition: all .12s;
    line-height: 1.2;
    text-align: center;
}
.tool-btn:hover { border-color: var(--accent); color: var(--text); }
.tool-btn.active {
    border-color: var(--accent);
    background: rgba(99,102,241,.2);
    color: var(--accent2);
}

/* annotation list */
#annot-list { flex: 1; overflow-y: auto; padding: 0 8px 8px; }
.annot-item {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 7px 8px;
    border-radius: 6px;
    border: 1px solid transparent;
    cursor: pointer;
    transition: background .1s;
    font-size: 11.5px;
    color: var(--dim);
}
.annot-item:hover { background: var(--raised); color: var(--text); }
.annot-item.selected { background: rgba(99,102,241,.15); border-color: rgba(99,102,241,.3); color: var(--text); }
.annot-item .del-btn {
    margin-left: auto;
    opacity: 0;
    background: none;
    border: none;
    cursor: pointer;
    color: var(--bad);
    padding: 2px 4px;
    border-radius: 4px;
    line-height: 1;
}
.annot-item:hover .del-btn { opacity: 1; }
.annot-dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    flex-shrink: 0;
}
.dot-text    { background: #67e8f9; }
.dot-sig     { background: #a78bfa; }
.dot-check   { background: #34d399; }
.dot-date    { background: #fbbf24; }

/* main canvas area */
#canvas-area {
    flex: 1;
    overflow: auto;
    background: #141418;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 24px 0;
    gap: 24px;
}

/* page wrapper -- positions annotation overlays */
.page-wrap {
    position: relative;
    box-shadow: 0 4px 32px rgba(0,0,0,.5);
    background: #fff;
    cursor: crosshair;
}
.page-wrap canvas { display: block; }

/* annotation overlays */
.annot-overlay {
    position: absolute;
    z-index: 10;
    cursor: move;
    user-select: none;
}
.annot-overlay.selected { outline: 2px solid var(--accent); outline-offset: 2px; }
.annot-overlay input.annot-input {
    background: rgba(200,210,255,.12);
    border: 1px dashed rgba(99,102,241,.6);
    border-radius: 3px;
    color: #111;
    font-size: var(--annot-fs, 13px);
    padding: 2px 4px;
    min-width: 80px;
    outline: none;
    width: 100%;
}
.annot-overlay input.annot-input:focus { border-style: solid; background: rgba(200,210,255,.22); }
.annot-check-box {
    width: 20px; height: 20px;
    display: flex; align-items: center; justify-content: center;
    font-size: 16px;
    color: #1a1a8c;
    background: rgba(200,220,255,.08);
    border: 1.5px dashed rgba(99,102,241,.5);
    border-radius: 3px;
    cursor: pointer;
}
.annot-check-box.checked { background: rgba(52,211,153,.15); border-color: #34d399; }
.annot-sig-img {
    display: block;
    max-width: 180px;
    max-height: 80px;
    cursor: move;
    -webkit-user-drag: none;
}
.annot-resize {
    position: absolute;
    bottom: -5px; right: -5px;
    width: 10px; height: 10px;
    border-radius: 50%;
    background: var(--accent);
    cursor: se-resize;
    z-index: 12;
}

/* drop zone */
#drop-zone {
    position: fixed;
    inset: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 16px;
    background: var(--bg);
    z-index: 100;
}
#drop-zone.hidden { display: none; }
.dz-box {
    width: 380px;
    border: 2px dashed rgba(99,102,241,.45);
    border-radius: 16px;
    padding: 40px 24px;
    text-align: center;
    transition: border-color .15s;
}
.dz-box.drag-over { border-color: var(--accent2); background: rgba(99,102,241,.07); }
.dz-box h2 { font-size: 18px; color: var(--text); margin-bottom: 8px; }
.dz-box p  { color: var(--dim); font-size: 13px; margin-bottom: 20px; }
.dz-file-btn {
    display: inline-block;
    padding: 9px 22px;
    background: var(--accent);
    color: #fff;
    border-radius: 8px;
    font-size: 13px;
    cursor: pointer;
    transition: background .12s;
}
.dz-file-btn:hover { background: var(--accent2); }

/* signature modal */
#sig-modal {
    display: none;
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,.6);
    z-index: 200;
    align-items: center;
    justify-content: center;
}
#sig-modal.open { display: flex; }
.sig-panel {
    background: var(--surface);
    border-radius: 14px;
    border: 1px solid var(--border);
    width: 520px;
    max-width: 96vw;
    overflow: hidden;
}
.sig-header {
    display: flex;
    align-items: center;
    padding: 14px 18px;
    border-bottom: 1px solid var(--border);
}
.sig-header h3 { font-size: 14px; }
.sig-header .close-btn {
    margin-left: auto;
    background: none;
    border: none;
    color: var(--dim);
    cursor: pointer;
    font-size: 18px;
    line-height: 1;
    padding: 2px 6px;
    border-radius: 4px;
}
.sig-header .close-btn:hover { color: var(--text); background: var(--raised); }
.sig-tabs {
    display: flex;
    gap: 0;
    border-bottom: 1px solid var(--border);
}
.sig-tab {
    flex: 1;
    padding: 10px;
    text-align: center;
    cursor: pointer;
    font-size: 12.5px;
    color: var(--dim);
    border-bottom: 2px solid transparent;
    transition: color .1s, border-color .1s;
}
.sig-tab.active { color: var(--accent2); border-color: var(--accent); }
.sig-body { padding: 16px 18px; }
.sig-tab-content { display: none; }
.sig-tab-content.active { display: block; }

/* draw tab */
#sig-canvas-wrap {
    position: relative;
    background: #fff;
    border-radius: 8px;
    overflow: hidden;
    border: 1px solid var(--border);
}
#sig-canvas { display: block; cursor: crosshair; touch-action: none; }
.sig-canvas-hint {
    position: absolute;
    inset: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    pointer-events: none;
    color: #aab;
    font-size: 13px;
}
/* type tab */
#sig-type-input {
    width: 100%;
    padding: 10px 14px;
    background: #fff;
    border: 1px solid var(--border);
    border-radius: 8px;
    font-size: 28px;
    font-family: 'Dancing Script', 'Brush Script MT', cursive;
    color: #1a1a8c;
    outline: none;
    text-align: center;
}
/* upload tab */
#sig-upload-area {
    border: 2px dashed rgba(99,102,241,.35);
    border-radius: 8px;
    padding: 30px;
    text-align: center;
    color: var(--dim);
    cursor: pointer;
    transition: all .12s;
}
#sig-upload-area:hover { border-color: var(--accent); color: var(--text); }
#sig-upload-preview { display: none; margin-top: 10px; max-height: 80px; border-radius: 4px; }

.sig-footer {
    display: flex;
    gap: 8px;
    justify-content: flex-end;
    padding: 12px 18px;
    border-top: 1px solid var(--border);
}
.sig-action {
    padding: 8px 18px;
    border-radius: 7px;
    border: none;
    font-size: 13px;
    cursor: pointer;
    transition: all .12s;
}
.sig-action.primary { background: var(--accent); color: #fff; }
.sig-action.primary:hover { background: var(--accent2); }
.sig-action.ghost { background: var(--raised); color: var(--dim); }
.sig-action.ghost:hover { color: var(--text); }

/* font size control */
.fs-control {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 8px 14px;
    border-bottom: 1px solid var(--border);
}
.fs-control label { font-size: 11px; color: var(--dim); }
.fs-control input[type=range] { flex: 1; accent-color: var(--accent); }
.fs-control span { font-size: 11px; color: var(--accent2); min-width: 26px; }

/* toast */
#toast {
    position: fixed;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%) translateY(60px);
    background: var(--raised);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 9px 18px;
    font-size: 13px;
    color: var(--text);
    pointer-events: none;
    transition: transform .22s ease;
    z-index: 999;
    white-space: nowrap;
}
#toast.show { transform: translateX(-50%) translateY(0); }

/* upload PDF spinner */
#loading-bar {
    position: fixed;
    top: 0; left: 0; right: 0;
    height: 2px;
    background: var(--accent);
    transform-origin: left;
    transform: scaleX(0);
    transition: transform .3s;
    z-index: 500;
}
</style>
</head>
<body>
<div id="loading-bar"></div>

<!-- ── Drop zone ───────────────────────────────────────────── -->
<div id="drop-zone">
  <div class="dz-box" id="dz-box">
    <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="#6366f1" stroke-width="1.5" style="margin:0 auto 12px">
      <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
      <polyline points="14 2 14 8 20 8"/>
      <line x1="12" y1="18" x2="12" y2="12"/>
      <line x1="9" y1="15" x2="15" y2="15"/>
    </svg>
    <h2>PDF Fill &amp; Sign</h2>
    <p>Drop a PDF here, or click to browse</p>
    <label class="dz-file-btn">
      Open PDF
      <input type="file" id="file-input" accept=".pdf,application/pdf" style="display:none">
    </label>
  </div>
</div>

<!-- ── Main app (hidden until PDF loaded) ─────────────────── -->
<div id="app" style="display:none">

  <!-- toolbar -->
  <div id="toolbar">
    <!-- file name -->
    <span id="file-name" style="font-size:12px;color:var(--dim);max-width:200px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap"></span>
    <div class="sep"></div>

    <!-- page nav -->
    <button class="tb-btn" id="btn-prev" title="Previous page">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="15 18 9 12 15 6"/></svg>
    </button>
    <button class="tb-btn" id="btn-next" title="Next page">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="9 18 15 12 9 6"/></svg>
    </button>

    <!-- zoom -->
    <div class="sep"></div>
    <button class="tb-btn" id="btn-zoom-out" title="Zoom out">&#x2212;</button>
    <span id="zoom-label" style="font-size:12px;color:var(--dim);min-width:38px;text-align:center">100%</span>
    <button class="tb-btn" id="btn-zoom-in" title="Zoom in">&#x2b;</button>

    <div class="sep"></div>

    <!-- load new -->
    <label class="tb-btn" style="cursor:pointer" title="Open another PDF">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 15v4a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
      Open
      <input type="file" id="file-input-2" accept=".pdf,application/pdf" style="display:none">
    </label>

    <button class="tb-btn danger" id="btn-clear-all" title="Remove all annotations">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/><path d="M10 11v6"/><path d="M14 11v6"/></svg>
      Clear
    </button>

    <button class="tb-btn download" id="btn-download" disabled>
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>
      Download PDF
    </button>

    <div id="page-info"></div>
  </div>

  <!-- body -->
  <div id="body">

    <!-- sidebar -->
    <div id="sidebar">

      <!-- font size -->
      <div class="fs-control">
        <label>Font size</label>
        <input type="range" id="font-size-range" min="8" max="36" value="13">
        <span id="font-size-val">13</span>
      </div>

      <!-- tools -->
      <div class="sb-section">
        <div class="sb-label">Annotations</div>
        <div class="tool-grid">
          <button class="tool-btn active" data-tool="text">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><polyline points="4 7 4 4 20 4 20 7"/><line x1="9" y1="20" x2="15" y2="20"/><line x1="12" y1="4" x2="12" y2="20"/></svg>
            Text
          </button>
          <button class="tool-btn" data-tool="signature">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M17 3a2.828 2.828 0 1 1 4 4L7.5 20.5 2 22l1.5-5.5L17 3z"/></svg>
            Signature
          </button>
          <button class="tool-btn" data-tool="checkmark">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><polyline points="20 6 9 17 4 12"/></svg>
            Checkmark
          </button>
          <button class="tool-btn" data-tool="date">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><rect x="3" y="4" width="18" height="18" rx="2"/><line x1="16" y1="2" x2="16" y2="6"/><line x1="8" y1="2" x2="8" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/></svg>
            Date
          </button>
          <button class="tool-btn" data-tool="initials">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="8" r="4"/><path d="M4 20c0-4 3.6-7 8-7s8 3 8 7"/></svg>
            Initials
          </button>
          <button class="tool-btn" data-tool="select" id="tool-select">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M5 3l14 9-7 1-4 7z"/></svg>
            Select
          </button>
        </div>
      </div>

      <!-- annotation list -->
      <div class="sb-section" style="padding-bottom:4px">
        <div class="sb-label">Placed (<span id="annot-count">0</span>)</div>
      </div>
      <div id="annot-list"></div>
    </div>

    <!-- canvas area -->
    <div id="canvas-area"></div>
  </div>
</div>

<!-- ── Signature modal ────────────────────────────────────── -->
<div id="sig-modal">
  <div class="sig-panel">
    <div class="sig-header">
      <h3 id="sig-modal-title">Add Signature</h3>
      <button class="close-btn" id="sig-close">&times;</button>
    </div>
    <div class="sig-tabs">
      <div class="sig-tab active" data-sigtab="draw">Draw</div>
      <div class="sig-tab" data-sigtab="type">Type</div>
      <div class="sig-tab" data-sigtab="upload">Upload Image</div>
    </div>
    <div class="sig-body">
      <!-- draw -->
      <div class="sig-tab-content active" id="sigtab-draw">
        <div id="sig-canvas-wrap">
          <canvas id="sig-canvas" width="480" height="160"></canvas>
          <div class="sig-canvas-hint" id="sig-hint">Sign here</div>
        </div>
        <div style="margin-top:8px;display:flex;gap:8px;align-items:center">
          <label style="font-size:11px;color:var(--dim)">Color</label>
          <input type="color" id="sig-color" value="#1a1a8c" style="border:none;background:none;cursor:pointer;width:28px;height:24px">
          <label style="font-size:11px;color:var(--dim);margin-left:8px">Weight</label>
          <input type="range" id="sig-weight" min="1" max="6" value="2" style="width:80px;accent-color:var(--accent)">
          <button class="sig-action ghost" id="sig-canvas-clear" style="margin-left:auto;padding:5px 12px">Clear</button>
        </div>
      </div>
      <!-- type -->
      <div class="sig-tab-content" id="sigtab-type">
        <input type="text" id="sig-type-input" placeholder="Type your signature..." maxlength="60">
        <p style="margin-top:8px;font-size:11px;color:var(--dim)">Rendered in a handwriting font.</p>
      </div>
      <!-- upload -->
      <div class="sig-tab-content" id="sigtab-upload">
        <div id="sig-upload-area">
          <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" style="margin:0 auto 8px;display:block"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>
          <p>Click or drag an image file here (PNG, JPG)</p>
          <input type="file" id="sig-upload-input" accept="image/*" style="display:none">
        </div>
        <img id="sig-upload-preview" alt="Signature preview">
      </div>
    </div>
    <div class="sig-footer">
      <button class="sig-action ghost" id="sig-cancel">Cancel</button>
      <button class="sig-action primary" id="sig-apply">Add to Document</button>
    </div>
  </div>
</div>

<div id="toast"></div>

<!-- Google Font for typed signature -->
<link href="https://fonts.googleapis.com/css2?family=Dancing+Script:wght@600&display=swap" rel="stylesheet">

<script>
/* ================================================================
   PDF Fill & Sign -- main logic
   ================================================================ */
'use strict';

// PDF.js worker
pdfjsLib.GlobalWorkerOptions.workerSrc =
    'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.worker.min.js';

/* ── state ──────────────────────────────────────────────────── */
var state = {
    pdfBytes:    null,   // Uint8Array of original PDF
    pdfDoc:      null,   // PDF.js document
    pageCount:   0,
    currentPage: 1,
    scale:       1.3,
    tool:        'text',
    annotations: [],     // [{id, type, page, x, y, value, sigDataUrl, fontSize, checked}]
    nextId:      1,
    selectedId:  null,
    sigPending:  null,   // coordinates waiting for signature
    // signature modal
    sigTool:     'draw',
    sigDrawing:  false,
    sigCanvas:   null,
    sigCtx:      null,
    sigLastX:    0,
    sigLastY:    0,
};

/* ── DOM refs ───────────────────────────────────────────────── */
var $dropZone   = document.getElementById('drop-zone');
var $dzBox      = document.getElementById('dz-box');
var $app        = document.getElementById('app');
var $canvasArea = document.getElementById('canvas-area');
var $fileInput  = document.getElementById('file-input');
var $fileInput2 = document.getElementById('file-input-2');
var $fileName   = document.getElementById('file-name');
var $pageInfo   = document.getElementById('page-info');
var $annotList  = document.getElementById('annot-list');
var $annotCount = document.getElementById('annot-count');
var $btnPrev    = document.getElementById('btn-prev');
var $btnNext    = document.getElementById('btn-next');
var $btnZoomIn  = document.getElementById('btn-zoom-in');
var $btnZoomOut = document.getElementById('btn-zoom-out');
var $zoomLabel  = document.getElementById('zoom-label');
var $btnDownload = document.getElementById('btn-download');
var $btnClearAll = document.getElementById('btn-clear-all');
var $loadingBar  = document.getElementById('loading-bar');
var $fsRange     = document.getElementById('font-size-range');
var $fsVal       = document.getElementById('font-size-val');
var $sigModal        = document.getElementById('sig-modal');
var $sigCanvas       = document.getElementById('sig-canvas');
var $sigCanvasClear  = document.getElementById('sig-canvas-clear');
var $sigHint         = document.getElementById('sig-hint');
var $sigColor        = document.getElementById('sig-color');
var $sigWeight       = document.getElementById('sig-weight');
var $sigTypeInput    = document.getElementById('sig-type-input');
var $sigUploadArea   = document.getElementById('sig-upload-area');
var $sigUploadInput  = document.getElementById('sig-upload-input');
var $sigUploadPreview = document.getElementById('sig-upload-preview');
var $sigClose    = document.getElementById('sig-close');
var $sigCancel   = document.getElementById('sig-cancel');
var $sigApply    = document.getElementById('sig-apply');
var $sigModalTitle = document.getElementById('sig-modal-title');
var $toast       = document.getElementById('toast');

state.sigCanvas = $sigCanvas;
state.sigCtx    = $sigCanvas.getContext('2d');

/* ── toast ──────────────────────────────────────────────────── */
var _toastTimer = null;
function toast(msg, dur) {
    $toast.textContent = msg;
    $toast.classList.add('show');
    if (_toastTimer) clearTimeout(_toastTimer);
    _toastTimer = setTimeout(function () { $toast.classList.remove('show'); }, dur || 2400);
}

/* ── loading bar ────────────────────────────────────────────── */
function setProgress(v) {
    $loadingBar.style.transform = 'scaleX(' + v + ')';
    if (v >= 1) { setTimeout(function () { $loadingBar.style.transform = 'scaleX(0)'; }, 400); }
}

/* ── file handling ──────────────────────────────────────────── */
function openPDFFile(file) {
    if (!file || file.type !== 'application/pdf' && !file.name.endsWith('.pdf')) {
        toast('Please select a valid PDF file.'); return;
    }
    $fileName.textContent = file.name;
    var reader = new FileReader();
    reader.onload = function (e) {
        state.pdfBytes = new Uint8Array(e.target.result);
        loadPDF(state.pdfBytes);
    };
    reader.readAsArrayBuffer(file);
}

function loadPDF(bytes) {
    setProgress(0.3);
    state.annotations = [];
    state.nextId = 1;
    state.selectedId = null;
    pdfjsLib.getDocument({ data: bytes }).promise.then(function (doc) {
        state.pdfDoc = doc;
        state.pageCount = doc.numPages;
        state.currentPage = 1;
        setProgress(0.7);
        renderAllPages().then(function () {
            setProgress(1);
            $dropZone.classList.add('hidden');
            $app.style.display = 'flex';
            $btnDownload.disabled = false;
            updatePageInfo();
            updateAnnotList();
        });
    }).catch(function (err) {
        toast('Failed to load PDF: ' + (err.message || err));
        setProgress(0);
    });
}

/* ── render all pages ───────────────────────────────────────── */
function renderAllPages() {
    $canvasArea.innerHTML = '';
    var promises = [];
    for (var i = 1; i <= state.pageCount; i++) {
        promises.push(renderPage(i));
    }
    return Promise.all(promises);
}

function renderPage(pageNum) {
    return state.pdfDoc.getPage(pageNum).then(function (page) {
        var scale   = state.scale;
        var vp      = page.getViewport({ scale: scale });

        var wrap    = document.createElement('div');
        wrap.className = 'page-wrap';
        wrap.style.width  = vp.width  + 'px';
        wrap.style.height = vp.height + 'px';
        wrap.dataset.page = pageNum;
        wrap.addEventListener('click', onPageClick);
        $canvasArea.appendChild(wrap);

        var canvas  = document.createElement('canvas');
        canvas.width  = vp.width;
        canvas.height = vp.height;
        wrap.appendChild(canvas);

        return page.render({ canvasContext: canvas.getContext('2d'), viewport: vp }).promise;
    });
}

function getPageWrap(pageNum) {
    return $canvasArea.querySelector('[data-page="' + pageNum + '"]');
}

/* ── page nav ───────────────────────────────────────────────── */
function updatePageInfo() {
    $pageInfo.textContent = 'Page ' + state.currentPage + ' / ' + state.pageCount;
}

function scrollToPage(pageNum) {
    var wrap = getPageWrap(pageNum);
    if (wrap) wrap.scrollIntoView({ behavior: 'smooth', block: 'start' });
    state.currentPage = pageNum;
    updatePageInfo();
}

$btnPrev.addEventListener('click', function () {
    if (state.currentPage > 1) scrollToPage(state.currentPage - 1);
});
$btnNext.addEventListener('click', function () {
    if (state.currentPage < state.pageCount) scrollToPage(state.currentPage + 1);
});

/* Update currentPage on scroll */
$canvasArea.addEventListener('scroll', function () {
    var wraps = $canvasArea.querySelectorAll('.page-wrap');
    var midY  = $canvasArea.scrollTop + $canvasArea.clientHeight / 2;
    var best  = 1, bestDist = Infinity;
    wraps.forEach(function (w) {
        var wMid = w.offsetTop + w.offsetHeight / 2;
        var dist = Math.abs(wMid - midY);
        if (dist < bestDist) { bestDist = dist; best = parseInt(w.dataset.page, 10); }
    });
    if (best !== state.currentPage) { state.currentPage = best; updatePageInfo(); }
}, { passive: true });

/* ── zoom ───────────────────────────────────────────────────── */
function setZoom(newScale) {
    state.scale = Math.max(0.4, Math.min(3, newScale));
    $zoomLabel.textContent = Math.round(state.scale * 100) + '%';
    // Re-render all pages and re-draw annotations
    if (!state.pdfDoc) return;
    $btnDownload.disabled = true;
    renderAllPages().then(function () {
        state.annotations.forEach(function (a) { drawAnnotation(a); });
        $btnDownload.disabled = false;
    });
}

$btnZoomIn.addEventListener('click',  function () { setZoom(state.scale + 0.15); });
$btnZoomOut.addEventListener('click', function () { setZoom(state.scale - 0.15); });

/* ── tool selection ─────────────────────────────────────────── */
document.querySelectorAll('.tool-btn').forEach(function (btn) {
    btn.addEventListener('click', function () {
        document.querySelectorAll('.tool-btn').forEach(function (b) { b.classList.remove('active'); });
        btn.classList.add('active');
        state.tool = btn.dataset.tool;
        state.selectedId = null;
        redrawAllAnnotations();
    });
});

/* ── font size ──────────────────────────────────────────────── */
$fsRange.addEventListener('input', function () {
    $fsVal.textContent = $fsRange.value;
    // Update selected annotation's font size
    if (state.selectedId != null) {
        var a = findAnnot(state.selectedId);
        if (a && (a.type === 'text' || a.type === 'date' || a.type === 'initials')) {
            a.fontSize = parseInt($fsRange.value, 10);
            redrawAnnotation(a);
        }
    }
});

/* ── click on page to place annotation ──────────────────────── */
function onPageClick(e) {
    // Clicks on annotation overlays are consumed by those overlays
    if (e.target !== this && e.target.closest('.annot-overlay')) return;
    if (state.tool === 'select') return;

    var wrap   = e.currentTarget;
    var rect   = wrap.getBoundingClientRect();
    var wrapX  = e.clientX - rect.left;
    var wrapY  = e.clientY - rect.top;
    var pageNum = parseInt(wrap.dataset.page, 10);

    var a = {
        id:       state.nextId++,
        type:     state.tool,
        page:     pageNum,
        x:        wrapX,
        y:        wrapY,
        fontSize: parseInt($fsRange.value, 10),
        value:    '',
        checked:  false,
        sigDataUrl: null,
        sigW: 160,
        sigH: 60,
    };

    if (state.tool === 'signature' || state.tool === 'initials') {
        state.sigPending = a;
        $sigModalTitle.textContent = state.tool === 'initials' ? 'Add Initials' : 'Add Signature';
        openSigModal();
        return;
    }
    if (state.tool === 'date') {
        var now = new Date();
        a.value = (now.getMonth()+1) + '/' + now.getDate() + '/' + now.getFullYear();
    }

    state.annotations.push(a);
    drawAnnotation(a);
    updateAnnotList();
    selectAnnot(a.id);
}

/* ── draw annotation (creates DOM overlay) ──────────────────── */
function drawAnnotation(a) {
    // Remove existing overlay for this annotation
    var existing = document.querySelector('[data-annot-id="' + a.id + '"]');
    if (existing) existing.remove();

    var wrap = getPageWrap(a.page);
    if (!wrap) return;

    var overlay = document.createElement('div');
    overlay.className = 'annot-overlay';
    overlay.dataset.annotId = a.id;
    overlay.style.left = a.x + 'px';
    overlay.style.top  = a.y + 'px';
    if (a.id === state.selectedId) overlay.classList.add('selected');

    overlay.addEventListener('mousedown', function (e) {
        if (state.tool !== 'select') return;
        selectAnnot(a.id);
        startDrag(e, overlay, a);
        e.stopPropagation();
    });
    overlay.addEventListener('click', function (e) {
        if (state.tool !== 'select') { e.stopPropagation(); }
        selectAnnot(a.id);
        e.stopPropagation();
    });

    if (a.type === 'text' || a.type === 'date' || a.type === 'initials') {
        var input = document.createElement('input');
        input.className = 'annot-input';
        input.style.setProperty('--annot-fs', a.fontSize + 'px');
        input.style.fontSize = a.fontSize + 'px';
        input.value = a.value;
        if (a.type === 'date') input.type = 'date';
        input.addEventListener('input', function () { a.value = input.value; });
        input.addEventListener('focus', function () { selectAnnot(a.id); });
        input.addEventListener('mousedown', function (e) { e.stopPropagation(); });
        overlay.appendChild(input);
        if (a.type === 'text' || a.type === 'initials') {
            setTimeout(function () { input.focus(); }, 30);
        }
    } else if (a.type === 'checkmark') {
        var box = document.createElement('div');
        box.className = 'annot-check-box' + (a.checked ? ' checked' : '');
        box.textContent = a.checked ? '\u2713' : '';
        box.addEventListener('click', function (e) {
            a.checked = !a.checked;
            box.className = 'annot-check-box' + (a.checked ? ' checked' : '');
            box.textContent = a.checked ? '\u2713' : '';
            e.stopPropagation();
        });
        overlay.appendChild(box);
    } else if (a.type === 'signature' || a.type === 'initials_sig') {
        if (a.sigDataUrl) {
            var img = document.createElement('img');
            img.className = 'annot-sig-img';
            img.src = a.sigDataUrl;
            img.style.width  = a.sigW + 'px';
            img.style.height = a.sigH + 'px';
            overlay.appendChild(img);
            // resize handle
            var rh = document.createElement('div');
            rh.className = 'annot-resize';
            rh.addEventListener('mousedown', function (e) { startResizeSig(e, overlay, img, a); e.stopPropagation(); });
            overlay.appendChild(rh);
        }
    }

    wrap.appendChild(overlay);
}

function redrawAnnotation(a) { drawAnnotation(a); }
function redrawAllAnnotations() {
    state.annotations.forEach(function (a) { drawAnnotation(a); });
}

function findAnnot(id) {
    return state.annotations.find(function (a) { return a.id === id; });
}

function selectAnnot(id) {
    state.selectedId = id;
    document.querySelectorAll('.annot-overlay').forEach(function (el) {
        el.classList.toggle('selected', el.dataset.annotId == id);
    });
    document.querySelectorAll('.annot-item').forEach(function (el) {
        el.classList.toggle('selected', el.dataset.annotId == id);
    });
    var a = findAnnot(id);
    if (a && (a.type === 'text' || a.type === 'date' || a.type === 'initials')) {
        $fsRange.value = a.fontSize;
        $fsVal.textContent = a.fontSize;
    }
}

function deleteAnnot(id) {
    state.annotations = state.annotations.filter(function (a) { return a.id !== id; });
    var el = document.querySelector('[data-annot-id="' + id + '"]');
    if (el) el.remove();
    if (state.selectedId === id) state.selectedId = null;
    updateAnnotList();
}

$btnClearAll.addEventListener('click', function () {
    if (!state.annotations.length) return;
    if (!confirm('Remove all ' + state.annotations.length + ' annotation(s)?')) return;
    state.annotations = [];
    document.querySelectorAll('.annot-overlay').forEach(function (el) { el.remove(); });
    state.selectedId = null;
    updateAnnotList();
    toast('All annotations removed.');
});

/* ── drag / resize ──────────────────────────────────────────── */
function startDrag(e, overlay, a) {
    var startX = e.clientX - a.x;
    var startY = e.clientY - a.y;
    function onMove(ev) {
        var wrap = getPageWrap(a.page);
        if (!wrap) return;
        var wRect = wrap.getBoundingClientRect();
        a.x = Math.max(0, Math.min(ev.clientX - startX, wrap.offsetWidth  - 10));
        a.y = Math.max(0, Math.min(ev.clientY - startY, wrap.offsetHeight - 10));
        overlay.style.left = a.x + 'px';
        overlay.style.top  = a.y + 'px';
    }
    function onUp() {
        document.removeEventListener('mousemove', onMove);
        document.removeEventListener('mouseup', onUp);
    }
    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup', onUp);
}

function startResizeSig(e, overlay, img, a) {
    var startX = e.clientX, startW = a.sigW, startH = a.sigH;
    var aspect = startH / startW;
    function onMove(ev) {
        var dw = ev.clientX - startX;
        a.sigW = Math.max(30, startW + dw);
        a.sigH = Math.round(a.sigW * aspect);
        img.style.width  = a.sigW + 'px';
        img.style.height = a.sigH + 'px';
    }
    function onUp() {
        document.removeEventListener('mousemove', onMove);
        document.removeEventListener('mouseup', onUp);
    }
    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup', onUp);
    e.preventDefault();
}

/* ── annotation sidebar list ────────────────────────────────── */
function typeLabel(t) {
    return { text:'Text', signature:'Signature', checkmark:'Checkmark',
             date:'Date', initials:'Initials', initials_sig:'Initials' }[t] || t;
}
function dotClass(t) {
    if (t === 'signature' || t === 'initials' || t === 'initials_sig') return 'dot-sig';
    if (t === 'checkmark') return 'dot-check';
    if (t === 'date') return 'dot-date';
    return 'dot-text';
}

function updateAnnotList() {
    $annotCount.textContent = state.annotations.length;
    $annotList.innerHTML = '';
    state.annotations.forEach(function (a) {
        var item = document.createElement('div');
        item.className = 'annot-item' + (a.id === state.selectedId ? ' selected' : '');
        item.dataset.annotId = a.id;
        item.onclick = function () { selectAnnot(a.id); scrollAnnotIntoView(a); };

        var dot = document.createElement('span');
        dot.className = 'annot-dot ' + dotClass(a.type);

        var lbl = document.createElement('span');
        lbl.textContent = typeLabel(a.type) + '  p.' + a.page;

        var del = document.createElement('button');
        del.className = 'del-btn';
        del.innerHTML = '&times;';
        del.title = 'Delete';
        del.onclick = function (e) { e.stopPropagation(); deleteAnnot(a.id); };

        item.appendChild(dot);
        item.appendChild(lbl);
        item.appendChild(del);
        $annotList.appendChild(item);
    });
}

function scrollAnnotIntoView(a) {
    var el = document.querySelector('[data-annot-id="' + a.id + '"]');
    if (el) el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

/* ── signature modal ────────────────────────────────────────── */
function openSigModal() {
    clearSigCanvas();
    $sigTypeInput.value = '';
    $sigUploadPreview.style.display = 'none';
    $sigModal.classList.add('open');
}

function closeSigModal() {
    $sigModal.classList.remove('open');
    state.sigPending = null;
}

$sigClose.addEventListener('click', closeSigModal);
$sigCancel.addEventListener('click', closeSigModal);
$sigModal.addEventListener('click', function (e) { if (e.target === $sigModal) closeSigModal(); });

/* sig tabs */
document.querySelectorAll('.sig-tab').forEach(function (tab) {
    tab.addEventListener('click', function () {
        document.querySelectorAll('.sig-tab').forEach(function (t) { t.classList.remove('active'); });
        document.querySelectorAll('.sig-tab-content').forEach(function (c) { c.classList.remove('active'); });
        tab.classList.add('active');
        state.sigTool = tab.dataset.sigtab;
        document.getElementById('sigtab-' + state.sigTool).classList.add('active');
    });
});

/* sig canvas draw */
function clearSigCanvas() {
    var ctx = state.sigCtx;
    ctx.clearRect(0, 0, $sigCanvas.width, $sigCanvas.height);
    $sigHint.style.display = '';
}

$sigCanvasClear.addEventListener('click', clearSigCanvas);

function clientXY(e) {
    if (e.touches) return { x: e.touches[0].clientX, y: e.touches[0].clientY };
    return { x: e.clientX, y: e.clientY };
}

$sigCanvas.addEventListener('pointerdown', function (e) {
    state.sigDrawing = true;
    var rect = $sigCanvas.getBoundingClientRect();
    var scaleX = $sigCanvas.width  / rect.width;
    var scaleY = $sigCanvas.height / rect.height;
    var pt = clientXY(e);
    state.sigLastX = (pt.x - rect.left) * scaleX;
    state.sigLastY = (pt.y - rect.top)  * scaleY;
    $sigHint.style.display = 'none';
    e.preventDefault();
});

$sigCanvas.addEventListener('pointermove', function (e) {
    if (!state.sigDrawing) return;
    var rect = $sigCanvas.getBoundingClientRect();
    var scaleX = $sigCanvas.width  / rect.width;
    var scaleY = $sigCanvas.height / rect.height;
    var pt = clientXY(e);
    var nx = (pt.x - rect.left) * scaleX;
    var ny = (pt.y - rect.top)  * scaleY;
    var ctx = state.sigCtx;
    ctx.beginPath();
    ctx.moveTo(state.sigLastX, state.sigLastY);
    ctx.lineTo(nx, ny);
    ctx.strokeStyle = $sigColor.value;
    ctx.lineWidth   = parseFloat($sigWeight.value);
    ctx.lineCap     = 'round';
    ctx.lineJoin    = 'round';
    ctx.stroke();
    state.sigLastX = nx;
    state.sigLastY = ny;
    e.preventDefault();
});

$sigCanvas.addEventListener('pointerup',    function () { state.sigDrawing = false; });
$sigCanvas.addEventListener('pointerleave', function () { state.sigDrawing = false; });

/* sig upload */
$sigUploadArea.addEventListener('click', function () { $sigUploadInput.click(); });
$sigUploadArea.addEventListener('dragover', function (e) { e.preventDefault(); $sigUploadArea.style.borderColor = 'var(--accent2)'; });
$sigUploadArea.addEventListener('dragleave', function () { $sigUploadArea.style.borderColor = ''; });
$sigUploadArea.addEventListener('drop', function (e) {
    e.preventDefault();
    $sigUploadArea.style.borderColor = '';
    var file = e.dataTransfer.files[0];
    if (file) loadUploadedSigImage(file);
});
$sigUploadInput.addEventListener('change', function () {
    if (this.files[0]) loadUploadedSigImage(this.files[0]);
});

function loadUploadedSigImage(file) {
    var reader = new FileReader();
    reader.onload = function (e) {
        $sigUploadPreview.src = e.target.result;
        $sigUploadPreview.style.display = 'block';
    };
    reader.readAsDataURL(file);
}

/* ── collect signature data URL ─────────────────────────────── */
function getSigDataUrl() {
    if (state.sigTool === 'draw') {
        // Check if anything was drawn
        var ctx = state.sigCtx;
        var d   = ctx.getImageData(0, 0, $sigCanvas.width, $sigCanvas.height).data;
        var hasPixels = false;
        for (var i = 3; i < d.length; i += 4) { if (d[i] > 10) { hasPixels = true; break; } }
        if (!hasPixels) { toast('Please draw a signature first.'); return null; }
        return $sigCanvas.toDataURL('image/png');
    }
    if (state.sigTool === 'type') {
        var text = $sigTypeInput.value.trim();
        if (!text) { toast('Please type a signature.'); return null; }
        var oc  = document.createElement('canvas');
        oc.width = 400; oc.height = 100;
        var ctx = oc.getContext('2d');
        ctx.fillStyle   = 'transparent';
        ctx.font        = '52px "Dancing Script", cursive';
        ctx.fillStyle   = '#1a1a8c';
        ctx.textBaseline = 'middle';
        var tm = ctx.measureText(text);
        var scale = Math.min(1, (oc.width - 20) / tm.width);
        ctx.setTransform(scale, 0, 0, scale, 10, oc.height / 2);
        ctx.fillText(text, 0, 0);
        return oc.toDataURL('image/png');
    }
    if (state.sigTool === 'upload') {
        if (!$sigUploadPreview.src || $sigUploadPreview.style.display === 'none') {
            toast('Please upload a signature image.'); return null;
        }
        return $sigUploadPreview.src;
    }
    return null;
}

$sigApply.addEventListener('click', function () {
    var dataUrl = getSigDataUrl();
    if (!dataUrl) return;
    if (!state.sigPending) { closeSigModal(); return; }

    var a = state.sigPending;
    a.sigDataUrl = dataUrl;
    state.annotations.push(a);
    drawAnnotation(a);
    updateAnnotList();
    selectAnnot(a.id);
    closeSigModal();
});

/* ── file drop / input ──────────────────────────────────────── */
$fileInput.addEventListener('change', function () { if (this.files[0]) openPDFFile(this.files[0]); });
$fileInput2.addEventListener('change', function () { if (this.files[0]) openPDFFile(this.files[0]); });

document.addEventListener('dragover', function (e) {
    e.preventDefault();
    if (!state.pdfDoc) $dzBox.classList.add('drag-over');
});
document.addEventListener('dragleave', function () { $dzBox.classList.remove('drag-over'); });
document.addEventListener('drop', function (e) {
    e.preventDefault();
    $dzBox.classList.remove('drag-over');
    var file = e.dataTransfer.files[0];
    if (file) openPDFFile(file);
});

/* ── keyboard shortcuts ─────────────────────────────────────── */
document.addEventListener('keydown', function (e) {
    if (e.target.tagName === 'INPUT') return;
    if (e.key === 'Delete' || e.key === 'Backspace') {
        if (state.selectedId != null) { deleteAnnot(state.selectedId); updateAnnotList(); }
    }
    if (e.key === 'Escape') { state.selectedId = null; redrawAllAnnotations(); }
    if ((e.metaKey || e.ctrlKey) && e.key === 'z') {
        if (state.annotations.length) {
            var last = state.annotations[state.annotations.length - 1];
            deleteAnnot(last.id);
            updateAnnotList();
        }
        e.preventDefault();
    }
    if ((e.metaKey || e.ctrlKey) && e.key === 's') { e.preventDefault(); doDownload(); }
});

/* ── PDF download with pdf-lib ───────────────────────────────── */
async function doDownload() {
    if (!state.pdfBytes) { toast('No PDF loaded.'); return; }
    $btnDownload.disabled = true;
    toast('Building PDF...', 8000);

    try {
        const { PDFDocument, rgb, StandardFonts } = PDFLib;
        const pdfDoc = await PDFDocument.load(state.pdfBytes);
        pdfDoc.registerFontkit(fontkit);

        const font  = await pdfDoc.embedFont(StandardFonts.Helvetica);
        const pages = pdfDoc.getPages();

        // Group annotations by page
        for (const a of state.annotations) {
            const pdfPage = pages[a.page - 1];
            if (!pdfPage) continue;
            const { width, height } = pdfPage.getSize();

            // Convert pixel coords (scaled) to PDF points
            // PDF.js renders at state.scale; PDF native points = 72dpi
            var px2pt = 1.0 / state.scale;
            var pdfX  = a.x * px2pt;
            // PDF Y axis is bottom-up; PDF.js canvas Y is top-down
            // Get rendered page height from page wrap
            var wrap = getPageWrap(a.page);
            var renderedH = wrap ? wrap.offsetHeight : height;
            var pdfY = height - (a.y * px2pt);

            if (a.type === 'text' || a.type === 'date' || a.type === 'initials') {
                var val = a.type === 'date'
                    ? (a.value ? new Date(a.value + 'T12:00:00').toLocaleDateString() : '')
                    : (a.value || '');
                if (!val) continue;
                pdfPage.drawText(val, {
                    x:    pdfX,
                    y:    pdfY - (a.fontSize * px2pt),
                    size: a.fontSize * px2pt,
                    font: font,
                    color: rgb(0.08, 0.08, 0.08),
                });
            } else if (a.type === 'checkmark') {
                if (!a.checked) continue;
                pdfPage.drawText('\u2713', {
                    x: pdfX,
                    y: pdfY - (14 * px2pt),
                    size: 14 * px2pt,
                    font: font,
                    color: rgb(0.08, 0.2, 0.54),
                });
            } else if (a.type === 'signature') {
                if (!a.sigDataUrl) continue;
                var imgBytes;
                if (a.sigDataUrl.startsWith('data:image/png')) {
                    imgBytes = dataUrlToUint8Array(a.sigDataUrl);
                    var embeddedImg = await pdfDoc.embedPng(imgBytes);
                } else {
                    imgBytes = dataUrlToUint8Array(a.sigDataUrl);
                    var embeddedImg = await pdfDoc.embedJpg(imgBytes);
                }
                var sigW = a.sigW * px2pt;
                var sigH = a.sigH * px2pt;
                pdfPage.drawImage(embeddedImg, {
                    x:      pdfX,
                    y:      pdfY - sigH,
                    width:  sigW,
                    height: sigH,
                });
            }
        }

        const pdfBytesOut = await pdfDoc.save();
        const blob = new Blob([pdfBytesOut], { type: 'application/pdf' });
        const url  = URL.createObjectURL(blob);
        const a    = document.createElement('a');
        a.href     = url;
        a.download = ($fileName.textContent || 'document').replace(/\.pdf$/i, '') + '-signed.pdf';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        setTimeout(function () { URL.revokeObjectURL(url); }, 5000);
        toast('Downloaded!');
    } catch (err) {
        toast('Export error: ' + (err.message || err));
        console.error(err);
    }
    $btnDownload.disabled = false;
}

function dataUrlToUint8Array(dataUrl) {
    var b64 = dataUrl.split(',')[1];
    var bin = atob(b64);
    var arr = new Uint8Array(bin.length);
    for (var i = 0; i < bin.length; i++) arr[i] = bin.charCodeAt(i);
    return arr;
}

$btnDownload.addEventListener('click', doDownload);
</script>
</body>
</html>
