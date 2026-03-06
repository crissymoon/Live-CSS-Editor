#!/usr/bin/env bash
# server/status.sh
# Show the current state of nginx, PHP-FPM, and the auth server.

set -uo pipefail

SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"

C_GREEN='\033[38;5;114m'
C_RED='\033[38;5;203m'
C_YELLOW='\033[38;5;222m'
C_CYAN='\033[38;5;116m'
C_GREY='\033[38;5;244m'
C_WHITE='\033[38;5;255m'
R='\033[0m'
BOLD='\033[1m'

ok()   { printf "  ${C_GREEN}[ OK ]${R}  %s\n" "$*"; }
fail() { printf "  ${C_RED}[FAIL]${R}  %s\n" "$*"; }
warn() { printf "  ${C_YELLOW}[WARN]${R}  %s\n" "$*"; }

_port_open() { nc -z 127.0.0.1 "$1" 2>/dev/null; }

_proc_info() {
    local port="$1"
    lsof -iTCP:"$port" -sTCP:LISTEN -n -P 2>/dev/null \
        | awk 'NR>1 {printf "PID %-7s  %s\n", $2, $1}' \
        | head -2
}

printf "\n  ${C_CYAN}${BOLD}LIVE CSS EDITOR -- server status${R}\n"
printf "  ${C_GREY}$(date '+%Y-%m-%d  %H:%M:%S')${R}\n\n"

# ── Ports ─────────────────────────────────────────────────────────────────────
printf "  ${C_GREY}Ports${R}\n"

for port in 8080 8443 9000 9100; do
    label=""
    case "$port" in
        8080) label="nginx (redirect) " ;;
        8443) label="nginx (HTTPS)    " ;;
        9000) label="PHP-FPM          " ;;
        9100) label="auth (Go)        " ;;
    esac
    if _port_open "$port"; then
        info="$(_proc_info "$port")"
        ok "${label}:${port}  ${C_GREY}${info}${R}"
    else
        fail "${label}:${port}"
    fi
done

# ── brew services ─────────────────────────────────────────────────────────────
printf "\n  ${C_GREY}brew services${R}\n"
brew services list 2>/dev/null \
    | grep -E "^(nginx|php)" \
    | while IFS= read -r line; do
        if echo "$line" | grep -q "started"; then
            ok "$line"
        elif echo "$line" | grep -q "none\|stopped\|error"; then
            fail "$line"
        else
            warn "$line"
        fi
    done

# ── nginx pid + config test ────────────────────────────────────────────────────
printf "\n  ${C_GREY}nginx config test${R}\n"
NGINX_CONF="$SERVER_DIR/conf/nginx.conf"
if [[ -f "$NGINX_CONF" ]]; then
    if nginx -c "$NGINX_CONF" -t 2>/dev/null; then
        ok "nginx -t passed"
    else
        fail "nginx -t FAILED -- run: nginx -c $NGINX_CONF -t"
    fi
else
    warn "nginx.conf not found at $SERVER_DIR/conf/nginx.conf"
fi

# ── PHP-FPM socket ────────────────────────────────────────────────────────────
printf "\n  ${C_GREY}PHP-FPM socket${R}\n"
SOCK="$SERVER_DIR/run/php-fpm.sock"
if [[ -S "$SOCK" ]]; then
    ok "socket exists: $SOCK"
else
    fail "socket missing: $SOCK  (php-fpm may not be running with our pool)"
fi

# ── HTTPS smoke test ─────────────────────────────────────────────────────────
printf "\n  ${C_GREY}HTTPS smoke test${R}\n"
# Use system curl which reads the macOS keychain and trusts the mkcert local CA.
SYS_CURL="/usr/bin/curl"
if _port_open 8443; then
    STATUS="$("$SYS_CURL" -so /dev/null -w "%{http_code}" \
                   --max-time 3 https://localhost:8443/pb_admin/login.php 2>/dev/null)"
    if [[ "$STATUS" == "200" || "$STATUS" == "302" ]]; then
        ok "GET /pb_admin/login.php -> HTTP $STATUS"
    else
        fail "GET /pb_admin/login.php -> HTTP $STATUS"
    fi
    STATUS2="$("$SYS_CURL" -so /dev/null -w "%{http_code}" \
                    --max-time 3 https://localhost:8443/pb_admin/dashboard.php 2>/dev/null)"
    if [[ "$STATUS2" == "200" || "$STATUS2" == "302" ]]; then
        ok "GET /pb_admin/dashboard.php -> HTTP $STATUS2"
    else
        fail "GET /pb_admin/dashboard.php -> HTTP $STATUS2"
    fi
else
    warn "Port 8443 not open -- skipping HTTPS test"
fi

# ── Log tail ──────────────────────────────────────────────────────────────────
NGINX_ERR="$SERVER_DIR/logs/nginx-error.log"
if [[ -f "$NGINX_ERR" && -s "$NGINX_ERR" ]]; then
    printf "\n  ${C_GREY}Last 5 nginx errors${R}\n"
    tail -5 "$NGINX_ERR" | while IFS= read -r l; do
        printf "  ${C_YELLOW}%s${R}\n" "$l"
    done
fi

PHP_ERR="$SERVER_DIR/logs/php-fpm-pool.log"
if [[ -f "$PHP_ERR" && -s "$PHP_ERR" ]]; then
    printf "\n  ${C_GREY}Last 5 PHP-FPM errors${R}\n"
    tail -5 "$PHP_ERR" | while IFS= read -r l; do
        printf "  ${C_YELLOW}%s${R}\n" "$l"
    done
fi

printf "\n"
