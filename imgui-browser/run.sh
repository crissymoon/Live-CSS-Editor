#!/usr/bin/env bash
# run.sh -- build (if needed) and launch imgui-browser
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

# --- debug logging setup ---
DEBUG_DIR="$ROOT/debug"
mkdir -p "$DEBUG_DIR"
LOG="$DEBUG_DIR/app.log"
# Rotate: keep the last run as app.log.1
[[ -f "$LOG" ]] && mv "$LOG" "${LOG}.1"
exec > >(tee -a "$LOG") 2>&1
echo "========================================"
echo "[run] started at $(date)"
echo "========================================"
trap 'EXIT_CODE=$?; echo "[run] exited with code $EXIT_CODE at $(date)" >> "$LOG"' EXIT
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
LIVECSS_ROOT="$(cd "$ROOT/.." && pwd)"

# Ensure the live-css PHP server on port 8080 is up before launching.
_port_open() { nc -z 127.0.0.1 "$1" &>/dev/null; }
if ! _port_open 8080; then
    echo "[run] port 8080 not listening -- starting live-css PHP server..."
    ROUTER="$LIVECSS_ROOT/pb_admin/router.php"
    if [[ -f "$ROUTER" ]]; then
        mkdir -p "$LIVECSS_ROOT/logs"
        php -S 127.0.0.1:8080 "$ROUTER" -t "$LIVECSS_ROOT" \
            >> "$LIVECSS_ROOT/logs/php-8080.log" 2>&1 &
        disown
        for i in {1..12}; do
            _port_open 8080 && break
            sleep 0.5
        done
        _port_open 8080 && echo "[run] port 8080 ready" \
                        || echo "[run] WARNING: port 8080 still not up after 6s"
    else
        echo "[run] WARNING: $ROUTER not found -- cannot start port 8080"
    fi
else
    echo "[run] port 8080 already listening"
fi

exec "$BIN" \
    --url "http://127.0.0.1:8080/pb_admin/dashboard.php" \
    --apps-dir "$APPS_DIR" \
    --php-port 9879 \
    --cmd-port 9878 \
    "$@"
