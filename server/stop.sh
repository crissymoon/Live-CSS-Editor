#!/usr/bin/env bash
# server/stop.sh
# Stop nginx and PHP-FPM for the Live CSS Editor.
# Pass --kill to force-kill any lingering processes on ports 8080 / 9000.

set -euo pipefail

SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"
PID_FILE="$SERVER_DIR/run/nginx.pid"

FORCE=0
for arg in "$@"; do [[ "$arg" == "--kill" ]] && FORCE=1; done

echo "==> Stopping Live CSS Editor server stack..."

# ── nginx ─────────────────────────────────────────────────────────────────────
if brew services list 2>/dev/null | grep -q "^nginx.*started"; then
    brew services stop nginx
    echo "    nginx   : stopped via brew services"
elif [[ -f "$PID_FILE" ]]; then
    PID="$(cat "$PID_FILE")"
    kill "$PID" 2>/dev/null && echo "    nginx   : stopped (PID $PID)" \
                             || echo "    nginx   : PID $PID not found"
    rm -f "$PID_FILE"
else
    echo "    nginx   : not running"
fi

# ── PHP-FPM ───────────────────────────────────────────────────────────────────
if brew services list 2>/dev/null | grep -q "^php.*started"; then
    brew services stop php
    echo "    php-fpm : stopped via brew services"
else
    echo "    php-fpm : not in brew services"
fi

# ── Force-kill fallback ───────────────────────────────────────────────────────
if [[ $FORCE -eq 1 ]]; then
    echo ""
    echo "    --kill: removing anything still bound to 8080 / 8443..."
    for _p in 8080 8443; do
        PIDS="$(lsof -iTCP:"$_p" -sTCP:LISTEN -t 2>/dev/null || true)"
        if [[ -n "$PIDS" ]]; then
            echo "$PIDS" | xargs kill 2>/dev/null && echo "    killed PIDs on :$_p: $PIDS"
        else
            echo "    nothing on $_p"
        fi
    done
fi

# ── Clean up socket ───────────────────────────────────────────────────────────
SOCK="$SERVER_DIR/run/php-fpm.sock"
[[ -S "$SOCK" ]] && rm -f "$SOCK" && echo "    removed stale socket: $SOCK"

echo ""
echo "==> Done."
