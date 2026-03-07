#!/usr/bin/env zsh
# Live CSS Editor.command
# Double-click in Finder to launch the admin panel or dev tools.
# macOS: right-click -> Open the first time to bypass Gatekeeper.

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR" || exit 1

# ── ANSI colours ───────────────────────────────────────────────────────────────
R='\033[0m'
BOLD='\033[1m'
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

box_top()     { printf "${C_DARK}$(printf '━%.0s' {1..66})${R}\n"; }
box_bot()     { printf "${C_DARK}$(printf '━%.0s' {1..66})${R}\n"; }
box_mid()     { printf "${C_DARK}$(printf '─%.0s' {1..66})${R}\n"; }
box_empty()   { printf '\n'; }
box_section() { printf "  ${C_GREY}${BOLD}%s${R}\n" "$1"; }
box_item()    { printf "  ${C_LAVENDER}${BOLD}%-3s${R}  ${C_WHITE}%-14s${R}  ${C_GREY}%s${R}\n" "$1" "$2" "$3"; }
status_ok()   { printf "  ${C_GREEN}[ OK ]${R}  %s\n" "$1"; }
status_fail() { printf "  ${C_RED}[FAIL]${R}  %s\n" "$*"; }
status_info() { printf "  ${C_GREY}  ..  ${R}  %s\n" "$1"; }
step()        { printf "\n  ${C_VIOLET}${BOLD}>>> ${R}  ${C_LAVENDER}%s${R}\n" "$*"; }
die()         { printf "\n  ${C_RED}${BOLD}FATAL:${R}  %s\n\n" "$*" >&2; read -r "?Press ENTER to close..."; exit 1; }

# ── Spinner ────────────────────────────────────────────────────────────────────
spin() {
    local msg="$1" i=0
    local chars=('|' '/' '-' '\')
    while true; do
        printf "\r  ${C_VIOLET}${BOLD}[%s]${R}  ${C_LAVENDER}%s${R}   " "${chars[$((i % 4))]}" "$msg"
        (( i++ )); sleep 0.1
    done
}

# ── Port check ─────────────────────────────────────────────────────────────────
_port_open() {
    if command -v nc &>/dev/null; then
        nc -z 127.0.0.1 "$1" 2>/dev/null; return $?
    fi
    (echo "" > /dev/tcp/127.0.0.1/"$1") 2>/dev/null; return $?
}

# ── Push helper ────────────────────────────────────────────────────────────────
_push_repo() {
    local repo_dir="$1" stage_path="$2" default_msg="$3" label="$4" ask_msg="${5:-yes}"
    step "Pushing $label ..."
    if [[ ! -d "$repo_dir/.git" ]]; then
        printf "  ${C_RED}ERROR: %s is not a git repository${R}\n" "$repo_dir" >&2; return 1
    fi
    local prev_dir="$PWD"
    cd "$repo_dir" || { printf "  ${C_RED}ERROR: cannot cd to %s${R}\n" "$repo_dir" >&2; return 1; }
    local _add_out _add_rc
    _add_out="$(git add "$stage_path" 2>&1)"; _add_rc=$?
    [[ -n "$_add_out" ]] && printf '%s\n' "$_add_out" | while IFS= read -r _l; do printf "    ${C_GREY}%s${R}\n" "$_l"; done
    if [[ $_add_rc -ne 0 ]]; then
        local _hard
        _hard="$(printf '%s\n' "$_add_out" | grep -v 'ignored by one of your .gitignore' | grep -v '^hint:' | grep -v '^$')"
        if [[ -n "$_hard" ]]; then
            printf "  ${C_RED}ERROR: git add failed for %s${R}\n" "$stage_path" >&2; cd "$prev_dir"; return 1
        fi
    fi
    if git diff --cached --quiet; then
        printf "  ${C_YELLOW}Nothing new to commit in %s${R}\n" "$label"
    else
        printf "  ${C_GREY}Files staged:${R}\n"
        git diff --cached --name-only 2>/dev/null | while read -r f; do printf "    ${C_LAVENDER}%s${R}\n" "$f"; done
        local commit_msg="$default_msg"
        if [[ "$ask_msg" == "yes" ]]; then
            printf "\n  ${C_GREY}Commit message (blank = \"%s\"):${R}  " "$default_msg"
            read -r _m; [[ -n "$_m" ]] && commit_msg="$_m"
        fi
        git commit -m "$commit_msg" 2>&1 || { printf "  ${C_RED}ERROR: git commit failed${R}\n" >&2; cd "$prev_dir"; return 1; }
    fi
    local branch
    branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || printf 'main')"
    printf "  ${C_GREY}Pushing branch %s to origin ...${R}\n" "$branch"
    local _push_out _push_rc
    _push_out="$(git push -v origin "$branch" 2>&1)"; _push_rc=$?
    printf '%s\n' "$_push_out" | while IFS= read -r _l; do printf "    ${C_GREY}%s${R}\n" "$_l"; done
    if [[ $_push_rc -ne 0 ]]; then
        printf "  ${C_RED}ERROR: git push failed (exit %s)${R}\n" "$_push_rc" >&2
        cd "$prev_dir"; return 1
    fi
    printf "  ${C_GREEN}Done: %s${R}\n" "$label"
    cd "$prev_dir"; return 0
}

# ── Agent-flow helper ──────────────────────────────────────────────────────────
_open_agent_flow() {
    local port=9090
    local dir="$DIR/dev-tools/agent-flow"
    local logfile="$DIR/.agent-flow-server.log"
    if [[ ! -d "$dir" ]]; then
        printf "  ${C_RED}ERROR: agent-flow directory not found at %s${R}\n" "$dir" >&2; return 1
    fi
    if lsof -i ":$port" -sTCP:LISTEN &>/dev/null 2>&1; then
        printf "  ${C_YELLOW}PHP server already running on :%s${R}\n" "$port"
    else
        printf "  ${C_GREY}Starting PHP server on localhost:%s ...${R}\n" "$port"
        php -S "localhost:$port" -t "$dir" >"$logfile" 2>&1 &
        local pid=$!
        sleep 0.6
        if ! kill -0 "$pid" 2>/dev/null; then
            printf "  ${C_RED}ERROR: PHP server failed to start${R}\n" >&2; return 1
        fi
        printf "  ${C_GREEN}PHP server started (PID %s)${R}\n" "$pid"
    fi
    printf "  ${C_GREY}Opening http://localhost:%s ...${R}\n" "$port"
    local imgui_run="$DIR/imgui-browser/run.sh"
    if [[ -f "$imgui_run" ]]; then
        bash "$imgui_run" --url "http://localhost:$port" >/tmp/agent-flow-browser.log 2>&1 &
        sleep 0.8
        printf "  ${C_GREEN}Browser launched${R}\n"
    elif command -v open &>/dev/null; then
        open "http://localhost:$port" 2>/dev/null || true
    fi
}

# ── Start admin helper ─────────────────────────────────────────────────────────
AUTH_BASH_PID=""

_start_admin() {
    local AUTH_SCRIPT="$DIR/page-builder/pb_admin/start-auth.sh"
    [[ ! -f "$AUTH_SCRIPT" ]] && die "page-builder/pb_admin/start-auth.sh not found"

    if _port_open 8443 && _port_open 9100; then
        status_ok "Servers already running  (nginx :8443, Auth :9100)"
    else
        status_info "Starting auth servers..."
        bash "$AUTH_SCRIPT" >/tmp/live-css-auth.log 2>&1 &
        AUTH_BASH_PID=$!
        printf "\n"
        spin "Waiting for servers  (Go compiling if first run...)" &
        local SPIN_PID=$!
        local _i=0 _go_ready=0 _nginx_ready=0
        while (( _i < 450 )); do
            (( ! _go_ready )) && _port_open 9100 && _go_ready=1
            (( _go_ready )) && _port_open 8443 && { _nginx_ready=1; break; }
            sleep 0.2; (( _i++ ))
        done
        kill "$SPIN_PID" 2>/dev/null; wait "$SPIN_PID" 2>/dev/null
        printf "\r%72s\r" ""
        (( _go_ready ))   && status_ok   "Auth server ready  :9100" \
                          || status_fail "Auth server did not start -- see /tmp/live-css-auth.log"
        (( _nginx_ready )) && status_ok   "nginx HTTPS ready  :8443" \
                           || { status_fail "nginx not responding on :8443 -- run: bash server/start.sh"
                                status_info "Try again after: bash $DIR/server/start.sh"; }
    fi

    printf "\n"
    # show credentials
    local DB_PATH="$DIR/page-builder/xcm_auth/xcm_auth_dev.db"
    local CRED_FILE="$DIR/page-builder/xcm_auth/dev-credentials.json"
    box_top
    printf "  ${C_VIOLET}${BOLD}LOGIN CREDENTIALS${R}\n"
    box_mid
    if [[ -f "$DB_PATH" ]] && command -v sqlite3 &>/dev/null; then
        local _count
        _count=$(sqlite3 "$DB_PATH" "SELECT COUNT(*) FROM users;" 2>/dev/null || echo "0")
        if [[ -z "$_count" || "$_count" == "0" ]]; then
            status_info "No users yet -- run setup:"
            printf "  ${C_CYAN}  https://localhost:8443/pb_admin/setup.php${R}\n"
        else
            # read plaintext password from dev-credentials.json if available
            local _devpass=""
            if [[ -f "$CRED_FILE" ]]; then
                _devpass=$(python3 -c "import json,sys; d=json.load(open('$CRED_FILE')); print(d.get('password',''))" 2>/dev/null || true)
            fi
            while IFS='|' read -r _user _email _role; do
                printf "  ${C_GREY}Username :${R}  ${C_WHITE}%s${R}\n" "$_user"
                printf "  ${C_GREY}Email    :${R}  ${C_WHITE}%s${R}\n" "$_email"
                printf "  ${C_GREY}Role     :${R}  ${C_LAVENDER}%s${R}\n" "$_role"
                if [[ -n "$_devpass" ]]; then
                    printf "  ${C_GREY}Password :${R}  ${C_YELLOW}%s${R}\n" "$_devpass"
                else
                    printf "  ${C_GREY}Password :${R}  ${C_YELLOW}(see xcm_auth/dev-credentials.json)${R}\n"
                fi
                printf "\n"
            done < <(sqlite3 "$DB_PATH" \
                "SELECT username, email, role FROM users WHERE is_active=1 ORDER BY created_at LIMIT 5;" 2>/dev/null)
            printf "  ${C_GREY}Note     :${R}  for testing -- to reset: delete xcm_auth/xcm_auth_dev.db\n"
            printf "  ${C_GREY}         ${R}  then re-run setup at /pb_admin/setup.php\n"
        fi
    elif [[ ! -f "$DB_PATH" ]]; then
        status_info "Database not created yet -- starts on first server run"
    else
        status_info "sqlite3 not found -- install with: brew install sqlite"
    fi
    box_mid
    printf "  ${C_GREY}URL   :${R}  ${C_CYAN}https://localhost:8443/pb_admin/login.php${R}\n"
    box_bot
    printf "\n"

    # launch browser
    local IMGUI_RUN="$DIR/imgui-browser/run.sh"
    local LOGIN_URL="https://localhost:8443/page-builder/pb_admin/dashboard.php"
    if [[ -f "$IMGUI_RUN" ]]; then
        status_info "Launching imgui-browser..."
        bash "$IMGUI_RUN" >/tmp/live-css-browser.log 2>&1 &
        local BROWSER_PID=$!
        sleep 1.0
        if kill -0 "$BROWSER_PID" 2>/dev/null; then
            status_ok "imgui-browser started  (PID $BROWSER_PID)"
        else
            status_fail "imgui-browser exited early -- check /tmp/live-css-browser.log"
            open "$LOGIN_URL" 2>/dev/null || true
        fi
    else
        status_info "imgui-browser/run.sh not found -- opening system browser..."
        open "$LOGIN_URL" 2>/dev/null || printf "  ${C_YELLOW}Visit: %s${R}\n" "$LOGIN_URL"
    fi

    printf "\n"
    box_top
    printf "  ${C_GREY}Servers running. Press Ctrl+C to stop.${R}\n"
    printf "  ${C_GREY}Auth log    : /tmp/live-css-auth.log${R}\n"
    printf "  ${C_GREY}Browser log : /tmp/live-css-browser.log${R}\n"
    box_bot
    printf "\n"
    [[ -n "$AUTH_BASH_PID" ]] && wait "$AUTH_BASH_PID" 2>/dev/null || true
    printf "\n  ${C_GREY}Servers stopped. Press ENTER to return to menu.${R}\n"
    read -r "?" 2>/dev/null || true
}

# ── Cleanup on Ctrl+C ──────────────────────────────────────────────────────────
cleanup() {
    printf "\n\n  ${C_GREY}Shutting down...${R}\n"
    [[ -n "$AUTH_BASH_PID" ]] && kill "$AUTH_BASH_PID" 2>/dev/null || true
    # Kill only the auth Go server -- nginx is managed by launchd and should stay running.
    lsof -iTCP:9100 -sTCP:LISTEN -t 2>/dev/null | xargs kill 2>/dev/null || true
    printf "  ${C_GREEN}Done.${R}\n\n"; exit 0
}
trap cleanup INT TERM

# ── Maximize Terminal window ───────────────────────────────────────────────────
if command -v osascript &>/dev/null; then
    osascript 2>/dev/null <<'APPLESCRIPT'
tell application "Terminal"
    set zoomed of front window to true
end tell
APPLESCRIPT
fi

# ── Main loop ──────────────────────────────────────────────────────────────────
while true; do
    clear

    # Banner
    if [[ -f "$DIR/my_project/xcm-moon-ascii.txt" ]]; then
        _moon_colors=('\033[38;5;54m' '\033[38;5;54m' '\033[38;5;99m' '\033[38;5;99m' '\033[38;5;99m'
                      '\033[38;5;141m' '\033[38;5;141m' '\033[38;5;141m' '\033[38;5;189m' '\033[38;5;189m')
        _ci=0; _total=${#_moon_colors[@]}
        while IFS= read -r _mline; do
            _col="${_moon_colors[$(( (_ci % _total) + 1 ))]}"
            printf "${_col}${BOLD}%s${R}\n" "$_mline"
            (( _ci++ )); sleep 0.06
        done < "$DIR/my_project/xcm-moon-ascii.txt"
    fi

    box_top
    printf "  ${C_VIOLET}${BOLD}LIVE CSS EDITOR${R}   ${C_GREY}Native App Launcher${R}\n"
    box_mid
    printf "  ${C_GREY}Project :${R}  ${C_WHITE}$DIR${R}\n"
    printf "  ${C_GREY}Date    :${R}  ${C_WHITE}$(date '+%Y-%m-%d  %H:%M:%S')${R}\n"
    box_mid
    box_empty
    box_section "ADMIN PANEL"
    box_empty
    box_item "1" "Start"       "Start auth servers + dev browser  (login.php)"
    box_item "2" "Server"     "Start nginx + PHP-FPM server stack  (:8443 HTTPS)"
    box_item "3" "Stop"        "Stop server stack (nginx + auth)"
    box_empty
    box_mid
    box_section "PUSH"
    box_empty
    box_item "4" "Push live"   "Commit and push Live CSS Editor to GitHub"
    box_item "5" "Push moon"   "Commit and push moon-lang  (xcm-editor repo)"
    box_item "6" "Push gram"   "Commit and push gram-model (gramcheck)"
    box_item "7" "Push all"    "Push all 3 repos in sequence"
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
    printf "  ${C_GREY}Choice  [ 1-7 / t / a / q ] :${R}  "
    read -r CHOICE

    case "$CHOICE" in

        1)
            printf "\n"; step "Starting admin panel..."
            _start_admin
            ;;

        2)
            printf "\n"; step "Starting nginx + PHP-FPM server stack..."
            bash "$DIR/server/start.sh" \
                || printf "  ${C_RED}ERROR: server/start.sh failed${R}\n" >&2
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        3)
            printf "\n"; step "Stopping server stack (nginx + PHP-FPM) and auth (:9100)..."
            [[ -n "$AUTH_BASH_PID" ]] && kill "$AUTH_BASH_PID" 2>/dev/null || true
            AUTH_BASH_PID=""
            bash "$DIR/server/stop.sh" --kill \
                || printf "  ${C_RED}ERROR: server/stop.sh failed${R}\n" >&2
            for _port in 9100; do
                local _pids
                _pids=$(lsof -iTCP:"$_port" -sTCP:LISTEN -t 2>/dev/null || true)
                if [[ -n "$_pids" ]]; then
                    echo "$_pids" | xargs kill 2>/dev/null || true
                    status_ok "Killed process(es) on :$_port"
                else
                    status_info "Nothing listening on :$_port"
                fi
            done
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        4)
            printf "\n"
            bash "$DIR/push.sh" \
                || printf "  ${C_RED}ERROR: push.sh failed${R}\n" >&2
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        5)
            printf "\n"
            _push_repo \
                "/Users/mac/Desktop/xcm-editor" \
                "moon-lang/" \
                "Update moon-lang" \
                "moon-lang"
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        6)
            printf "\n"
            _push_repo \
                "/Users/mac/Documents/literature-in-ascii/co-edit-model/gramcheck" \
                "." \
                "Update gramcheck" \
                "gram-model"
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        7)
            printf "\n"
            step "Push All -- Live CSS Editor  /  moon-lang  /  gram-model"
            printf "\n  ${C_GREY}Single commit message for all 3 repos${R}\n"
            printf "  ${C_GREY}(blank = each repo uses its own default):${R}  "
            read -r ALL_MSG
            PUSH_ERRORS=0

            step "1 / 3  Live CSS Editor..."
            (
                cd "$DIR" || exit 1
                printf "  ${C_GREY}Running: git add .${R}\n"
                _add_out="$(git add . 2>&1)"
                _add_rc=$?
                [[ -n "$_add_out" ]] && printf '%s\n' "$_add_out" | while IFS= read -r _l; do printf "    ${C_GREY}%s${R}\n" "$_l"; done
                if [[ $_add_rc -ne 0 ]]; then
                    printf "  ${C_RED}ERROR: git add failed (exit %s)${R}\n" "$_add_rc" >&2; exit 1
                fi
                if git diff --cached --quiet; then
                    printf "  ${C_YELLOW}Nothing new to commit (Live CSS Editor)${R}\n"
                else
                    printf "  ${C_GREY}Staged files:${R}\n"
                    git diff --cached --name-only 2>/dev/null | while read -r f; do printf "    ${C_LAVENDER}%s${R}\n" "$f"; done
                    _msg="${ALL_MSG:-Update Live CSS Editor}"
                    git commit -m "$_msg" 2>&1 || { printf "  ${C_RED}ERROR: commit failed${R}\n" >&2; exit 1; }
                fi
                _br="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || printf 'main')"
                printf "  ${C_GREY}Running: git push -v origin %s${R}\n" "$_br"
                _pout="$(git push -v origin "$_br" 2>&1)"
                _prc=$?
                printf '%s\n' "$_pout" | while IFS= read -r _l; do printf "    ${C_GREY}%s${R}\n" "$_l"; done
                if [[ $_prc -ne 0 ]]; then
                    printf "  ${C_RED}ERROR: push failed (exit %s)${R}\n" "$_prc" >&2; exit 1
                fi
                printf "  ${C_GREEN}Live CSS Editor pushed${R}\n"
            ) || PUSH_ERRORS=$((PUSH_ERRORS + 1))

            step "2 / 3  moon-lang..."
            _push_repo "/Users/mac/Desktop/xcm-editor" "moon-lang/" "${ALL_MSG:-Update moon-lang}" "moon-lang" "no" \
                || PUSH_ERRORS=$((PUSH_ERRORS + 1))

            step "3 / 3  gram-model..."
            _push_repo "/Users/mac/Documents/literature-in-ascii/co-edit-model/gramcheck" "." "${ALL_MSG:-Update gramcheck}" "gram-model" "no" \
                || PUSH_ERRORS=$((PUSH_ERRORS + 1))

            printf "\n"
            if [[ $PUSH_ERRORS -eq 0 ]]; then
                printf "  ${C_GREEN}${BOLD}All 3 repos pushed successfully.${R}\n"
            else
                printf "  ${C_RED}${BOLD}%d repo(s) had errors -- check output above.${R}\n" "$PUSH_ERRORS" >&2
            fi
            printf "\n"; read -r "?Press ENTER to return..."; ;;

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
                    printf "  ${C_RED}ERROR: python3 not found${R}\n" >&2
                    read -r "?Press ENTER to return..."; break
                fi
                LINES_PY="$DIR/dev-tools/code-review/lines_count.py"
                SEC_PY="$DIR/dev-tools/code-review/security_ck.py"

                case "$TOOL_CHOICE" in
                    a)
                        step "lines_count.py -- this project"
                        [[ -f "$LINES_PY" ]] && "$PY3" "$LINES_PY" "$DIR" 2>&1 \
                            || printf "  ${C_RED}ERROR: lines_count.py not found${R}\n" >&2
                        printf "\n"; read -r "?Press ENTER to continue...";;
                    l)
                        step "lines_count.py -- custom directory"
                        printf "  ${C_GREY}Directory (blank = this project, b = cancel):${R}  "; read -r SCAN_DIR
                        [[ "$SCAN_DIR" == "b" ]] && continue
                        [[ -z "$SCAN_DIR" ]] && SCAN_DIR="$DIR"
                        printf "  ${C_GREY}Threshold (blank = 1000):${R}  "; read -r THRESH; [[ -z "$THRESH" ]] && THRESH=1000
                        [[ -f "$LINES_PY" && -d "$SCAN_DIR" ]] && "$PY3" "$LINES_PY" "$SCAN_DIR" "$THRESH" 2>&1 \
                            || printf "  ${C_RED}ERROR${R}\n" >&2
                        printf "\n"; read -r "?Press ENTER to continue...";;
                    c)
                        step "security_ck.py -- this project"
                        [[ -f "$SEC_PY" ]] && "$PY3" "$SEC_PY" "$DIR" --no-color 2>&1 \
                            || printf "  ${C_RED}ERROR: security_ck.py not found${R}\n" >&2
                        printf "\n"; read -r "?Press ENTER to continue...";;
                    d)
                        step "security_ck.py -- moon-lang"
                        MOON_SCAN="/Users/mac/Desktop/xcm-editor/moon-lang"
                        [[ -f "$SEC_PY" && -d "$MOON_SCAN" ]] && "$PY3" "$SEC_PY" "$MOON_SCAN" --no-color 2>&1 \
                            || printf "  ${C_RED}ERROR${R}\n" >&2
                        printf "\n"; read -r "?Press ENTER to continue...";;
                    e)
                        step "security_ck.py -- gram-model"
                        GRAM_SCAN="/Users/mac/Documents/literature-in-ascii/co-edit-model/gramcheck"
                        [[ -f "$SEC_PY" && -d "$GRAM_SCAN" ]] && "$PY3" "$SEC_PY" "$GRAM_SCAN" --no-color 2>&1 \
                            || printf "  ${C_RED}ERROR${R}\n" >&2
                        printf "\n"; read -r "?Press ENTER to continue...";;
                    j)
                        step "security_ck.py --json -- this project"
                        printf "  ${C_GREY}Output file (blank = screen):${R}  "; read -r JSON_OUT
                        if [[ -f "$SEC_PY" ]]; then
                            if [[ -n "$JSON_OUT" ]]; then
                                "$PY3" "$SEC_PY" "$DIR" --json > "$JSON_OUT" 2>&1 \
                                    && printf "  ${C_GREEN}JSON written to: %s${R}\n" "$JSON_OUT" \
                                    || printf "  ${C_RED}ERROR${R}\n" >&2
                            else
                                "$PY3" "$SEC_PY" "$DIR" --json 2>&1
                            fi
                        else
                            printf "  ${C_RED}ERROR: security_ck.py not found${R}\n" >&2
                        fi
                        printf "\n"; read -r "?Press ENTER to continue...";;
                    b|back) break;;
                    *) printf "  ${C_YELLOW}Unknown choice: %s${R}\n" "$TOOL_CHOICE";;
                esac
            done;;

        a|A)
            printf "\n"; step "Opening Agent Flow..."
            _open_agent_flow \
                || printf "  ${C_RED}ERROR: could not open agent-flow${R}\n" >&2
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        q|Q|"")
            printf "\n  ${C_GREY}Exiting.${R}\n\n"; exit 0;;

        *)
            printf "\n  ${C_YELLOW}Unknown choice '%s' -- returning to menu.${R}\n\n" "$CHOICE"
            sleep 1;;
    esac
done
