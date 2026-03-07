#!/usr/bin/env bash
# build.sh – Top-level orchestrator for the PHP WASM build.
# ─────────────────────────────────────────────────────────────────────────────
# Usage:
#   ./build.sh [OPTIONS]
#
# Options:
#   --php-version VERSION   PHP version to build (default: 8.3.6)
#   --skip-fetch            Skip downloading PHP sources (reuse existing php-src/)
#   --skip-configure        Skip ./configure step (reuse existing Makefile)
#   --skip-make             Skip make step (reuse existing libphp.a)
#   --clean                 Delete build artefacts before starting
#   --help                  Print this help and exit
#
# The build MUST run inside the Docker container unless you already have
# Emscripten installed locally. Use:
#   make docker-build
# for a fully containerised build.
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

# ── Emscripten SDK bootstrap ───────────────────────────────────────────────────
# Source the local EMSDK environment so emcc / emconfigure / emmake are on PATH.
# Override EMSDK_ENV if your installation lives elsewhere.
EMSDK_ENV="${EMSDK_ENV:-/opt/emsdk/emsdk_env.sh}"
if [[ -f "${EMSDK_ENV}" ]]; then
    # shellcheck source=/dev/null
    source "${EMSDK_ENV}" > /dev/null 2>&1
else
    echo "EMSDK not found at ${EMSDK_ENV} -- set EMSDK_ENV or run make docker-build" >&2
    exit 1
fi

# ── Defaults ──────────────────────────────────────────────────────────────────
PHP_VERSION="8.3.20"
SKIP_FETCH=false
SKIP_CONFIGURE=false
SKIP_MAKE=false
DO_CLEAN=false
ROOT="$(cd "$(dirname "$0")" && pwd)"

# ── Argument parsing ───────────────────────────────────────────────────────────

while [[ $# -gt 0 ]]; do
    case "$1" in
        --php-version)   PHP_VERSION="$2"; shift 2 ;;
        --skip-fetch)    SKIP_FETCH=true;  shift ;;
        --skip-configure) SKIP_CONFIGURE=true; shift ;;
        --skip-make)     SKIP_MAKE=true;   shift ;;
        --clean)         DO_CLEAN=true;    shift ;;
        --help)
            sed -n '/^# Usage:/,/^[^#]/p' "$0" | head -n -1 | sed 's/^# //'
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

# ── Helpers ────────────────────────────────────────────────────────────────────

log()  { echo "$(date +%T) ▸ $*"; }
ok()   { echo "$(date +%T) ✓ $*"; }
fail() { echo "$(date +%T) ✗ $*" >&2; exit 1; }

require_tool() {
    command -v "$1" &>/dev/null || fail "$1 not found — install Emscripten SDK or run via Docker."
}

# ── Main pipeline ──────────────────────────────────────────────────────────────

main() {
    cd "${ROOT}"

    if ${DO_CLEAN}; then
        log "Cleaning previous build artefacts…"
        rm -rf php-src wasm/php.js wasm/php.wasm
        ok "Clean complete."
    fi

    require_tool emcc
    require_tool emconfigure

    # Step 1 – Fetch PHP sources
    if ! ${SKIP_FETCH}; then
        log "Fetching PHP ${PHP_VERSION} sources…"
        bash scripts/fetch-php.sh "${PHP_VERSION}"
    fi

    # Step 2 – Apply patches
    log "Applying patches…"
    bash scripts/apply-patches.sh

    # Step 3 – Configure
    if ! ${SKIP_CONFIGURE}; then
        log "Configuring PHP for WebAssembly…"
        bash scripts/configure-php.sh
    fi

    # Step 4 – Compile libphp
    if ! ${SKIP_MAKE}; then
        log "Compiling PHP (this takes a few minutes)…"
        compile_libphp
    fi

    # Step 5 – Link WASM module
    log "Linking WASM module…"
    bash scripts/link-wasm.sh

    ok "Build complete! Output:"
    ls -lh wasm/php.js wasm/php.wasm 2>/dev/null || true
    echo ""
    echo "  Copy wasm/php.js and wasm/php.wasm to your web server"
    echo "  or run:  make serve"
}

compile_libphp() {
    local PHP_SRC="${ROOT}/php-src"
    cd "${PHP_SRC}"

    # Build only the embed static library (libs/libphp.a).
    # The CLI binary links against swapcontext which does not exist in WASM;
    # building only the lib target avoids that link step entirely.
    emmake make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)" \
        libs/libphp.a \
        2>&1 | tee /tmp/php-make.log

    cd "${ROOT}"
}

main "$@"
