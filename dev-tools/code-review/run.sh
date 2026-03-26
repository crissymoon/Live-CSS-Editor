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
DEFAULT_SCAN_DIR="$(python3 "$DIR/scan_config.py" default-scan-path 2>/dev/null || (cd "$DIR/../.." && pwd))"
echo "Starting Code Review TUI..."
echo "  Scan dir default: $DEFAULT_SCAN_DIR"
echo "  Reports dir:      $DIR/reports"
echo ""

exec love "$DIR/review_tui/" "$@"
