"""
adblock_lists.py -- EasyList / EasyPrivacy integration.

Downloads the two main open-source filter lists from GitHub CDN mirrors,
parses the plain-host network-blocking rules (||domain.tld^), deduplicates
with the built-in hardcoded _AD_DOMAINS set, and exposes the combined
result as a frozenset of lowercase hostnames.

The download happens once per 24 hours in a background daemon thread.
The caller receives a live _HostSet object whose contents are swapped
atomically when the download completes -- no restart needed.

Usage
-----
    from .adblock_lists import get_block_set, refresh_in_background

    # Called once at startup.  Returns a _HostSet immediately.
    # The set is populated from the on-disk cache (if fresh) or from the
    # hardcoded fallback.  A background thread fetches new data.
    block_set = get_block_set(seed_domains)
    refresh_in_background(block_set)

    # Later, in an interceptor:
    if block_set.matches(host):
        info.block(True)
"""

from __future__ import annotations

import os
import re
import time
import threading
import urllib.request
import ssl as _ssl

# ---------------------------------------------------------------------------
# Filter list sources
# ---------------------------------------------------------------------------
# easylist.to is the canonical host for the compiled list files.
# jsDelivr cannot serve them because the .txt files are generated artifacts
# not committed to the easylist/easylist GitHub repo.
_LISTS = [
    {
        'name': 'easylist',
        'primary': 'https://easylist.to/easylist/easylist.txt',
        'fallback': 'https://ublockorigin.pages.dev/thirdparties/easylist.txt',
    },
    {
        'name': 'easyprivacy',
        'primary': 'https://easylist.to/easylist/easyprivacy.txt',
        'fallback': 'https://ublockorigin.pages.dev/thirdparties/easyprivacy.txt',
    },
]

# Refresh if on-disk cache is older than 24 hours.
_TTL_SECONDS = 24 * 3600

_CACHE_DIR = os.path.expanduser('~/.xcaliburmoon_profile/adblock')

# ---------------------------------------------------------------------------
# Rule parser
# ---------------------------------------------------------------------------
# EasyList rule format we care about:
#   ||domain.tld^              block domain + subdomains (any type, any party)
#   ||domain.tld^$option,...   same with options
#   @@||...                    exception (whitelist) -- skipped
#   ! comment                  skipped
#   ##selector                 cosmetic -- skipped
#   Lines with /path           path-specific -- skipped
#   Lines with *               wildcard -- skipped (too broad)
#   Lines that look like /regex/ -- skipped
_DOMAIN_RULE_RE = re.compile(
    r'^\|\|'               # starts with ||
    r'([a-z0-9]'           # first char: alphanumeric
    r'[a-z0-9\-\.]*'       # rest of domain chars (dots and hyphens ok)
    r'\.[a-z]{2,})'        # TLD with at least 2 chars
    r'\^'                  # anchor separator
    r'(?:\$[^/\n]*)?$',    # optional options ($third-party etc.) no path
    re.IGNORECASE,
)


def _parse_rules(text: str) -> set[str]:
    """Extract blocked hostnames from an ABP-format filter list."""
    blocked: set[str] = set()
    for line in text.splitlines():
        line = line.strip()
        # Skip empty, comments, cosmetic rules, exceptions, regex rules.
        if not line:
            continue
        if line.startswith(('!', '#', '@@', '[Adblock')):
            continue
        if '##' in line or '#@#' in line or '#?#' in line:
            continue
        if line.startswith('/') and line.endswith('/'):
            continue
        m = _DOMAIN_RULE_RE.match(line)
        if m:
            blocked.add(m.group(1).lower())
    return blocked


# ---------------------------------------------------------------------------
# _HostSet -- a thread-safe swappable host set
# ---------------------------------------------------------------------------

class _HostSet:
    """Mutable container holding the current set of blocked hostnames.

    Thread-safe: _data is replaced atomically via a lock.  The matching
    loop is read-only so it holds no lock (GIL protects the reference read).
    """

    def __init__(self, initial: frozenset[str]):
        self._data: frozenset[str] = initial
        self._lock = threading.Lock()

    def update(self, new_set: frozenset[str]) -> None:
        with self._lock:
            self._data = new_set

    def matches(self, host: str) -> bool:
        """Return True if *host* or any parent domain is in the blocked set.

        For host 'ads.tracker.example.com' this checks:
            ads.tracker.example.com
            tracker.example.com
            example.com
            com
        O(depth) -- typically 3-4 checks, no regex.
        """
        data = self._data   # single reference read -- atomic under GIL
        parts = host.split('.')
        for i in range(len(parts)):
            if '.'.join(parts[i:]) in data:
                return True
        return False

    def __len__(self) -> int:
        return len(self._data)


# ---------------------------------------------------------------------------
# Download / cache
# ---------------------------------------------------------------------------

def _fetch_text(url: str, timeout: int = 20) -> str | None:
    ctx = _ssl.create_default_context()
    ctx.check_hostname = False
    ctx.verify_mode  = _ssl.CERT_NONE
    req = urllib.request.Request(url, headers={
        'User-Agent': 'Mozilla/5.0 (compatible; xcm-adblock/1.0)',
        'Accept-Encoding': 'gzip',
    })
    try:
        resp = urllib.request.urlopen(req, context=ctx, timeout=timeout)
        data = resp.read()
        resp.close()
        # Handle gzip-encoded response.
        enc = resp.headers.get('Content-Encoding', '')
        if enc == 'gzip':
            import gzip
            data = gzip.decompress(data)
        return data.decode('utf-8', errors='replace')
    except Exception as exc:
        print(f'[adblock] fetch failed {url}: {exc}', flush=True)
        return None


def _cache_path(name: str) -> str:
    os.makedirs(_CACHE_DIR, exist_ok=True)
    return os.path.join(_CACHE_DIR, f'{name}.txt')


def _is_cache_fresh(name: str) -> bool:
    path = _cache_path(name)
    if not os.path.exists(path):
        return False
    age = time.time() - os.path.getmtime(path)
    return age < _TTL_SECONDS


def _load_cache(name: str) -> str | None:
    path = _cache_path(name)
    try:
        with open(path, 'r', encoding='utf-8', errors='replace') as f:
            return f.read()
    except Exception:
        return None


def _save_cache(name: str, text: str) -> None:
    path = _cache_path(name)
    try:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(text)
    except Exception as exc:
        print(f'[adblock] cache write failed: {exc}', flush=True)


def _download_list(info: dict) -> set[str]:
    """Fetch (from cache or network) and parse a single filter list."""
    name = info['name']
    if _is_cache_fresh(name):
        text = _load_cache(name)
        if text:
            parsed = _parse_rules(text)
            print(f'[adblock] {name}: {len(parsed)} domain rules (from cache)',
                  flush=True)
            return parsed
    # Try primary then fallback.
    text = _fetch_text(info['primary']) or _fetch_text(info['fallback'])
    if not text:
        # Use stale cache if available.
        text = _load_cache(name)
    if text:
        parsed = _parse_rules(text)
        _save_cache(name, text)
        print(f'[adblock] {name}: {len(parsed)} domain rules (downloaded)',
              flush=True)
        return parsed
    return set()


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

_instance: _HostSet | None = None
_instance_lock = threading.Lock()


def get_block_set(seed_domains: set[str]) -> '_HostSet':
    """Return the singleton _HostSet, seeded immediately with *seed_domains*.

    If a fresh on-disk cache exists, it is loaded synchronously on the first
    call so the interceptor starts with good data before the first page loads.
    Subsequent calls return the same instance.
    """
    global _instance
    with _instance_lock:
        if _instance is not None:
            return _instance
        # Try loading from cache synchronously (fast path -- file I/O only).
        combined: set[str] = set(seed_domains)
        all_fresh = all(_is_cache_fresh(info['name']) for info in _LISTS)
        if all_fresh:
            for info in _LISTS:
                text = _load_cache(info['name'])
                if text:
                    combined.update(_parse_rules(text))
            print(f'[adblock] loaded {len(combined)} total blocked domains '
                  f'(cache hit)', flush=True)
        else:
            print(f'[adblock] starting with {len(combined)} built-in domains '
                  f'(lists will download in background)', flush=True)
        _instance = _HostSet(frozenset(combined))
        return _instance


def refresh_in_background(block_set: '_HostSet', seed_domains: set[str]) -> None:
    """Spawn a daemon thread to download and apply updated filter lists.

    Safe to call even if the cache is still fresh -- the thread will exit
    immediately after confirming freshness without downloading.
    """
    def _worker():
        combined: set[str] = set(seed_domains)
        for info in _LISTS:
            combined.update(_download_list(info))
        new_frozen = frozenset(combined)
        block_set.update(new_frozen)
        print(f'[adblock] updated: {len(new_frozen)} total blocked domains',
              flush=True)

    t = threading.Thread(target=_worker, name='adblock-refresh', daemon=True)
    t.start()
