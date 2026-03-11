#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="$ROOT_DIR/dist"
RELEASE_DIR="$ROOT_DIR/release"

VERSION="${1:-}"
if [[ -z "$VERSION" ]]; then
  VERSION="$(date +%Y%m%d-%H%M%S)"
fi

PLATFORM="$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m)"
BIN_NAME="prompt-inj-guard-mcp"
OUT_NAME="${BIN_NAME}-${VERSION}-${PLATFORM}"

echo "[1/5] Building one-file binary"
"$ROOT_DIR/build_onefile_mcp.sh"

echo "[2/5] Preparing release folder"
mkdir -p "$RELEASE_DIR"
cp "$DIST_DIR/$BIN_NAME" "$RELEASE_DIR/$OUT_NAME"

echo "[3/5] Generating checksums"
(
  cd "$RELEASE_DIR"
  shasum -a 256 "$OUT_NAME" > "${OUT_NAME}.sha256"
)

echo "[4/5] Writing build metadata"
{
  echo "version=$VERSION"
  echo "platform=$PLATFORM"
  echo "built_at_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "python=$(python3 --version 2>&1)"
  echo "pyinstaller=$(python3 -m PyInstaller --version 2>&1)"
  echo "git_commit=$(git -C "$ROOT_DIR" rev-parse --short HEAD 2>/dev/null || echo unknown)"
} > "$RELEASE_DIR/${OUT_NAME}.build-info.txt"

echo "[5/5] Release artifact ready"
echo "Binary:   $RELEASE_DIR/$OUT_NAME"
echo "SHA256:   $RELEASE_DIR/${OUT_NAME}.sha256"
echo "BuildInfo:$RELEASE_DIR/${OUT_NAME}.build-info.txt"
