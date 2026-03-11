#!/usr/bin/env bash
# run.sh  -  Launch the Code Review TUI
# Located at: dev-tools/code-review/run.sh

set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Export so bridge.py can resolve its own sibling scripts
export CODE_REVIEW_DIR="$DIR"

# -- Dependency checks -------------------------------------------------------

if ! command -v love &>/dev/null; then
    echo "Error: love2d not found.  Install with:"
    echo "  macOS:  brew install love"
    echo "  Ubuntu: sudo apt install love"
    echo "  Arch:   sudo pacman -S love"
    exit 1
fi

if ! command -v python3 &>/dev/null; then
    echo "Error: python3 not found."
    exit 1
fi

# -- Create reports dir if missing -------------------------------------------
mkdir -p "$DIR/reports"

# -- Launch ------------------------------------------------------------------
echo "Starting Code Review TUI..."
echo "  Scan dir default: $HOME"
echo "  Reports dir:      $DIR/reports"
echo ""

exec love "$DIR/review_tui/" "$@"
