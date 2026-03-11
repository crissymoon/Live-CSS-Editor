<?php
$target = trim((string)($_GET['target'] ?? ''));
if ($target === '') {
    $target = 'https://localhost:8443/page-builder/composer.php?page=landing';
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Page Builder Debug</title>
<style>
:root {
  --bg:#0d0f16; --panel:#151927; --panel2:#1b2031; --line:#2b3550;
  --text:#e8edf8; --sub:#9aa7c0; --ok:#31c48d; --warn:#f59e0b; --err:#ef4444; --acc:#60a5fa;
}
*{box-sizing:border-box}
body{margin:0;font-family:ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--text)}
.wrap{max-width:1100px;margin:0 auto;padding:18px}
.h1{font-size:22px;margin:0 0 4px}
.sub{color:var(--sub);font-size:13px;margin:0 0 16px}
.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:12px}
.card h3{margin:0 0 10px;font-size:12px;letter-spacing:.06em;color:var(--sub);text-transform:uppercase}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.input{flex:1;min-width:260px;background:#0f1422;border:1px solid var(--line);color:var(--text);border-radius:8px;padding:9px 10px}
.btn{background:var(--acc);color:#08101d;border:0;border-radius:8px;padding:9px 12px;font-weight:700;cursor:pointer}
.btn.ghost{background:transparent;border:1px solid var(--line);color:var(--text)}
.kv{display:flex;justify-content:space-between;gap:10px;padding:6px 0;border-bottom:1px dashed #2b355044}
.kv:last-child{border-bottom:0}
.k{color:var(--sub);font-size:12px}.v{font-size:12px;word-break:break-all}
.dot{width:9px;height:9px;border-radius:999px;display:inline-block;margin-right:6px}
.ok{background:var(--ok)}.warn{background:var(--warn)}.err{background:var(--err)}
.log{height:170px;overflow:auto;background:var(--panel2);border:1px solid var(--line);border-radius:8px;padding:8px;font:12px/1.45 ui-monospace,SFMono-Regular,Menlo,monospace;color:#c6d3ee}
.small{font-size:12px;color:var(--sub)}
@media(max-width:980px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="wrap">
  <h1 class="h1">Page Builder Debug</h1>
  <p class="sub">Auth-first staging checks and quick controls for page-builder workflows.</p>

  <div class="card" style="margin-bottom:12px">
    <h3>Target Page</h3>
    <div class="row">
      <input id="target" class="input" value="<?= htmlspecialchars($target, ENT_QUOTES) ?>" />
      <button class="btn" onclick="openTarget()">Open Target</button>
      <button class="btn ghost" onclick="openComposer()">Composer</button>
      <button class="btn ghost" onclick="openDashboard()">Admin</button>
    </div>
    <p class="small" style="margin-top:8px">Tip: this app receives the current tab URL automatically from the ImGui menu.</p>
  </div>

  <div class="grid">
    <div class="card">
      <h3>Server Health</h3>
      <div class="kv"><span class="k">Auth server (:9100)</span><span id="auth" class="v">checking...</span></div>
      <div class="kv"><span class="k">App server (:9879)</span><span id="app" class="v">checking...</span></div>
      <div class="kv"><span class="k">Command API (:9878)</span><span id="cmd" class="v">checking...</span></div>
      <div class="kv"><span class="k">Nginx (:8443)</span><span id="nginx" class="v">checking...</span></div>
      <div class="row" style="margin-top:10px">
        <button class="btn ghost" onclick="runChecks()">Recheck</button>
      </div>
    </div>

    <div class="card">
      <h3>Viewport Shortcuts</h3>
      <div class="row">
        <button class="btn ghost" onclick="resizeHost(390,844)">390x844</button>
        <button class="btn ghost" onclick="resizeHost(768,1024)">768x1024</button>
        <button class="btn ghost" onclick="resizeHost(1366,768)">1366x768</button>
      </div>
      <p class="small" style="margin-top:10px">Uses the native bridge via xcm('resize_window').</p>
    </div>

    <div class="card">
      <h3>Debug Notes</h3>
      <div id="log" class="log"></div>
    </div>
  </div>
</div>
<script>
function append(msg){
  const el=document.getElementById('log');
  const ts=new Date().toLocaleTimeString();
  el.textContent += '['+ts+'] '+msg+'\n';
  el.scrollTop = el.scrollHeight;
}
function setStatus(id, ok, text){
  const el=document.getElementById(id);
  const c=ok?'ok':(text==='warn'?'warn':'err');
  el.innerHTML = '<span class="dot '+c+'"></span>' + (typeof text==='string' && text!=='warn' ? text : (ok?'online':'offline'));
}
async function probe(url){
  try{ const r = await fetch(url, { method:'GET' }); return r.ok; }
  catch(_){ return false; }
}
async function runChecks(){
  append('Running health checks...');
  const auth = await probe('http://127.0.0.1:9100/health');
  setStatus('auth', auth, auth ? 'online' : 'offline');
  if (!auth) append('Auth server offline. Start with Live CSS Editor option s or option 5.');

  const app = await probe('http://127.0.0.1:9879/');
  setStatus('app', app, app ? 'online' : 'offline');

  const cmd = await probe('http://127.0.0.1:9878/ping');
  setStatus('cmd', cmd, cmd ? 'online' : 'offline');

  const nginx = await probe('https://localhost:8443/page-builder/');
  setStatus('nginx', nginx, nginx ? 'online' : 'offline');

  append('Checks complete.');
}
function openTarget(){
  const t=document.getElementById('target').value.trim();
  if (!t) return;
  if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.xcmBridge) {
    window.webkit.messageHandlers.xcmBridge.postMessage({ action:'open_url', payload:t });
  } else {
    location.href = t;
  }
}
function openComposer(){
  document.getElementById('target').value='https://localhost:8443/page-builder/composer.php?page=landing';
  openTarget();
}
function openDashboard(){
  document.getElementById('target').value='https://localhost:8443/page-builder/pb_admin/dashboard.php';
  openTarget();
}
function resizeHost(w,h){
  append('Resize request: '+w+'x'+h);
  if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.xcmBridge) {
    window.webkit.messageHandlers.xcmBridge.postMessage({ action:'resize_window', payload:w+'x'+h });
  }
}
append('Page Builder Debug initialized.');
runChecks();
</script>
</body>
</html>
