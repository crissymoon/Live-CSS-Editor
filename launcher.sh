#!/usr/bin/env zsh
# ============================================================
#  Live CSS Editor -- Launcher  (TUI edition)
#  Works as: zsh launcher.sh  OR  double-click launcher.command
# ============================================================

SCRIPT_DIR="${0:a:h}"
cd "$SCRIPT_DIR" || exit 1

# ============================================================
#  ANSI palette
# ============================================================
R='\033[0m'
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
C_CYAN='\033[38;5;116m'
C_ORANGE='\033[38;5;215m'

# ============================================================
#  Line-separator TUI helpers  (no left/right borders)
# ============================================================

# Thick separator  (section breaks)
box_top() { printf "${C_DARK}$(printf '━%.0s' {1..66})${R}\n"; }
box_bot() { printf "${C_DARK}$(printf '━%.0s' {1..66})${R}\n"; }
# Thin separator  (sub-section dividers)
box_mid() { printf "${C_DARK}$(printf '─%.0s' {1..66})${R}\n"; }

box_row() {
    # box_row "content (may contain ANSI)"  -- just indent, no right border
    printf '  %b\n' "$1"
}

box_empty() {
    printf '\n'
}

box_section() {
    printf "  ${C_GREY}${BOLD}%s${R}\n" "$1"
}

box_item() {
    # box_item  key  label  desc
    local key="$1" lbl="$2" desc="$3"
    printf "  ${C_LAVENDER}${BOLD}%-3s${R}  ${C_WHITE}%-13s${R}  ${C_GREY}%s${R}\n" \
        "$key" "$lbl" "$desc"
}

# status rows
status_ok()   { printf "  ${C_GREEN}[ OK ]${R}  %s\n" "$1"; }
status_miss() { printf "  ${C_RED}[MISS]${R}  %s\n" "$1"; }
status_warn() { printf "  ${C_YELLOW}[WARN]${R}  %s\n" "$1"; }
status_info() { printf "  ${C_GREY}[ -- ]${R}  %s\n" "$1"; }

step() {
    printf "\n  ${C_VIOLET}${BOLD}>>> ${R}  ${C_LAVENDER}%s${R}\n" "$*"
}

die() {
    printf "\n  ${C_RED}${BOLD}FATAL:${R}  %s\n\n" "$*" >&2
    read -r "?Press ENTER to close..."
    exit 1
}

# ============================================================
#  Banner
# ============================================================
clear

# Animate moon ASCII art line by line
if [[ -f "$SCRIPT_DIR/xcm-moon-ascii.txt" ]]; then
    # Colour cycles: fade from dark purple -> violet -> lavender
    _moon_colors=(
        '\033[38;5;54m'   # dark indigo
        '\033[38;5;54m'
        '\033[38;5;99m'   # purple
        '\033[38;5;99m'
        '\033[38;5;99m'
        '\033[38;5;141m'  # violet
        '\033[38;5;141m'
        '\033[38;5;141m'
        '\033[38;5;189m'  # lavender
        '\033[38;5;189m'
    )
    _ci=0
    _total=${#_moon_colors[@]}
    while IFS= read -r _mline; do
        _col="${_moon_colors[$(( (_ci % _total) + 1 ))]}"
        printf "${_col}${BOLD}%s${R}\n" "$_mline"
        _ci=$(( _ci + 1 ))
        sleep 0.06 2>/dev/null || true
    done < "$SCRIPT_DIR/xcm-moon-ascii.txt"
    unset _mline _ci _total _col _moon_colors
fi

box_top
printf "  ${C_VIOLET}${BOLD}LIVE CSS EDITOR${R}   ${C_GREY}Native App Launcher${R}\n"
box_mid
printf "  ${C_GREY}Project :${R}  ${C_WHITE}$SCRIPT_DIR${R}\n"
printf "  ${C_GREY}Date    :${R}  ${C_WHITE}$(date '+%Y-%m-%d  %H:%M:%S')${R}\n"
box_mid

# ============================================================
#  Dependency check
# ============================================================
box_section "DEPENDENCIES"
box_empty

MISSING=0

if command -v php &>/dev/null; then
    PHP_VER=$(php -r 'echo PHP_MAJOR_VERSION.".".PHP_MINOR_VERSION;' 2>/dev/null)
    status_ok "PHP $PHP_VER  ($(command -v php))"
else
    status_miss "PHP not found -- install PHP 8+ and add to PATH"
    MISSING=$((MISSING + 1))
fi

if command -v node &>/dev/null; then
    NODE_VER=$(node -e 'process.stdout.write(process.version)' 2>/dev/null)
    status_ok "Node.js $NODE_VER  ($(command -v node))"
else
    status_miss "Node.js not found -- install from https://nodejs.org"
    MISSING=$((MISSING + 1))
fi

if command -v npm &>/dev/null; then
    status_ok "npm $(npm -v 2>/dev/null)"
else
    status_miss "npm not found"
    MISSING=$((MISSING + 1))
fi

if command -v cargo &>/dev/null; then
    CARGO_VER=$(cargo --version 2>/dev/null | awk '{print $2}')
    status_ok "Rust / cargo $CARGO_VER  ($(command -v cargo))"
else
    status_miss "Rust / cargo not found -- install from https://rustup.rs"
    MISSING=$((MISSING + 1))
fi

if [[ -f "node_modules/.bin/tauri" ]]; then
    TAURI_VER=$(node_modules/.bin/tauri --version 2>/dev/null | awk '{print $NF}')
    status_ok "Tauri CLI $TAURI_VER  (local node_modules)"
elif command -v tauri &>/dev/null; then
    TAURI_VER=$(tauri --version 2>/dev/null | awk '{print $NF}')
    status_ok "Tauri CLI $TAURI_VER  (global)"
else
    status_warn "Tauri CLI not found -- run npm install first"
fi

if [[ -d "node_modules" ]]; then
    status_ok "node_modules present"
else
    status_warn "node_modules missing -- will run npm install"
fi

if [[ -d "src-tauri/www" && -f "src-tauri/www/index.php" ]]; then
    WWW_COUNT=$(find src-tauri/www -type f | wc -l | tr -d ' ')
    status_ok "src-tauri/www/ populated  ($WWW_COUNT files)"
else
    status_warn "src-tauri/www/ not populated -- will copy automatically"
fi

if [[ -f "src-tauri/icons/icon.png" ]]; then
    status_ok "App icon present"
else
    status_warn "App icon missing -- will generate automatically"
fi

MOON_BIN=""
for _mb in \
    "/Users/mac/Desktop/xcm-editor/moon-lang/moon" \
    "/usr/local/bin/moon" \
    "$(command -v moon 2>/dev/null)"; do
    if [[ -x "$_mb" ]]; then MOON_BIN="$_mb"; break; fi
done
if [[ -n "$MOON_BIN" ]]; then
    status_ok "moon binary  ($MOON_BIN)"
else
    status_warn "moon binary not found -- agent-flow Run will not work"
fi

box_empty

if [[ $MISSING -gt 0 ]]; then
    box_row "  ${C_RED}${BOLD}$MISSING required dep(s) missing.${R}  Resolve above and re-run."
    box_bot
    printf "\n"; read -r "?Press ENTER to close..."; exit 1
fi

box_mid

# ============================================================
#  Ensure node_modules
# ============================================================
if [[ ! -d "node_modules" ]]; then
    box_bot
    step "Installing npm packages..."
    npm install || die "npm install failed"
    clear; exec "$0"
fi

# ============================================================
#  Menu
# ============================================================
box_section "DEVELOPMENT"
box_empty
box_item "1" "Dev"         "Start the app in dev mode  (hot-reload, DevTools)"
box_item "2" "Build"       "Compile a release bundle   (.app / .dmg on macOS)"
box_item "3" "PHP only"    "PHP dev server at localhost:8080  (no Tauri)"
box_item "4" "Refresh"     "Re-copy www/ and regenerate icon"
box_item "5" "Clean"       "Remove Rust build artifacts  (src-tauri/target/)"
box_empty
box_mid
box_section "PUSH"
box_empty
box_item "6" "Push live"   "Commit and push Live CSS Editor to GitHub"
box_item "7" "Push moon"   "Commit and push moon-lang  (xcm-editor repo)"
box_item "8" "Push gram"   "Commit and push gram-model (gramcheck)"
box_item "9" "Push all"    "Push all 3 repos in sequence"
box_empty
box_mid
box_section "TOOLS  &  PROJECTS"
box_empty
box_item "t" "Tools"       "lines_count.py  /  security_ck.py"
box_item "a" "Agent Flow"  "Start agent-flow UI at localhost:9090"
box_empty
box_mid
box_item "q" "Quit"        ""
box_bot
printf "\n"
printf "  ${C_GREY}Choice  [ 1-9 / t / a / q ] :${R}  "
read -r CHOICE

# ============================================================
#  Asset helper
# ============================================================
_ensure_assets() {
    if [[ ! -f "src-tauri/icons/icon.png" ]]; then
        step "Generating app icon..."
        node scripts/gen-icon.js || die "Icon generation failed"
    fi
    step "Copying app files to src-tauri/www/..."
    node scripts/copy-www.js || die "copy-www failed"
}

# ============================================================
#  Push helper
#  _push_repo  REPO_DIR  STAGE_PATH  DEFAULT_MSG  LABEL  [ask=yes|no]
# ============================================================
_push_repo() {
    local repo_dir="$1"
    local stage_path="$2"
    local default_msg="$3"
    local label="$4"
    local ask_msg="${5:-yes}"

    step "Pushing $label ..."

    if [[ ! -d "$repo_dir/.git" ]]; then
        printf "  ${C_RED}ERROR: %s is not a git repository${R}\n" "$repo_dir" >&2
        return 1
    fi

    local prev_dir="$PWD"
    cd "$repo_dir" || { printf "  ${C_RED}ERROR: cannot cd to %s${R}\n" "$repo_dir" >&2; return 1; }

    printf "  ${C_GREY}Staging %s ...${R}\n" "$stage_path"
    local _add_out _add_rc
    _add_out="$(git add "$stage_path" 2>&1)"
    _add_rc=$?
    if [[ -n "$_add_out" ]]; then
        printf '%s\n' "$_add_out" | while IFS= read -r _l; do
            printf "    ${C_GREY}%s${R}\n" "$_l"
        done
    fi
    if [[ $_add_rc -ne 0 ]]; then
        # Check if the ONLY failures are gitignore advisories (not real errors)
        local _hard
        _hard="$(printf '%s\n' "$_add_out" \
            | grep -v 'ignored by one of your .gitignore' \
            | grep -v '^hint:' \
            | grep -v '^$')"
        if [[ -n "$_hard" ]]; then
            printf "  ${C_RED}ERROR: git add failed for %s (exit %d)${R}\n" "$stage_path" "$_add_rc" >&2
            cd "$prev_dir"; return 1
        else
            printf "  ${C_YELLOW}WARN: some paths in %s are gitignored and were skipped -- continuing${R}\n" "$stage_path" >&2
        fi
    fi

    if git diff --cached --quiet; then
        printf "  ${C_YELLOW}Nothing new to commit in %s${R}\n" "$label"
    else
        printf "  ${C_GREY}Files staged:${R}\n"
        git diff --cached --name-only 2>/dev/null \
            | while read -r f; do printf "    ${C_LAVENDER}%s${R}\n" "$f"; done

        local commit_msg="$default_msg"
        if [[ "$ask_msg" == "yes" ]]; then
            printf "\n  ${C_GREY}Commit message (blank = \"%s\"):${R}  " "$default_msg"
            read -r _m; [[ -n "$_m" ]] && commit_msg="$_m"
        fi

        git commit -m "$commit_msg" 2>&1 \
            || { printf "  ${C_RED}ERROR: git commit failed${R}\n" >&2; cd "$prev_dir"; return 1; }
    fi

    local branch
    branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || printf 'main')"
    printf "  ${C_GREY}Pushing branch %s to origin ...${R}\n" "$branch"
    git push origin "$branch" 2>&1 \
        || { printf "  ${C_RED}ERROR: git push failed -- check credentials / network${R}\n" >&2; cd "$prev_dir"; return 1; }

    printf "  ${C_GREEN}Done: %s${R}\n" "$label"
    cd "$prev_dir"; return 0
}

# ============================================================
#  Agent-flow helper
# ============================================================
_open_agent_flow() {
    local port=9090
    local dir="$SCRIPT_DIR/agent-flow"
    local logfile="$SCRIPT_DIR/.agent-flow-server.log"

    if [[ ! -d "$dir" ]]; then
        printf "  ${C_RED}ERROR: agent-flow directory not found at %s${R}\n" "$dir" >&2
        return 1
    fi

    if lsof -i ":$port" -sTCP:LISTEN &>/dev/null 2>&1; then
        printf "  ${C_YELLOW}PHP server already running on :%s${R}\n" "$port"
    else
        printf "  ${C_GREY}Starting PHP server on localhost:%s ...${R}\n" "$port"
        php -S "localhost:$port" -t "$dir" >"$logfile" 2>&1 &
        local pid=$!
        sleep 0.6
        if ! kill -0 "$pid" 2>/dev/null; then
            printf "  ${C_RED}ERROR: PHP server failed to start${R}\n" >&2
            printf "  ${C_GREY}Check log: %s${R}\n" "$logfile" >&2
            return 1
        fi
        printf "  ${C_GREEN}PHP server started (PID %s)${R}\n" "$pid"
        printf "  ${C_GREY}Log file: %s${R}\n" "$logfile"
        printf "  ${C_GREY}Stop with: kill %s${R}\n" "$pid"
    fi

    printf "  ${C_GREY}Opening http://localhost:%s ...${R}\n" "$port"

    # Dev testing browser (PyQt6)
    local wb_script="/Users/mac/Documents/mind-mapping/is_working_pyqt6_webbrowse/webbrowse.py"
    local wb_python="/Users/mac/Documents/mind-mapping/is_working_pyqt6_webbrowse/venv/bin/python"
    local wb_url="http://localhost:$port"
    local wb_log="$SCRIPT_DIR/.agent-flow-browser.log"

    if [[ -f "$wb_script" && -x "$wb_python" ]]; then
        printf "  ${C_GREY}Launching dev browser (webbrowse.py) ...${R}\n"
        "$wb_python" "$wb_script" --url "$wb_url" >"$wb_log" 2>&1 &
        local wb_pid=$!
        sleep 0.8
        if kill -0 "$wb_pid" 2>/dev/null; then
            printf "  ${C_GREEN}Dev browser started (PID %s)${R}\n" "$wb_pid"
            printf "  ${C_GREY}Browser log: %s${R}\n" "$wb_log"
        else
            printf "  ${C_RED}ERROR: dev browser exited early -- check log: %s${R}\n" "$wb_log" >&2
            printf "  ${C_GREY}Last output:${R}\n" >&2
            tail -n 8 "$wb_log" 2>/dev/null | while IFS= read -r _l; do
                printf "    ${C_GREY}%s${R}\n" "$_l" >&2
            done
            printf "  ${C_YELLOW}Falling back to system browser...${R}\n" >&2
            if command -v open &>/dev/null; then
                open "$wb_url" 2>&1 \
                    || printf "  ${C_YELLOW}WARN: system browser also failed -- visit %s manually${R}\n" "$wb_url" >&2
            else
                printf "  ${C_YELLOW}WARN: visit %s manually${R}\n" "$wb_url" >&2
            fi
        fi
    elif [[ ! -f "$wb_script" ]]; then
        printf "  ${C_YELLOW}WARN: webbrowse.py not found at %s${R}\n" "$wb_script" >&2
        printf "  ${C_YELLOW}Falling back to system browser...${R}\n" >&2
        if command -v open &>/dev/null; then
            open "$wb_url" 2>&1 \
                || printf "  ${C_YELLOW}WARN: could not open browser -- visit %s manually${R}\n" "$wb_url" >&2
        else
            printf "  ${C_YELLOW}WARN: visit %s manually${R}\n" "$wb_url" >&2
        fi
    else
        printf "  ${C_YELLOW}WARN: venv python not executable at %s${R}\n" "$wb_python" >&2
        printf "  ${C_GREY}Try: chmod +x %s${R}\n" "$wb_python" >&2
        printf "  ${C_YELLOW}Falling back to system browser...${R}\n" >&2
        if command -v open &>/dev/null; then
            open "$wb_url" 2>&1 \
                || printf "  ${C_YELLOW}WARN: could not open browser -- visit %s manually${R}\n" "$wb_url" >&2
        else
            printf "  ${C_YELLOW}WARN: visit %s manually${R}\n" "$wb_url" >&2
        fi
    fi
    return 0
}

# ============================================================
#  Cases
# ============================================================
case "$CHOICE" in

    1)
        printf "\n"; step "Starting Live CSS Editor in DEV mode..."
        _ensure_assets
        printf "\n${C_GREEN}Starting tauri dev...${R}\n\n"
        npm run copy-www && npx tauri dev \
            || printf "  ${C_RED}ERROR: tauri dev failed -- check output above${R}\n" >&2
        ;;

    2)
        printf "\n"; step "Building release bundle..."
        _ensure_assets
        printf "\n${C_GREEN}Running tauri build...${R}\n\n"
        npm run copy-www && npx tauri build \
            || { printf "  ${C_RED}ERROR: tauri build failed${R}\n" >&2; read -r "?Press ENTER..."; exec "$0"; }
        printf "\n"
        DIST_DIR="src-tauri/target/release/bundle"
        if [[ -d "$DIST_DIR" ]]; then
            printf "${C_GREY}Artifacts:${R}\n"
            find "$DIST_DIR" -maxdepth 2 -type f \
                \( -name "*.dmg" -o -name "*.app" -o -name "*.deb" \
                   -o -name "*.rpm" -o -name "*.exe" -o -name "*.msi" \) \
                | while read -r f; do
                    SZ=$(du -sh "$f" 2>/dev/null | awk '{print $1}')
                    printf "  ${C_LAVENDER}%s${R}  ${C_GREY}(%s)${R}\n" "$f" "$SZ"
                done
        fi
        printf "\n"; read -r "?Press ENTER to close..."
        ;;

    3)
        printf "\n"; step "Starting PHP dev server at http://localhost:8080 ..."
        printf "  ${C_YELLOW}Press Ctrl+C to stop.${R}\n\n"
        php -S localhost:8080 \
            || printf "  ${C_RED}ERROR: php -S failed -- is PHP installed?${R}\n" >&2
        ;;

    4)
        printf "\n"; step "Refreshing assets..."
        node scripts/gen-icon.js \
            || printf "  ${C_RED}ERROR: gen-icon.js failed${R}\n" >&2
        node scripts/copy-www.js \
            || printf "  ${C_RED}ERROR: copy-www.js failed${R}\n" >&2
        printf "\n  ${C_GREEN}Done.${R}  %s files in src-tauri/www/\n" \
            "$(find src-tauri/www -type f 2>/dev/null | wc -l | tr -d ' ')"
        printf "\n"; read -r "?Press ENTER to return..."; exec "$0"
        ;;

    5)
        printf "\n"; step "Cleaning Rust build artifacts..."
        if [[ -d "src-tauri/target" ]]; then
            SZ=$(du -sh src-tauri/target 2>/dev/null | awk '{print $1}')
            printf "  ${C_GREY}Removing src-tauri/target/  (%s) ...${R}\n" "$SZ"
            rm -rf src-tauri/target \
                || printf "  ${C_RED}ERROR: rm -rf failed${R}\n" >&2
            printf "  ${C_GREEN}Done.${R}\n"
        else
            printf "  ${C_GREY}src-tauri/target/ not present -- nothing to clean.${R}\n"
        fi
        printf "\n"; read -r "?Press ENTER to return..."; exec "$0"
        ;;

    6)
        printf "\n"
        bash "$SCRIPT_DIR/push.sh" \
            || printf "  ${C_RED}ERROR: push.sh failed -- check output above${R}\n" >&2
        printf "\n"; read -r "?Press ENTER to return..."; exec "$0"
        ;;

    7)
        printf "\n"
        _push_repo \
            "/Users/mac/Desktop/xcm-editor" \
            "moon-lang/" \
            "Update moon-lang" \
            "moon-lang"
        printf "\n"; read -r "?Press ENTER to return..."; exec "$0"
        ;;

    8)
        printf "\n"
        if [[ -d "/Users/mac/Documents/literature-in-ascii/.git" ]]; then
            _push_repo \
                "/Users/mac/Documents/literature-in-ascii" \
                "co-edit-model/gramcheck" \
                "Update gramcheck" \
                "gram-model"
        else
            printf "  ${C_RED}ERROR: literature-in-ascii is not a git repo${R}\n" >&2
            printf "  ${C_GREY}gramcheck source lives at: /Users/mac/Documents/literature-in-ascii/co-edit-model/gramcheck${R}\n" >&2
            printf "  ${C_GREY}Run: cd /Users/mac/Documents/literature-in-ascii && git init  to initialise it${R}\n" >&2
            printf "  ${C_YELLOW}Skipping gram-model push -- moon-lang/gramcheck in xcm-editor is gitignored (binary)${R}\n" >&2
        fi
        printf "\n"; read -r "?Press ENTER to return..."; exec "$0"
        ;;

    9)
        printf "\n"
        step "Push All -- Live CSS Editor  /  moon-lang  /  gram-model"
        printf "\n  ${C_GREY}Single commit message for all 3 repos${R}\n"
        printf "  ${C_GREY}(blank = each repo uses its own default):${R}  "
        read -r ALL_MSG

        PUSH_ERRORS=0

        # ---- 1: Live CSS Editor --------------------------------
        step "1 / 3  Live CSS Editor..."
        (
            cd "$SCRIPT_DIR" || exit 1
            git add . 2>&1 \
                || { printf "  ${C_RED}ERROR: git add failed${R}\n" >&2; exit 1; }
            if git diff --cached --quiet; then
                printf "  ${C_YELLOW}Nothing new to commit (Live CSS Editor)${R}\n"
            else
                git diff --cached --name-only 2>/dev/null \
                    | while read -r f; do printf "    ${C_LAVENDER}%s${R}\n" "$f"; done
                _msg="${ALL_MSG:-Update Live CSS Editor}"
                git commit -m "$_msg" 2>&1 \
                    || { printf "  ${C_RED}ERROR: commit failed${R}\n" >&2; exit 1; }
            fi
            _br="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || printf 'master')"
            git remote set-url origin "https://github.com/crissymoon/Live-CSS-Editor.git" 2>/dev/null || true
            git push origin "$_br" 2>&1 \
                || { printf "  ${C_RED}ERROR: push failed${R}\n" >&2; exit 1; }
            printf "  ${C_GREEN}Live CSS Editor pushed${R}\n"
        ) || PUSH_ERRORS=$((PUSH_ERRORS + 1))

        # ---- 2: moon-lang -------------------------------------
        step "2 / 3  moon-lang..."
        _push_repo \
            "/Users/mac/Desktop/xcm-editor" \
            "moon-lang/" \
            "${ALL_MSG:-Update moon-lang}" \
            "moon-lang" \
            "no" || PUSH_ERRORS=$((PUSH_ERRORS + 1))

        # ---- 3: gram-model ------------------------------------
        step "3 / 3  gram-model..."
        if [[ -d "/Users/mac/Documents/literature-in-ascii/.git" ]]; then
            _push_repo \
                "/Users/mac/Documents/literature-in-ascii" \
                "co-edit-model/gramcheck" \
                "${ALL_MSG:-Update gramcheck}" \
                "gram-model" \
                "no" || PUSH_ERRORS=$((PUSH_ERRORS + 1))
        else
            printf "  ${C_RED}ERROR: literature-in-ascii is not a git repo -- skipping gram-model${R}\n" >&2
            printf "  ${C_GREY}gramcheck source: /Users/mac/Documents/literature-in-ascii/co-edit-model/gramcheck${R}\n" >&2
            printf "  ${C_GREY}moon-lang/gramcheck in xcm-editor is gitignored (binary) -- nothing to commit there${R}\n" >&2
            PUSH_ERRORS=$((PUSH_ERRORS + 1))
        fi

        printf "\n"
        if [[ $PUSH_ERRORS -eq 0 ]]; then
            printf "  ${C_GREEN}${BOLD}All 3 repos pushed successfully.${R}\n"
        else
            printf "  ${C_RED}${BOLD}%d repo(s) had errors -- check output above.${R}\n" "$PUSH_ERRORS" >&2
        fi
        printf "\n"; read -r "?Press ENTER to return..."; exec "$0"
        ;;

    t|T)
        while true; do
            printf "\n"
            box_top
            box_section "TOOLS"
            box_empty
            box_item "a" "lines count"  "Count lines in this project  (threshold 1000)"
            box_item "l" "lines dir"    "Count lines in a custom directory"
            box_item "c" "sec scan"     "Scan this project for leaked keys / secrets"
            box_item "d" "sec moon"     "Scan moon-lang for leaked keys / secrets"
            box_item "e" "sec gram"     "Scan gram-model (gramcheck) for leaked keys"
            box_item "j" "sec JSON"     "Scan this project, output as JSON"
            box_empty
            box_item "b" "back"         "Return to main menu"
            box_bot
            printf "\n  ${C_GREY}Tools  [ a / l / c / d / e / j / b ] :${R}  "
            read -r TOOL_CHOICE

            PY3="$(command -v python3 2>/dev/null || command -v python 2>/dev/null)"
            if [[ -z "$PY3" ]]; then
                printf "  ${C_RED}ERROR: python3 not found -- cannot run Python tools${R}\n" >&2
                read -r "?Press ENTER to return..."; break
            fi

            LINES_PY="$SCRIPT_DIR/lines_count.py"
            SEC_PY="$SCRIPT_DIR/security_ck.py"

            case "$TOOL_CHOICE" in
                a)
                    step "lines_count.py -- this project"
                    if [[ ! -f "$LINES_PY" ]]; then
                        printf "  ${C_RED}ERROR: lines_count.py not found at %s${R}\n" "$LINES_PY" >&2
                    else
                        printf "  ${C_GREY}Threshold: 1000 (default)${R}\n\n"
                        "$PY3" "$LINES_PY" "$SCRIPT_DIR" 2>&1 \
                            || printf "  ${C_RED}ERROR: lines_count.py exited with error${R}\n" >&2
                    fi
                    printf "\n"; read -r "?Press ENTER to continue..."
                    ;;
                l)
                    step "lines_count.py -- custom directory"
                    printf "  ${C_GREY}Directory  (blank = this project, b = cancel):${R}  "
                    read -r SCAN_DIR
                    [[ "$SCAN_DIR" == "b" ]] && continue
                    [[ -z "$SCAN_DIR" ]] && SCAN_DIR="$SCRIPT_DIR"
                    printf "  ${C_GREY}Threshold  (blank = 1000):${R}  "
                    read -r THRESH; [[ -z "$THRESH" ]] && THRESH=1000
                    if [[ ! -f "$LINES_PY" ]]; then
                        printf "  ${C_RED}ERROR: lines_count.py not found${R}\n" >&2
                    elif [[ ! -d "$SCAN_DIR" ]]; then
                        printf "  ${C_RED}ERROR: directory not found: %s${R}\n" "$SCAN_DIR" >&2
                    else
                        printf "\n"
                        "$PY3" "$LINES_PY" "$SCAN_DIR" "$THRESH" 2>&1 \
                            || printf "  ${C_RED}ERROR: lines_count.py exited with error${R}\n" >&2
                    fi
                    printf "\n"; read -r "?Press ENTER to continue..."
                    ;;
                c)
                    step "security_ck.py -- this project"
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found at %s${R}\n" "$SEC_PY" >&2
                    else
                        "$PY3" "$SEC_PY" "$SCRIPT_DIR" --no-color 2>&1 \
                            || printf "  ${C_RED}ERROR: security_ck.py exited with error${R}\n" >&2
                    fi
                    printf "\n"; read -r "?Press ENTER to continue..."
                    ;;
                d)
                    step "security_ck.py -- moon-lang"
                    MOON_SCAN="/Users/mac/Desktop/xcm-editor/moon-lang"
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found${R}\n" >&2
                    elif [[ ! -d "$MOON_SCAN" ]]; then
                        printf "  ${C_RED}ERROR: moon-lang not found: %s${R}\n" "$MOON_SCAN" >&2
                    else
                        "$PY3" "$SEC_PY" "$MOON_SCAN" --no-color 2>&1 \
                            || printf "  ${C_RED}ERROR: security_ck.py exited with error${R}\n" >&2
                    fi
                    printf "\n"; read -r "?Press ENTER to continue..."
                    ;;
                e)
                    step "security_ck.py -- gram-model"
                    GRAM_SCAN="/Users/mac/Documents/literature-in-ascii/co-edit-model/gramcheck"
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found${R}\n" >&2
                    elif [[ ! -d "$GRAM_SCAN" ]]; then
                        printf "  ${C_RED}ERROR: gramcheck not found: %s${R}\n" "$GRAM_SCAN" >&2
                    else
                        "$PY3" "$SEC_PY" "$GRAM_SCAN" --no-color 2>&1 \
                            || printf "  ${C_RED}ERROR: security_ck.py exited with error${R}\n" >&2
                    fi
                    printf "\n"; read -r "?Press ENTER to continue..."
                    ;;
                j)
                    step "security_ck.py --json -- this project"
                    printf "  ${C_GREY}Output file  (blank = screen):${R}  "
                    read -r JSON_OUT
                    if [[ ! -f "$SEC_PY" ]]; then
                        printf "  ${C_RED}ERROR: security_ck.py not found${R}\n" >&2
                    elif [[ -n "$JSON_OUT" ]]; then
                        "$PY3" "$SEC_PY" "$SCRIPT_DIR" --json > "$JSON_OUT" 2>&1 \
                            && printf "  ${C_GREEN}JSON written to: %s${R}\n" "$JSON_OUT" \
                            || printf "  ${C_RED}ERROR: failed -- check %s${R}\n" "$JSON_OUT" >&2
                    else
                        "$PY3" "$SEC_PY" "$SCRIPT_DIR" --json 2>&1 \
                            || printf "  ${C_RED}ERROR: security_ck.py exited with error${R}\n" >&2
                    fi
                    printf "\n"; read -r "?Press ENTER to continue..."
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

    a|A)
        printf "\n"
        step "Opening Agent Flow..."
        _open_agent_flow \
            || printf "  ${C_RED}ERROR: could not open agent-flow -- check output above${R}\n" >&2
        printf "\n"; read -r "?Press ENTER to return..."; exec "$0"
        ;;

    q|Q|"")
        printf "\n  ${C_GREY}Exiting.${R}\n\n"
        exit 0
        ;;

    *)
        printf "\n  ${C_YELLOW}Unknown choice '%s' -- returning to menu.${R}\n\n" "$CHOICE"
        sleep 1; exec "$0"
        ;;
esac
