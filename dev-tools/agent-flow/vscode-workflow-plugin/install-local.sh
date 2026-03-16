#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG_JSON="$SCRIPT_DIR/package.json"

if [[ ! -f "$PKG_JSON" ]]; then
  echo "package.json not found at $PKG_JSON" >&2
  exit 1
fi

if command -v jq >/dev/null 2>&1; then
  PUBLISHER="$(jq -r '.publisher' "$PKG_JSON")"
  NAME="$(jq -r '.name' "$PKG_JSON")"
  VERSION="$(jq -r '.version' "$PKG_JSON")"
else
  PUBLISHER="$(node -p "require('$PKG_JSON').publisher")"
  NAME="$(node -p "require('$PKG_JSON').name")"
  VERSION="$(node -p "require('$PKG_JSON').version")"
fi

EXT_ID="${PUBLISHER}.${NAME}-${VERSION}"
TARGET_ROOT="$HOME/.vscode/extensions"
TARGET="$TARGET_ROOT/$EXT_ID"

mkdir -p "$TARGET_ROOT"
rm -rf "$TARGET"
mkdir -p "$TARGET"

cp -R "$SCRIPT_DIR"/* "$TARGET"

echo "Installed: $EXT_ID"
echo "Path: $TARGET"
echo "Restart VS Code (or run Developer: Reload Window)."
