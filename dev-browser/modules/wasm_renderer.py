"""
modules/wasm_renderer.py
------------------------
Python bridge to render_core.wasm (the C++17 rendering engine).

Supports two backends, tried in order:
  1. wasmtime-py   -- pure Python WASM runtime (`pip install wasmtime`)
  2. ctypes        -- loads the native librender_core.dylib/.so built by
                      `render_core/build.sh native`

Typical usage
-------------
from modules.wasm_renderer import WasmRenderer

rend = WasmRenderer()           # lazy – nothing loads until first call
png  = rend.render_to_png(html, css, 1280, 900)
raw  = rend.render(html, css, 1280, 900)   # -> bytes  (RGBA8, w*h*4)
b64  = rend.render_to_base64(html, css)    # -> str    (PNG base-64)

Used by
  - modules/command_server.py  (AI automation pipeline)
  - wasm_bridge.php            (PHP-side renders via subprocess)
  - dev-browser/webbrowse.py   (_start_wasm_renderer() optional worker)
"""

from __future__ import annotations

import base64
import io
import os
import struct
import threading
import time
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
HERE          = Path(__file__).parent
DEV_BROWSER   = HERE.parent if HERE.name == 'modules' else HERE
WASM_PATH     = DEV_BROWSER / 'src' / 'render_core.wasm'
NATIVE_LIB_DYLIB = DEV_BROWSER / 'render_core' / 'build' / 'native' / 'librender_core.dylib'
NATIVE_LIB_SO    = DEV_BROWSER / 'render_core' / 'build' / 'native' / 'librender_core.so'


# ---------------------------------------------------------------------------
# Tiny PNG encoder (no Pillow dependency)
# ---------------------------------------------------------------------------
def _encode_png(rgba_bytes: bytes, width: int, height: int) -> bytes:
    """Encode raw RGBA8 bytes into a minimal valid PNG."""
    import zlib

    def _pack(*args):
        return struct.pack(*args)

    PNG_SIG = b'\x89PNG\r\n\x1a\n'

    def chunk(tag: bytes, data: bytes) -> bytes:
        n = struct.pack('>I', len(data))
        c = struct.pack('>I', (~zlib.crc32(tag + data, 0xFFFFFFFF) & 0xFFFFFFFF) ^ 0xFFFFFFFF)
        # Actually use normal crc32
        crc = zlib.crc32(tag + data) & 0xFFFFFFFF
        return n + tag + data + struct.pack('>I', crc)

    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)
    # colour type 6 = RGBA
    ihdr_data = struct.pack('>II', width, height) + bytes([8, 6, 0, 0, 0])
    ihdr = chunk(b'IHDR', ihdr_data)

    # Build raw image data (filter byte 0 per row).
    raw_rows = bytearray()
    row_size = width * 4
    for y in range(height):
        raw_rows += b'\x00'                        # filter = None
        raw_rows += rgba_bytes[y * row_size: (y + 1) * row_size]

    idat = chunk(b'IDAT', zlib.compress(bytes(raw_rows), 6))
    iend = chunk(b'IEND', b'')

    return PNG_SIG + ihdr + idat + iend


# ---------------------------------------------------------------------------
# Backend: wasmtime-py
# ---------------------------------------------------------------------------
class _WasmtimeBackend:
    """Drives render_core.wasm through the wasmtime Python bindings."""

    def __init__(self, wasm_path: Path):
        from wasmtime import Engine, Store, Module, Instance, Linker, WasiConfig  # type: ignore

        engine   = Engine()
        store    = Store(engine)
        wasi_cfg = WasiConfig()
        wasi_cfg.inherit_stdout()
        wasi_cfg.inherit_stderr()
        store.set_wasi(wasi_cfg)

        linker = Linker(engine)
        linker.define_wasi()

        with open(wasm_path, 'rb') as f:
            module = Module(engine, f.read())

        inst = linker.instantiate(store, module)

        # Grab exports.
        mem        = inst.exports(store)['memory']
        self._store  = store
        self._mem    = mem
        self._inst   = inst

        def ex(name):
            return inst.exports(store)[name]

        self._create      = ex('xcm_create')
        self._destroy     = ex('xcm_destroy')
        self._render      = ex('xcm_render')
        self._pixels      = ex('xcm_pixels')
        self._width       = ex('xcm_width')
        self._height      = ex('xcm_height')
        self._alloc       = ex('xcm_alloc')
        self._free        = ex('xcm_free')
        self._metrics_json= ex('xcm_metrics_json')

        self._ctx = None
        self._ctx_w = 0
        self._ctx_h = 0

    def _write_str(self, s: str) -> tuple[int, int]:
        """Allocate WASM memory, write UTF-8 string, return (ptr, len)."""
        data = s.encode('utf-8')
        n    = len(data)
        ptr  = self._alloc(self._store, n + 1)
        view = self._mem.data_ptr(self._store)
        # view is a ctypes char array.
        import ctypes
        buf = (ctypes.c_uint8 * (n + 1)).from_address(
            ctypes.addressof(view.contents) + ptr
        )
        for i, b in enumerate(data):
            buf[i] = b
        buf[n] = 0
        return ptr, n

    def _read_bytes(self, ptr: int, length: int) -> bytes:
        import ctypes
        view = self._mem.data_ptr(self._store)
        buf = (ctypes.c_uint8 * length).from_address(
            ctypes.addressof(view.contents) + ptr
        )
        return bytes(buf)

    def _ensure_ctx(self, w: int, h: int):
        if self._ctx is not None and (self._ctx_w != w or self._ctx_h != h):
            self._destroy(self._store, self._ctx)
            self._ctx = None
        if self._ctx is None:
            self._ctx   = self._create(self._store, w, h)
            self._ctx_w = w
            self._ctx_h = h

    def render(self, html: str, css: str, width: int, height: int) -> tuple[bytes, int, int, str]:
        self._ensure_ctx(width, height)
        hp, hl = self._write_str(html)
        cp, cl = self._write_str(css)
        rc = self._render(self._store, self._ctx, hp, hl, cp, cl)
        self._free(self._store, hp)
        self._free(self._store, cp)
        if rc != 0:
            raise RuntimeError(f'xcm_render returned {rc}')
        w   = self._width (self._store, self._ctx)
        h   = self._height(self._store, self._ctx)
        pix_ptr = self._pixels(self._store, self._ctx)
        pixels  = self._read_bytes(pix_ptr, w * h * 4)
        mj_ptr  = self._metrics_json(self._store, self._ctx)
        # Read null-terminated string.
        import ctypes
        view = self._mem.data_ptr(self._store)
        base = ctypes.addressof(view.contents)
        i = 0
        mj = []
        while True:
            b = (ctypes.c_uint8 * 1).from_address(base + mj_ptr + i)[0]
            if b == 0: break
            mj.append(chr(b))
            i += 1
        metrics = ''.join(mj)
        return pixels, w, h, metrics

    def close(self):
        if self._ctx is not None:
            try: self._destroy(self._store, self._ctx)
            except Exception: pass
            self._ctx = None


# ---------------------------------------------------------------------------
# Backend: ctypes (native shared library)
# ---------------------------------------------------------------------------
class _CtypesBackend:
    def __init__(self, lib_path: Path):
        import ctypes
        from ctypes import c_int, c_uint8, c_size_t, c_void_p, c_char_p, POINTER

        lib = ctypes.CDLL(str(lib_path))

        def fn(name, restype, *argtypes):
            f = getattr(lib, name)
            f.restype  = restype
            f.argtypes = list(argtypes)
            return f

        self._create       = fn('xcm_create',       c_void_p,    c_int, c_int)
        self._destroy      = fn('xcm_destroy',       None,        c_void_p)
        self._render       = fn('xcm_render',        c_int,       c_void_p, c_char_p, c_int, c_char_p, c_int)
        self._pixels       = fn('xcm_pixels',        POINTER(c_uint8), c_void_p)
        self._width        = fn('xcm_width',         c_int,       c_void_p)
        self._height       = fn('xcm_height',        c_int,       c_void_p)
        self._metrics_json = fn('xcm_metrics_json',  c_char_p,    c_void_p)

        self._ctx   = None
        self._ctx_w = 0
        self._ctx_h = 0

    def _ensure_ctx(self, w, h):
        if self._ctx is not None and (self._ctx_w != w or self._ctx_h != h):
            self._destroy(self._ctx)
            self._ctx = None
        if self._ctx is None:
            self._ctx   = self._create(w, h)
            self._ctx_w = w
            self._ctx_h = h

    def render(self, html: str, css: str, width: int, height: int) -> tuple[bytes, int, int, str]:
        self._ensure_ctx(width, height)
        hb = html.encode('utf-8')
        cb = css.encode('utf-8')
        rc = self._render(self._ctx, hb, len(hb), cb, len(cb))
        if rc != 0:
            raise RuntimeError(f'xcm_render returned {rc}')
        w   = self._width (self._ctx)
        h   = self._height(self._ctx)
        ptr = self._pixels(self._ctx)
        import ctypes
        pixels  = bytes((ctypes.c_uint8 * (w * h * 4)).from_address(
            ctypes.addressof(ptr.contents)))
        metrics = self._metrics_json(self._ctx).decode('utf-8', errors='replace')
        return pixels, w, h, metrics

    def close(self):
        if self._ctx is not None:
            try: self._destroy(self._ctx)
            except Exception: pass
            self._ctx = None


# ---------------------------------------------------------------------------
# Public: WasmRenderer
# ---------------------------------------------------------------------------
class WasmRenderer:
    """
    Thread-safe Python bridge to the C++ rendering engine.

    Each instance holds one rendering context (viewport-sized).
    For concurrent renders, create one WasmRenderer per thread.
    """

    def __init__(
        self,
        default_width:  int = 1280,
        default_height: int = 900,
        backend: Optional[str] = None,   # 'wasmtime' | 'ctypes' | None (auto)
    ):
        self._default_w = default_width
        self._default_h = default_height
        self._backend_hint = backend
        self._lock    = threading.Lock()
        self._backend = None

    # ------------------------------------------------------------------
    # Lazy init
    # ------------------------------------------------------------------
    def _init(self):
        if self._backend is not None:
            return
        hint = self._backend_hint

        errors = []

        # 1. Try wasmtime if wasm file exists.
        if hint in (None, 'wasmtime') and WASM_PATH.exists():
            try:
                self._backend = _WasmtimeBackend(WASM_PATH)
                return
            except Exception as e:
                errors.append(f'wasmtime: {e}')

        # 2. Try ctypes (native dylib/so).
        if hint in (None, 'ctypes'):
            for lib_path in (NATIVE_LIB_DYLIB, NATIVE_LIB_SO):
                if lib_path.exists():
                    try:
                        self._backend = _CtypesBackend(lib_path)
                        return
                    except Exception as e:
                        errors.append(f'ctypes ({lib_path.name}): {e}')

        raise RuntimeError(
            'WasmRenderer: no backend available.\n'
            + '\n'.join(errors) + '\n\n'
            'Build the engine first:\n'
            '  cd dev-browser/render_core && ./build.sh        # WASM\n'
            '  cd dev-browser/render_core && ./build.sh native # native lib\n'
            'Then optionally: pip install wasmtime'
        )

    # ------------------------------------------------------------------
    # Core render (raw RGBA bytes)
    # ------------------------------------------------------------------
    def render(
        self,
        html:   str,
        css:    str = '',
        width:  Optional[int] = None,
        height: Optional[int] = None,
    ) -> bytes:
        """
        Render HTML+CSS at the given viewport size.
        Returns raw RGBA8 bytes (length = width * height * 4).
        """
        w = width  or self._default_w
        h = height or self._default_h
        with self._lock:
            self._init()
            pixels, _w, _h, _metrics = self._backend.render(html, css, w, h)
        return pixels

    def render_with_meta(
        self,
        html:   str,
        css:    str = '',
        width:  Optional[int] = None,
        height: Optional[int] = None,
    ) -> dict:
        """
        Returns {'pixels': bytes, 'width': int, 'height': int, 'metrics': str}
        """
        import json
        w = width  or self._default_w
        h = height or self._default_h
        with self._lock:
            self._init()
            pixels, rw, rh, metrics_json = self._backend.render(html, css, w, h)
        try:
            metrics = json.loads(metrics_json)
        except Exception:
            metrics = {'raw': metrics_json}
        return {'pixels': pixels, 'width': rw, 'height': rh, 'metrics': metrics}

    # ------------------------------------------------------------------
    # PNG helpers
    # ------------------------------------------------------------------
    def render_to_png(
        self,
        html:   str,
        css:    str = '',
        width:  Optional[int] = None,
        height: Optional[int] = None,
    ) -> bytes:
        """Render and return a PNG-encoded byte string."""
        w = width  or self._default_w
        h = height or self._default_h
        with self._lock:
            self._init()
            pixels, rw, rh, _ = self._backend.render(html, css, w, h)
        return _encode_png(pixels, rw, rh)

    def render_to_base64(
        self,
        html:   str,
        css:    str = '',
        width:  Optional[int] = None,
        height: Optional[int] = None,
    ) -> str:
        """Render and return a data-URI-ready base64 PNG string."""
        png = self.render_to_png(html, css, width, height)
        return 'data:image/png;base64,' + base64.b64encode(png).decode()

    def render_to_file(
        self,
        path:   str,
        html:   str,
        css:    str = '',
        width:  Optional[int] = None,
        height: Optional[int] = None,
    ) -> str:
        """Render and write a PNG to the given file path. Returns the path."""
        png = self.render_to_png(html, css, width, height)
        with open(path, 'wb') as f:
            f.write(png)
        return path

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------
    def close(self):
        with self._lock:
            if self._backend:
                self._backend.close()
                self._backend = None

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


# ---------------------------------------------------------------------------
# Module-level singleton (convenient for one-off calls)
# ---------------------------------------------------------------------------
_global_renderer: Optional[WasmRenderer] = None


def get_renderer() -> WasmRenderer:
    """Return the module-level singleton WasmRenderer (lazy init)."""
    global _global_renderer
    if _global_renderer is None:
        _global_renderer = WasmRenderer()
    return _global_renderer


# ---------------------------------------------------------------------------
# CLI  --  python -m modules.wasm_renderer <html_file> [css_file] [out.png]
# ---------------------------------------------------------------------------
if __name__ == '__main__':
    import sys

    def _usage():
        print('Usage: python wasm_renderer.py <html_file|-> [css_file|-] [out.png] [WxH]')
        sys.exit(1)

    args = sys.argv[1:]
    if not args:
        _usage()

    html_arg = args[0]
    css_arg  = args[1] if len(args) > 1 else '-'
    out_arg  = args[2] if len(args) > 2 else 'render_out.png'
    size_arg = args[3] if len(args) > 3 else '1280x900'

    html = sys.stdin.read() if html_arg == '-' else Path(html_arg).read_text()
    css  = ''
    if css_arg not in ('-', ''):
        try: css = Path(css_arg).read_text()
        except FileNotFoundError: css = css_arg   # treat as raw CSS string

    try:
        w, h = map(int, size_arg.lower().split('x'))
    except ValueError:
        w, h = 1280, 900

    t0   = time.perf_counter()
    rend = WasmRenderer(w, h)
    png  = rend.render_to_png(html, css, w, h)
    dt   = (time.perf_counter() - t0) * 1000

    Path(out_arg).write_bytes(png)
    print(f'Rendered {w}x{h} in {dt:.1f} ms  ->  {out_arg}  ({len(png)} bytes)')
    rend.close()
