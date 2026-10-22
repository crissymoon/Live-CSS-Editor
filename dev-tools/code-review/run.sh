#!/usr/bin/env bash
# run.sh  -  Launch the Code Review TUI
# Located at: dev-tools/code-review/run.sh

set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Export so bridge.py can resolve its own sibling scripts
export CODE_REVIEW_DIR="$DIR"

# -- Load local API keys if present (gitignored) -----------------------------
if [[ -f "$DIR/.keys" ]]; then
    # shellcheck disable=SC1090
    set -a; source "$DIR/.keys"; set +a
fi

# -- Dependency checks -------------------------------------------------------

if ! command -v python3 &>/dev/null; then
    echo "Error: python3 not found."
    exit 1
fi

if ! LOVE_BIN="$(command -v love)"; then
    echo "Error: love2d not found. Install with:"
    echo "  macOS:  brew install --cask love"
    echo "  Ubuntu: sudo apt install love"
    echo "  Arch:   sudo pacman -S love"
    exit 1
fi

# Homebrew's LÖVE cask can retain a download quarantine attribute. In a local
# development install that makes macOS block the unsigned executable even when
# it was launched from this trusted script. Remove quarantine only from the
# resolved LÖVE application bundle, never globally or from the project.
if [[ "$(uname -s)" == "Darwin" ]]; then
    LOVE_REAL_BIN="$(python3 -c 'import os, sys; print(os.path.realpath(sys.argv[1]))' "$LOVE_BIN")"
    LOVE_APP="$LOVE_REAL_BIN"
    while [[ "$LOVE_APP" != "/" && "$LOVE_APP" != *.app ]]; do
        LOVE_APP="$(dirname "$LOVE_APP")"
    done

    if [[ "$LOVE_APP" == *.app ]] && xattr -p com.apple.quarantine "$LOVE_APP" &>/dev/null; then
        echo "Removing macOS quarantine from development dependency: $LOVE_APP"
        if ! xattr -dr com.apple.quarantine "$LOVE_APP" 2>/dev/null; then
            echo "Error: macOS is blocking LÖVE and its quarantine flag could not be removed."
            echo "Run: xattr -dr com.apple.quarantine \"$LOVE_APP\""
            echo "Then allow LÖVE in System Settings > Privacy & Security if prompted."
            exit 1
        fi
    fi
fi

# -- Create reports dir if missing -------------------------------------------
mkdir -p "$DIR/reports"

# -- Launch ------------------------------------------------------------------
DEFAULT_SCAN_DIR="$(python3 "$DIR/scan_config.py" default-scan-path 2>/dev/null || (cd "$DIR/../.." && pwd))"
echo "Starting Code Review TUI..."
echo "  Scan dir default: $DEFAULT_SCAN_DIR"
echo "  Reports dir:      $DIR/reports"
echo ""

exec love "$DIR/review_tui/" "$@"
