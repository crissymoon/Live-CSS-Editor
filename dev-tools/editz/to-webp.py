#!/usr/bin/env python3
"""
to-webp.py — Convert PNG/JPG images in this folder to WebP using Pillow.

Install dependency:
    pip install Pillow

Usage:
    python3 to-webp.py              # converts all images, quality=85
    python3 to-webp.py 90           # override quality (0-100)
    python3 to-webp.py 85 ./subdir  # custom directory
"""

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow is not installed.")
    print("  Install it with:  pip install Pillow")
    sys.exit(1)

# --- Config ---
QUALITY = int(sys.argv[1]) if len(sys.argv) > 1 else 85
SRC_DIR = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(__file__).parent
EXTS    = {".png", ".jpg", ".jpeg"}

converted = skipped = failed = 0

print(f"Converting images in: {SRC_DIR}  (quality={QUALITY})")
print("---")

for src in sorted(SRC_DIR.iterdir()):
    if src.suffix.lower() not in EXTS:
        continue

    dst = src.with_suffix(".webp")

    if dst.exists():
        print(f"  skip  {dst.name}  (already exists)")
        skipped += 1
        continue

    try:
        with Image.open(src) as img:
            # Preserve RGBA/transparency for PNG sources
            if img.mode in ("RGBA", "LA"):
                img.save(dst, "WEBP", quality=QUALITY, lossless=False)
            else:
                rgb = img.convert("RGB")
                rgb.save(dst, "WEBP", quality=QUALITY)
        print(f"  ok    {src.name} → {dst.name}")
        converted += 1
    except Exception as exc:
        print(f"  FAIL  {src.name}: {exc}")
        failed += 1

print("---")
print(f"Done: {converted} converted, {skipped} skipped, {failed} failed.")
