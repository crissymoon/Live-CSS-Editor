#!/usr/bin/env bash
# page-builder/pb_admin/start-xcm-auth-only.sh
# 
# Standalone starter for the Go auth server (xcm_auth).
# Use this if the full start-auth.sh is failing or you need just the auth server.
#
# Usage:
#   bash page-builder/pb_admin/start-xcm-auth-only.sh
#   ./page-builder/pb_admin/start-xcm-auth-only.sh   (from repo root)
#
# Opens on port 9100 at http://localhost:9100/health

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
AUTH_DIR="$ROOT_DIR/xcm_auth"
AUTH_PORT=9100

# Colors
C_GREEN='\033[38;5;114m'
C_RED='\033[38;5;203m'
C_GREY='\033[38;5;244m'
C_YELLOW='\033[38;5;222m'
R='\033[0m'

log_info()  { printf "${C_GREY}[xcm_auth]${R}  %s\n" "$1"; }
log_ok()    { printf "${C_GREEN}[  OK  ]${R}  %s\n" "$1"; }
log_err()   { printf "${C_RED}[FAIL]${R}  %s\n" "$1" >&2; }
die()       { log_err "$1"; exit 1; }

echo ""
log_info "Starting Go auth server (xcm_auth)"
echo ""

# Check dependencies
log_info "Checking dependencies..."

if [[ ! -d "$AUTH_DIR" ]]; then
    die "xcm_auth directory not found: $AUTH_DIR"
fi

if ! command -v go &>/dev/null; then
    die "Go is not installed. Install with: brew install go"
fi

log_ok "Go version: $(go version | awk '{print $3}')"

# Check if port is available
if lsof -i ":$AUTH_PORT" -sTCP:LISTEN &>/dev/null; then
    log_info "Port $AUTH_PORT is already in use. Killing existing process..."
    lsof -i ":$AUTH_PORT" -sTCP:LISTEN -t | xargs kill -9 2>/dev/null || true
    sleep 0.5
fi

# Change to auth directory
cd "$AUTH_DIR" || die "Cannot cd to $AUTH_DIR"

# Download dependencies if needed
log_info "Checking Go dependencies..."
go mod download 2>/dev/null || log_info "Dependencies already downloaded"

# Build/run the auth server
log_info "Building and starting auth server..."
log_info "Port: $AUTH_PORT"
log_info "Database: $AUTH_DIR/xcm_auth_dev.db"
log_info "Config: $AUTH_DIR/.env"
echo ""

# Use 'go run' for dev (automatically rebuilds if source changes)
# The server reads SERVER_ADDR from .env file (default: :9100)
go run ./cmd/main.go

# If we get here, the server stopped
echo ""
log_err "Auth server stopped"
