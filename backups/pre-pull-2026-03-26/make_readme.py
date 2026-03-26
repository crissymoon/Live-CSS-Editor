#!/usr/bin/env python3
"""
make_readme.py
Generates README.md at the project root from embedded static content plus
an auto-built file tree and directory-map table.

Run directly:   python3 make_readme.py
Called by:      push.sh before git add .
"""

import os
from datetime import datetime

ROOT = os.path.dirname(os.path.abspath(__file__))
OUT  = os.path.join(ROOT, 'README.md')

# Dirs to skip when building the file tree
SKIP_DIRS = {
    '.git', '__pycache__', 'node_modules', 'vendor', 'venv', '.venv',
    'logs', 'build', 'dist', 'target', 'WidevineCdm', 'widevine',
    'backups', 'spam_injection_model', 'reports', 'run', 'certs',
    '.mypy_cache', '.pytest_cache',
}

# ── Static content blocks ─────────────────────────────────────────────────────

HEADER = """\
# Live CSS Editor

**Author:** Crissy Deutsch  
**Company:** XcaliburMoon Web Development  
**Website:** https://xcaliburmoon.net/  
**License:** MIT

A live HTML / CSS / JS editor with an AI agent, VSCode Copilot bridge, page builder, Go auth API, and a cross-platform native browser shell. The style tool, page builder, auth system, dev utilities, and desktop app packaging pipeline all run locally with no cloud dependency for the core workflow.
"""

QUICK_START = """\
## Quick Start

```bash
# Start PHP server (style tool only)
cd my_project
php -S 127.0.0.1:9879 index.php

# Start full stack (nginx + auth + browser)
bash server/start.sh
bash page-builder/pb_admin/start-auth.sh
bash imgui-browser/run.sh

# Push to GitHub (auto-updates this README first)
bash push.sh
```
"""

TECH_STACK = """\
## Technology Stack

| Layer | Technology |
|-------|-----------|
| Style tool backend | PHP 8.x built-in server |
| Desktop shell | C++ / Dear ImGui / WKWebView -- cross-platform packaging target |
| Dev browser (local only) | Python 3 + PyQt6 / WKWebView via pyobjc |
| Auth API | Go (xcm_auth, port 9100) |
| Editors | CodeMirror 5.65.16 (local vendor) |
| AI streaming | Server-Sent Events via PHP |
| MCP server | Node.js 18+ with @modelcontextprotocol/sdk |
| TUI agent | Python 3, curses, Anthropic Haiku, OpenAI GPT-4o mini |
| DB browser | C, GTK+3, libsqlite3 |
| Mood check | Python 3, EMI bio-inspired engine, 370K-word lexicon |
| Prompt guard | Python 3, DistilBERT (transformers + torch), Flask |
| Email smoke | Python 3 stdlib only |
| Phrase storage | SQLite via PHP |
| JS modules | Vanilla ES5/ES6 under window.LiveCSS |
"""

LICENSE_SECTION = """\
## License

MIT License -- see [LICENSE](LICENSE) for full text.

Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
"""

# ── Directory descriptions (order is the display order) ──────────────────────

DIR_MAP = [
    ('my_project',    'Live HTML/CSS/JS editor with AI agent and VSCode bridge', True),
    ('page-builder',  'JSON-driven page composer + admin panel + Go auth',       True),
    ('dev-tools',     'Dev utilities: browser, debug tracker, TUI agent, tools', True),
    ('imgui-browser', 'Cross-platform native browser shell (C++ / Dear ImGui) -- packaging target', True),
    ('server',        'nginx + PHP-FPM configuration and startup scripts',        True),
    ('db_bridge',     'Shared SQLite bridge API (PHP + JS client)',               True),
]

# ── Tree builder ──────────────────────────────────────────────────────────────

def build_tree(path, prefix='', depth=0, max_depth=1):
    """Return a list of lines forming a compact directory tree."""
    if depth > max_depth:
        return []
    lines = []
    try:
        entries = sorted(os.listdir(path))
    except PermissionError:
        return []
    entries = [e for e in entries if not e.startswith('.')]
    dirs  = [e for e in entries if os.path.isdir(os.path.join(path, e)) and e not in SKIP_DIRS]
    files = [e for e in entries if os.path.isfile(os.path.join(path, e))]
    # show up to 5 notable files at root level only
    if depth == 0:
        notable = [f for f in files if f.endswith(('.php', '.sh', '.py', '.js', '.md', '.json'))][:5]
        for f in notable:
            lines.append(f'{prefix}  {f}')
    for i, d in enumerate(dirs):
        connector = '  ' if i < len(dirs) - 1 else '  '
        lines.append(f'{prefix}  {d}/')
        if depth < max_depth:
            lines.extend(build_tree(os.path.join(path, d), prefix + '  ', depth + 1, max_depth))
    return lines

# ── Section builders ──────────────────────────────────────────────────────────

def build_dir_map():
    rows = ['## Directory Map\n',
            '| Folder | Description | README |',
            '|--------|-------------|--------|']
    for folder, desc, has_readme in DIR_MAP:
        readme_path = os.path.join(ROOT, folder, 'README.md')
        if os.path.isfile(readme_path):
            link = f'[README]({folder}/README.md)'
        else:
            link = '--'
        rows.append(f'| `{folder}/` | {desc} | {link} |')
    return '\n'.join(rows) + '\n'

def build_file_tree():
    lines = ['## Project Structure\n', '```', 'live-css/']
    for folder, desc, _ in DIR_MAP:
        lines.append(f'  {folder}/{"  -- " + desc}')
    # also show root-level notable files
    root_files = sorted([
        f for f in os.listdir(ROOT)
        if os.path.isfile(os.path.join(ROOT, f))
        and f.endswith(('.php', '.sh', '.py', '.md', '.json', '.txt'))
        and not f.startswith('.')
    ])
    notable = [f for f in root_files if f not in ('README.md',)][:6]
    for f in notable:
        lines.append(f'  {f}')
    lines.append('```')
    return '\n'.join(lines) + '\n'

def build_timestamp():
    ts = datetime.now().strftime('%Y-%m-%d  %H:%M:%S')
    return f'---\n\n*README last generated: {ts}*\n'

# ── Main ──────────────────────────────────────────────────────────────────────

def generate():
    sections = [
        HEADER,
        QUICK_START,
        build_dir_map(),
        build_file_tree(),
        TECH_STACK,
        LICENSE_SECTION,
        build_timestamp(),
    ]
    return '\n'.join(sections)

def main():
    content = generate()
    with open(OUT, 'w', encoding='utf-8') as fh:
        fh.write(content)
    print(f'README.md written ({len(content)} chars)  --  {OUT}')

if __name__ == '__main__':
    main()
