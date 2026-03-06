#!/usr/bin/env bash
# server/start.sh
# Start PHP-FPM and nginx for the Live CSS Editor.
# Uses brew services so the processes are tracked by launchd.
# If you just want a temporary foreground start (no launchd), pass --fg.

set -euo pipefail

SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"
NGINX_CONF="$SERVER_DIR/conf/nginx.conf"

FG=0
for arg in "$@"; do [[ "$arg" == "--fg" ]] && FG=1; done

_port_open() { nc -z 127.0.0.1 "$1" 2>/dev/null; }

echo "==> Starting Live CSS Editor server stack..."

# ── PHP-FPM ───────────────────────────────────────────────────────────────────
if _port_open 9000 || [[ -S "$SERVER_DIR/run/php-fpm.sock" ]]; then
    echo "    php-fpm : already running"
else
    if [[ $FG -eq 1 ]]; then
        echo "    php-fpm : starting (foreground mode -- Ctrl+C to stop)"
        BREW_PREFIX="$(brew --prefix)"
        php-fpm --nodaemonize \
            --fpm-config "$BREW_PREFIX/etc/php/8.4/php-fpm.conf" &
        PHP_PID=$!
        echo "    php-fpm : PID $PHP_PID"
    else
        brew services start php
        echo "    php-fpm : started via brew services"
    fi
fi

# ── nginx ─────────────────────────────────────────────────────────────────────
PID_FILE="$SERVER_DIR/run/nginx.pid"
if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
    echo "    nginx   : already running  (PID $(cat "$PID_FILE"))"
else
    if [[ $FG -eq 1 ]]; then
        echo "    nginx   : starting (foreground mode)"
        nginx -c "$NGINX_CONF" &
        echo "    nginx   : started"
    else
        brew services start nginx
        echo "    nginx   : started via brew services"
    fi
fi

# ── Wait and confirm ──────────────────────────────────────────────────────────
echo ""
echo "    Waiting for port 8443..."
for i in {1..20}; do
    _port_open 8443 && break
    sleep 0.5
done

_port_open 8443 \
    && echo "    Port 8443 : OK -- https://localhost:8443/pb_admin/dashboard.php" \
    || echo "    Port 8443 : NOT responding -- check logs/nginx-error.log"
