#!/usr/bin/env bash
# build.sh  --  build xcm render_core for WASM (Emscripten) or native
#
# Usage:
#   ./build.sh          -- WASM build (requires emcc in PATH)
#   ./build.sh native   -- native shared library (for Python wasmtime testing)
#   ./build.sh check    -- just verify compiler/tool availability
#
# Output (WASM):
#   build/wasm/render_core.js     -- Emscripten glue JS
#   build/wasm/render_core.wasm   -- WASM binary
#
# Output (native):
#   build/native/librender_core.dylib  (macOS)
#   build/native/librender_core.so     (Linux)
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
MODE="${1:-wasm}"

# -------------------------------------------------------------------------
# Helpers
# -------------------------------------------------------------------------
info()  { echo "[xcm-build] $*"; }
warn()  { echo "[xcm-build] WARN: $*" >&2; }
die()   { echo "[xcm-build] ERROR: $*" >&2; exit 1; }

# -------------------------------------------------------------------------
# Check mode
# -------------------------------------------------------------------------
if [[ "$MODE" == "check" ]]; then
    echo "--- Tool check ---"
    command -v emcc   && echo "emcc:   $(emcc --version 2>&1 | head -1)"  || echo "emcc:   NOT FOUND"
    command -v cmake  && echo "cmake:  $(cmake --version | head -1)"    || echo "cmake:  NOT FOUND"
    command -v clang  && echo "clang:  $(clang --version 2>&1 | head -1)" || echo "clang: NOT FOUND"
    command -v python3 && echo "python3: $(python3 --version)"           || echo "python3: NOT FOUND"
    command -v node   && echo "node:   $(node --version)"               || echo "node:  NOT FOUND"
    exit 0
fi

# -------------------------------------------------------------------------
# WASM build
# -------------------------------------------------------------------------
if [[ "$MODE" == "wasm" ]]; then
    info "Checking for Emscripten..."
    if ! command -v emcc &>/dev/null; then
        die "emcc not found. Install Emscripten: https://emscripten.org/docs/getting_started/downloads.html
    Quick install:
        git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk
        cd /opt/emsdk && ./emsdk install latest && ./emsdk activate latest
        source /opt/emsdk/emsdk_env.sh"
    fi
    info "emcc: $(emcc --version 2>&1 | head -1)"

    WASM_BUILD="$ROOT/build/wasm"
    mkdir -p "$WASM_BUILD"

    info "Running emcmake cmake..."
    cd "$WASM_BUILD"
    emcmake cmake "$ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -G "Unix Makefiles" \
        2>&1 | tail -20

    info "Building WASM module..."
    emmake make -j"$(sysctl -n hw.logicalcpu 2>/dev/null || nproc)" 2>&1

    info "WASM build complete:"
    ls -lh "$WASM_BUILD/render_core.js" "$WASM_BUILD/render_core.wasm" 2>/dev/null || \
        ls -lh "$WASM_BUILD/"*.wasm "$WASM_BUILD/"*.js 2>/dev/null || \
        die "Output files not found in $WASM_BUILD"

    # Copy to src/ so the JS bridge can load them directly.
    info "Copying to dev-browser/src/ ..."
    cp "$WASM_BUILD/render_core.js"   "$ROOT/../src/render_core.js"
    cp "$WASM_BUILD/render_core.wasm" "$ROOT/../src/render_core.wasm"
    info "Copied render_core.js + render_core.wasm to src/"

    exit 0
fi

# -------------------------------------------------------------------------
# Native build (for local testing with Python / wasmtime CLI)
# -------------------------------------------------------------------------
if [[ "$MODE" == "native" ]]; then
    info "Building native shared library..."

    NATIVE_BUILD="$ROOT/build/native"
    mkdir -p "$NATIVE_BUILD"
    cd "$NATIVE_BUILD"

    cmake "$ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -G "Unix Makefiles" \
        2>&1 | tail -20

    make -j"$(sysctl -n hw.logicalcpu 2>/dev/null || nproc)" 2>&1

    info "Native build complete:"
    ls -lh "$NATIVE_BUILD/"librender_core.* 2>/dev/null || true
    exit 0
fi

die "Unknown mode '$MODE'. Use: wasm | native | check"
