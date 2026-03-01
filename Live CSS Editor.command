#!/usr/bin/env zsh
# Double-click this file in Finder to open the Live CSS Editor launcher.
# macOS: right-click -> Open the first time to bypass Gatekeeper.

DIR="$(cd "$(dirname "$0")" && pwd)"

# Ensure launcher.sh exists and is executable before handing off.
if [[ ! -f "$DIR/launcher.sh" ]]; then
    echo "ERROR: launcher.sh not found at $DIR/launcher.sh" >&2
    read -r "?Press ENTER to close..."
    exit 1
fi

if [[ ! -x "$DIR/launcher.sh" ]]; then
    echo "launcher.sh is not executable -- fixing permissions now..." >&2
    chmod +x "$DIR/launcher.sh" || {
        echo "ERROR: chmod +x failed on $DIR/launcher.sh" >&2
        read -r "?Press ENTER to close..."
        exit 1
    }
fi

# Maximize the Terminal window on macOS.
# Uses osascript to zoom the frontmost Terminal window.
# Falls back silently if osascript is unavailable or the zoom fails.
if command -v osascript &>/dev/null; then
    osascript 2>/dev/null <<'APPLESCRIPT'
tell application "Terminal"
    set zoomed of front window to true
end tell
APPLESCRIPT
    if [[ $? -ne 0 ]]; then
        echo "DEBUG: osascript window-maximize failed (non-fatal)" >&2
    fi
else
    echo "DEBUG: osascript not found -- window will not auto-maximize" >&2
fi

exec "$DIR/launcher.sh"
