#!/usr/bin/env bash
# scripts/link-wasm.sh
# ─────────────────────────────────────────────────────────────────────────────
# Link the compiled PHP static library + the C shim into php.wasm / php.js
# using Emscripten (emcc).
#
# Prerequisites:
#   - php-src/ built with configure-php.sh + make
#   - EMSDK installed at /opt/emsdk  (or set EMSDK_ENV)
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

# Source EMSDK so emcc is on PATH.
EMSDK_ENV="${EMSDK_ENV:-/opt/emsdk/emsdk_env.sh}"
if [[ -f "${EMSDK_ENV}" ]]; then
    # shellcheck source=/dev/null
    source "${EMSDK_ENV}" > /dev/null 2>&1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PHP_SRC="${ROOT}/php-src"
SHIM="${ROOT}/src/php_wasm_shim.c"
OUT="${ROOT}/wasm"

mkdir -p "${OUT}"

# Virtual filesystem: embed the entire src/ PHP tree into the WASM module.
# PHP scripts will be accessible at /src/... inside the runtime.
FS_EMBED="--embed-file ${ROOT}/src@/src"

# Exported C functions the JS layer calls via ccall()
EXPORTS='["_php_wasm_init","_wasm_exec","_php_get_output","_php_get_headers","_php_get_status","_php_set_server_var","_php_set_request_body","_php_reset","_php_wasm_destroy","_malloc","_free"]'

echo "▸ Linking PHP WASM module…"

emcc "${SHIM}"                          \
    -I"${PHP_SRC}"                      \
    -I"${PHP_SRC}/main"                 \
    -I"${PHP_SRC}/Zend"                 \
    -I"${PHP_SRC}/TSRM"                 \
    -I"${PHP_SRC}/sapi/embed"           \
    -L"${PHP_SRC}/libs"                 \
    -lphp                               \
    ${FS_EMBED}                         \
    -s MODULARIZE=1                     \
    -s EXPORT_NAME="PHP"                \
    -s EXPORTED_FUNCTIONS="${EXPORTS}"  \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","FS","HEAP8"]' \
    -s ALLOW_MEMORY_GROWTH=1            \
    -s INITIAL_MEMORY=67108864          \
    -s MAXIMUM_MEMORY=536870912         \
    -s ENVIRONMENT=web                  \
    -s ASSERTIONS=0                     \
    -s WASM=1                           \
    -O2                                 \
    -o "${OUT}/php.js"

echo "✓ Output files:"
ls -lh "${OUT}/php.js" "${OUT}/php.wasm"
