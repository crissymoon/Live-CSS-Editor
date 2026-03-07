#!/usr/bin/env bash
# scripts/fetch-php.sh
# Download and unpack the PHP source tree ready for the WASM build.
#
# Usage:  ./scripts/fetch-php.sh [PHP_VERSION]

set -euo pipefail

PHP_VERSION="${1:-8.3.20}"
ARCHIVE="php-${PHP_VERSION}.tar.gz"
DOWNLOAD_URL="https://www.php.net/distributions/${ARCHIVE}"
DEST_DIR="$(cd "$(dirname "$0")/.." && pwd)/php-src"

echo "Fetching PHP ${PHP_VERSION}..."

if [[ -d "${DEST_DIR}" ]]; then
    echo "  php-src/ already exists -- skipping download."
    exit 0
fi

TMP_DIR=$(mktemp -d)
trap 'rm -rf "${TMP_DIR}"' EXIT

echo "  Downloading ${DOWNLOAD_URL}"
curl -fsSL --progress-bar "${DOWNLOAD_URL}" -o "${TMP_DIR}/${ARCHIVE}"

# php.net no longer publishes a .sha256 file alongside the tarball.
# Verify the download is not empty/truncated (>= 10 MB).
FILE_BYTES=$(wc -c < "${TMP_DIR}/${ARCHIVE}" | tr -d ' ')
if [[ "${FILE_BYTES}" -lt 10000000 ]]; then
    echo "  Download looks truncated (${FILE_BYTES} bytes) -- aborting."
    exit 1
fi
echo "  Size OK ($(( FILE_BYTES / 1024 / 1024 )) MB)"

echo "  Unpacking..."
mkdir -p "${DEST_DIR}"
tar -xzf "${TMP_DIR}/${ARCHIVE}" --strip-components=1 -C "${DEST_DIR}"

echo "PHP ${PHP_VERSION} source ready at ${DEST_DIR}"
