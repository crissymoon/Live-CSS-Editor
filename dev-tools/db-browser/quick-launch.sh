#!/bin/bash
#
# Crissy's DB Browser - Quick Launch
# Builds if needed, then launches directly.
# Theme is chosen inside the app and remembered automatically.
#
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DB_PATH="$SCRIPT_DIR/../xcm-editor.db"
BROWSER_BIN="$SCRIPT_DIR/build/bin/db-browser"
CSS_SRC="$SCRIPT_DIR/css"
CSS_DST="$SCRIPT_DIR/build/bin/css"
# Build if the binary is missing
if [ ! -f "$BROWSER_BIN" ]; then
    echo "[INFO] Browser not built yet. Building now..."
    cd "$SCRIPT_DIR" && ./build-with-validation.sh
    if [ $? -ne 0 ]; then
        echo "[ERROR] Build failed. Check output above." >&2
        exit 1
    fi
    echo ""
fi
# Keep both theme files in sync to the build output directory
mkdir -p "$CSS_DST" 2>/dev/null
for f in theme.css theme-simple.css; do
    if [ -f "$CSS_SRC/$f" ]; then
        cp "$CSS_SRC/$f" "$CSS_DST/$f"
    fi
done
# Launch - the app loads whichever theme was last used automatically
"$BROWSER_BIN" "$DB_PATH"
if [ $? -ne 0 ]; then
    echo "[ERROR] Browser exited with error" >&2
    exit 1
fi

