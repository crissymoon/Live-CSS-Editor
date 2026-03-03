#!/bin/bash
# vscode-bridge/update-live-css.sh
#
# Single script that pushes all vscode-bridge changes into the running
# Live CSS tool so the user can load the updates immediately.
#
# What it does:
#   1. Copies custom_design_assets into src-tauri/www/ (image files)
#   2. Copies updated bridge PHP + JS into src-tauri/www/vscode-bridge/
#   3. Copies updated app files (index.php, js/app.js, js/editor.js) into src-tauri/www/
#   4. Pushes the project editor files into the SQLite database
#
# Usage:
#   ./update-live-css.sh                     (defaults to project name "crissys-style-tool")
#   ./update-live-css.sh my-project-name

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
WWW_DIR="$ROOT_DIR/src-tauri/www"
PROJECT_NAME="${1:-crissys-style-tool}"

err_count=0

log()  { echo "[update] $*"; }
warn() { echo "[update] WARN: $*" >&2; }
fail() { echo "[update] ERROR: $*" >&2; err_count=$((err_count + 1)); }

# -------------------------------------------------------------------
# 1. Copy custom_design_assets into www/
# -------------------------------------------------------------------
log "Step 1: Copying custom_design_assets to www/"
# Assets live in vscode-bridge/projects/custom_design_assets -- that is the
# single source of truth. Fall back to the root-level copy if present.
SRC_ASSETS="$SCRIPT_DIR/projects/custom_design_assets"
if [ ! -d "$SRC_ASSETS" ]; then
    SRC_ASSETS="$ROOT_DIR/custom_design_assets"
fi
DST_ASSETS="$WWW_DIR/custom_design_assets"

if [ -d "$SRC_ASSETS" ]; then
    rm -rf "$DST_ASSETS"
    cp -R "$SRC_ASSETS" "$DST_ASSETS"
    log "  Source: $SRC_ASSETS"
    log "  Copied $(find "$DST_ASSETS" -type f | wc -l | tr -d ' ') asset files"
else
    fail "custom_design_assets not found at $SRC_ASSETS"
fi

# -------------------------------------------------------------------
# 2. Copy bridge PHP + JS into www/vscode-bridge/
# -------------------------------------------------------------------
log "Step 2: Syncing vscode-bridge api + js to www/"

# API files
mkdir -p "$WWW_DIR/vscode-bridge/api"
for f in bridge.php projects.php projects-cli.php pull-from-vscode.php; do
    src="$SCRIPT_DIR/api/$f"
    dst="$WWW_DIR/vscode-bridge/api/$f"
    if [ -f "$src" ]; then
        cp "$src" "$dst"
        log "  Copied api/$f"
    else
        fail "Missing api/$f"
    fi
done

# JS files
mkdir -p "$WWW_DIR/vscode-bridge/js"
for f in bridge-sync.js; do
    src="$SCRIPT_DIR/js/$f"
    dst="$WWW_DIR/vscode-bridge/js/$f"
    if [ -f "$src" ]; then
        cp "$src" "$dst"
        log "  Copied js/$f"
    else
        fail "Missing js/$f"
    fi
done

# -------------------------------------------------------------------
# 3. Copy updated app files into www/
# -------------------------------------------------------------------
log "Step 3: Syncing app files to www/"

# index.php
if [ -f "$ROOT_DIR/index.php" ]; then
    cp "$ROOT_DIR/index.php" "$WWW_DIR/index.php"
    log "  Copied index.php"
else
    fail "Missing index.php"
fi

# js/ directory (app.js, editor.js, etc.)
if [ -d "$ROOT_DIR/js" ]; then
    rm -rf "$WWW_DIR/js"
    cp -R "$ROOT_DIR/js" "$WWW_DIR/js"
    log "  Copied js/ ($(find "$WWW_DIR/js" -type f | wc -l | tr -d ' ') files)"
else
    fail "Missing js/ directory"
fi

# -------------------------------------------------------------------
# 4. Push project to SQLite database
# -------------------------------------------------------------------
log "Step 4: Pushing project \"$PROJECT_NAME\" to database"

PUSH_SCRIPT="$SCRIPT_DIR/push-project.sh"
if [ -x "$PUSH_SCRIPT" ]; then
    "$PUSH_SCRIPT" "$PROJECT_NAME"
else
    fail "push-project.sh not found or not executable"
fi

# -------------------------------------------------------------------
# Done
# -------------------------------------------------------------------
echo ""
if [ "$err_count" -gt 0 ]; then
    warn "Finished with $err_count error(s). Check output above."
    exit 1
else
    log "All done. Reload the Live CSS tool and load project: $PROJECT_NAME"
fi
