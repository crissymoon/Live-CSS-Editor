#!/usr/bin/env bash
# run.sh -- build (if needed) and launch imgui-browser
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN="$ROOT/build/imgui_browser"

if [[ ! -f "$BIN" ]]; then
    echo "[run] binary not found -- building first..."
    bash "$ROOT/build.sh"
fi

APPS_DIR="$(cd "$ROOT/../dev-browser/apps" && pwd)"

exec "$BIN" \
    --url "http://127.0.0.1:8080/pb_admin/login.php" \
    --apps-dir "$APPS_DIR" \
    --php-port 9879 \
    --cmd-port 9878 \
    "$@"
