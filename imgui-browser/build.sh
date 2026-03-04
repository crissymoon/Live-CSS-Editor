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

echo ""
echo "[build] done -> $BUILD/imgui_browser"
echo "[build] run with: ./run.sh"
