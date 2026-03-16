#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Render Scene JSON to an image using Python bindings")
    parser.add_argument("--scene", required=True, help="Path to scene JSON file")
    parser.add_argument("--output", required=True, help="Output image path (.ppm or .png)")
    parser.add_argument("--width", type=int, default=1200, help="Viewport width")
    parser.add_argument("--height", type=int, default=800, help="Viewport height")
    parser.add_argument("--lib", default="", help="Optional path to render_core shared library")
    return parser.parse_args()


def _load_scene(scene_path: Path) -> str:
    data = json.loads(scene_path.read_text(encoding="utf-8"))
    return json.dumps(data, separators=(",", ":"), ensure_ascii=False)


# Font candidates per platform: (family, weight, italic, path)
_SYSTEM_FONTS: list[tuple[str, int, bool, str]] = [
    # Windows
    ("Segoe UI",  400, False, r"C:\Windows\Fonts\segoeui.ttf"),
    ("Segoe UI",  700, False, r"C:\Windows\Fonts\segoeuib.ttf"),
    # macOS
    ("Helvetica Neue", 400, False, "/System/Library/Fonts/HelveticaNeue.ttc"),
    ("Helvetica Neue", 700, False, "/System/Library/Fonts/HelveticaNeue.ttc"),
    # Common Linux paths
    ("DejaVu Sans", 400, False, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),
    ("DejaVu Sans", 700, False, "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"),
]


def _register_system_fonts(renderer) -> None:
    registered = 0
    for family, weight, italic, path in _SYSTEM_FONTS:
        if Path(path).exists():
            try:
                ok = renderer.register_font(family, weight, italic, path)
                if ok:
                    registered += 1
            except Exception:
                pass
    if registered:
        print(f"[ Info: Registered {registered} system font variant(s) ]")


def _find_repo_root(script_dir: Path) -> Path:
    candidates: list[Path] = []
    for base in (Path.cwd(), script_dir, script_dir.resolve()):
        if base not in candidates:
            candidates.append(base)
        for parent in base.parents:
            if parent not in candidates:
                candidates.append(parent)
    for base in candidates:
        probe = base / "bindings" / "python" / "src" / "xcm_render_core"
        if probe.exists():
            return base
    raise RuntimeError("Could not locate render_eng root containing bindings/python/src/xcm_render_core")


def _main() -> int:
    args = _parse_args()
    script_path = Path(__file__)
    if not script_path.is_absolute():
        script_path = (Path.cwd() / script_path).absolute()
    script_dir = script_path.parent
    repo_root = _find_repo_root(script_dir)

    binding_src = repo_root / "bindings" / "python" / "src"
    if str(binding_src) not in sys.path:
        sys.path.insert(0, str(binding_src))

    from xcm_render_core import Renderer  # type: ignore
    from xcm_render_core.image import save_png, save_ppm  # type: ignore

    scene_path = Path(args.scene)
    if not scene_path.is_absolute():
        scene_path = (Path.cwd() / scene_path).absolute()
    out_path = Path(args.output)
    if not out_path.is_absolute():
        out_path = (Path.cwd() / out_path).absolute()
    out_path.parent.mkdir(parents=True, exist_ok=True)

    scene_json = _load_scene(scene_path)

    with Renderer(args.width, args.height, lib_path=(args.lib or None)) as renderer:
        _register_system_fonts(renderer)
        frame = renderer.render_scene_json(scene_json)
        if out_path.suffix.lower() == ".png":
            try:
                save_png(str(out_path), frame.width, frame.height, frame.rgba)
            except RuntimeError:
                ppm_fallback = out_path.with_suffix(".ppm")
                save_ppm(str(ppm_fallback), frame.width, frame.height, frame.rgba)
                print(str(ppm_fallback))
                return 0
        else:
            save_ppm(str(out_path), frame.width, frame.height, frame.rgba)

    print(str(out_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(_main())
