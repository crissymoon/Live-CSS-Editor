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
if [[ -f "$SCRIPT_DIR/xcm-moon-ascii.txt" ]]; then
    cat "$SCRIPT_DIR/xcm-moon-ascii.txt"
fi
printf "${R}"
printf "${C_VIOLET}${BOLD}                       Live CSS Editor${R}\n"
printf "${C_GREY}                       Native App Launcher${R}\n"
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
printf "  ${C_LAVENDER}${BOLD}6${R}  ${C_WHITE}Push${R}       ${C_GREY}Commit and push Live CSS Editor to GitHub${R}\n"
printf "  ${C_LAVENDER}${BOLD}7${R}  ${C_WHITE}Push moon${R}  ${C_GREY}Commit and push moon-lang changes to xcm-editor repo${R}\n"
printf "  ${C_LAVENDER}${BOLD}8${R}  ${C_WHITE}Push gram${R}  ${C_GREY}Commit and push gram-model (gramcheck) changes${R}\n"
printf "  ${C_LAVENDER}${BOLD}t${R}  ${C_WHITE}Tools${R}      ${C_GREY}Run lines_count.py / security_ck.py${R}\n"
printf "  ${C_LAVENDER}${BOLD}q${R}  ${C_GREY}Quit${R}\n"
printf "\n"
printf "${C_GREY}Choice [1-8 / t / q]:${R} "
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
        step "Pushing Live CSS Editor to GitHub..."
        line
        printf "\n"
        bash push.sh || { printf "${C_RED}push.sh failed - check output above${R}\n" >&2; }
        printf "\n"
        read -r "?Press ENTER to return..."
        exec "$0"
        ;;

    7)
        printf "\n"
        line
        step "Pushing moon-lang to xcm-editor repo..."
        line
        MOON_DIR="/Users/mac/Desktop/xcm-editor"
        if [[ ! -d "$MOON_DIR/.git" ]]; then
            printf "  ${C_RED}ERROR: $MOON_DIR is not a git repository${R}\n" >&2
            read -r "?Press ENTER to return..."
            exec "$0"
        fi
        printf "  ${C_GREY}Working directory: $MOON_DIR${R}\n"
        cd "$MOON_DIR" || { printf "  ${C_RED}ERROR: cannot cd to $MOON_DIR${R}\n" >&2; exec "$SCRIPT_DIR/launcher.sh"; }
        printf "  ${C_GREY}Staging moon-lang source files...${R}\n"
        if ! git add moon-lang/ 2>&1; then
            printf "  ${C_RED}ERROR: git add failed${R}\n" >&2
            cd "$SCRIPT_DIR"
            read -r "?Press ENTER to return..."
            exec "$SCRIPT_DIR/launcher.sh"
        fi
        if git diff --cached --quiet; then
            printf "  ${C_YELLOW}Nothing new to commit in moon-lang.${R}\n"
        else
            printf "  ${C_GREY}Files staged:${R}\n"
            git diff --cached --name-only | while read -r f; do printf "    ${C_LAVENDER}%s${R}\n" "$f"; done
            printf "\n  ${C_GREY}Commit message (leave blank for default):${R} "
            read -r MOON_MSG
            MOON_MSG="${MOON_MSG:-Update moon-lang}"
            git commit -m "$MOON_MSG" || { printf "  ${C_RED}ERROR: git commit failed${R}\n" >&2; }
        fi
        MOON_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo main)"
        printf "  ${C_GREY}Pushing branch %s to origin...${R}\n" "$MOON_BRANCH"
        git push origin "$MOON_BRANCH" 2>&1 || printf "  ${C_RED}ERROR: git push failed - check credentials and network${R}\n" >&2
        cd "$SCRIPT_DIR"
        printf "\n"
        read -r "?Press ENTER to return..."
        exec "$0"
        ;;

    8)
        printf "\n"
        line
        step "Pushing gram-model (gramcheck) changes..."
        line
        GRAM_DIR="/Users/mac/Documents/literature-in-ascii"
        GRAM_SRC="$GRAM_DIR/co-edit-model/gramcheck"
        if [[ ! -d "$GRAM_DIR/.git" ]]; then
            printf "  ${C_YELLOW}WARN: $GRAM_DIR is not a git repository.${R}\n"
            printf "  ${C_GREY}Trying xcm-editor repo as fallback...${R}\n"
            GRAM_DIR="/Users/mac/Desktop/xcm-editor"
            if [[ ! -d "$GRAM_DIR/.git" ]]; then
                printf "  ${C_RED}ERROR: no git repo found for gram-model.${R}\n" >&2
                read -r "?Press ENTER to return..."
                exec "$SCRIPT_DIR/launcher.sh"
            fi
        fi
        cd "$GRAM_DIR" || { printf "  ${C_RED}ERROR: cannot cd to $GRAM_DIR${R}\n" >&2; exec "$SCRIPT_DIR/launcher.sh"; }
        printf "  ${C_GREY}Working directory: $GRAM_DIR${R}\n"
        REL_PATH="${GRAM_SRC#$GRAM_DIR/}"
        printf "  ${C_GREY}Staging %s...${R}\n" "$REL_PATH"
        if ! git add "$REL_PATH" 2>&1; then
            printf "  ${C_RED}ERROR: git add failed${R}\n" >&2
            cd "$SCRIPT_DIR"
            read -r "?Press ENTER to return..."
            exec "$SCRIPT_DIR/launcher.sh"
        fi
        if git diff --cached --quiet; then
            printf "  ${C_YELLOW}Nothing new to commit in gram-model.${R}\n"
        else
            printf "  ${C_GREY}Files staged:${R}\n"
            git diff --cached --name-only | while read -r f; do printf "    ${C_LAVENDER}%s${R}\n" "$f"; done
            printf "\n  ${C_GREY}Commit message (leave blank for default):${R} "
            read -r GRAM_MSG
            GRAM_MSG="${GRAM_MSG:-Update gramcheck}"
            git commit -m "$GRAM_MSG" || { printf "  ${C_RED}ERROR: git commit failed${R}\n" >&2; }
        fi
        GRAM_BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo main)"
        printf "  ${C_GREY}Pushing branch %s to origin...${R}\n" "$GRAM_BRANCH"
        git push origin "$GRAM_BRANCH" 2>&1 || printf "  ${C_RED}ERROR: git push failed - check credentials and network${R}\n" >&2
        cd "$SCRIPT_DIR"
        printf "\n"
        read -r "?Press ENTER to return..."
        exec "$0"
        ;;

    t|T)
        while true; do
            printf "\n"
            line
            printf "  ${C_VIOLET}${BOLD}Tools${R}\n"
            line
            printf "  ${C_LAVENDER}${BOLD}a${R}  ${C_WHITE}lines_count${R}       ${C_GREY}Count lines in this project (default threshold 1000)${R}\n"
            printf "  ${C_LAVENDER}${BOLD}l${R}  ${C_WHITE}lines_count dir${R}   ${C_GREY}Count lines in a specific directory${R}\n"
            printf "  ${C_LAVENDER}${BOLD}c${R}  ${C_WHITE}security scan${R}     ${C_GREY}Scan this project for leaked keys / secrets${R}\n"
            printf "  ${C_LAVENDER}${BOLD}d${R}  ${C_WHITE}security moon${R}     ${C_GREY}Scan moon-lang for leaked keys / secrets${R}\n"
            printf "  ${C_LAVENDER}${BOLD}e${R}  ${C_WHITE}security gram${R}     ${C_GREY}Scan gram-model (gramcheck) for leaked keys / secrets${R}\n"
            printf "  ${C_LAVENDER}${BOLD}j${R}  ${C_WHITE}security JSON${R}     ${C_GREY}Scan this project and output results as JSON${R}\n"
            printf "  ${C_LAVENDER}${BOLD}b${R}  ${C_WHITE}back${R}              ${C_GREY}Return to main menu${R}\n"
            printf "\n"
            printf "${C_GREY}Tools choice [a/l/c/d/e/j / b=back]:${R} "
            read -r TOOL_CHOICE

            PY3="$(command -v python3 2>/dev/null || command -v python 2>/dev/null)"
            if [[ -z "$PY3" ]]; then
                printf "  ${C_RED}ERROR: python3 not found - cannot run Python tools${R}\n" >&2
                read -r "?Press ENTER to return..."
                break
            fi

            LINES_PY="$SCRIPT_DIR/lines_count.py"
            SEC_PY="$SCRIPT_DIR/security_ck.py"

            case "$TOOL_CHOICE" in
                a)
                    printf "\n"
                    line
                    step "Running lines_count.py on this project..."
                    if [[ ! -f "$LINES_PY" ]]; then
                        printf "  ${C_RED}ERROR: lines_count.py not found at %s${R}\n" "$LINES_PY" >&2
                    else
                        printf "  ${C_GREY}Threshold: 1000 lines (default)${R}\n\n"
                        "$PY3" "$LINES_PY" "$SCRIPT_DIR" 2>&1 || printf "  ${C_RED}ERROR: lines_count.py exited with an error${R}\n" >&2
                    fi
                    printf "\n"
                    read -r "?Press ENTER to continue..."
                    ;;
                l)
                    printf "\n"
                    line
                    step "Running lines_count.py on a custom directory..."
                    printf "  ${C_GREY}Enter directory path (blank = this project, t = return):${R} "
                    read -r SCAN_DIR
                    [[ "$SCAN_DIR" == "t" ]] && continue
                    [[ -z "$SCAN_DIR" ]] && SCAN_DIR="$SCRIPT_DIR"
                    printf "  ${C_GREY}Enter line threshold (blank = 1000):${R} "
                    read -r THRESH
                    [[ -z "$THRESH" ]] && THRESH=1000
                    if [[ ! -f "$LINES_PY" ]]; then
                        printf "  ${C_RED}ERROR: lines_count.py not found at %s${R}\n" "$LINES_PY" >&2
                    elif [[ ! -d "$SCAN_DIR" ]]; then
                        printf "  ${C_RED}ERROR: directory not found: %s${R}\n" "$SCAN_DIR" >&2
                    else
                        printf "\n"
                        "$PY3" "$LINES_PY" "$SCAN_DIR" "$THRESH" 2>&1 || printf "  ${C_RED}ERROR: lines_count.py exited with an error${R}\n" >&2
                    fi
                    printf "\n"
                    read -r "?Press ENTER to continue..."
                    ;;
                c)
                    printf "\n"
                    line
                    step "Running security_ck.py on this project..."
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found at %s${R}\n" "$SEC_PY" >&2
                    else
                        "$PY3" "$SEC_PY" "$SCRIPT_DIR" --no-color 2>&1 || printf "  ${C_RED}ERROR: security_ck.py exited with an error${R}\n" >&2
                    fi
                    printf "\n"
                    read -r "?Press ENTER to continue..."
                    ;;
                d)
                    printf "\n"
                    line
                    step "Running security_ck.py on moon-lang..."
                    MOON_SCAN="/Users/mac/Desktop/xcm-editor/moon-lang"
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found at %s${R}\n" "$SEC_PY" >&2
                    elif [[ ! -d "$MOON_SCAN" ]]; then
                        printf "  ${C_RED}ERROR: moon-lang directory not found: %s${R}\n" "$MOON_SCAN" >&2
                    else
                        "$PY3" "$SEC_PY" "$MOON_SCAN" --no-color 2>&1 || printf "  ${C_RED}ERROR: security_ck.py exited with an error${R}\n" >&2
                    fi
                    printf "\n"
                    read -r "?Press ENTER to continue..."
                    ;;
                e)
                    printf "\n"
                    line
                    step "Running security_ck.py on gram-model (gramcheck)..."
                    GRAM_SCAN="/Users/mac/Documents/literature-in-ascii/co-edit-model/gramcheck"
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found at %s${R}\n" "$SEC_PY" >&2
                    elif [[ ! -d "$GRAM_SCAN" ]]; then
                        printf "  ${C_RED}ERROR: gramcheck directory not found: %s${R}\n" "$GRAM_SCAN" >&2
                    else
                        "$PY3" "$SEC_PY" "$GRAM_SCAN" --no-color 2>&1 || printf "  ${C_RED}ERROR: security_ck.py exited with an error${R}\n" >&2
                    fi
                    printf "\n"
                    read -r "?Press ENTER to continue..."
                    ;;
                j)
                    printf "\n"
                    line
                    step "Running security_ck.py --json on this project..."
                    printf "  ${C_GREY}Enter output file path (blank = print to screen):${R} "
                    read -r JSON_OUT
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found at %s${R}\n" "$SEC_PY" >&2
                    elif [[ -n "$JSON_OUT" ]]; then
                        "$PY3" "$SEC_PY" "$SCRIPT_DIR" --json > "$JSON_OUT" 2>&1 \
                            && printf "  ${C_GREEN}JSON written to: %s${R}\n" "$JSON_OUT" \
                            || printf "  ${C_RED}ERROR: security_ck.py exited with an error - check %s${R}\n" "$JSON_OUT" >&2
                    else
                        "$PY3" "$SEC_PY" "$SCRIPT_DIR" --json 2>&1 || printf "  ${C_RED}ERROR: security_ck.py exited with an error${R}\n" >&2
                    fi
                    printf "\n"
                    read -r "?Press ENTER to continue..."
                    ;;
                back|b)
                    break
                    ;;
                *)
                    printf "  ${C_YELLOW}Unknown choice: %s${R}\n" "$TOOL_CHOICE"
                    ;;
            esac
        done
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
