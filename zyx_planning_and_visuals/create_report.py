#!/usr/bin/env python3
"""
create_report.py
----------------
Generates a full project report by running three analysis tools:
  - lines_count.py   large-file audit (files over line threshold)
  - security_ck.py   hardcoded secret / API key detection
  - search.py        AI model scan across watch files

Outputs a single timestamped self-contained HTML report in:
  zyx_planning_and_visuals/reports/report_YYYY-MM-DD_HHMMSS.html

Also regenerates:
  zyx_planning_and_visuals/reports/index.html
  with all prior reports listed, searchable and paginated.

Usage:
  python3 create_report.py
  python3 create_report.py --root /path/to/project
  python3 create_report.py --threshold 500
  python3 create_report.py --open         # open in browser after generating
"""

import argparse
import json
import os
import subprocess
import sys
import traceback
import webbrowser
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
_HERE    = Path(__file__).resolve().parent          # zyx_planning_and_visuals/
_ROOT    = _HERE.parent                             # live-css/
_REPORTS = _HERE / "reports"

# Add root to sys.path so we can import the sibling tools directly
if str(_ROOT) not in sys.path:
    sys.path.insert(0, str(_ROOT))

# ---------------------------------------------------------------------------
# Import analysis tools
# ---------------------------------------------------------------------------
try:
    from lines_count import scan as _lc_scan
except Exception as exc:
    print(f"[create_report] ERROR importing lines_count: {exc}", file=sys.stderr, flush=True)
    traceback.print_exc()
    sys.exit(1)

try:
    from security_ck import walk_directory as _sec_walk, scan_file as _sec_scan_file, severity_of as _sec_severity
except Exception as exc:
    print(f"[create_report] ERROR importing security_ck: {exc}", file=sys.stderr, flush=True)
    traceback.print_exc()
    sys.exit(1)

try:
    from search import AI_MODELS, WATCH_FILES
except Exception as exc:
    print(f"[create_report] ERROR importing search: {exc}", file=sys.stderr, flush=True)
    traceback.print_exc()
    sys.exit(1)

# ---------------------------------------------------------------------------
# Severity helpers
# ---------------------------------------------------------------------------

# Map security severity label to 1-10 score
_SEC_SCORE = {
    "CRITICAL": 10,
    "HIGH":     8,
    "MEDIUM":   5,
    "LOW":      2,
}

# Map line count to 1-10 score
def _lines_score(n: int) -> int:
    if n >= 5000: return 9
    if n >= 3000: return 7
    if n >= 2000: return 5
    if n >= 1000: return 3
    return 1


def _score_color(score: int) -> str:
    """Return a CSS hex color for a 1-10 severity score."""
    if score >= 9:  return "#f87171"   # red
    if score >= 7:  return "#fb923c"   # orange
    if score >= 5:  return "#facc15"   # yellow
    if score >= 3:  return "#22d3ee"   # cyan
    return "#4ade80"                   # green


def _score_label(score: int) -> str:
    if score >= 9:  return "CRITICAL"
    if score >= 7:  return "HIGH"
    if score >= 5:  return "MEDIUM"
    if score >= 3:  return "LOW"
    return "INFO"


# ---------------------------------------------------------------------------
# Data collection
# ---------------------------------------------------------------------------

def collect_lines_data(root: Path, threshold: int) -> list[dict]:
    """Run lines_count scan and return structured results."""
    try:
        raw = _lc_scan(str(root), threshold)
        results = []
        for line_count, rel_path in raw:
            score = _lines_score(line_count)
            results.append({
                "file":        rel_path,
                "line_count":  line_count,
                "score":       score,
                "severity":    _score_label(score),
            })
        return results
    except Exception as exc:
        print(f"[create_report] ERROR collecting lines data: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        return []


def collect_security_data(root: Path) -> list[dict]:
    """Run security_ck scan and return structured results."""
    results = []
    files_scanned = 0
    try:
        for file_path in _sec_walk(root):
            try:
                findings = _sec_scan_file(file_path)
                for f in findings:
                    sev = _sec_severity(f["label"])
                    score = _SEC_SCORE.get(sev, 2)
                    results.append({
                        "file":      str(Path(f["file"]).relative_to(root)),
                        "line_no":   f["line_no"],
                        "label":     f["label"],
                        "preview":   f["preview"],
                        "severity":  sev,
                        "score":     score,
                    })
                files_scanned += 1
            except Exception as exc:
                print(f"[create_report] WARNING scanning {file_path}: {exc}", file=sys.stderr, flush=True)
    except Exception as exc:
        print(f"[create_report] ERROR during security walk: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()

    # Sort CRITICAL first
    _rank = {"CRITICAL": 0, "HIGH": 1, "MEDIUM": 2, "LOW": 3}
    results.sort(key=lambda x: (_rank.get(x["severity"], 9), x["file"], x["line_no"]))
    print(f"[create_report] Security scan: {files_scanned} files, {len(results)} findings", file=sys.stderr, flush=True)
    return results


def collect_model_data(root: Path) -> list[dict]:
    """Scan WATCH_FILES for known model strings and return located references."""
    import re as _re
    model_names = [m["name"] for m in AI_MODELS if m["provider"] != "local"]
    pattern = _re.compile("|".join(_re.escape(n) for n in model_names), _re.IGNORECASE)

    results = []
    for rel in WATCH_FILES:
        fpath = root / rel
        if not fpath.is_file():
            continue
        try:
            lines = fpath.read_text(encoding="utf-8", errors="replace").splitlines()
            for lineno, line in enumerate(lines, 1):
                m = pattern.search(line)
                if m:
                    results.append({
                        "file":    rel,
                        "line_no": lineno,
                        "match":   m.group(0),
                        "context": line.strip()[:120],
                    })
        except Exception as exc:
            print(f"[create_report] WARNING reading {fpath}: {exc}", file=sys.stderr, flush=True)
    return results


# ---------------------------------------------------------------------------
# HTML building
# ---------------------------------------------------------------------------

_CSS = """
:root {
  --bg:       #0d0d14;
  --bg2:      #13131f;
  --bg3:      #1a1a2b;
  --border:   #1e1e30;
  --border2:  #2a2a40;
  --text:     #d4d4e8;
  --text-dim: #8888aa;
  --blue:     #5a9cf6;
  --green:    #4ade80;
  --yellow:   #facc15;
  --orange:   #fb923c;
  --red:      #f87171;
  --cyan:     #22d3ee;
  --purple:   #a78bfa;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background: var(--bg);
  color: var(--text);
  font-family: "JetBrains Mono", "Fira Code", "Cascadia Code", ui-monospace, monospace;
  font-size: 13px;
  line-height: 1.6;
}
a { color: var(--blue); text-decoration: none; }
a:hover { text-decoration: underline; }

/* layout */
.wrap { max-width: 1100px; margin: 0 auto; padding: 24px 20px; }

/* header */
.report-header {
  border-bottom: 1px solid var(--border2);
  padding-bottom: 18px;
  margin-bottom: 24px;
}
.report-header h1 { font-size: 20px; color: var(--blue); letter-spacing: 0.04em; }
.report-header .meta { color: var(--text-dim); font-size: 11px; margin-top: 6px; }

/* concern bar */
.concern-bar {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  background: var(--bg2);
  border: 1px solid var(--border2);
  border-radius: 6px;
  padding: 14px 18px;
  margin-bottom: 24px;
}
.concern-pill {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  background: var(--bg3);
  border: 1px solid var(--border);
  border-radius: 4px;
  padding: 4px 10px;
  font-size: 11px;
}
.concern-pill .badge {
  font-size: 10px;
  font-weight: bold;
  padding: 1px 5px;
  border-radius: 3px;
  color: #0d0d14;
}

/* section headings */
.section { margin-bottom: 32px; }
.section-title {
  font-size: 13px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-dim);
  border-bottom: 1px solid var(--border);
  padding-bottom: 6px;
  margin-bottom: 14px;
}
.section-title span { color: var(--blue); }

/* score badge */
.score-badge {
  display: inline-block;
  min-width: 36px;
  text-align: center;
  padding: 2px 7px;
  border-radius: 3px;
  font-size: 11px;
  font-weight: bold;
  color: #0d0d14;
}

/* table */
.report-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
}
.report-table th {
  background: var(--bg3);
  color: var(--text-dim);
  text-align: left;
  padding: 6px 10px;
  border: 1px solid var(--border);
  font-weight: normal;
  white-space: nowrap;
}
.report-table td {
  padding: 5px 10px;
  border: 1px solid var(--border);
  vertical-align: top;
  word-break: break-word;
}
.report-table tr:hover td { background: var(--bg3); }
.dim { color: var(--text-dim); }
.mono { font-family: inherit; }

/* checkbox rows */
.check-row { display: flex; align-items: flex-start; gap: 10px; margin-bottom: 8px; }
.check-row input[type=checkbox] { margin-top: 2px; accent-color: var(--green); flex-shrink: 0; }
.check-row .file-label { flex: 1; }
.check-row .ts { font-size: 10px; color: var(--green); margin-left: 6px; }
.check-row.checked-row .file-label { opacity: 0.45; text-decoration: line-through; }

/* empty notice */
.empty-notice {
  color: var(--green);
  background: var(--bg2);
  border: 1px solid var(--border);
  border-radius: 4px;
  padding: 10px 14px;
  font-size: 12px;
}

/* footer */
.report-footer {
  border-top: 1px solid var(--border);
  margin-top: 32px;
  padding-top: 14px;
  color: var(--text-dim);
  font-size: 11px;
}
"""

_JS = r"""
const LS_KEY = 'report_checks_' + document.querySelector('meta[name=report-id]').content;

function loadChecks() {
  try {
    return JSON.parse(localStorage.getItem(LS_KEY) || '{}');
  } catch(e) {
    console.warn('[report] localStorage parse error:', e);
    return {};
  }
}

function saveChecks(data) {
  try {
    localStorage.setItem(LS_KEY, JSON.stringify(data));
  } catch(e) {
    console.warn('[report] localStorage save error:', e);
  }
}

function applyChecks() {
  const data = loadChecks();
  document.querySelectorAll('.check-row input[type=checkbox]').forEach(cb => {
    const id = cb.dataset.id;
    if (data[id]) {
      cb.checked = true;
      const row = cb.closest('.check-row');
      row.classList.add('checked-row');
      let ts = row.querySelector('.ts');
      if (!ts) {
        ts = document.createElement('span');
        ts.className = 'ts';
        row.querySelector('.file-label').appendChild(ts);
      }
      ts.textContent = '  checked ' + data[id];
    }
  });
}

document.addEventListener('DOMContentLoaded', () => {
  applyChecks();

  document.querySelectorAll('.check-row input[type=checkbox]').forEach(cb => {
    cb.addEventListener('change', () => {
      const id = cb.dataset.id;
      const data = loadChecks();
      const row = cb.closest('.check-row');
      if (cb.checked) {
        const now = new Date().toISOString().replace('T', ' ').slice(0, 19) + ' UTC';
        data[id] = now;
        row.classList.add('checked-row');
        let ts = row.querySelector('.ts');
        if (!ts) {
          ts = document.createElement('span');
          ts.className = 'ts';
          row.querySelector('.file-label').appendChild(ts);
        }
        ts.textContent = '  checked ' + now;
      } else {
        delete data[id];
        row.classList.remove('checked-row');
        const ts = row.querySelector('.ts');
        if (ts) ts.remove();
      }
      saveChecks(data);
    });
  });
});
"""

def _e(s: str) -> str:
    """HTML-escape a string."""
    return (str(s)
        .replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;"))


def _score_html(score: int) -> str:
    color = _score_color(score)
    return (f'<span class="score-badge" style="background:{color}">'
            f'{score}/10&nbsp;{_score_label(score)}</span>')


def _concern_pill(label: str, count: int, score: int) -> str:
    color = _score_color(score)
    return (
        f'<div class="concern-pill">'
        f'<span class="badge" style="background:{color}">{count}</span>'
        f'{_e(label)}'
        f'</div>'
    )


def build_html(
    report_id: str,
    ts_utc: str,
    root: Path,
    lines_data: list[dict],
    sec_data: list[dict],
    model_data: list[dict],
    threshold: int,
) -> str:

    # ---- concerns summary ----
    sec_critical = [x for x in sec_data if x["severity"] == "CRITICAL"]
    sec_high     = [x for x in sec_data if x["severity"] == "HIGH"]
    sec_medium   = [x for x in sec_data if x["severity"] == "MEDIUM"]
    sec_low      = [x for x in sec_data if x["severity"] == "LOW"]
    large_files  = [x for x in lines_data if x["score"] >= 7]

    concern_html = ""
    if sec_critical: concern_html += _concern_pill("Security CRITICAL", len(sec_critical), 10)
    if sec_high:     concern_html += _concern_pill("Security HIGH",     len(sec_high),     8)
    if sec_medium:   concern_html += _concern_pill("Security MEDIUM",   len(sec_medium),   5)
    if sec_low:      concern_html += _concern_pill("Security LOW",      len(sec_low),      2)
    if large_files:  concern_html += _concern_pill(f"Files &gt;3K lines",  len(large_files), 7)
    if model_data:   concern_html += _concern_pill("Model refs found", len(model_data),  2)
    if not concern_html:
        concern_html = '<span style="color:#4ade80">No major concerns detected.</span>'

    # ---- lines section ----
    if lines_data:
        rows = ""
        for i, item in enumerate(lines_data):
            rows += (
                f'<div class="check-row" id="lc-{i}">'
                f'<input type="checkbox" data-id="lc-{i}">'
                f'<div class="file-label">'
                f'{_score_html(item["score"])}&nbsp;&nbsp;'
                f'<span class="mono">{_e(item["file"])}</span>'
                f'&nbsp;<span class="dim">({item["line_count"]:,} lines)</span>'
                f'</div>'
                f'</div>'
            )
        lines_body = rows
    else:
        lines_body = f'<div class="empty-notice">No files found over {threshold} lines.</div>'

    # ---- security section ----
    if sec_data:
        table_rows = ""
        for i, item in enumerate(sec_data):
            color = _score_color(item["score"])
            table_rows += (
                f'<tr>'
                f'<td><input type="checkbox" data-id="sec-{i}"></td>'
                f'<td>{_score_html(item["score"])}</td>'
                f'<td class="mono dim">{_e(item["file"])}</td>'
                f'<td class="dim" style="text-align:right">{item["line_no"]}</td>'
                f'<td style="color:{color}">{_e(item["label"])}</td>'
                f'<td class="dim mono">{_e(item["preview"])}</td>'
                f'</tr>'
            )
        sec_body = (
            '<table class="report-table">'
            '<thead><tr>'
            '<th></th><th>Score</th><th>File</th><th>Line</th><th>Pattern</th><th>Preview</th>'
            '</tr></thead>'
            f'<tbody>{table_rows}</tbody>'
            '</table>'
        )
    else:
        sec_body = '<div class="empty-notice">No secrets or hardcoded credentials detected.</div>'

    # ---- model refs section ----
    if model_data:
        rows = ""
        for i, item in enumerate(model_data):
            rows += (
                f'<div class="check-row" id="ml-{i}">'
                f'<input type="checkbox" data-id="ml-{i}">'
                f'<div class="file-label">'
                f'<span class="dim mono">{_e(item["file"])}</span>'
                f':<span class="dim">{item["line_no"]}</span>'
                f'&nbsp;&mdash;&nbsp;'
                f'<span style="color:#a78bfa">{_e(item["match"])}</span>'
                f'&nbsp;<span class="dim">{_e(item["context"])}</span>'
                f'</div>'
                f'</div>'
            )
        model_body = rows
    else:
        model_body = '<div class="empty-notice">No hardcoded model strings found in watch files.</div>'

    # ---- AI model catalog ----
    cat_rows = ""
    active   = [m for m in AI_MODELS if m.get("active_in_project", True)]
    inactive = [m for m in AI_MODELS if not m.get("active_in_project", True)]
    for m in active + inactive:
        dim = "" if m.get("active_in_project", True) else ' class="dim"'
        cat_rows += (
            f'<tr{dim}>'
            f'<td class="mono" style="color:#a78bfa">{_e(m["name"])}</td>'
            f'<td>{_e(m["provider"])}</td>'
            f'<td>{"yes" if m.get("active_in_project", True) else "no"}</td>'
            f'<td class="dim">{"; ".join(_e(u) for u in m.get("used_in", []))}</td>'
            f'</tr>'
        )
    catalog_body = (
        '<table class="report-table">'
        '<thead><tr>'
        '<th>Model</th><th>Provider</th><th>Active</th><th>Used in</th>'
        '</tr></thead>'
        f'<tbody>{cat_rows}</tbody>'
        '</table>'
    )

    total_findings = len(sec_data)
    worst_score    = max((x["score"] for x in sec_data), default=0)
    worst_score    = max(worst_score, max((x["score"] for x in lines_data), default=0))

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="report-id" content="{_e(report_id)}">
<title>Project Report {ts_utc[:10]}</title>
<style>{_CSS}</style>
</head>
<body>
<div class="wrap">

  <div class="report-header">
    <h1>live-css / Project Report</h1>
    <div class="meta">
      Generated: {_e(ts_utc)} &nbsp;|&nbsp;
      Root: {_e(str(root))} &nbsp;|&nbsp;
      Lines threshold: {threshold:,} &nbsp;|&nbsp;
      Security findings: {total_findings} &nbsp;|&nbsp;
      Worst score: {_score_html(worst_score)}
    </div>
  </div>

  <div class="section">
    <div class="section-title">Items of <span>Concern</span></div>
    <div class="concern-bar">{concern_html}</div>
  </div>

  <div class="section">
    <div class="section-title">Large Files Audit &nbsp;<span>(threshold: {threshold:,} lines)</span></div>
    {lines_body}
  </div>

  <div class="section">
    <div class="section-title">Security Scan &nbsp;<span>({total_findings} findings)</span></div>
    {sec_body}
  </div>

  <div class="section">
    <div class="section-title">Model References in Watch Files &nbsp;<span>({len(model_data)} refs)</span></div>
    {model_body}
  </div>

  <div class="section">
    <div class="section-title">AI Model Catalog</div>
    {catalog_body}
  </div>

  <div class="report-footer">
    Report ID: {_e(report_id)} &nbsp;|&nbsp;
    Checkbox state saved to localStorage under key: report_checks_{_e(report_id)}
    &nbsp;|&nbsp; <a href="index.html">Back to all reports</a>
  </div>

</div>
<script>{_JS}</script>
</body>
</html>"""
    return html


# ---------------------------------------------------------------------------
# Index regeneration
# ---------------------------------------------------------------------------

_INDEX_CSS = """
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background: #0d0d14;
  color: #d4d4e8;
  font-family: "JetBrains Mono", "Fira Code", ui-monospace, monospace;
  font-size: 13px;
  line-height: 1.6;
  padding: 24px 20px;
}
h1 { color: #5a9cf6; font-size: 18px; margin-bottom: 6px; }
.subtitle { color: #8888aa; font-size: 11px; margin-bottom: 20px; }
.controls {
  display: flex;
  gap: 10px;
  margin-bottom: 18px;
  flex-wrap: wrap;
}
#search {
  background: #13131f;
  border: 1px solid #1e1e30;
  color: #d4d4e8;
  border-radius: 4px;
  padding: 6px 12px;
  font-family: inherit;
  font-size: 12px;
  width: 280px;
}
#search:focus { outline: none; border-color: #5a9cf6; }
.btn {
  background: #13131f;
  border: 1px solid #1e1e30;
  color: #d4d4e8;
  border-radius: 4px;
  padding: 5px 12px;
  font-family: inherit;
  font-size: 11px;
  cursor: pointer;
}
.btn:hover { border-color: #5a9cf6; color: #5a9cf6; }
.btn.active { border-color: #5a9cf6; color: #5a9cf6; }
.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(320px,1fr)); gap: 12px; }
.card {
  background: #13131f;
  border: 1px solid #1e1e30;
  border-radius: 6px;
  padding: 14px 16px;
  transition: border-color 0.15s;
  cursor: pointer;
  text-decoration: none;
  color: inherit;
  display: block;
}
.card:hover { border-color: #5a9cf6; }
.card-date { font-size: 12px; color: #5a9cf6; margin-bottom: 4px; }
.card-id { font-size: 10px; color: #8888aa; margin-bottom: 8px; }
.card-pills { display: flex; flex-wrap: wrap; gap: 6px; }
.pill {
  font-size: 10px;
  padding: 2px 7px;
  border-radius: 3px;
  color: #0d0d14;
  font-weight: bold;
}
.pager { margin-top: 18px; display: flex; gap: 8px; align-items: center; }
.count-label { color: #8888aa; font-size: 11px; margin-top: 12px; }
.hidden { display: none !important; }
"""

_INDEX_JS = r"""
const PER_PAGE = 12;
let page = 1;
let filtered = [];

function pill(label, n, bg) {
  if (!n) return '';
  return '<span class="pill" style="background:' + bg + '">' + n + ' ' + label + '</span>';
}

function renderCards(data) {
  filtered = data;
  page = 1;
  render();
}

function render() {
  const grid = document.getElementById('grid');
  const pager = document.getElementById('pager');
  const countLabel = document.getElementById('count-label');
  const total = filtered.length;
  const totalPages = Math.max(1, Math.ceil(total / PER_PAGE));
  if (page > totalPages) page = totalPages;
  const slice = filtered.slice((page - 1) * PER_PAGE, page * PER_PAGE);

  grid.innerHTML = slice.map(r => {
    const pills = [
      pill('CRITICAL', r.sec_critical, '#f87171'),
      pill('HIGH',     r.sec_high,     '#fb923c'),
      pill('MEDIUM',   r.sec_medium,   '#facc15'),
      pill('LOW',      r.sec_low,      '#22d3ee'),
      pill('large files', r.large_files, '#a78bfa'),
    ].join('');
    return '<a class="card" href="' + r.file + '">'
      + '<div class="card-date">' + r.ts + '</div>'
      + '<div class="card-id">' + r.id + '</div>'
      + '<div class="card-pills">' + (pills || '<span style="color:#4ade80;font-size:10px">no concerns</span>') + '</div>'
      + '</a>';
  }).join('');

  // pager
  const btns = [];
  if (page > 1)      btns.push('<button class="btn" onclick="goPage(' + (page-1) + ')">&lt; prev</button>');
  btns.push('<span style="color:#8888aa;font-size:11px">page ' + page + ' / ' + totalPages + '</span>');
  if (page < totalPages) btns.push('<button class="btn" onclick="goPage(' + (page+1) + ')">next &gt;</button>');
  pager.innerHTML = btns.join('');

  countLabel.textContent = total + ' report(s)';
}

function goPage(n) {
  page = n;
  render();
}

function applySearch() {
  const q = document.getElementById('search').value.trim().toLowerCase();
  if (!q) { renderCards(window.ALL_REPORTS); return; }
  renderCards(window.ALL_REPORTS.filter(r =>
    r.ts.toLowerCase().includes(q) ||
    r.id.toLowerCase().includes(q)
  ));
}

document.addEventListener('DOMContentLoaded', () => {
  renderCards(window.ALL_REPORTS);
  document.getElementById('search').addEventListener('input', applySearch);
});
"""


def build_index_html(reports_dir: Path) -> str:
    """Scan reports_dir for report_*.html files and build index."""
    entries = []
    try:
        for f in sorted(reports_dir.glob("report_*.html"), reverse=True):
            # Extract metadata from filename: report_YYYY-MM-DD_HHMMSS.html
            stem = f.stem  # e.g. report_2026-03-02_143055
            parts = stem.split("_", 1)
            date_time = parts[1] if len(parts) > 1 else stem
            ts_display = date_time.replace("_", " ")

            # Try to read concern counts from embedded meta tags -- fall back to zeros
            sec_critical = sec_high = sec_medium = sec_low = large_files = 0
            try:
                content = f.read_text(encoding="utf-8", errors="replace")
                import re as _re
                def _meta(name):
                    m = _re.search(r'<meta name="' + name + r'" content="(\d+)"', content)
                    return int(m.group(1)) if m else 0
                sec_critical = _meta("sec-critical")
                sec_high     = _meta("sec-high")
                sec_medium   = _meta("sec-medium")
                sec_low      = _meta("sec-low")
                large_files  = _meta("large-files")
            except Exception as exc:
                print(f"[create_report] WARNING reading meta from {f.name}: {exc}", file=sys.stderr, flush=True)

            entries.append({
                "file":         f.name,
                "id":           stem,
                "ts":           ts_display,
                "sec_critical": sec_critical,
                "sec_high":     sec_high,
                "sec_medium":   sec_medium,
                "sec_low":      sec_low,
                "large_files":  large_files,
            })
    except Exception as exc:
        print(f"[create_report] ERROR scanning reports dir: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()

    data_json = json.dumps(entries, indent=2)

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Project Reports Index</title>
<style>{_INDEX_CSS}</style>
</head>
<body>
<h1>live-css / Project Reports</h1>
<div class="subtitle">{len(entries)} report(s) &nbsp;|&nbsp; auto-generated index</div>
<div class="controls">
  <input id="search" type="search" placeholder="Search by date or ID...">
</div>
<div class="grid" id="grid"></div>
<div class="pager" id="pager"></div>
<div class="count-label" id="count-label"></div>
<script>
window.ALL_REPORTS = {data_json};
{_INDEX_JS}
</script>
</body>
</html>"""


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a project health report (lines, security, models).",
    )
    parser.add_argument("--root",      default=str(_ROOT),  help="Project root to scan (default: live-css root)")
    parser.add_argument("--threshold", default=1000, type=int, help="Line count threshold for large-file audit (default: 1000)")
    parser.add_argument("--open",      action="store_true",  help="Open generated report in browser after creation")

    try:
        args = parser.parse_args()
    except SystemExit:
        raise
    except Exception as exc:
        print(f"[create_report] ERROR parsing args: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"[create_report] ERROR: root directory does not exist: {root}", file=sys.stderr, flush=True)
        sys.exit(1)

    # Ensure reports dir exists
    try:
        _REPORTS.mkdir(parents=True, exist_ok=True)
    except Exception as exc:
        print(f"[create_report] ERROR creating reports dir {_REPORTS}: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)

    ts_utc = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    ts_id  = datetime.now(timezone.utc).strftime("%Y-%m-%d_%H%M%S")
    report_id = f"report_{ts_id}"

    print(f"[create_report] Root:      {root}",          file=sys.stderr, flush=True)
    print(f"[create_report] Threshold: {args.threshold}",file=sys.stderr, flush=True)
    print(f"[create_report] Report ID: {report_id}",     file=sys.stderr, flush=True)

    # --- collect data ---
    print("[create_report] Running lines audit...",    file=sys.stderr, flush=True)
    lines_data = collect_lines_data(root, args.threshold)
    print(f"[create_report] Lines findings: {len(lines_data)}", file=sys.stderr, flush=True)

    print("[create_report] Running security scan...", file=sys.stderr, flush=True)
    sec_data   = collect_security_data(root)

    print("[create_report] Running model scan...",    file=sys.stderr, flush=True)
    model_data = collect_model_data(root)
    print(f"[create_report] Model refs: {len(model_data)}", file=sys.stderr, flush=True)

    # --- build HTML ---
    try:
        html = build_html(
            report_id  = report_id,
            ts_utc     = ts_utc,
            root       = root,
            lines_data = lines_data,
            sec_data   = sec_data,
            model_data = model_data,
            threshold  = args.threshold,
        )
    except Exception as exc:
        print(f"[create_report] ERROR building HTML: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)

    # Inject data-meta tags for index to read
    sec_critical = len([x for x in sec_data if x["severity"] == "CRITICAL"])
    sec_high     = len([x for x in sec_data if x["severity"] == "HIGH"])
    sec_medium   = len([x for x in sec_data if x["severity"] == "MEDIUM"])
    sec_low      = len([x for x in sec_data if x["severity"] == "LOW"])
    lf_count     = len([x for x in lines_data if x["score"] >= 7])
    meta_tags = (
        f'<meta name="sec-critical" content="{sec_critical}">\n'
        f'<meta name="sec-high"     content="{sec_high}">\n'
        f'<meta name="sec-medium"   content="{sec_medium}">\n'
        f'<meta name="sec-low"      content="{sec_low}">\n'
        f'<meta name="large-files"  content="{lf_count}">\n'
    )
    html = html.replace("</head>", meta_tags + "</head>", 1)

    # --- write report ---
    report_path = _REPORTS / f"{report_id}.html"
    try:
        report_path.write_text(html, encoding="utf-8")
        print(f"[create_report] Report written: {report_path}", file=sys.stderr, flush=True)
    except Exception as exc:
        print(f"[create_report] ERROR writing report: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)

    # --- regenerate index ---
    try:
        index_html  = build_index_html(_REPORTS)
        index_path  = _REPORTS / "index.html"
        index_path.write_text(index_html, encoding="utf-8")
        print(f"[create_report] Index updated: {index_path}", file=sys.stderr, flush=True)
    except Exception as exc:
        print(f"[create_report] ERROR writing index: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        # non-fatal -- report still usable without index

    # --- summary to stdout ---
    print(f"\nReport: {report_path}")
    print(f"Index:  {_REPORTS / 'index.html'}")
    print(f"\nSummary:")
    print(f"  Large files (>={args.threshold} lines): {len(lines_data)}")
    print(f"  Security findings:  {len(sec_data)}  "
          f"(CRITICAL:{sec_critical} HIGH:{sec_high} MEDIUM:{sec_medium} LOW:{sec_low})")
    print(f"  Model refs found:   {len(model_data)}")

    if args.open:
        try:
            webbrowser.open(report_path.as_uri())
        except Exception as exc:
            print(f"[create_report] WARNING could not open browser: {exc}", file=sys.stderr, flush=True)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[create_report] Interrupted.", file=sys.stderr, flush=True)
        sys.exit(130)
    except Exception as exc:
        print(f"[create_report] UNHANDLED EXCEPTION: {exc}", file=sys.stderr, flush=True)
        traceback.print_exc()
        sys.exit(1)
