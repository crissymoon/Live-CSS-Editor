#!/usr/bin/env bash
# server/setup.sh
# One-time setup: install nginx via Homebrew, wire up our PHP-FPM pool,
# apply our nginx config, and register both services so they start at login.
#
# Run once:  bash /Users/mac/Documents/live-css/server/setup.sh
# After this just use start.sh / stop.sh, or let launchd handle it.

set -euo pipefail

SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SERVER_DIR/.." && pwd)"
BREW_PREFIX="$(brew --prefix)"

NGINX_CONF="$SERVER_DIR/conf/nginx.conf"
NGINX_TPL="$SERVER_DIR/conf/nginx.conf.tpl"
POOL_CONF="$SERVER_DIR/conf/php-fpm-pool.conf"
POOL_TPL="$SERVER_DIR/conf/php-fpm-pool.conf.tpl"
POOL_LINK="$BREW_PREFIX/etc/php/8.4/php-fpm.d/livecss.conf"
CERTS_DIR="$SERVER_DIR/certs"

echo "==> Live CSS Editor -- server setup"
echo "    Project : $PROJECT_DIR"
echo "    Brew    : $BREW_PREFIX"
echo ""

# ── 1. Install nginx if missing ───────────────────────────────────────────────
if ! command -v nginx &>/dev/null; then
    echo "==> Installing nginx..."
    brew install nginx
else
    echo "==> nginx already installed: $(nginx -v 2>&1)"
fi

# ── 2. Verify PHP 8.4 + PHP-FPM ──────────────────────────────────────────────
if ! command -v php-fpm &>/dev/null; then
    echo "==> PHP-FPM not found -- installing php..."
    brew install php
else
    echo "==> PHP-FPM already installed: $(php-fpm --version 2>&1 | head -1)"
fi

# ── 3. Install mkcert and generate localhost TLS certificate ─────────────────
echo ""
echo "==> Setting up mkcert (locally-trusted TLS)..."
if ! command -v mkcert &>/dev/null; then
    echo "    Installing mkcert via brew..."
    brew install mkcert nss
else
    echo "    mkcert already installed: $(mkcert --version 2>&1)"
fi

# Install local CA into macOS system keychain (idempotent)
mkcert -install
echo "    Local CA installed in system keychain"

mkdir -p "$CERTS_DIR"

CERT_FILE="$CERTS_DIR/localhost.pem"
KEY_FILE="$CERTS_DIR/localhost-key.pem"
if [[ -f "$CERT_FILE" && -f "$KEY_FILE" ]]; then
    echo "    Certificates already exist -- delete certs/*.pem to regenerate"
else
    echo "    Generating certificate for: localhost 127.0.0.1 ::1"
    mkcert \
        -cert-file "$CERT_FILE" \
        -key-file  "$KEY_FILE" \
        localhost 127.0.0.1 ::1
    echo "    Certificate : $CERT_FILE"
    echo "    Key         : $KEY_FILE"
fi

# ── 4. Generate nginx.conf and php-fpm-pool.conf from templates ───────────────
echo ""
echo "==> Generating configuration files from templates..."
RUN_USER="$(id -un)"
RUN_GROUP="$(id -gn)"

_gen_conf() {
    local tpl="$1" out="$2"
    sed \
        -e "s|@@PROJECT_DIR@@|$PROJECT_DIR|g" \
        -e "s|@@SERVER_DIR@@|$SERVER_DIR|g" \
        -e "s|@@RUN_USER@@|$RUN_USER|g" \
        -e "s|@@RUN_GROUP@@|$RUN_GROUP|g" \
        "$tpl" > "$out"
    echo "    Generated : $out"
}

_gen_conf "$NGINX_TPL" "$NGINX_CONF"
_gen_conf "$POOL_TPL"  "$POOL_CONF"

# ── 5. Symlink generated PHP-FPM pool config ──────────────────────────────────
echo ""
echo "==> Symlinking PHP-FPM pool config..."
if [[ -L "$POOL_LINK" ]]; then
    echo "    Already linked: $POOL_LINK"
elif [[ -f "$POOL_LINK" ]]; then
    echo "    WARNING: $POOL_LINK is a regular file (not our symlink) -- skipping."
    echo "    Remove it manually and re-run if you want our pool to take effect."
else
    ln -s "$POOL_CONF" "$POOL_LINK"
    echo "    Linked: $POOL_LINK -> $POOL_CONF"
fi

# ── 6. Test generated nginx config ───────────────────────────────────────────
echo ""
echo "==> Testing nginx configuration..."
nginx -c "$NGINX_CONF" -t
echo "    OK"

# ── 7. Register brew services (launchd -- starts at login, no root needed) ───
echo ""
echo "==> Registering brew services for always-on startup..."

# PHP-FPM service
brew services start php 2>/dev/null || brew services restart php
echo "    php (php-fpm) service registered"

# nginx -- brew services uses its own plist which always points to
# /usr/local/etc/nginx/nginx.conf.  We override that with a symlink
# so brew services picks up our config on restart.
BREW_NGINX_CONF="$BREW_PREFIX/etc/nginx/nginx.conf"
if [[ ! -L "$BREW_NGINX_CONF" ]]; then
    # Back up the default brew nginx.conf
    cp "$BREW_NGINX_CONF" "${BREW_NGINX_CONF}.brew-default.bak"
    echo "    Backed up default nginx.conf to ${BREW_NGINX_CONF}.brew-default.bak"
    # Replace with symlink to ours
    ln -sf "$NGINX_CONF" "$BREW_NGINX_CONF"
    echo "    Linked: $BREW_NGINX_CONF -> $NGINX_CONF"
else
    echo "    $BREW_NGINX_CONF is already a symlink (pointing to $(readlink "$BREW_NGINX_CONF"))"
fi

brew services start nginx 2>/dev/null || brew services restart nginx
echo "    nginx service registered"

# ── 8. Wait for ports and report ─────────────────────────────────────────────
echo ""
echo "==> Waiting for services to come up..."
for i in {1..20}; do
    nc -z 127.0.0.1 8443 2>/dev/null && break
    sleep 0.5
done

nc -z 127.0.0.1 8443 2>/dev/null \
    && echo "    Port 8443 : LISTENING (HTTPS)" \
    || echo "    Port 8443 : NOT responding (check logs)"

echo ""
echo "==> Setup complete."
echo "    URL         : https://localhost:8443/page-builder/pb_admin/dashboard.php"
echo "    Cert dir    : $CERTS_DIR"
echo "    Nginx log   : $SERVER_DIR/logs/nginx-error.log"
echo "    PHP-FPM log : $SERVER_DIR/logs/php-fpm-pool.log"
echo "    Manage      : brew services list | grep -E 'nginx|php'"
echo "    Stop all    : bash $SERVER_DIR/stop.sh"
echo "    Start all   : bash $SERVER_DIR/start.sh"
