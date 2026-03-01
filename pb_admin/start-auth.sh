#!/usr/bin/env bash
# pb_admin/start-auth.sh
# Starts both servers needed for the admin panel in dev mode:
#   1. xcm_auth Go API   -- http://localhost:9100  (auth backend, 2FA disabled)
#   2. PHP dev server    -- http://localhost:8080  (serves the PHP admin panel)
#
# Open in browser: http://localhost:8080/pb_admin/
#
# Usage:
#   bash pb_admin/start-auth.sh
#   ./pb_admin/start-auth.sh   (from repo root)

# Do NOT use set -e - we want to log errors instead of silently aborting

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
AUTH_DIR="$ROOT_DIR/xcm_auth"

PHP_PORT=8080
AUTH_PORT=9100

echo ""
echo "[start-auth] root: $ROOT_DIR"
echo "[start-auth] auth: $AUTH_DIR"
echo ""

# -- Preflight checks ----------------------------------------------------------

if [ ! -f "$AUTH_DIR/go.mod" ]; then
    echo "[start-auth] ERROR: xcm_auth directory not found at $AUTH_DIR"
    exit 1
fi

if ! command -v go &>/dev/null; then
    echo "[start-auth] ERROR: 'go' is not in PATH"
    exit 1
fi

if ! command -v php &>/dev/null; then
    echo "[start-auth] ERROR: 'php' is not in PATH"
    exit 1
fi

# -- Kill anything already on these ports so we always get a fresh start -------

kill_port() {
    local port="$1"
    local pids
    pids=$(lsof -iTCP:"$port" -sTCP:LISTEN -t 2>/dev/null || true)
    if [ -n "$pids" ]; then
        echo "[start-auth] killing existing process on port $port (PID $pids)"
        echo "$pids" | xargs kill 2>/dev/null || true
        sleep 1
    fi
}

kill_port "$PHP_PORT"
kill_port "$AUTH_PORT"

# -- Start xcm_auth in background ----------------------------------------------

echo "[start-auth] building and starting xcm_auth on :$AUTH_PORT ..."
cd "$AUTH_DIR"
go run ./cmd/main.go &
AUTH_PID=$!
echo "[start-auth] xcm_auth started (PID $AUTH_PID)"
cd "$ROOT_DIR"

# Give Go time to compile and bind
echo "[start-auth] waiting for xcm_auth to bind..."
for i in 1 2 3 4 5 6 7 8 9 10; do
    sleep 1
    if lsof -iTCP:"$AUTH_PORT" -sTCP:LISTEN -t &>/dev/null; then
        echo "[start-auth] xcm_auth is ready on :$AUTH_PORT"
        break
    fi
    if ! kill -0 "$AUTH_PID" 2>/dev/null; then
        echo "[start-auth] ERROR: xcm_auth process exited unexpectedly - check Go errors above"
        exit 1
    fi
    echo "[start-auth] still waiting... ($i/10)"
done

# -- Print the URLs ------------------------------------------------------------

echo ""
echo "----------------------------------------------"
echo " pb_admin dev stack"
echo "----------------------------------------------"
echo "  xcm_auth (Go)  : http://localhost:$AUTH_PORT"
echo "  admin panel    : http://localhost:$PHP_PORT/pb_admin/"
echo "  first-time     : http://localhost:$PHP_PORT/pb_admin/setup.php"
echo "  2FA            : disabled (dev mode)"
echo "  database       : xcm_auth_dev.db (SQLite)"
echo ""
echo "  First run? Open: http://localhost:$PHP_PORT/pb_admin/setup.php"
echo "  Already set up? Open: http://localhost:$PHP_PORT/pb_admin/"
echo ""
echo "  Ctrl+C to stop both servers"
echo "----------------------------------------------"
echo ""


# -- Test pass ----------------------------------------------------------------
# Verify both servers respond before printing the final URL.
# Uses curl if available, falls back to a plain TCP check.

run_tests() {
    local pass=0
    local fail=0

    echo "[test] running startup checks..."

    # Test 1: xcm_auth /health
    if command -v curl &>/dev/null; then
        local health
        health=$(curl -s --max-time 3 "http://localhost:$AUTH_PORT/health" 2>/dev/null || true)
        if echo "$health" | grep -q '"ok":true'; then
            echo "[test] PASS  xcm_auth /health responded correctly"
            pass=$((pass + 1))
        else
            echo "[test] FAIL  xcm_auth /health did not return ok:true - got: $health"
            fail=$((fail + 1))
        fi
    else
        echo "[test] SKIP  curl not found - skipping xcm_auth HTTP test"
    fi

    # Test 2: PHP server responds on the root
    if command -v curl &>/dev/null; then
        local php_code
        php_code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 3 "http://localhost:$PHP_PORT/" 2>/dev/null || true)
        if [ "$php_code" = "200" ] || [ "$php_code" = "302" ] || [ "$php_code" = "301" ]; then
            echo "[test] PASS  PHP server responded HTTP $php_code on :$PHP_PORT"
            pass=$((pass + 1))
        else
            echo "[test] FAIL  PHP server returned HTTP $php_code on :$PHP_PORT (expected 200 or 302)"
            fail=$((fail + 1))
        fi
    fi

    # Test 3: PHP proxy health check (PHP -> Go)
    if command -v curl &>/dev/null; then
        local proxy
        proxy=$(curl -s --max-time 5 "http://localhost:$PHP_PORT/pb_admin/api_proxy.php?action=health" 2>/dev/null || true)
        if echo "$proxy" | grep -q '"ok":true'; then
            echo "[test] PASS  PHP proxy can reach xcm_auth"
            pass=$((pass + 1))
        else
            echo "[test] FAIL  PHP proxy could not reach xcm_auth - got: $proxy"
            echo "[test]       Check that XCMAUTH_BASE_URL in pb_admin/config.php matches :$AUTH_PORT"
            fail=$((fail + 1))
        fi
    fi

    echo ""
    echo "[test] results: $pass passed, $fail failed"
    if [ "$fail" -gt 0 ]; then
        echo "[test] WARNING: $fail check(s) failed - the panel may not work correctly"
    else
        echo "[test] all checks passed"
    fi
    echo ""
}

# Start PHP in background temporarily so we can run tests against both servers
cd "$ROOT_DIR"
echo "[start-auth] starting PHP dev server on :$PHP_PORT (router: pb_admin/router.php) ..."
# The router script is required so PHP's built-in server does NOT fall back
# to serving root index.php for unknown paths like /dashboard.php.
php -S "localhost:$PHP_PORT" pb_admin/router.php &>/tmp/pb_admin_php.log &
PHP_PID=$!
echo "[start-auth] PHP started (PID $PHP_PID)"

# Update cleanup to also stop PHP
cleanup() {
    echo ""
    echo "[start-auth] shutting down..."
    if [ -n "$AUTH_PID" ] && kill -0 "$AUTH_PID" 2>/dev/null; then
        kill "$AUTH_PID" 2>/dev/null || true
        echo "[start-auth] xcm_auth stopped (PID $AUTH_PID)"
    fi
    if [ -n "$PHP_PID" ] && kill -0 "$PHP_PID" 2>/dev/null; then
        kill "$PHP_PID" 2>/dev/null || true
        echo "[start-auth] PHP server stopped (PID $PHP_PID)"
    fi
}
trap cleanup EXIT INT TERM

# Wait for PHP to bind
sleep 1

# Run the test pass
run_tests

# Tail PHP logs to stdout so errors stay visible
echo "[start-auth] PHP log: /tmp/pb_admin_php.log"
echo "[start-auth] press Ctrl+C to stop"
echo ""
tail -f /tmp/pb_admin_php.log

