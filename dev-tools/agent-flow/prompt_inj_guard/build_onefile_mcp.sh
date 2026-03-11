#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="$ROOT_DIR/dist"

echo "[1/3] Installing build dependencies"
python3 -m pip install -r "$ROOT_DIR/model/requirements_inference.txt"
python3 -m pip install pyinstaller

echo "[2/3] Building one-file MCP binary"
python3 -m PyInstaller \
  --clean \
  --onefile \
  --name prompt-inj-guard-mcp \
  --paths "$ROOT_DIR/model" \
  --hidden-import guard_classifier \
  --hidden-import rule_guard \
  --add-data "$ROOT_DIR/api/pattern_db.json:api" \
  "$ROOT_DIR/mcp_guard_server.py"

echo "[3/3] Build complete"
echo "Binary: $DIST_DIR/prompt-inj-guard-mcp"
