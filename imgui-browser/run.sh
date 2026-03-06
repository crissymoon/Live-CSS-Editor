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

# Ensure the nginx HTTPS server on port 8443 is up before launching.
# nginx + PHP-FPM are managed by brew services (launchd) and should already
# be running.  If not, run: bash "$LIVECSS_ROOT/server/start.sh"
_port_open() { nc -z 127.0.0.1 "$1" &>/dev/null; }
if ! _port_open 8443; then
    echo "[run] WARNING: port 8443 (nginx HTTPS) not listening."
    echo "[run] Attempting to start server stack..."
    if bash "$LIVECSS_ROOT/server/start.sh" 2>/dev/null; then
        for i in {1..14}; do
            _port_open 8443 && break
            sleep 0.5
        done
        _port_open 8443 && echo "[run] port 8443 ready" \
                        || echo "[run] WARNING: port 8443 still not up after 7s"
    else
        echo "[run] server/start.sh failed -- open a terminal and run:"
        echo "[run]   bash $LIVECSS_ROOT/server/start.sh"
    fi
else
    echo "[run] port 8443 already listening"
fi

exec "$BIN" \
    --url "https://localhost:8443/pb_admin/dashboard.php" \
    --apps-dir "$APPS_DIR" \
    --php-port 9879 \
    --cmd-port 9878 \
    "$@"
