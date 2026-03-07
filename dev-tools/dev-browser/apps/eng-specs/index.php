<?php
/**
 * eng-specs/index.php
 *
 * Engine diagnostics panel for the xcm render core.
 * Reports binary info, render latency, GPU caps, display DPR,
 * WASM heap, input lag, and worker channel round-trip.
 */

// PHP-side static info (filesystem, does not need JS)
$src_dir   = dirname(dirname(__DIR__)) . '/src';
$wasm_path = $src_dir . '/render_core.wasm';
$glue_path = $src_dir . '/render_core.js';

function fmt_bytes($n) {
    if ($n === false || $n === null) return 'n/a';
    if ($n >= 1048576) return round($n / 1048576, 2) . ' MB';
    if ($n >= 1024)    return round($n / 1024, 1)    . ' KB';
    return $n . ' B';
}

$wasm_size  = file_exists($wasm_path) ? filesize($wasm_path) : false;
$glue_size  = file_exists($glue_path) ? filesize($glue_path) : false;
$wasm_mtime = file_exists($wasm_path) ? date('Y-m-d H:i', filemtime($wasm_path)) : 'n/a';
$wasm_size_fmt = fmt_bytes($wasm_size);
$glue_size_fmt = fmt_bytes($glue_size);
?><!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>ENG Specs</title>
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#080810;--surface:#0f0f1c;--card:#13131f;--bdr:#1e1e38;
  --acc:#6366f1;--acc2:#818cf8;--acc3:#a5b4fc;
  --text:#e2e8f0;--sub:#64748b;--dim:#374151;
  --ok:#34d399;--warn:#fbbf24;--bad:#f87171;--info:#38bdf8;
  --mono:'JetBrains Mono','Fira Code','Cascadia Code',monospace;
}
html{height:100%;overflow-x:hidden}
body{
  font-family:'Segoe UI',system-ui,-apple-system,sans-serif;
  background:var(--bg);color:var(--text);
  padding:1.4rem 1.6rem 2rem;min-height:100%;
  font-size:14px;line-height:1.5;
}

/* Header */
.hdr{display:flex;align-items:baseline;gap:1rem;margin-bottom:1.4rem;flex-wrap:wrap}
.hdr h1{font-size:1.3rem;color:var(--acc);letter-spacing:-.01em;font-weight:700}
.hdr .sub{color:var(--sub);font-size:.8rem}

/* Action bar */
.actions{display:flex;gap:.6rem;margin-bottom:1.4rem;flex-wrap:wrap}
button{
  padding:.45rem 1.1rem;border:1px solid var(--bdr);
  border-radius:6px;cursor:pointer;font-size:.82rem;
  font-family:var(--mono);transition:all .15s;
}
button.primary{background:var(--acc);color:#fff;border-color:var(--acc)}
button.primary:hover{background:#4f46e5}
button.primary:disabled{background:#3d3d5c;border-color:#3d3d5c;cursor:not-allowed}
button.ghost{background:transparent;color:var(--sub)}
button.ghost:hover{color:var(--text);border-color:var(--acc2)}

/* Grid layouts */
.grid{display:grid;gap:1rem;margin-bottom:1rem}
.g2{grid-template-columns:repeat(2,1fr)}
.g3{grid-template-columns:repeat(3,1fr)}
.g4{grid-template-columns:repeat(4,1fr)}
@media(max-width:900px){.g4{grid-template-columns:repeat(2,1fr)}.g3{grid-template-columns:repeat(2,1fr)}}
@media(max-width:600px){.g2,.g3,.g4{grid-template-columns:1fr}}

/* Card */
.card{
  background:var(--card);border:1px solid var(--bdr);
  border-radius:9px;padding:1rem 1.1rem;
}
.card h2{
  font-size:.72rem;letter-spacing:.1em;text-transform:uppercase;
  color:var(--acc2);margin-bottom:.8rem;font-weight:600;
}

/* Stat rows */
.stat{display:flex;justify-content:space-between;align-items:baseline;
  padding:.22rem 0;border-bottom:1px solid rgba(255,255,255,.04);
  gap:.5rem;flex-wrap:wrap;}
.stat:last-child{border-bottom:none}
.stat .lbl{color:var(--sub);font-size:.77rem;white-space:nowrap}
.stat .val{
  font-family:var(--mono);font-size:.8rem;color:var(--text);
  text-align:right;word-break:break-all;
}
.val.ok{color:var(--ok)}
.val.warn{color:var(--warn)}
.val.bad{color:var(--bad)}
.val.info{color:var(--info)}
.val.dim{color:var(--dim)}

/* Benchmark result table */
.bench-row{
  display:grid;grid-template-columns:auto 1fr auto auto auto;
  gap:.4rem .8rem;align-items:center;padding:.2rem 0;
  font-family:var(--mono);font-size:.78rem;
  border-bottom:1px solid rgba(255,255,255,.04);
}
.bench-row:last-child{border-bottom:none}
.bench-row .lbl{color:var(--sub)}
.bench-row .bar{height:5px;border-radius:3px;background:var(--acc);min-width:2px}
.bench-row .num{color:var(--text);text-align:right}

/* Progress bar */
.progress-wrap{height:3px;background:var(--bdr);border-radius:2px;margin:.4rem 0}
.progress-fill{height:100%;border-radius:2px;background:var(--acc);width:0;
  transition:width .1s linear}

/* Input lag tap zone */
#lag-zone{
  border:1.5px dashed var(--bdr);border-radius:7px;
  padding:1rem;text-align:center;cursor:pointer;
  color:var(--sub);font-size:.82rem;user-select:none;
  transition:border-color .15s;margin-top:.6rem;touch-action:none;
}
#lag-zone:hover{border-color:var(--acc2);color:var(--text)}
#lag-zone.active{border-color:var(--ok);color:var(--ok)}

/* Status chip */
.chip{
  display:inline-block;padding:.15rem .5rem;
  border-radius:4px;font-size:.7rem;font-family:var(--mono);
  font-weight:600;letter-spacing:.04em;
}
.chip.ok{background:rgba(52,211,153,.15);color:var(--ok)}
.chip.warn{background:rgba(251,191,36,.15);color:var(--warn)}
.chip.bad{background:rgba(248,113,113,.15);color:var(--bad)}
.chip.info{background:rgba(56,189,248,.15);color:var(--info)}
.chip.idle{background:rgba(100,116,139,.15);color:var(--sub)}

/* Log strip */
#log{
  font-family:var(--mono);font-size:.75rem;color:var(--sub);
  line-height:1.7;max-height:90px;overflow-y:auto;
  margin-top:.5rem;
}
#log .ok{color:var(--ok)} #log .warn{color:var(--warn)} #log .bad{color:var(--bad)}
#log .step{color:var(--acc2)}
</style>
</head>
<body>

<div class="hdr">
  <h1>ENG SPECS</h1>
  <span class="sub">xcm render core diagnostics</span>
  <span id="run-chip" class="chip idle">IDLE</span>
</div>

<div class="actions">
  <button class="primary" id="btn-bench">Run Benchmark</button>
  <button class="ghost"   id="btn-reset">Reset</button>
</div>

<div id="log"></div>

<!-- Row 1: Binary + Display -->
<div class="grid g2">
  <div class="card">
    <h2>Binary</h2>
    <div class="stat"><span class="lbl">render_core.wasm</span><span class="val info" id="bi-wasm"><?= htmlspecialchars($wasm_size_fmt) ?></span></div>
    <div class="stat"><span class="lbl">render_core.js glue</span><span class="val info" id="bi-glue"><?= htmlspecialchars($glue_size_fmt) ?></span></div>
    <div class="stat"><span class="lbl">build timestamp</span><span class="val" id="bi-mtime"><?= htmlspecialchars($wasm_mtime) ?></span></div>
    <div class="stat"><span class="lbl">wasm + glue total</span><span class="val" id="bi-total"><?php
      if ($wasm_size !== false && $glue_size !== false)
          echo fmt_bytes($wasm_size + $glue_size);
      else echo 'n/a';
    ?></span></div>
    <div class="stat"><span class="lbl">instantiation time</span><span class="val dim" id="bi-init">--</span></div>
    <div class="stat"><span class="lbl">WASM heap (initial)</span><span class="val dim" id="bi-heap">--</span></div>
    <div class="stat"><span class="lbl">memory pages</span><span class="val dim" id="bi-pages">--</span></div>
  </div>

  <div class="card">
    <h2>Display</h2>
    <div class="stat"><span class="lbl">device pixel ratio</span><span class="val info" id="di-dpr">--</span></div>
    <div class="stat"><span class="lbl">physical resolution</span><span class="val" id="di-phys">--</span></div>
    <div class="stat"><span class="lbl">logical viewport</span><span class="val" id="di-css">--</span></div>
    <div class="stat"><span class="lbl">screen color depth</span><span class="val" id="di-depth">--</span></div>
    <div class="stat"><span class="lbl">color gamut</span><span class="val" id="di-gamut">--</span></div>
    <div class="stat"><span class="lbl">detected refresh</span><span class="val info" id="di-hz">--</span></div>
    <div class="stat"><span class="lbl">HDR</span><span class="val" id="di-hdr">--</span></div>
  </div>
</div>

<!-- Row 2: GPU + Memory -->
<div class="grid g2">
  <div class="card">
    <h2>GPU / WebGL2</h2>
    <div class="stat"><span class="lbl">renderer</span><span class="val" id="gl-renderer">--</span></div>
    <div class="stat"><span class="lbl">vendor</span><span class="val" id="gl-vendor">--</span></div>
    <div class="stat"><span class="lbl">GLSL version</span><span class="val" id="gl-glsl">--</span></div>
    <div class="stat"><span class="lbl">max texture size</span><span class="val info" id="gl-tex">--</span></div>
    <div class="stat"><span class="lbl">max viewport</span><span class="val" id="gl-vp">--</span></div>
    <div class="stat"><span class="lbl">max draw buffers</span><span class="val" id="gl-draw">--</span></div>
    <div class="stat"><span class="lbl">anisotropy</span><span class="val" id="gl-aniso">--</span></div>
    <div class="stat"><span class="lbl">float textures</span><span class="val" id="gl-float">--</span></div>
  </div>

  <div class="card">
    <h2>Memory</h2>
    <div class="stat"><span class="lbl">JS heap used</span><span class="val" id="mem-used">--</span></div>
    <div class="stat"><span class="lbl">JS heap total</span><span class="val" id="mem-total">--</span></div>
    <div class="stat"><span class="lbl">JS heap limit</span><span class="val" id="mem-limit">--</span></div>
    <div class="stat"><span class="lbl">WASM heap (post-benchmark)</span><span class="val dim" id="mem-wasm">--</span></div>
    <div class="stat"><span class="lbl">WASM pages used</span><span class="val dim" id="mem-wpages">--</span></div>
    <div class="stat"><span class="lbl">device RAM</span><span class="val" id="mem-ram">--</span></div>
    <div class="stat"><span class="lbl">CPU concurrency</span><span class="val" id="mem-cpu">--</span></div>
  </div>
</div>

<!-- Row 3: Render Benchmark (full width) -->
<div class="grid" style="grid-template-columns:1fr">
  <div class="card">
    <h2>Render Benchmark <span id="bench-status" class="chip idle" style="font-size:.65rem;margin-left:.4rem">not run</span></h2>
    <div class="progress-wrap"><div class="progress-fill" id="bench-prog"></div></div>
    <div id="bench-results" style="margin-top:.6rem">
      <div class="stat" style="color:var(--sub);font-size:.78rem">Click "Run Benchmark" to measure layout and paint latency across viewport sizes.</div>
    </div>
  </div>
</div>

<!-- Row 4: Input Lag + Worker Channel -->
<div class="grid g2">
  <div class="card">
    <h2>Input Lag</h2>
    <div class="stat"><span class="lbl">event-to-rAF (avg)</span><span class="val dim" id="lag-avg">--</span></div>
    <div class="stat"><span class="lbl">event-to-rAF (min)</span><span class="val dim" id="lag-min">--</span></div>
    <div class="stat"><span class="lbl">event-to-rAF (max)</span><span class="val dim" id="lag-max">--</span></div>
    <div class="stat"><span class="lbl">samples collected</span><span class="val dim" id="lag-n">0 / 10</span></div>
    <div id="lag-zone">Tap or click here 10 times to measure input lag</div>
  </div>

  <div class="card">
    <h2>Worker Channel</h2>
    <div class="stat"><span class="lbl">MessageChannel RTT (avg)</span><span class="val dim" id="wch-avg">--</span></div>
    <div class="stat"><span class="lbl">MessageChannel RTT (min)</span><span class="val dim" id="wch-min">--</span></div>
    <div class="stat"><span class="lbl">MessageChannel RTT (max)</span><span class="val dim" id="wch-max">--</span></div>
    <div class="stat"><span class="lbl">postMessage RTT (avg)</span><span class="val dim" id="wpm-avg">--</span></div>
    <div class="stat"><span class="lbl">samples</span><span class="val dim" id="wch-n">--</span></div>
    <div class="stat"><span class="lbl">status</span><span id="wch-status" class="chip idle">--</span></div>
  </div>
</div>

<!-- Row 5: Frame Stats (live) -->
<div class="grid" style="grid-template-columns:1fr">
  <div class="card">
    <h2>Frame Stats (live rAF) <span id="fs-live" class="chip idle" style="font-size:.65rem;margin-left:.4rem">measuring</span></h2>
    <div class="grid g4">
      <div><div class="stat"><span class="lbl">fps (last)</span><span class="val ok" id="fs-fps">--</span></div></div>
      <div><div class="stat"><span class="lbl">fps (avg 2s)</span><span class="val" id="fs-avg">--</span></div></div>
      <div><div class="stat"><span class="lbl">frame time</span><span class="val" id="fs-ft">--</span></div></div>
      <div><div class="stat"><span class="lbl">budget</span><span class="val" id="fs-budget">--</span></div></div>
      <div><div class="stat"><span class="lbl">dropped frames</span><span class="val warn" id="fs-drop">--</span></div></div>
      <div><div class="stat"><span class="lbl">long task avg</span><span class="val" id="fs-lt">--</span></div></div>
      <div><div class="stat"><span class="lbl">long task count</span><span class="val" id="fs-ltc">--</span></div></div>
      <div><div class="stat"><span class="lbl">display Hz</span><span class="val info" id="fs-hz">--</span></div></div>
    </div>
  </div>
</div>

<script>
'use strict';

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------
function $id(id) { return document.getElementById(id); }
function set(id, text, cls) {
    var el = $id(id);
    if (!el) return;
    el.textContent = text;
    if (cls) { el.className = 'val ' + cls; }
}
function fmtMs(v) { return (typeof v === 'number') ? v.toFixed(2) + ' ms' : '--'; }
function fmtMB(bytes) {
    if (!bytes) return '--';
    return (bytes / 1048576).toFixed(2) + ' MB';
}
function log(msg, cls) {
    var el = $id('log');
    var line = document.createElement('div');
    if (cls) line.className = cls;
    line.textContent = msg;
    el.appendChild(line);
    el.scrollTop = el.scrollHeight;
}
function chip(id, label, cls) {
    var el = $id(id);
    if (!el) return;
    el.textContent = label;
    el.className   = 'chip ' + (cls || 'idle');
}

// ---------------------------------------------------------------------------
// Display info (immediate)
// ---------------------------------------------------------------------------
(function detectDisplay() {
    var dpr = window.devicePixelRatio || 1;
    set('di-dpr',  dpr.toFixed(2) + 'x', dpr >= 2 ? 'ok' : 'warn');
    set('di-phys', (screen.width * dpr | 0) + ' x ' + (screen.height * dpr | 0));
    set('di-css',  window.innerWidth + ' x ' + window.innerHeight + ' (logical)');
    set('di-depth', screen.colorDepth + '-bit');

    var gamut = 'sRGB';
    if (window.matchMedia('(color-gamut: p3)').matches)  gamut = 'P3';
    if (window.matchMedia('(color-gamut: rec2020)').matches) gamut = 'Rec.2020';
    set('di-gamut', gamut, gamut !== 'sRGB' ? 'ok' : '');

    var hdr = window.matchMedia('(dynamic-range: high)').matches ? 'yes' : 'no';
    set('di-hdr', hdr, hdr === 'yes' ? 'ok' : '');

    // Detect Hz via two rAF deltas
    var _ts = [];
    (function _m(ts) {
        _ts.push(ts);
        if (_ts.length < 10) { requestAnimationFrame(_m); return; }
        var diffs = [];
        for (var i = 1; i < _ts.length; i++) diffs.push(_ts[i] - _ts[i-1]);
        var avg = diffs.reduce(function(a,b){return a+b},0) / diffs.length;
        var raw = Math.round(1000 / avg);
        var snaps = [24,30,60,90,120,144,165,240], best = snaps[0];
        for (var j = 1; j < snaps.length; j++)
            if (Math.abs(snaps[j]-raw) < Math.abs(best-raw)) best = snaps[j];
        set('di-hz', best + ' Hz', best >= 90 ? 'ok' : best >= 60 ? 'info' : 'warn');
    })(performance.now());
})();

// ---------------------------------------------------------------------------
// Memory info (static read - performance.memory is Chromium only)
// ---------------------------------------------------------------------------
(function detectMemory() {
    set('mem-ram', navigator.deviceMemory ? navigator.deviceMemory + ' GB' : 'n/a');
    set('mem-cpu', navigator.hardwareConcurrency ? navigator.hardwareConcurrency + ' cores' : 'n/a');
    function readMem() {
        var m = performance.memory;
        if (!m) { set('mem-used','n/a'); set('mem-total','n/a'); set('mem-limit','n/a'); return; }
        set('mem-used',  fmtMB(m.usedJSHeapSize));
        set('mem-total', fmtMB(m.totalJSHeapSize));
        set('mem-limit', fmtMB(m.jsHeapSizeLimit));
    }
    readMem();
    setInterval(readMem, 2000);
})();

// ---------------------------------------------------------------------------
// WebGL2 caps
// ---------------------------------------------------------------------------
(function detectGPU() {
    var cvs = document.createElement('canvas');
    var gl  = cvs.getContext('webgl2', { powerPreference: 'high-performance' });
    if (!gl) { set('gl-renderer','WebGL2 unavailable','bad'); return; }

    // Try unmasked renderer info (works in Chromium, may return '' in Firefox)
    var dbgExt = gl.getExtension('WEBGL_debug_renderer_info');
    var renderer = dbgExt
        ? gl.getParameter(dbgExt.UNMASKED_RENDERER_WEBGL)
        : gl.getParameter(gl.RENDERER);
    var vendor = dbgExt
        ? gl.getParameter(dbgExt.UNMASKED_VENDOR_WEBGL)
        : gl.getParameter(gl.VENDOR);

    var maxTex  = gl.getParameter(gl.MAX_TEXTURE_SIZE);
    var maxVP   = gl.getParameter(gl.MAX_VIEWPORT_DIMS);
    var maxDraw = gl.getParameter(gl.MAX_DRAW_BUFFERS);

    var anisoExt = gl.getExtension('EXT_texture_filter_anisotropic');
    var aniso = anisoExt ? gl.getParameter(anisoExt.MAX_TEXTURE_MAX_ANISOTROPY_EXT) : 'n/a';

    var floatExt = gl.getExtension('EXT_color_buffer_float') ? 'yes' : 'no';

    set('gl-renderer', renderer || 'n/a');
    set('gl-vendor',   vendor   || 'n/a');
    set('gl-glsl',     gl.getParameter(gl.SHADING_LANGUAGE_VERSION));
    set('gl-tex',      maxTex + ' px', maxTex >= 8192 ? 'ok' : 'warn');
    set('gl-vp',       maxVP[0] + ' x ' + maxVP[1]);
    set('gl-draw',     maxDraw);
    set('gl-aniso',    aniso + 'x');
    set('gl-float',    floatExt, floatExt === 'yes' ? 'ok' : '');

    gl.getExtension('WEBGL_lose_context') &&
        gl.getExtension('WEBGL_lose_context').loseContext();
})();

// ---------------------------------------------------------------------------
// Live frame stats (from __xcmStats if present, else inline ring)
// ---------------------------------------------------------------------------
(function liveFrameStats() {
    // ts/ft hold entries for the 2-second sliding window.
    // dropTs holds timestamps of dropped frames in the same window.
    var ts = [], ft = [], dropTs = [], _prev = 0;

    function tick(now) {
        if (_prev) {
            var d = now - _prev;
            var hz = window.__xcmHz || 60;
            var frameBudget = 1000 / hz;
            ft.push(d);
            ts.push(now);
            // A frame is "dropped" when its duration exceeds 1.5x the budget
            if (d > frameBudget * 1.5) dropTs.push(now);
        }
        _prev = now;
        // Evict entries older than 2 s
        var cut = now - 2000;
        while (ts.length && ts[0] < cut) { ts.shift(); ft.shift(); }
        while (dropTs.length && dropTs[0] < cut) dropTs.shift();
        requestAnimationFrame(tick);
    }
    requestAnimationFrame(tick);

    function avg(arr) {
        if (!arr.length) return 0;
        return arr.reduce(function(a,b){return a+b}, 0) / arr.length;
    }

    setInterval(function() {
        var s = window.__xcmStats || (window.__xcm && window.__xcm.stats);
        var fps, fpsAvg, frameTime, ltAvg, ltCount, bgt, dropCount;
        var devHz = window.__xcmHz || 0;
        var targetBudget = devHz ? (1000 / devHz) : (1000 / 60);

        if (s && typeof s.fps === 'function') {
            fps       = +s.fps()           || 0;
            fpsAvg    = +s.fpsAvg()        || 0;
            frameTime = +s.frameTime()     || 0;
            ltAvg     = +s.longTaskAvg()   || 0;
            ltCount   = +s.longTaskCount() || 0;
            bgt       = +s.budget()        || targetBudget;
            dropCount = +s.dropped()       || 0;
        } else {
            // fps (last): count frames in the last 1-second window
            var now  = performance.now();
            var cut1 = now - 1000;
            var n1   = 0;
            for (var i = ts.length - 1; i >= 0; i--) {
                if (ts[i] >= cut1) n1++;
                else break;
            }
            fps       = n1;
            // fps (avg 2s): total frames in 2s window divided by window length
            fpsAvg    = Math.round(ts.length / 2);
            frameTime = ft.length ? avg(ft) : 0;
            ltAvg     = 0; ltCount = 0;
            bgt       = targetBudget;
            // drops in the current 2-second window (windowed, not cumulative)
            dropCount = dropTs.length;
        }

        var fpsCls = fps >= 50 ? 'ok' : fps >= 30 ? 'warn' : fps > 0 ? 'bad' : 'dim';

        set('fs-fps',    fps    ? fps.toFixed(0)    : '--', fpsCls);
        set('fs-avg',    fpsAvg ? fpsAvg.toFixed(0) : '--');
        set('fs-ft',     frameTime ? fmtMs(frameTime) : '--');
        set('fs-budget', bgt ? fmtMs(bgt) : '--');
        set('fs-drop',   dropCount, dropCount > 0 ? 'warn' : 'ok');
        set('fs-lt',     ltAvg ? fmtMs(ltAvg) : 'n/a');
        set('fs-ltc',    ltCount || '0');
        set('fs-hz',     devHz ? devHz + ' Hz' : '--', devHz >= 90 ? 'ok' : 'info');
        chip('fs-live', fps ? 'live' : 'warming up', fps ? 'ok' : 'warn');
    }, 500);
})();

// ---------------------------------------------------------------------------
// Input lag measurement
// ---------------------------------------------------------------------------
(function inputLag() {
    var samples = [], target = 10;
    var zone = $id('lag-zone');

    function update() {
        var n = samples.length;
        set('lag-n', n + ' / ' + target);
        if (!n) return;
        var s = 0, mn = Infinity, mx = -Infinity;
        for (var i=0;i<n;i++){s+=samples[i];if(samples[i]<mn)mn=samples[i];if(samples[i]>mx)mx=samples[i];}
        var avg = s / n;
        var cls = avg < 5 ? 'ok' : avg < 16 ? 'warn' : 'bad';
        set('lag-avg', fmtMs(avg), cls);
        set('lag-min', fmtMs(mn),  mn < 5 ? 'ok' : '');
        set('lag-max', fmtMs(mx),  mx > 16 ? 'bad' : '');
    }

    function onPointer(e) {
        if (samples.length >= target) {
            zone.textContent = 'Done. ' + target + ' samples collected.';
            zone.className = 'active';
            zone.removeEventListener('pointerdown', onPointer);
            return;
        }
        var evTs = e.timeStamp || performance.now();
        requestAnimationFrame(function(rafTs) {
            var lag = rafTs - evTs;
            if (lag >= 0 && lag < 500) {
                samples.push(lag);
                update();
                zone.textContent = samples.length + ' / ' + target + ' taps -- keep going';
            }
        });
    }
    zone.addEventListener('pointerdown', onPointer);
})();

// ---------------------------------------------------------------------------
// Worker channel round-trip
// ---------------------------------------------------------------------------
(function workerRTT() {
    chip('wch-status', 'testing', 'info');
    var SAMPLES = 200;

    // Inline echo worker via Blob URL
    var src = 'self.onmessage=function(e){self.postMessage(e.data);}';
    var blob = new Blob([src], { type: 'application/javascript' });
    var url  = URL.createObjectURL(blob);

    var worker = new Worker(url);
    var postSamples = [];
    var portSamples = [];
    var mc = new MessageChannel();

    // 1. Test plain postMessage round-trip
    function testPostMessage(done) {
        var pending = SAMPLES, times = [];
        function send() {
            var t0 = performance.now();
            worker.onmessage = function() {
                times.push(performance.now() - t0);
                if (--pending > 0) send();
                else done(times);
            };
            worker.postMessage(1);
        }
        send();
    }

    // 2. Test MessageChannel round-trip (dedicated port)
    function testPort(done) {
        var echoSrc = 'self.onmessage=function(e){if(e.data.port){self._p=e.data.port;self._p.onmessage=function(m){self._p.postMessage(m.data);}}else{self.postMessage(e.data);}}';
        var echoBlob = new Blob([echoSrc], {type:'application/javascript'});
        var w2 = new Worker(URL.createObjectURL(echoBlob));
        var mc2 = new MessageChannel();
        w2.postMessage({port: mc2.port2}, [mc2.port2]);

        var pending = SAMPLES, times = [];
        function send() {
            var t0 = performance.now();
            mc2.port1.onmessage = function() {
                times.push(performance.now() - t0);
                if (--pending > 0) send();
                else { w2.terminate(); done(times); }
            };
            mc2.port1.postMessage(1);
        }
        send();
    }

    function stats(arr) {
        if (!arr.length) return {avg:0,min:0,max:0};
        var s=0,mn=Infinity,mx=-Infinity;
        for(var i=0;i<arr.length;i++){s+=arr[i];if(arr[i]<mn)mn=arr[i];if(arr[i]>mx)mx=arr[i];}
        return {avg:s/arr.length, min:mn, max:mx};
    }

    testPostMessage(function(pmTimes) {
        var pm = stats(pmTimes);
        set('wpm-avg', fmtMs(pm.avg), pm.avg < 1 ? 'ok' : pm.avg < 3 ? 'warn' : 'bad');

        testPort(function(portTimes) {
            var pt = stats(portTimes);
            var cls = pt.avg < 1 ? 'ok' : pt.avg < 3 ? 'warn' : 'bad';
            set('wch-avg', fmtMs(pt.avg), cls);
            set('wch-min', fmtMs(pt.min), pt.min < 1 ? 'ok' : '');
            set('wch-max', fmtMs(pt.max), pt.max > 5 ? 'bad' : '');
            set('wch-n', SAMPLES + ' pM + ' + SAMPLES + ' port');
            chip('wch-status', cls === 'ok' ? 'fast' : cls === 'warn' ? 'ok' : 'slow', cls);
            worker.terminate();
            URL.revokeObjectURL(url);
        });
    });
})();

// ---------------------------------------------------------------------------
// WASM benchmark
// ---------------------------------------------------------------------------
var _wasmModule = null;

function progress(pct) {
    var fill = $id('bench-prog');
    if (fill) fill.style.width = pct + '%';
}

function runBenchmark() {
    $id('btn-bench').disabled = true;
    chip('run-chip', 'RUNNING', 'warn');
    chip('bench-status', 'loading wasm', 'warn');
    progress(5);
    log('[step] Loading render_core.js glue...', 'step');

    // Load the Emscripten glue script dynamically
    var script = document.createElement('script');
    script.src = './src-proxy.php?f=render_core.js';
    script.onerror = function() {
        log('[error] Failed to load render_core.js from proxy.', 'bad');
        chip('bench-status', 'load error', 'bad');
        chip('run-chip', 'ERROR', 'bad');
        $id('btn-bench').disabled = false;
    };
    script.onload = function() {
        if (typeof XcmRenderCore !== 'function') {
            log('[error] XcmRenderCore factory not found after script load.', 'bad');
            chip('bench-status', 'error', 'bad');
            $id('btn-bench').disabled = false;
            return;
        }
        chip('bench-status', 'instantiating', 'warn');
        progress(20);
        log('[step] Instantiating WASM module...', 'step');

        var t0 = performance.now();
        XcmRenderCore({
            locateFile: function(path) {
                return './src-proxy.php?f=' + path;
            },
        }).then(function(mod) {
            var initMs = performance.now() - t0;
            _wasmModule = mod;
            log('[ok] WASM instantiated in ' + initMs.toFixed(1) + ' ms', 'ok');
            progress(35);

            // Read initial heap size
            var heapBytes = mod.HEAPU8 ? mod.HEAPU8.buffer.byteLength : 0;
            var pages     = heapBytes / 65536;
            set('bi-init',  fmtMs(initMs), initMs < 200 ? 'ok' : initMs < 500 ? 'warn' : 'bad');
            set('bi-heap',  fmtMB(heapBytes), 'info');
            set('bi-pages', pages.toFixed(0) + ' x 64 KB');

            chip('bench-status', 'warming up', 'info');
            log('[step] Running render benchmark...', 'step');

            // Wrap API
            try {
                var xcm_create       = mod.cwrap('xcm_create',      'number', ['number','number']);
                var xcm_destroy      = mod.cwrap('xcm_destroy',      null,     ['number']);
                var xcm_render       = mod.cwrap('xcm_render',       'number', ['number','number','number','number','number','number']);
                var xcm_alloc        = mod.cwrap('xcm_alloc',        'number', ['number']);
                var xcm_free         = mod.cwrap('xcm_free',         null,     ['number']);
            } catch(e) {
                log('[error] cwrap failed: ' + e.message, 'bad');
                chip('bench-status', 'error', 'bad');
                $id('btn-bench').disabled = false;
                return;
            }

            function allocStr(str) {
                var enc = new TextEncoder().encode(str);
                var ptr = xcm_alloc(enc.length + 1);
                mod.HEAPU8.set(enc, ptr);
                mod.HEAPU8[ptr + enc.length] = 0;
                return { ptr: ptr, len: enc.length };
            }

            var SIZES = [
                { label: '400x300',  w: 400,  h: 300  },
                { label: '800x600',  w: 800,  h: 600  },
                { label: '1400x900', w: 1400, h: 900  },
                { label: '2800x1800 (Retina)', w: 2800, h: 1800 },
            ];
            var WARMUP = 3;
            var RUNS   = 10;

            var html = '<html><body style="font-family:sans-serif;padding:20px">' +
                '<h1 style="color:#6366f1;font-size:28px">xcm render benchmark</h1>' +
                '<p style="color:#64748b;margin:8px 0">Layout and paint latency test.</p>' +
                '<div style="display:flex;gap:12px;flex-wrap:wrap;margin-top:16px">' +
                '<div style="background:#13131f;border:1px solid #1e1e38;border-radius:8px;padding:16px;min-width:140px"><strong>Block A</strong><p>60fps target</p></div>' +
                '<div style="background:#0f0f1c;border:1px solid #6366f1;border-radius:8px;padding:16px;min-width:140px"><strong>Block B</strong><p>Retina 2x</p></div>' +
                '<div style="background:#080810;border:1px solid #1e1e38;border-radius:8px;padding:16px;min-width:140px"><strong>Block C</strong><p>WASM heap</p></div>' +
                '</div><ul style="margin-top:12px;padding-left:20px">' +
                '<li>Item one</li><li>Item two</li><li>Item three</li></ul>' +
                '</body></html>';
            var css = 'body{background:#080810;color:#e2e8f0}';

            var results = [];
            var sizeIdx = 0;

            function benchSize() {
                if (sizeIdx >= SIZES.length) {
                    displayResults(results);
                    return;
                }
                var sz = SIZES[sizeIdx];
                var ctx = xcm_create(sz.w, sz.h);
                var htmlBuf = allocStr(html);
                var cssBuf  = allocStr(css);

                // Warm-up
                for (var i = 0; i < WARMUP; i++) {
                    xcm_render(ctx, htmlBuf.ptr, htmlBuf.len, cssBuf.ptr, cssBuf.len, 0);
                }

                // Timed runs
                var times = [];
                for (var r = 0; r < RUNS; r++) {
                    var ts = performance.now();
                    xcm_render(ctx, htmlBuf.ptr, htmlBuf.len, cssBuf.ptr, cssBuf.len, 0);
                    times.push(performance.now() - ts);
                }

                xcm_free(htmlBuf.ptr);
                xcm_free(cssBuf.ptr);
                xcm_destroy(ctx);

                var sum = 0, mn = Infinity, mx = -Infinity;
                for (var t = 0; t < times.length; t++) {
                    sum += times[t];
                    if (times[t] < mn) mn = times[t];
                    if (times[t] > mx) mx = times[t];
                }
                var avg = sum / times.length;
                var fps = Math.round(1000 / avg);
                results.push({ label: sz.label, avg: avg, min: mn, max: mx, fps: fps });
                log('[bench] ' + sz.label + ' avg=' + avg.toFixed(2) + 'ms fps=' + fps, avg < 16 ? 'ok' : avg < 33 ? 'warn' : 'bad');
                progress(35 + (sizeIdx + 1) / SIZES.length * 55);
                sizeIdx++;

                // Yield to event loop between sizes so UI does not freeze
                setTimeout(benchSize, 0);
            }

            benchSize();

        }).catch(function(err) {
            log('[error] WASM init failed: ' + err, 'bad');
            chip('bench-status', 'error', 'bad');
            chip('run-chip', 'ERROR', 'bad');
            $id('btn-bench').disabled = false;
        });
    };
    document.head.appendChild(script);
}

function displayResults(results) {
    progress(100);
    chip('bench-status', 'done', 'ok');
    chip('run-chip', 'DONE', 'ok');

    // Update WASM heap post-benchmark
    if (_wasmModule && _wasmModule.HEAPU8) {
        var heapBytes = _wasmModule.HEAPU8.buffer.byteLength;
        set('mem-wasm',   fmtMB(heapBytes), 'info');
        set('mem-wpages', (heapBytes / 65536).toFixed(0) + ' pages');
    }

    var maxAvg = Math.max.apply(null, results.map(function(r){return r.avg;}));
    var container = $id('bench-results');
    container.innerHTML = '';

    // Header row
    var hdr = document.createElement('div');
    hdr.className = 'bench-row';
    hdr.style.fontWeight = '600';
    hdr.innerHTML = '<span class="lbl">size</span><span></span>' +
        '<span class="num" style="color:var(--sub)">avg</span>' +
        '<span class="num" style="color:var(--sub)">min/max</span>' +
        '<span class="num" style="color:var(--sub)">fps cap</span>';
    container.appendChild(hdr);

    results.forEach(function(r) {
        var row  = document.createElement('div');
        row.className = 'bench-row';
        var barPct = Math.max(2, (r.avg / maxAvg) * 100).toFixed(1);
        var cls    = r.avg < 16 ? 'ok' : r.avg < 33 ? 'warn' : 'bad';
        var color  = r.avg < 16 ? 'var(--ok)' : r.avg < 33 ? 'var(--warn)' : 'var(--bad)';
        var fpsCls = r.fps >= 60 ? 'ok' : r.fps >= 30 ? 'warn' : 'bad';
        row.innerHTML =
            '<span class="lbl">' + r.label + '</span>' +
            '<div class="bar" style="width:' + barPct + '%;background:' + color + '"></div>' +
            '<span class="num ' + cls + '">' + r.avg.toFixed(2) + ' ms</span>' +
            '<span class="num" style="color:var(--sub);font-size:.72rem">' + r.min.toFixed(1) + ' / ' + r.max.toFixed(1) + '</span>' +
            '<span class="num ' + fpsCls + '">' + r.fps + ' fps</span>';
        container.appendChild(row);
    });

    log('[ok] Benchmark complete.', 'ok');
    $id('btn-bench').disabled = false;
}

$id('btn-bench').addEventListener('click', function() {
    // Clear log and reset state
    $id('log').innerHTML = '';
    $id('bench-results').innerHTML = '<div class="stat" style="color:var(--sub);font-size:.78rem">Running...</div>';
    progress(0);
    runBenchmark();
});

$id('btn-reset').addEventListener('click', function() {
    $id('log').innerHTML = '';
    $id('bench-results').innerHTML = '<div class="stat" style="color:var(--sub);font-size:.78rem">Click "Run Benchmark" to measure layout and paint latency.</div>';
    progress(0);
    chip('run-chip', 'IDLE', 'idle');
    chip('bench-status', 'not run', 'idle');
    ['bi-init','bi-heap','bi-pages','mem-wasm','mem-wpages'].forEach(function(id){
        set(id,'--','dim');
    });
});
</script>
</body>
</html>
