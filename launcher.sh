#!/usr/bin/env zsh
# ============================================================
#  Live CSS Editor — Launcher
#  Works as: bash launcher.sh  OR  double-click launcher.command
# ============================================================

# Change to the directory this script lives in
SCRIPT_DIR="${0:a:h}"
cd "$SCRIPT_DIR" || exit 1

# ── ANSI palette (dark purple theme) ────────────────────────
R='\033[0m'         # reset
BOLD='\033[1m'
DIM='\033[2m'

C_PURPLE='\033[38;5;99m'
C_VIOLET='\033[38;5;141m'
C_LAVENDER='\033[38;5;189m'
C_DARK='\033[38;5;54m'
C_GREEN='\033[38;5;114m'
C_RED='\033[38;5;203m'
C_YELLOW='\033[38;5;222m'
C_GREY='\033[38;5;244m'
C_WHITE='\033[38;5;255m'

BG_DARK='\033[48;5;17m'
BG_BAR='\033[48;5;55m'

# ── Helpers ──────────────────────────────────────────────────
line() {
    printf "${C_DARK}%s${R}\n" \
        "------------------------------------------------------------"
}

status_ok()   { printf "  ${C_GREEN}[   OK  ]${R}  %s\n" "$1"; }
status_miss() { printf "  ${C_RED}[MISSING]${R}  %s\n"   "$1"; }
status_warn() { printf "  ${C_YELLOW}[  WARN ]${R}  %s\n" "$1"; }
status_info() { printf "  ${C_GREY}[  ---  ]${R}  %s\n"  "$1"; }

step() {
    printf "\n${C_VIOLET}${BOLD}>>>${R} ${C_LAVENDER}%s${R}\n" "$*"
}

die() {
    printf "\n${C_RED}${BOLD}FATAL:${R} %s\n\n" "$*" >&2
    read -r "?Press ENTER to close..."
    exit 1
}

# ── Banner ───────────────────────────────────────────────────
clear
printf "${C_PURPLE}${BOLD}"
cat << 'BANNER'
  _     _              ___  ___  ___  ___  _ _ _
 | |   (_)_  _____    / __\/ __\/ __|/ _ \| | | |
 | |__ | \ \/ / -_)  | (__ \__ \\_  | |_| |_|_|_|
 |____||_|_\_\\___|  \___||___/____|____/
                            E  D  I  T  O  R
BANNER
printf "${R}"
printf "${C_DARK}                       Native App Launcher${R}\n"
line
printf "  ${C_GREY}Project:${R}  %s\n" "$SCRIPT_DIR"
printf "  ${C_GREY}Date   :${R}  %s\n" "$(date '+%Y-%m-%d %H:%M:%S')"
line

# ── Dependency check ─────────────────────────────────────────
printf "\n${C_VIOLET}${BOLD}Checking dependencies...${R}\n\n"

MISSING=0

# PHP
if command -v php &>/dev/null; then
    PHP_VER=$(php -r 'echo PHP_MAJOR_VERSION.".".PHP_MINOR_VERSION;' 2>/dev/null)
    status_ok "PHP $PHP_VER  ($(command -v php))"
else
    status_miss "PHP not found — install PHP 8+ and add to PATH"
    MISSING=$((MISSING + 1))
fi

# Node
if command -v node &>/dev/null; then
    NODE_VER=$(node -e 'process.stdout.write(process.version)' 2>/dev/null)
    status_ok "Node.js $NODE_VER  ($(command -v node))"
else
    status_miss "Node.js not found — install from https://nodejs.org"
    MISSING=$((MISSING + 1))
fi

# npm
if command -v npm &>/dev/null; then
    NPM_VER=$(npm -v 2>/dev/null)
    status_ok "npm $NPM_VER"
else
    status_miss "npm not found"
    MISSING=$((MISSING + 1))
fi

# Cargo / Rust
if command -v cargo &>/dev/null; then
    CARGO_VER=$(cargo --version 2>/dev/null | awk '{print $2}')
    status_ok "Rust / cargo $CARGO_VER  ($(command -v cargo))"
else
    status_miss "Rust / cargo not found — install from https://rustup.rs"
    MISSING=$((MISSING + 1))
fi

# Tauri CLI (installed via npm)
if [[ -f "node_modules/.bin/tauri" ]]; then
    TAURI_VER=$(node_modules/.bin/tauri --version 2>/dev/null | awk '{print $NF}')
    status_ok "Tauri CLI $TAURI_VER  (local node_modules)"
elif command -v tauri &>/dev/null; then
    TAURI_VER=$(tauri --version 2>/dev/null | awk '{print $NF}')
    status_ok "Tauri CLI $TAURI_VER  (global)"
else
    status_warn "Tauri CLI not in node_modules — run 'npm install' first"
fi

# node_modules present?
if [[ -d "node_modules" ]]; then
    status_ok "node_modules present"
else
    status_warn "node_modules missing — will run npm install automatically"
fi

# www populated?
if [[ -d "src-tauri/www" && -f "src-tauri/www/index.php" ]]; then
    WWW_COUNT=$(find src-tauri/www -type f | wc -l | tr -d ' ')
    status_ok "src-tauri/www/ populated  ($WWW_COUNT files)"
else
    status_warn "src-tauri/www/ not populated — will run copy-www automatically"
fi

# icon present?
if [[ -f "src-tauri/icons/icon.png" ]]; then
    status_ok "App icon present"
else
    status_warn "App icon missing — will generate automatically"
fi

line

# ── Hard-fail if required deps absent ────────────────────────
if [[ $MISSING -gt 0 ]]; then
    printf "\n${C_RED}${BOLD}%d required dependencies are missing.${R}\n" "$MISSING"
    printf "${C_GREY}Resolve the issues above, then re-run the launcher.${R}\n\n"
    read -r "?Press ENTER to close..."
    exit 1
fi

# ── Ensure node_modules installed ────────────────────────────
if [[ ! -d "node_modules" ]]; then
    step "Installing npm packages..."
    npm install || die "npm install failed"
fi

# ── Menu ─────────────────────────────────────────────────────
printf "\n${C_VIOLET}${BOLD}Select an action:${R}\n\n"
printf "  ${C_LAVENDER}${BOLD}1${R}  ${C_WHITE}Dev${R}        ${C_GREY}Start the app in development mode (hot-reload, DevTools)${R}\n"
printf "  ${C_LAVENDER}${BOLD}2${R}  ${C_WHITE}Build${R}      ${C_GREY}Compile a release bundle  (.app / .dmg on macOS)${R}\n"
printf "  ${C_LAVENDER}${BOLD}3${R}  ${C_WHITE}PHP only${R}   ${C_GREY}Run PHP dev server at localhost:8080  (no Tauri window)${R}\n"
printf "  ${C_LAVENDER}${BOLD}4${R}  ${C_WHITE}Refresh${R}    ${C_GREY}Re-copy www/ and regenerate icon${R}\n"
printf "  ${C_LAVENDER}${BOLD}5${R}  ${C_WHITE}Clean${R}      ${C_GREY}Remove Rust build artifacts  (src-tauri/target/)${R}\n"
printf "  ${C_LAVENDER}${BOLD}6${R}  ${C_WHITE}Push${R}       ${C_GREY}Commit and push all changes to GitHub${R}\n"
printf "  ${C_LAVENDER}${BOLD}q${R}  ${C_GREY}Quit${R}\n"
printf "\n"
printf "${C_GREY}Choice [1-6 / q]:${R} "
read -r CHOICE

_ensure_assets() {
    if [[ ! -f "src-tauri/icons/icon.png" ]]; then
        step "Generating app icon..."
        node scripts/gen-icon.js || die "Icon generation failed"
    fi
    step "Copying app files to src-tauri/www/..."
    node scripts/copy-www.js || die "copy-www failed"
}

case "$CHOICE" in

    1)
        printf "\n"
        line
        step "Starting Live CSS Editor in DEV mode..."
        line
        _ensure_assets
        printf "\n${C_GREEN}Starting tauri dev...${R}\n\n"
        npm run copy-www && npx tauri dev
        ;;

    2)
        printf "\n"
        line
        step "Building release bundle..."
        line
        _ensure_assets
        printf "\n${C_GREEN}Running tauri build...${R}\n\n"
        npm run copy-www && npx tauri build
        printf "\n"
        line
        printf "\n${C_GREEN}Build complete.${R}\n"
        DIST_DIR="src-tauri/target/release/bundle"
        if [[ -d "$DIST_DIR" ]]; then
            printf "${C_GREY}Artifacts:${R}\n"
            find "$DIST_DIR" -maxdepth 2 -type f \( -name "*.dmg" -o -name "*.app" -o -name "*.deb" -o -name "*.rpm" -o -name "*.exe" -o -name "*.msi" \) \
                | while read -r f; do
                    SIZE=$(du -sh "$f" 2>/dev/null | awk '{print $1}')
                    printf "  ${C_LAVENDER}%s${R}  ${C_GREY}(%s)${R}\n" "$f" "$SIZE"
                done
        fi
        printf "\n"
        read -r "?Press ENTER to close..."
        ;;

    3)
        printf "\n"
        line
        step "Starting PHP dev server at http://localhost:8080 ..."
        line
        printf "${C_YELLOW}Press Ctrl+C to stop.${R}\n\n"
        php -S localhost:8080
        ;;

    4)
        printf "\n"
        line
        step "Refreshing assets..."
        line
        step "Regenerating icon..."
        node scripts/gen-icon.js
        step "Copying www files..."
        node scripts/copy-www.js
        printf "\n${C_GREEN}Done.${R}\n"
        printf "${C_GREY}www files: %s${R}\n" \
            "$(find src-tauri/www -type f | wc -l | tr -d ' ') files in src-tauri/www/"
        printf "\n"
        read -r "?Press ENTER to return..."
        exec "$0"
        ;;

    5)
        printf "\n"
        line
        step "Cleaning Rust build artifacts..."
        line
        if [[ -d "src-tauri/target" ]]; then
            SIZE=$(du -sh src-tauri/target 2>/dev/null | awk '{print $1}')
            printf "  ${C_GREY}Removing src-tauri/target/  (%s)...${R}\n" "$SIZE"
            rm -rf src-tauri/target
            printf "  ${C_GREEN}Done.${R}\n"
        else
            printf "  ${C_GREY}src-tauri/target/ not present — nothing to clean.${R}\n"
        fi
        printf "\n"
        read -r "?Press ENTER to return..."
        exec "$0"
        ;;

    6)
        printf "\n"
        line
        step "Pushing to GitHub..."
        line
        printf "\n"
        bash push.sh
        printf "\n"
        read -r "?Press ENTER to return..."
        exec "$0"
        ;;

    q|Q|"")
        printf "\n${C_GREY}Exiting.${R}\n\n"
        exit 0
        ;;

    *)
        printf "\n${C_YELLOW}Unknown choice. Exiting.${R}\n\n"
        exit 0
        ;;
esac
