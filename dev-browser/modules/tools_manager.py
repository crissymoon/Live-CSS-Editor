"""
ToolsManager: persistent list of dev tools and links.

Storage: dev-browser/tools.json (list of {name, url, type}).
  type is "tool" for local server paths or "link" for external URLs.
"""

import os
import json

_HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_TOOLS_FILE = os.path.join(_HERE, 'tools.json')

_BASE_URL = 'https://localhost:8443'

_DEFAULTS = [
    {'name': 'Live CSS Tool',  'url': 'https://localhost:8443/index.php',     'type': 'tool'},
    {'name': 'Agent Flow',     'url': 'https://localhost:8443/agent-flow/',   'type': 'tool'},
    {'name': 'Page Builder',   'url': 'https://localhost:8443/page-builder/', 'type': 'tool'},
    {'name': 'PB Admin',       'url': 'https://localhost:8443/pb_admin/',     'type': 'tool'},
]


def _normalize_url(url: str) -> str:
    """If url looks like a path fragment, prepend the base URL."""
    url = url.strip()
    if url.startswith(('http://', 'https://')):
        return url
    # Treat it as a server-relative path.
    if not url.startswith('/'):
        url = '/' + url
    return _BASE_URL + url


class ToolsManager:
    def __init__(self):
        self._file = _TOOLS_FILE
        self._tools = self._load()

    # ── Persistence ───────────────────────────────────────────────

    def _load(self) -> list:
        if not os.path.exists(self._file):
            self._write(_DEFAULTS)
            return list(_DEFAULTS)
        try:
            with open(self._file, 'r') as f:
                data = json.load(f)
            if isinstance(data, list):
                return data
        except Exception:
            pass
        return list(_DEFAULTS)

    def _write(self, tools: list):
        with open(self._file, 'w') as f:
            json.dump(tools, f, indent=2)

    def _save(self):
        self._write(self._tools)

    # ── Public API ────────────────────────────────────────────────

    def get_tools(self) -> list:
        """Return a copy of the full tools list."""
        return list(self._tools)

    def add_tool(self, name: str, url: str, kind: str = 'tool') -> bool:
        name = name.strip()
        url  = _normalize_url(url)
        if not name or not url:
            return False
        self._tools.append({'name': name, 'url': url, 'type': kind})
        self._save()
        return True

    def edit_tool(self, index: int, name: str, url: str, kind: str) -> bool:
        if index < 0 or index >= len(self._tools):
            return False
        name = name.strip()
        url  = _normalize_url(url)
        if not name or not url:
            return False
        self._tools[index] = {'name': name, 'url': url, 'type': kind}
        self._save()
        return True

    def delete_tool(self, index: int) -> bool:
        if index < 0 or index >= len(self._tools):
            return False
        del self._tools[index]
        self._save()
        return True

    def move_up(self, index: int) -> bool:
        if index <= 0 or index >= len(self._tools):
            return False
        self._tools[index - 1], self._tools[index] = (
            self._tools[index], self._tools[index - 1]
        )
        self._save()
        return True

    def move_down(self, index: int) -> bool:
        if index < 0 or index >= len(self._tools) - 1:
            return False
        self._tools[index + 1], self._tools[index] = (
            self._tools[index], self._tools[index + 1]
        )
        self._save()
        return True
