<?php
/**
 * page-inspector/index.php
 *
 * Extracts metadata, links, and images from any URL using headless Chrome.
 * The live screenshot taken by automation.py is displayed and auto-refreshes
 * so you can watch the headless browser in real time.
 */
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Page Inspector</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  :root {
    --bg:   #0f0f1a;
    --card: #1a1a2e;
    --bdr:  #2d2d44;
    --acc:  #6366f1;
    --text: #e2e8f0;
    --sub:  #64748b;
    --ok:   #34d399;
    --warn: #fbbf24;
    --err:  #f87171;
  }
  body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: var(--text); padding: 1.6rem; }
  h1 { font-size: 1.5rem; color: var(--acc); }
  .sub { color: var(--sub); font-size: 0.82rem; margin: 0.3rem 0 1.4rem; }

  .row { display: flex; gap: 0.6rem; align-items: stretch; }
  input[type=text] {
    flex: 1; padding: 0.6rem 0.9rem;
    background: #131320; border: 1px solid var(--bdr);
    border-radius: 7px; color: var(--text); font-size: 0.92rem;
    outline: none;
  }
  input[type=text]:focus { border-color: var(--acc); }
  button {
    padding: 0.6rem 1.3rem; background: var(--acc); color: #fff;
    border: none; border-radius: 7px; font-size: 0.88rem;
    cursor: pointer; white-space: nowrap; transition: background 0.15s;
  }
  button:hover { background: #4f46e5; }
  button:disabled { background: #3d3d5c; cursor: not-allowed; }
  button.secondary {
    background: #2d2d44; color: #94a3b8; font-size: 0.8rem; padding: 0.5rem 1rem;
  }
  button.secondary:hover { background: #3d3d5c; }

  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 1rem; margin-top: 1.2rem; }
  @media (max-width: 760px) { .grid { grid-template-columns: 1fr; } }

  .card {
    background: var(--card); border: 1px solid var(--bdr);
    border-radius: 10px; padding: 1.1rem;
  }
  .card h3 { font-size: 0.78rem; text-transform: uppercase; letter-spacing: 0.07em; color: var(--sub); margin-bottom: 0.8rem; }

  #status-bar {
    margin-top: 0.9rem; padding: 0.55rem 0.9rem;
    background: var(--card); border: 1px solid var(--bdr);
    border-radius: 7px; font-size: 0.84rem; color: var(--sub);
    display: flex; align-items: center; gap: 0.6rem;
  }
  #status-bar.running { color: var(--warn); border-color: #78350f; }
  #status-bar.done    { color: var(--ok);   border-color: #065f46; }
  #status-bar.error   { color: var(--err);  border-color: #7f1d1d; }

  #log {
    font-family: 'JetBrains Mono', 'Fira Code', monospace;
    font-size: 0.76rem; color: #94a3b8;
    max-height: 150px; overflow-y: auto;
    white-space: pre-wrap;
    line-height: 1.5;
  }

  #shot-wrap { text-align: center; min-height: 120px; display: flex; align-items: center; justify-content: center; }
  #shot-wrap img { max-width: 100%; border-radius: 6px; border: 1px solid var(--bdr); }
  #shot-wrap .placeholder { color: var(--sub); font-size: 0.8rem; }

  .meta-table { width: 100%; border-collapse: collapse; font-size: 0.84rem; }
  .meta-table td { padding: 0.35rem 0.5rem; border-bottom: 1px solid var(--bdr); vertical-align: top; }
  .meta-table td:first-child { color: var(--sub); width: 38%; white-space: nowrap; }

  .link-list { list-style: none; max-height: 220px; overflow-y: auto; }
  .link-list li { padding: 0.3rem 0; border-bottom: 1px solid var(--bdr); font-size: 0.8rem; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .link-list a { color: #818cf8; text-decoration: none; }
  .link-list a:hover { text-decoration: underline; }

  .img-grid { display: flex; flex-wrap: wrap; gap: 6px; max-height: 220px; overflow-y: auto; }
  .img-grid img { width: 70px; height: 50px; object-fit: cover; border-radius: 4px; border: 1px solid var(--bdr); cursor: pointer; }

  .dot { width: 8px; height: 8px; border-radius: 50%; background: currentColor; display: inline-block; flex-shrink: 0; }
</style>
</head>
<body>

<h1>Page Inspector</h1>
<p class="sub">Inspect any page using a headless browser -- screenshot, metadata, links, and images.</p>

<div class="row">
  <input type="text" id="url_input" placeholder="https://example.com" value="https://example.com">
  <button id="run_btn" onclick="inspect()">Inspect</button>
  <button class="secondary" onclick="openInTab()">Open in tab</button>
</div>

<div id="status-bar"><span class="dot"></span><span id="status-msg">Ready.</span></div>

<div class="grid">
  <div class="card">
    <h3>Live View (headless screenshot)</h3>
    <div id="shot-wrap">
      <span class="placeholder">No screenshot yet. Run an inspection.</span>
    </div>
  </div>

  <div class="card">
    <h3>Automation Log</h3>
    <pre id="log">--</pre>
  </div>

  <div class="card">
    <h3>Page Metadata</h3>
    <table class="meta-table" id="meta-table">
      <tr><td>Title</td><td id="m-title">--</td></tr>
      <tr><td>Description</td><td id="m-desc">--</td></tr>
      <tr><td>URL</td><td id="m-url">--</td></tr>
      <tr><td>Status code</td><td id="m-status">--</td></tr>
      <tr><td>Links found</td><td id="m-links">--</td></tr>
      <tr><td>Images found</td><td id="m-imgs">--</td></tr>
    </table>
  </div>

  <div class="card">
    <h3>Links</h3>
    <ul class="link-list" id="link-list"><li style="color:var(--sub)">No links yet.</li></ul>
  </div>

  <div class="card" style="grid-column:1/-1">
    <h3>Images on page</h3>
    <div class="img-grid" id="img-grid"><span style="color:var(--sub);font-size:0.8rem">No images yet.</span></div>
  </div>
</div>

<script>
const BRIDGE  = 'http://127.0.0.1:9877';
const PHP_SRV = 'http://127.0.0.1:9879';
const SLUG    = 'page-inspector';
let pollTimer = null;
let shotTimer = null;

function openInTab() {
  const url = document.getElementById('url_input').value.trim();
  if (url) window.open(url, '_blank');
}

async function inspect() {
  const url = document.getElementById('url_input').value.trim();
  if (!url) { setStatus('error', 'Enter a URL first.'); return; }

  clearResults();
  setStatus('running', 'Connecting to bridge...');
  document.getElementById('run_btn').disabled = true;

  try {
    const res = await fetch(BRIDGE + '/apps/automation/run', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({app: SLUG, params: {url: url}})
    });
    if (!res.ok) throw new Error('Bridge returned ' + res.status);
    const data = await res.json();
    if (!data.job_id) throw new Error('No job_id in response');
    setStatus('running', 'Job queued: ' + data.job_id.slice(0,8) + '...');
    pollStatus(data.job_id);
    startShotRefresh();
  } catch(e) {
    setStatus('error', 'Bridge error: ' + e.message);
    document.getElementById('run_btn').disabled = false;
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
        setStatus('running', 'Running...');
      } else if (data.status === 'done') {
        clearInterval(pollTimer);
        stopShotRefresh();
        setStatus('done', 'Inspection complete.');
        document.getElementById('run_btn').disabled = false;
        renderResults(data.result || {});
        refreshShot(true);  // final shot
      } else if (data.status === 'error') {
        clearInterval(pollTimer);
        stopShotRefresh();
        setStatus('error', 'Error: ' + (data.error || 'unknown'));
        document.getElementById('run_btn').disabled = false;
      }
    } catch(e) {}
  }, 700);
}

function renderResults(r) {
  document.getElementById('m-title').textContent   = r.title   || '--';
  document.getElementById('m-desc').textContent    = r.description || '--';
  document.getElementById('m-url').textContent     = r.url     || '--';
  document.getElementById('m-status').textContent  = r.status_code || '--';
  document.getElementById('m-links').textContent   = (r.links  || []).length;
  document.getElementById('m-imgs').textContent    = (r.images || []).length;

  // Links
  const ul = document.getElementById('link-list');
  ul.innerHTML = '';
  const links = r.links || [];
  if (!links.length) {
    ul.innerHTML = '<li style="color:var(--sub)">No links found.</li>';
  } else {
    links.slice(0, 60).forEach(l => {
      const li = document.createElement('li');
      const a  = document.createElement('a');
      a.href        = l;
      a.textContent = l;
      a.target      = '_blank';
      li.appendChild(a);
      ul.appendChild(li);
    });
    if (links.length > 60) {
      const li = document.createElement('li');
      li.style.color = 'var(--sub)';
      li.textContent = `... and ${links.length - 60} more`;
      ul.appendChild(li);
    }
  }

  // Images
  const grid = document.getElementById('img-grid');
  grid.innerHTML = '';
  const imgs = r.images || [];
  if (!imgs.length) {
    grid.innerHTML = '<span style="color:var(--sub);font-size:0.8rem">No images found.</span>';
  } else {
    imgs.slice(0, 40).forEach(src => {
      const img = document.createElement('img');
      img.src    = src;
      img.title  = src;
      img.loading = 'lazy';
      img.onclick = () => window.open(src, '_blank');
      grid.appendChild(img);
    });
  }
}

function clearResults() {
  ['m-title','m-desc','m-url','m-status','m-links','m-imgs'].forEach(id => {
    document.getElementById(id).textContent = '--';
  });
  document.getElementById('link-list').innerHTML = '<li style="color:var(--sub)">No links yet.</li>';
  document.getElementById('img-grid').innerHTML  = '<span style="color:var(--sub);font-size:0.8rem">No images yet.</span>';
  document.getElementById('log').textContent     = '--';
}

function refreshShot(force) {
  const wrp  = document.getElementById('shot-wrap');
  const src  = PHP_SRV + '/' + SLUG + '/screenshots/latest.png?t=' + Date.now();
  let   img  = wrp.querySelector('img');
  if (!img) {
    img = document.createElement('img');
    img.alt = 'screenshot';
    wrp.innerHTML = '';
    wrp.appendChild(img);
  }
  img.src = src;
}

function startShotRefresh() {
  stopShotRefresh();
  shotTimer = setInterval(() => refreshShot(false), 1200);
}

function stopShotRefresh() {
  if (shotTimer) { clearInterval(shotTimer); shotTimer = null; }
}

function setStatus(cls, msg) {
  const bar = document.getElementById('status-bar');
  bar.className = cls;
  document.getElementById('status-msg').textContent = msg;
}
</script>
</body>
</html>
