#!/bin/bash
#
# Crissy's DB Browser - Quick Launch
# Easy launch with theme options
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DB_PATH="$SCRIPT_DIR/../xcm-editor.db"
BROWSER_BIN="$SCRIPT_DIR/build/bin/db-browser"
CURRENT_THEME="$SCRIPT_DIR/build/bin/css/theme.css"
DEFAULT_THEME="$SCRIPT_DIR/css/theme.css"
SIMPLE_THEME="$SCRIPT_DIR/css/theme-simple.css"


# Check if browser is built
if [ ! -f "$BROWSER_BIN" ]; then
    echo "[INFO] Browser not built yet. Building now..."
    cd "$SCRIPT_DIR" && ./build-with-validation.sh
    if [ $? -ne 0 ]; then
        echo "[ERROR] Build failed. Check output above." >&2
        exit 1
    fi
    echo ""
fi

# Ensure build theme dir exists
mkdir -p "$SCRIPT_DIR/build/bin/css" 2>/dev/null

echo "Crissy's DB Browser - Quick Launch"
echo ""
echo "Database: $DB_PATH"
echo ""
echo "[1] Launch with current theme"
echo "[2] Switch to Crissy theme (dark purple) and launch"
echo "[3] Switch to simple theme (light high-contrast) and launch"
echo "[4] Rebuild and launch"
echo "[5] Exit"
echo ""
echo -n "Choose option [1-5]: "
read -r choice

case $choice in
    1)
        echo "[INFO] Launching..."
        "$BROWSER_BIN" "$DB_PATH"
        if [ $? -ne 0 ]; then echo "[ERROR] Browser exited with error" >&2; fi
        ;;
    2)
        if [ ! -f "$DEFAULT_THEME" ]; then
            echo "[ERROR] Default theme not found at: $DEFAULT_THEME" >&2
            exit 1
        fi
        cp "$DEFAULT_THEME" "$CURRENT_THEME"
        if [ $? -ne 0 ]; then
            echo "[ERROR] Failed to copy theme to build dir" >&2
            exit 1
        fi
        echo "[INFO] Switched to Crissy theme"
        "$BROWSER_BIN" "$DB_PATH"
        if [ $? -ne 0 ]; then echo "[ERROR] Browser exited with error" >&2; fi
        ;;
    3)
        # Create simple (light) theme in css/ if it does not already exist
        if [ ! -f "$SIMPLE_THEME" ]; then
            echo "[INFO] Creating simple theme at $SIMPLE_THEME..."
            cat > "$SIMPLE_THEME" << 'SIMPLECSS'
/* Crissy's DB Browser - Simple Theme (light, high contrast) */

window {
    background-color: #f5f5f5;
    color: #111111;
}

window.dialog {
    border: 2px solid #4466cc;
}

menubar {
    background-color: #2c3e50;
    color: #ffffff;
    border-bottom: 1px solid #34495e;
    min-height: 32px;
}

menubar > menuitem {
    color: #ffffff;
    padding: 6px 14px;
}

menubar > menuitem:hover {
    background-color: #3498db;
}

toolbar {
    background-color: #ecf0f1;
    border-bottom: 1px solid #bdc3c7;
}

toolbar button {
    background-color: #3498db;
    color: #ffffff;
    border: 1px solid #2980b9;
    padding: 4px 12px;
    min-height: 26px;
    font-size: 11px;
}

toolbar button:hover {
    background-color: #2980b9;
}

button {
    background-color: #3498db;
    color: #ffffff;
    border: 1px solid #2980b9;
    padding: 5px 14px;
    min-height: 28px;
    font-size: 11px;
}

button:hover {
    background-color: #2980b9;
}

button:active {
    background-color: #1a6ea0;
}

button label {
    color: #ffffff;
}

label {
    color: #111111;
}

entry {
    background-color: #ffffff;
    color: #111111;
    border: 1px solid #3498db;
    padding: 6px 10px;
}

textview {
    background-color: #ffffff;
    color: #111111;
    border: 1px solid #bdc3c7;
    padding: 8px;
    font-family: monospace;
    font-size: 12px;
}

textview text {
    background-color: #ffffff;
    color: #111111;
}

treeview {
    background-color: #ffffff;
    color: #111111;
    border: 1px solid #bdc3c7;
}

treeview.view:selected {
    background-color: #3498db;
    color: #ffffff;
}

treeview.view row {
    border-bottom: 1px solid #ecf0f1;
}

treeview header button {
    background-color: #2c3e50;
    color: #ffffff;
    border-right: 1px solid #34495e;
    padding: 6px 10px;
    font-size: 11px;
    font-weight: bold;
}

notebook {
    background-color: #ecf0f1;
    border: 1px solid #bdc3c7;
}

notebook > header > tabs > tab {
    background-color: #dfe6e9;
    color: #2c3e50;
    border: 1px solid #bdc3c7;
    padding: 6px 16px;
}

notebook > header > tabs > tab:checked {
    background-color: #3498db;
    color: #ffffff;
}

statusbar {
    background-color: #2c3e50;
    color: #bdc3c7;
    border-top: 1px solid #34495e;
    padding: 3px 10px;
    font-size: 11px;
}

statusbar label {
    color: #bdc3c7;
}

dialog {
    background-color: #ffffff;
    color: #111111;
    border: 2px solid #3498db;
}

messagedialog {
    background-color: #ffffff;
    color: #111111;
}

messagedialog label {
    color: #111111;
    font-size: 13px;
}

filechooser {
    background-color: #ffffff;
    border: 2px solid #3498db;
    min-width: 800px;
    min-height: 560px;
}

filechooser treeview {
    min-height: 380px;
}
SIMPLECSS
            if [ $? -ne 0 ]; then
                echo "[ERROR] Failed to write simple theme" >&2
                exit 1
            fi
            echo "[INFO] Simple theme written to $SIMPLE_THEME"
        fi
        cp "$SIMPLE_THEME" "$CURRENT_THEME"
        if [ $? -ne 0 ]; then
            echo "[ERROR] Failed to copy simple theme to build dir" >&2
            exit 1
        fi
        echo "[INFO] Switched to simple theme"
        "$BROWSER_BIN" "$DB_PATH"
        if [ $? -ne 0 ]; then echo "[ERROR] Browser exited with error" >&2; fi
        ;;
    4)
        echo "[INFO] Rebuilding..."
        cd "$SCRIPT_DIR" && ./build-with-validation.sh
        if [ $? -ne 0 ]; then
            echo "[ERROR] Build failed" >&2
            exit 1
        fi
        echo "[INFO] Launching..."
        "$BROWSER_BIN" "$DB_PATH"
        if [ $? -ne 0 ]; then echo "[ERROR] Browser exited with error" >&2; fi
        ;;
    5)
        echo "Bye."
        exit 0
        ;;
    *)
        echo "[ERROR] Invalid option: $choice" >&2
        exit 1
        ;;
esac
