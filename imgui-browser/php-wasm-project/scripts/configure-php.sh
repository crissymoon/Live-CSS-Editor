#!/usr/bin/env bash
# scripts/configure-php.sh
# ─────────────────────────────────────────────────────────────────────────────
# Run emconfigure ./configure for PHP with the flags required for a WASM build.
# Works with a local EMSDK install (no Docker required).
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

# Source the local EMSDK so emconfigure is available.
EMSDK_ENV="${EMSDK_ENV:-/opt/emsdk/emsdk_env.sh}"
if [[ -f "${EMSDK_ENV}" ]]; then
    # shellcheck source=/dev/null
    source "${EMSDK_ENV}" > /dev/null 2>&1
fi

# PHP 8.x requires Bison >= 3.0. macOS ships Bison 2.3 from Command Line Tools.
# Homebrew bison (brew install bison) installs to a keg-only path; prepend it.
for _bison_prefix in /usr/local/opt/bison/bin /opt/homebrew/opt/bison/bin; do
    if [[ -x "${_bison_prefix}/bison" ]]; then
        export PATH="${_bison_prefix}:${PATH}"
        break
    fi
done

bison --version | head -1  # sanity-check: must print 3.x

PHP_SRC="$(dirname "$0")/../php-src"

if [[ ! -d "${PHP_SRC}" ]]; then
    echo "✗ php-src/ not found. Run scripts/fetch-php.sh first."
    exit 1
fi

cd "${PHP_SRC}"

echo "▸ Running buildconf…"
./buildconf --force

echo "Configuring PHP for WASM..."
emconfigure ./configure \
    --prefix=/out           \
    --host=wasm32-unknown-emscripten \
    --disable-all           \
    --disable-fiber-asm     \
    --disable-phpdbg        \
    --disable-cgi           \
    --disable-cli           \
    --disable-mbregex       \
    --enable-embed=static   \
    --enable-bcmath         \
    --enable-ctype          \
    --enable-mbstring       \
    --with-layout=GNU       \
    --without-pcre-jit      \
    --without-iconv         \
    --without-libxml        \
    --without-pdo-sqlite    \
    --without-sqlite3       \
    CFLAGS="-O2"            \
    LDFLAGS=""              \
    2>&1 | tee /tmp/php-configure.log

echo "✓ Configure complete. Run 'make' next."
