#!/usr/bin/env bash
# run.sh -- build (if needed) and launch imgui-browser
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
# When built as a proper macOS .app bundle the binary lives inside the bundle.
# Fall back to the flat binary path for non-bundle builds.
BIN_BUNDLE="$ROOT/build/imgui_browser.app/Contents/MacOS/imgui_browser"
BIN_FLAT="$ROOT/build/imgui_browser"

if [[ -f "$BIN_BUNDLE" ]]; then
    BIN="$BIN_BUNDLE"
elif [[ -f "$BIN_FLAT" ]]; then
    BIN="$BIN_FLAT"
else
    echo "[run] binary not found -- building first..."
    bash "$ROOT/build.sh"
    # Re-detect after build
    if [[ -f "$BIN_BUNDLE" ]]; then
        BIN="$BIN_BUNDLE"
    else
        BIN="$BIN_FLAT"
    fi
fi

APPS_DIR="$(cd "$ROOT/../dev-browser/apps" && pwd)"

exec "$BIN" \
    --url "http://127.0.0.1:8080/pb_admin/login.php" \
    --apps-dir "$APPS_DIR" \
    --php-port 9879 \
    --cmd-port 9878 \
    "$@"
