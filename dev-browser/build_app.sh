#!/usr/bin/env zsh
# build_app.sh -- build CrissyBrowser.app with a bundled PHP binary
#
# Usage:
#   cd dev-browser
#   ./build_app.sh
#
# Output:
#   dev-browser/dist/CrissyBrowser.app    -- macOS .app bundle
#   dev-browser/dist/CrissyBrowser/       -- single-dir build (same content)
#
# Requirements:
#   - Python 3 with PyQt6, PyQt6-WebEngine, pyobjc-* installed in the venv
#   - PHP installed (Homebrew: brew install php)
#   - pyinstaller (installed automatically if missing)

set -e

DIR="${0:a:h}"
cd "$DIR"

# Colours
R='\033[0m'
C_GREEN='\033[38;5;114m'
C_RED='\033[38;5;203m'
C_YELLOW='\033[38;5;222m'
C_GREY='\033[38;5;244m'
C_VIOLET='\033[38;5;141m'

step()  { printf "\n  ${C_VIOLET}>>> ${R}%s\n" "$*"; }
ok()    { printf "  ${C_GREEN}[ OK ]${R}  %s\n" "$*"; }
warn()  { printf "  ${C_YELLOW}[WARN]${R}  %s\n" "$*"; }
die()   { printf "  ${C_RED}[FAIL]${R}  %s\n" "$*" >&2; exit 1; }

# ── 1. Locate PHP ──────────────────────────────────────────────────────────────
step "Locating PHP binary..."
PHP_BIN=""
for _candidate in \
    "$(command -v php 2>/dev/null)" \
    /opt/homebrew/bin/php \
    /usr/local/bin/php; do
    if [[ -n "$_candidate" && -f "$_candidate" ]]; then
        PHP_BIN="$_candidate"
        break
    fi
done

if [[ -z "$PHP_BIN" ]]; then
    die "PHP not found. Install with: brew install php"
fi
ok "Found PHP at $PHP_BIN  ($(php --version 2>/dev/null | head -1))"

# ── 2. Copy PHP into bin/ ──────────────────────────────────────────────────────
step "Copying PHP binary to bin/php..."
mkdir -p "$DIR/bin"
cp -f "$PHP_BIN" "$DIR/bin/php"
chmod +x "$DIR/bin/php"
ok "bin/php ready"

# ── 3. Activate venv ──────────────────────────────────────────────────────────
step "Activating Python venv..."
VENV="$DIR/venv"
if [[ ! -f "$VENV/bin/activate" ]]; then
    die "venv not found at $VENV. Run: python3 -m venv venv && pip install -r requirements.txt"
fi
source "$VENV/bin/activate"
ok "venv active: $(python3 --version)"

# ── 4. Ensure PyInstaller is installed ────────────────────────────────────────
step "Checking PyInstaller..."
if ! python3 -c "import PyInstaller" 2>/dev/null; then
    warn "PyInstaller not found -- installing..."
    pip install pyinstaller
fi
ok "PyInstaller $(python3 -c 'import PyInstaller; print(PyInstaller.__version__)')"

# ── 5. Ensure pyobjc is installed (needed for WKWebView) ─────────────────────
step "Checking pyobjc..."
if ! python3 -c "import objc" 2>/dev/null; then
    warn "pyobjc not found -- installing..."
    pip install pyobjc-core pyobjc-framework-Cocoa pyobjc-framework-WebKit pyobjc-framework-Quartz pyobjc-framework-AppKit
fi
ok "pyobjc present"

# ── 6. Clean previous build ───────────────────────────────────────────────────
step "Cleaning previous build..."
rm -rf "$DIR/build" "$DIR/dist"
ok "build/ and dist/ cleared"

# ── 7. Run PyInstaller ────────────────────────────────────────────────────────
step "Running PyInstaller..."
python3 -m PyInstaller \
    --noconfirm \
    --clean \
    "$DIR/dev-browser.spec"

# ── 8. Report ─────────────────────────────────────────────────────────────────
APP="$DIR/dist/CrissyBrowser.app"
if [[ -d "$APP" ]]; then
    APP_SIZE="$(du -sh "$APP" 2>/dev/null | awk '{print $1}')"
    printf "\n  ${C_GREEN}Build complete.${R}\n"
    printf "  ${C_GREY}Location :${R}  %s\n" "$APP"
    printf "  ${C_GREY}Size     :${R}  %s\n" "$APP_SIZE"
    printf "\n  ${C_GREY}To open  :${R}  open %s\n" "$APP"
    printf "  ${C_GREY}          ${R}  Double-click CrissyBrowser.app in Finder\n\n"
else
    die "Build failed -- no .app found in dist/"
fi
