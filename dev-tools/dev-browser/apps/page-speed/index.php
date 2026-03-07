<?php
/**
 * page-speed/index.php
 *
 * Page speed checker frontend.
 * Triggers a headless Selenium run via the automation bridge and
 * visualises the Navigation Timing results in real time.
 */
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Page Speed</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
:root {
  --bg:    #0c0c18;
  --card:  #13131f;
  --bdr:   #262640;
  --acc:   #6366f1;
  --acc2:  #818cf8;
  --text:  #e2e8f0;
  --sub:   #64748b;
  --ok:    #34d399;
  --warn:  #fbbf24;
  --bad:   #f87171;
  --fast:  #34d399;
  --mid:   #fbbf24;
  --slow:  #f87171;
}
body {
  font-family: 'Segoe UI', system-ui, sans-serif;
  background: var(--bg); color: var(--text);
  padding: 1.6rem 1.8rem;
  min-height: 100vh;
}
h1 { font-size: 1.45rem; color: var(--acc); letter-spacing: -0.01em; }
.sub { color: var(--sub); font-size: 0.82rem; margin: 0.3rem 0 1.3rem; }

/* Input row */
.input-row { display: flex; gap: 0.55rem; flex-wrap: wrap; align-items: center; }
.input-row input[type=text] {
  flex: 1; min-width: 260px;
  padding: 0.6rem 0.9rem;
  background: #0d0d1a; border: 1px solid var(--bdr);
  border-radius: 7px; color: var(--text); font-size: 0.9rem; outline: none;
}
.input-row input[type=text]:focus { border-color: var(--acc); }
.input-row select {
  padding: 0.6rem 0.7rem;
  background: #0d0d1a; border: 1px solid var(--bdr);
  border-radius: 7px; color: var(--text); font-size: 0.85rem; outline: none;
}
button.primary {
  padding: 0.6rem 1.4rem;
  background: var(--acc); color: #fff; border: none;
  border-radius: 7px; font-size: 0.9rem; cursor: pointer;
  transition: background 0.15s;
}
button.primary:hover  { background: #4f46e5; }
button.primary:disabled { background: #3d3d5c; cursor: not-allowed; }

/* Status bar */
#status-bar {
  margin-top: 0.9rem; padding: 0.5rem 0.9rem;
  background: var(--card); border: 1px solid var(--bdr);
  border-radius: 7px; font-size: 0.83rem; color: var(--sub);
  display: flex; align-items: center; gap: 0.55rem;
}
#status-bar.running { color: var(--warn); border-color: #78350f; }
#status-bar.done    { color: var(--ok);   border-color: #065f46; }
#status-bar.error   { color: var(--bad);  border-color: #7f1d1d; }
.dot { width: 7px; height: 7px; border-radius: 50%; background: currentColor; flex-shrink: 0; }

/* Results grid */
#results { display: none; margin-top: 1.4rem; }
.grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 1rem; }
.grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 0.85rem; margin-bottom: 1rem; }
@media (max-width: 700px) { .grid-2, .grid-3 { grid-template-columns: 1fr; } }

.card {
  background: var(--card); border: 1px solid var(--bdr);
  border-radius: 10px; padding: 1.1rem;
}
.card h3 {
  font-size: 0.73rem; text-transform: uppercase;
  letter-spacing: 0.07em; color: var(--sub); margin-bottom: 0.9rem;
}

/* Score circle */
.score-wrap {
  display: flex; align-items: center; gap: 1.6rem;
  flex-wrap: wrap;
}
.score-circle {
  width: 80px; height: 80px; border-radius: 50%;
  display: flex; align-items: center; justify-content: center;
  font-size: 1.6rem; font-weight: 700;
  border: 4px solid;
  flex-shrink: 0;
}
.score-circle.fast  { color: var(--fast); border-color: var(--fast); background: rgba(52,211,153,.08); }
.score-circle.mid   { color: var(--mid);  border-color: var(--mid);  background: rgba(251,191,36,.08); }
.score-circle.slow  { color: var(--slow); border-color: var(--slow); background: rgba(248,113,113,.08); }
.score-meta { line-height: 1.6; }
.score-meta .title { font-size: 1rem; font-weight: 600; color: var(--text); margin-bottom: 0.2rem; }
.score-meta .url-disp { font-size: 0.75rem; color: var(--sub); word-break: break-all; }

/* Metric tiles */
.metric-tile {
  background: var(--card); border: 1px solid var(--bdr);
  border-radius: 9px; padding: 0.85rem 1rem;
  text-align: center;
}
.metric-tile .label { font-size: 0.7rem; text-transform: uppercase; letter-spacing: 0.06em; color: var(--sub); margin-bottom: 0.4rem; }
.metric-tile .value { font-size: 1.35rem; font-weight: 700; }
.metric-tile .unit  { font-size: 0.72rem; color: var(--sub); margin-top: 0.15rem; }
.metric-tile .bar-wrap { height: 3px; background: #1e1e30; border-radius: 2px; margin-top: 0.55rem; }
.metric-tile .bar-fill { height: 100%; border-radius: 2px; transition: width 0.6s ease; }
.c-fast { color: var(--fast); }
.c-mid  { color: var(--mid);  }
.c-slow { color: var(--slow); }

/* Waterfall bar chart */
.wf-row { display: flex; align-items: center; gap: 0.5rem; margin-bottom: 0.45rem; font-size: 0.75rem; }
.wf-label { width: 62px; flex-shrink: 0; text-align: right; color: var(--sub); }
.wf-bar-wrap { flex: 1; height: 18px; background: #0d0d1a; border-radius: 3px; overflow: hidden; position: relative; }
.wf-bar { height: 100%; border-radius: 3px; background: var(--acc); transition: width 0.5s ease; min-width: 2px; }
.wf-ms { width: 58px; flex-shrink: 0; text-align: right; color: var(--text); }

/* Resource breakdown */
.rb-row { display: flex; align-items: center; gap: 0.5rem; margin-bottom: 0.45rem; font-size: 0.8rem; }
.rb-type { width: 68px; flex-shrink: 0; color: var(--sub); }
.rb-bar-wrap { flex: 1; height: 14px; background: #0d0d1a; border-radius: 3px; overflow: hidden; }
.rb-bar { height: 100%; border-radius: 3px; }
.rb-count { width: 38px; flex-shrink: 0; text-align: right; color: var(--sub); font-size: 0.75rem; }
.rb-size  { width: 64px; flex-shrink: 0; text-align: right; font-size: 0.75rem; }

/* Slowest resources table */
.slowest-table { width: 100%; border-collapse: collapse; font-size: 0.78rem; }
.slowest-table th {
  text-align: left; padding: 0.3rem 0.5rem;
  border-bottom: 1px solid var(--bdr); color: var(--sub);
  font-weight: 500; text-transform: uppercase; font-size: 0.68rem; letter-spacing: 0.05em;
}
.slowest-table td { padding: 0.32rem 0.5rem; border-bottom: 1px solid #1a1a2e; vertical-align: middle; }
.slowest-table td:first-child { color: #94a3b8; max-width: 300px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.slowest-table td.badge {
  font-size: 0.68rem; color: var(--sub); font-family: monospace;
  white-space: nowrap;
}
.pill {
  display: inline-block; padding: 0.1rem 0.45rem;
  border-radius: 4px; font-size: 0.68rem; font-weight: 600;
  background: rgba(99,102,241,0.15); color: var(--acc2);
}

/* Screenshot */
#shot-wrap { min-height: 140px; display: flex; align-items: center; justify-content: center; }
#shot-wrap img { max-width: 100%; border-radius: 6px; border: 1px solid var(--bdr); }
#shot-wrap .ph { color: var(--sub); font-size: 0.8rem; }

/* Log */
#log {
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  font-size: 0.75rem; color: #94a3b8; max-height: 160px;
  overflow-y: auto; white-space: pre-wrap; line-height: 1.55;
}

/* Spinner */
@keyframes spin { to { transform: rotate(360deg); } }
.spinner { width: 14px; height: 14px; border: 2px solid rgba(250,190,36,0.3); border-top-color: var(--warn); border-radius: 50%; animation: spin 0.8s linear infinite; flex-shrink: 0; display: none; }
.running .spinner { display: inline-block; }
</style>
</head>
<body>

<h1>Page Speed</h1>
<p class="sub">Measure real load performance using a headless browser.</p>

<div class="input-row">
  <input type="text" id="url_input" placeholder="https://example.com" value="https://example.com">
  <select id="runs_select" title="Number of measurement runs">
    <option value="1">1 run</option>
    <option value="2">2 runs (avg)</option>
    <option value="3">3 runs (avg)</option>
  </select>
  <button class="primary" id="run_btn" onclick="measure()">Measure</button>
</div>

<div id="status-bar">
  <span class="dot"></span>
  <span class="spinner"></span>
  <span id="status-msg">Ready. Enter a URL and click Measure.</span>
</div>

<div id="results">

  <!-- Row 0: score + screenshot -->
  <div class="grid-2" style="margin-bottom:1rem">
    <div class="card">
      <h3>Score</h3>
      <div class="score-wrap">
        <div class="score-circle" id="score-circle">--</div>
        <div class="score-meta">
          <div class="title" id="page-title">--</div>
          <div class="url-disp" id="page-url">--</div>
        </div>
      </div>
    </div>
    <div class="card">
      <h3>Screenshot (headless)</h3>
      <div id="shot-wrap"><span class="ph">Waiting for screenshot...</span></div>
    </div>
  </div>

  <!-- Row 1: key metric tiles -->
  <div class="grid-3" id="metric-tiles">
    <!-- injected by JS -->
  </div>

  <!-- Row 2: waterfall + resource breakdown -->
  <div class="grid-2" style="margin-bottom:1rem">
    <div class="card">
      <h3>Timing Waterfall</h3>
      <div id="waterfall"></div>
    </div>
    <div class="card">
      <h3>Resource Breakdown</h3>
      <div id="resource-breakdown"></div>
      <div style="margin-top:0.7rem;font-size:0.78rem;color:var(--sub)" id="total-weight"></div>
    </div>
  </div>

  <!-- Row 3: slowest resources + log -->
  <div class="grid-2">
    <div class="card">
      <h3>Slowest Resources (top 10)</h3>
      <table class="slowest-table">
        <thead><tr><th>Resource</th><th>Type</th><th>Time</th></tr></thead>
        <tbody id="slowest-body"></tbody>
      </table>
    </div>
    <div class="card">
      <h3>Automation Log</h3>
      <pre id="log">--</pre>
    </div>
  </div>

</div><!-- /#results -->

<script>
const BRIDGE  = 'http://127.0.0.1:9877';
const PHP_SRV = 'http://127.0.0.1:9879';
const SLUG    = 'page-speed';

let pollTimer = null;
let shotTimer = null;

/* ── Color helpers ──────────────────────────────────────────── */
function speedColor(ms, fast, mid) {
  if (ms <= fast) return 'fast';
  if (ms <= mid)  return 'mid';
  return 'slow';
}
function fmtMs(ms) {
  if (ms >= 1000) return (ms / 1000).toFixed(2) + ' s';
  return ms.toFixed(0) + ' ms';
}
function fmtBytes(n) {
  if (!n) return '0 B';
  if (n < 1024)       return n + ' B';
  if (n < 1024*1024)  return (n/1024).toFixed(1) + ' KB';
  return (n/1024/1024).toFixed(2) + ' MB';
}

/* ── Main trigger ───────────────────────────────────────────── */
async function measure() {
  const url  = document.getElementById('url_input').value.trim();
  const runs = parseInt(document.getElementById('runs_select').value);
  if (!url) { setStatus('error', 'Enter a URL first.'); return; }

  clearResults();
  document.getElementById('results').style.display = 'block';
  setStatus('running', 'Sending job to automation bridge...');
  document.getElementById('run_btn').disabled = true;
  startShotRefresh();

  try {
    const res = await fetch(BRIDGE + '/apps/automation/run', {
      method:  'POST',
      headers: {'Content-Type': 'application/json'},
      body:    JSON.stringify({app: SLUG, params: {url, runs}})
    });
    if (!res.ok) throw new Error('Bridge returned ' + res.status);
    const data = await res.json();
    if (!data.job_id) throw new Error('No job_id returned');
    setStatus('running', 'Job started: ' + data.job_id.slice(0,8) + '...');
    pollStatus(data.job_id);
  } catch(e) {
    setStatus('error', 'Could not reach bridge: ' + e.message);
    document.getElementById('run_btn').disabled = false;
    stopShotRefresh();
  }
}

function pollStatus(jobId) {
  if (pollTimer) clearInterval(pollTimer);
  pollTimer = setInterval(async () => {
    try {
      const res  = await fetch(BRIDGE + '/apps/automation/status?id=' + jobId);
      const data = await res.json();
      if (data.log && data.log.length) {
        document.getElementById('log').textContent = data.log.join('\n');
      }
      if (data.status === 'running') {
        setStatus('running', 'Running headless measurements...');
      } else if (data.status === 'done') {
        clearInterval(pollTimer);
        stopShotRefresh();
        setStatus('done', 'Measurement complete.');
        document.getElementById('run_btn').disabled = false;
        renderResults(data.result || {});
        refreshShot(true);
      } else if (data.status === 'error') {
        clearInterval(pollTimer);
        stopShotRefresh();
        setStatus('error', 'Error: ' + (data.error || 'unknown'));
        document.getElementById('run_btn').disabled = false;
      }
    } catch(e) {}
  }, 700);
}

/* ── Render ─────────────────────────────────────────────────── */
function renderResults(r) {
  const t = r.timing || {};

  /* Score */
  const sc  = r.score || 0;
  const scEl = document.getElementById('score-circle');
  const cls  = sc >= 80 ? 'fast' : sc >= 50 ? 'mid' : 'slow';
  scEl.textContent = sc;
  scEl.className   = 'score-circle ' + cls;
  document.getElementById('page-title').textContent = r.title || '--';
  document.getElementById('page-url').textContent   = r.url   || '--';

  /* Metric tiles */
  const tiles = [
    {label: 'TTFB',         val: t.ttfb,       fast: 200,  mid: 600,   unit: 'time to first byte'},
    {label: 'DOM Ready',    val: t.dom_ready,  fast: 1000, mid: 3000,  unit: 'DOMContentLoaded'},
    {label: 'Full Load',    val: t.load_event, fast: 2000, mid: 5000,  unit: 'window.onload'},
    {label: 'DNS',          val: t.dns,        fast: 50,   mid: 150,   unit: 'dns lookup'},
    {label: 'Connect',      val: t.connect,    fast: 100,  mid: 400,   unit: 'TCP handshake'},
    {label: 'Transfer',     val: t.transfer,   fast: 300,  mid: 800,   unit: 'response transfer'},
  ];
  const tilesEl = document.getElementById('metric-tiles');
  tilesEl.innerHTML = '';
  const maxLoad = t.load_event || 1;
  tiles.forEach(tile => {
    const v   = tile.val || 0;
    const cls = 'c-' + speedColor(v, tile.fast, tile.mid);
    const pct = Math.min(100, Math.round(v / maxLoad * 100));
    const bar_cls = speedColor(v, tile.fast, tile.mid);
    const bar_colors = {fast:'#34d399', mid:'#fbbf24', slow:'#f87171'};
    tilesEl.insertAdjacentHTML('beforeend', `
      <div class="metric-tile">
        <div class="label">${tile.label}</div>
        <div class="value ${cls}">${fmtMs(v)}</div>
        <div class="unit">${tile.unit}</div>
        <div class="bar-wrap"><div class="bar-fill" style="width:${pct}%;background:${bar_colors[bar_cls]}"></div></div>
      </div>
    `);
  });

  /* Waterfall */
  const wf = [
    {label: 'DNS',      ms: t.dns       || 0, color: '#818cf8'},
    {label: 'Connect',  ms: t.connect   || 0, color: '#a78bfa'},
    {label: 'TTFB',     ms: t.ttfb      || 0, color: '#34d399'},
    {label: 'Transfer', ms: t.transfer  || 0, color: '#38bdf8'},
    {label: 'DOM',      ms: t.dom_ready || 0, color: '#fbbf24'},
    {label: 'Load',     ms: t.load_event|| 0, color: '#f87171'},
  ];
  const maxMs = Math.max(...wf.map(w => w.ms), 1);
  const wfEl  = document.getElementById('waterfall');
  wfEl.innerHTML = '';
  wf.forEach(w => {
    const pct = Math.round(w.ms / maxMs * 100);
    wfEl.insertAdjacentHTML('beforeend', `
      <div class="wf-row">
        <div class="wf-label">${w.label}</div>
        <div class="wf-bar-wrap">
          <div class="wf-bar" style="width:${pct}%;background:${w.color}"></div>
        </div>
        <div class="wf-ms">${fmtMs(w.ms)}</div>
      </div>
    `);
  });

  /* Resource breakdown */
  const res       = r.resources || {};
  const byType    = res.by_type || {};
  const byteType  = res.bytes_by_type || {};
  const totalReqs = res.total || 0;
  const maxReqs   = Math.max(...Object.values(byType), 1);
  const typeColors = {
    script:    '#818cf8', img: '#34d399', fetch: '#38bdf8',
    css:       '#fbbf24', xhr: '#a78bfa', font: '#fb923c',
    media:     '#f87171', other: '#64748b'
  };
  const rbEl = document.getElementById('resource-breakdown');
  rbEl.innerHTML = '';
  Object.keys(byType).sort((a,b) => byType[b]-byType[a]).forEach(type => {
    const cnt  = byType[type];
    const pct  = Math.round(cnt / maxReqs * 100);
    const col  = typeColors[type] || '#64748b';
    rbEl.insertAdjacentHTML('beforeend', `
      <div class="rb-row">
        <div class="rb-type">${type}</div>
        <div class="rb-bar-wrap"><div class="rb-bar" style="width:${pct}%;background:${col}"></div></div>
        <div class="rb-count">${cnt}</div>
        <div class="rb-size">${byteType[type] ? fmtBytes(byteType[type]) : ''}</div>
      </div>
    `);
  });
  document.getElementById('total-weight').textContent =
    `Total: ${totalReqs} requests  /  ${fmtBytes(res.total_bytes || 0)}`;

  /* Slowest resources */
  const tbody = document.getElementById('slowest-body');
  tbody.innerHTML = '';
  (r.slowest || []).forEach(s => {
    const short = s.url.replace(/^https?:\/\/[^/]+/, '').slice(0, 55) || s.url.slice(0, 55);
    const col   = typeColors[s.type] || '#64748b';
    const msVal = (s.ms || 0).toFixed(0);
    const msCls = speedColor(s.ms, 300, 800);
    tbody.insertAdjacentHTML('beforeend', `
      <tr>
        <td title="${s.url}">${short}</td>
        <td class="badge"><span class="pill" style="color:${col};background:${col}22">${s.type}</span></td>
        <td class="c-${msCls}">${fmtMs(s.ms)}</td>
      </tr>
    `);
  });
}

/* ── Screenshot ─────────────────────────────────────────────── */
function refreshShot(force) {
  const wrap = document.getElementById('shot-wrap');
  const src  = PHP_SRV + '/' + SLUG + '/screenshots/latest.png?t=' + Date.now();
  let   img  = wrap.querySelector('img');
  if (!img) {
    img = document.createElement('img');
    img.alt = 'screenshot';
    wrap.innerHTML = '';
    wrap.appendChild(img);
  }
  img.src = src;
}
function startShotRefresh() { shotTimer = setInterval(() => refreshShot(false), 1500); }
function stopShotRefresh()  { if (shotTimer) { clearInterval(shotTimer); shotTimer = null; } }

/* ── Helpers ────────────────────────────────────────────────── */
function setStatus(cls, msg) {
  const bar = document.getElementById('status-bar');
  bar.className = cls;
  document.getElementById('status-msg').textContent = msg;
}
function clearResults() {
  document.getElementById('score-circle').textContent = '--';
  document.getElementById('score-circle').className   = 'score-circle';
  document.getElementById('page-title').textContent   = '--';
  document.getElementById('page-url').textContent     = '--';
  document.getElementById('metric-tiles').innerHTML   = '';
  document.getElementById('waterfall').innerHTML      = '';
  document.getElementById('resource-breakdown').innerHTML = '';
  document.getElementById('total-weight').textContent = '';
  document.getElementById('slowest-body').innerHTML   = '';
  document.getElementById('log').textContent          = '--';
  document.getElementById('shot-wrap').innerHTML      = '<span class="ph">Waiting for screenshot...</span>';
}

/* Allow pressing Enter in the URL box */
document.getElementById('url_input').addEventListener('keydown', e => {
  if (e.key === 'Enter') measure();
});
</script>
</body>
</html>
