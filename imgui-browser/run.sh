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

LIVECSS_ROOT="$(cd "$ROOT/.." && pwd)"

# Serve mode controls which app root and default URL are used.
# - page_builder: serves imgui-browser/src/apps and opens page-builder URLs.
# - css_tool: serves dev-tools/dev-browser/apps and opens style-tool URLs.
SERVE_MODE="${IMGUI_SERVE_MODE:-page_builder}"
APPS_DIR_OVERRIDE="${IMGUI_APPS_DIR:-}"
DEFAULT_URL_OVERRIDE="${IMGUI_DEFAULT_URL:-}"

if [[ -n "$APPS_DIR_OVERRIDE" ]]; then
    APPS_DIR="$APPS_DIR_OVERRIDE"
else
    if [[ "$SERVE_MODE" == "css_tool" ]]; then
        APPS_DIR="$ROOT/../dev-tools/dev-browser/apps"
    else
        APPS_DIR="$ROOT/src/apps"
    fi
fi

if [[ ! -d "$APPS_DIR" ]]; then
    echo "[run] ERROR: apps directory not found: $APPS_DIR"
    exit 1
fi

if [[ -n "$DEFAULT_URL_OVERRIDE" ]]; then
    DEFAULT_URL="$DEFAULT_URL_OVERRIDE"
elif [[ "$SERVE_MODE" == "css_tool" ]]; then
    DEFAULT_URL="https://localhost:8443/my_project/index.php"
else
    DEFAULT_URL="https://localhost:8443/page-builder/composer.php?page=landing"
fi

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

# Page-builder mode requires xcm_auth on :9100 for authenticated flows.
if [[ "$SERVE_MODE" == "page_builder" ]]; then
    if ! _port_open 9100; then
        echo "[run] auth server :9100 is offline -- starting xcm_auth first..."
        AUTH_STARTER="$LIVECSS_ROOT/page-builder/pb_admin/start-xcm-auth-only.sh"
        if [[ -f "$AUTH_STARTER" ]]; then
            bash "$AUTH_STARTER" >/tmp/xcm-auth-startup.log 2>&1 &
            for i in {1..40}; do
                _port_open 9100 && break
                sleep 0.2
            done
            if _port_open 9100; then
                echo "[run] xcm_auth ready on :9100"
            else
                echo "[run] WARNING: xcm_auth did not come online (see /tmp/xcm-auth-startup.log)"
            fi
        else
            echo "[run] WARNING: auth starter not found at $AUTH_STARTER"
        fi
    else
        echo "[run] auth server already listening on :9100"
    fi
fi

# Use --url from caller args if provided; fall back to the page-builder default.
_has_url=0
for _a in "$@"; do [[ "$_a" == "--url" ]] && { _has_url=1; break; }; done

if (( _has_url )); then
    exec "$BIN" \
        --apps-dir "$APPS_DIR" \
        --php-port 9879 \
        --cmd-port 9878 \
        "$@"
else
    exec "$BIN" \
        --url "$DEFAULT_URL" \
        --apps-dir "$APPS_DIR" \
        --php-port 9879 \
        --cmd-port 9878 \
        "$@"
fi
