#!/usr/bin/env bash
# to-webp.sh — Convert PNG/JPG images in this folder to WebP
#
# macOS:  uses 'sips' (built-in, no install needed)
#         or 'cwebp' if available:  brew install webp
# Linux:  install cwebp:  sudo apt install webp
#
# Usage:
#   chmod +x to-webp.sh
#   ./to-webp.sh              # converts all images, QUALITY=85
#   ./to-webp.sh 90           # override quality (0-100)
#   ./to-webp.sh 85 ./subdir  # custom dir

set -euo pipefail

DIR="${2:-$(cd "$(dirname "$0")" && pwd)}"
QUALITY="${1:-85}"
converted=0
skipped=0
failed=0

echo "Converting images in: $DIR (quality=$QUALITY)"
echo "---"

for img in "$DIR"/*.{png,jpg,jpeg,PNG,JPG,JPEG} 2>/dev/null; do
    [[ -f "$img" ]] || continue
    out="${img%.*}.webp"

    if [[ -f "$out" ]]; then
        echo "  skip  $out (already exists)"
        ((skipped++))
        continue
    fi

    # Pick converter: prefer cwebp, fall back to sips (macOS), then ffmpeg
    if command -v cwebp &>/dev/null; then
        cwebp -quiet -q "$QUALITY" "$img" -o "$out" \
            && echo "  ok    $(basename "$img") → $(basename "$out")" \
            && ((converted++)) \
            || { echo "  FAIL  $img"; ((failed++)); }
    elif command -v sips &>/dev/null; then
        sips -s format webp -s formatOptions "$QUALITY" "$img" --out "$out" &>/dev/null \
            && echo "  ok    $(basename "$img") → $(basename "$out")" \
            && ((converted++)) \
            || { echo "  FAIL  $img"; ((failed++)); }
    elif command -v ffmpeg &>/dev/null; then
        ffmpeg -loglevel quiet -i "$img" -q:v "$QUALITY" "$out" \
            && echo "  ok    $(basename "$img") → $(basename "$out")" \
            && ((converted++)) \
            || { echo "  FAIL  $img"; ((failed++)); }
    else
        echo "ERROR: No WebP converter found."
        echo "  macOS built-in: sips (already available)"
        echo "  Install cwebp:  brew install webp"
        echo "  Install ffmpeg: brew install ffmpeg"
        exit 1
    fi
done

echo "---"
echo "Done: ${converted} converted, ${skipped} skipped, ${failed} failed."
