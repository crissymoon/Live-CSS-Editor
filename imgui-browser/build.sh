#!/usr/bin/env bash
# build.sh -- configure + compile the imgui-browser
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"

# Ensure deps exist
if [[ ! -d "$ROOT/vendor/imgui" ]]; then
    echo "[build] vendor/imgui missing -- running fetch_deps.sh first..."
    bash "$ROOT/fetch_deps.sh"
fi

mkdir -p "$BUILD"
cd "$BUILD"

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
    "$@"

NPROC=$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
cmake --build . --config Release -j "$NPROC"

# macOS: ad-hoc sign the app bundle so Gatekeeper does not block it.
# A Developer ID cert is not required; the "-" identity is sufficient for
# local use. Also clear the quarantine xattr if it got set.
if [[ "$(uname)" == "Darwin" ]]; then
    echo "[build] ad-hoc signing $BUILD/imgui_browser.app ..."
    codesign --force --deep --sign - "$BUILD/imgui_browser.app"
    xattr -rd com.apple.quarantine "$BUILD/imgui_browser.app" 2>/dev/null || true
    echo "[build] signed OK"
fi

echo ""
echo "[build] done -> $BUILD/imgui_browser"
echo "[build] run with: ./run.sh"
