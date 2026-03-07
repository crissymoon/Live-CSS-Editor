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
    local PROJECT_DIR="$DIR/my_project"
    local PROJECT_PORT=8080
    local PROJECT_URL="http://localhost:${PROJECT_PORT}/"

    # Start PHP built-in dev server for my_project/ if not already listening
    if ! _port_open "$PROJECT_PORT"; then
        status_info "Starting PHP dev server on port ${PROJECT_PORT}..."
        php -S "127.0.0.1:${PROJECT_PORT}" -t "$PROJECT_DIR" >/tmp/php-dev-server.log 2>&1 &
        local PHP_SRV_PID=$!
        local _pw=0
        while (( _pw < 20 )); do
            _port_open "$PROJECT_PORT" && break
            sleep 0.1; (( _pw++ ))
        done
        _port_open "$PROJECT_PORT" \
            && status_ok "PHP dev server ready  :${PROJECT_PORT}  (PID $PHP_SRV_PID)" \
            || status_fail "PHP dev server did not start -- see /tmp/php-dev-server.log"
    else
        status_ok "PHP dev server already running  :${PROJECT_PORT}"
    fi

    if [[ -f "$IMGUI_RUN" ]]; then
        status_info "Launching imgui-browser (grab bar)..."
        bash "$IMGUI_RUN" --url "$PROJECT_URL" --ui-mode grab_bar_only >/tmp/live-css-browser.log 2>&1 &
        local BROWSER_PID=$!
        sleep 1.0
        if kill -0 "$BROWSER_PID" 2>/dev/null; then
            status_ok "imgui-browser started  (PID $BROWSER_PID)"
        else
            status_fail "imgui-browser exited early -- check /tmp/live-css-browser.log"
            open "$PROJECT_URL" 2>/dev/null || true
        fi
    else
        status_info "imgui-browser/run.sh not found -- opening system browser..."
        open "$PROJECT_URL" 2>/dev/null || printf "  ${C_YELLOW}Visit: %s${R}\n" "$PROJECT_URL"
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

# ── Full browser open (starts servers, then uses system browser) ────────────────
_open_full_browser() {
    if _port_open 8443 && _port_open 9100; then
        status_ok "Servers already running  (nginx :8443, auth :9100)"
    else
        local AUTH_SCRIPT="$DIR/page-builder/pb_admin/start-auth.sh"
        [[ ! -f "$AUTH_SCRIPT" ]] && die "page-builder/pb_admin/start-auth.sh not found"
        status_info "Starting servers..."
        bash "$AUTH_SCRIPT" >/tmp/live-css-auth.log 2>&1 &
        AUTH_BASH_PID=$!
        spin "Waiting for servers..." &
        local SPIN_PID=$!
        local _i=0
        while (( _i < 200 )); do
            _port_open 8443 && break
            sleep 0.2; (( _i++ ))
        done
        kill "$SPIN_PID" 2>/dev/null; wait "$SPIN_PID" 2>/dev/null
        printf "\r%72s\r" ""
        _port_open 8443 && status_ok "Server ready  :8443" \
                        || status_fail "Server did not start -- see /tmp/live-css-auth.log"
    fi
    local URL="https://localhost:8443/"
    local IMGUI_RUN="$DIR/imgui-browser/run.sh"
    if [[ -f "$IMGUI_RUN" ]]; then
        status_info "Launching imgui-browser (full toolbar)..."
        bash "$IMGUI_RUN" --url "$URL" --ui-mode full >/tmp/live-css-browser.log 2>&1 &
        local BROWSER_PID=$!
        sleep 1.0
        if kill -0 "$BROWSER_PID" 2>/dev/null; then
            status_ok "imgui-browser started  (PID $BROWSER_PID)"
        else
            status_fail "imgui-browser exited early -- check /tmp/live-css-browser.log"
            open "$URL" 2>/dev/null || printf "  ${C_YELLOW}Visit: %s${R}\n" "$URL"
        fi
    else
        printf "  ${C_GREY}Opening: %s${R}\n" "$URL"
        open "$URL" 2>/dev/null || printf "  ${C_YELLOW}Visit: %s${R}\n" "$URL"
    fi
    printf "\n"
    printf "  ${C_GREY}Browser opened. Press ENTER to return to menu.${R}\n"
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
    # ── Load launcher.json each loop so edits are picked up right away ──────────
    PUSH_LABELS=(); PUSH_DESCS=(); PUSH_PATHS=(); PUSH_STAGES=(); PUSH_MSGS=()
    TOOL_LABELS=(); TOOL_DESCS=(); TOOL_SCRIPTS=()
    if [[ -f "$DIR/launcher.json" ]]; then
        while IFS=$'\t' read -r _l _d _p _s _m; do
            PUSH_LABELS+=("$_l"); PUSH_DESCS+=("$_d")
            PUSH_PATHS+=("$_p"); PUSH_STAGES+=("$_s"); PUSH_MSGS+=("$_m")
        done < <(python3 -c "
import json,sys
try:
    d=json.load(open(sys.argv[1]))
    pd=sys.argv[2]
    for r in d.get('push_repos',[]):
        p=r.get('path','').replace('__project__',pd)
        row=[r.get('label',''),r.get('description',''),p,r.get('stage','.'),r.get('default_message','Update')]
        print('\t'.join(c.replace('\t','').replace('\n','') for c in row))
except Exception:
    pass
" "$DIR/launcher.json" "$DIR" 2>/dev/null)
        while IFS=$'\t' read -r _l _d _sc; do
            TOOL_LABELS+=("$_l"); TOOL_DESCS+=("$_d"); TOOL_SCRIPTS+=("$_sc")
        done < <(python3 -c "
import json,sys
try:
    d=json.load(open(sys.argv[1]))
    pd=sys.argv[2]
    for t in d.get('tools',[]):
        sc=t.get('script','').replace('__project__',pd)
        row=[t.get('label',''),t.get('description',''),sc]
        print('\t'.join(c.replace('\t','').replace('\n','') for c in row))
except Exception:
    pass
" "$DIR/launcher.json" "$DIR" 2>/dev/null)
    fi

    box_empty
    box_section "MY PROJECT"
    box_empty
    box_item "1" "Grab bar"      "Open with just a small title bar"
    box_item "2" "Full browser"  "Open in a full browser window"
    box_item "3" "Server"        "Start the web server  (nginx + PHP)"
    box_item "4" "Stop"          "Stop the web server"
    box_empty
    box_mid
    box_section "PUSH"
    box_empty
    box_item "p" "Push repos"    "Commit and push your repos to GitHub"
    box_empty
    box_mid
    box_section "TOOLS"
    box_empty
    box_item "t" "Tools"         "Code review, DB browser, and more"
    box_item "a" "Agent Flow"    "Start agent-flow UI at localhost:9090"
    box_empty
    box_mid
    box_item "q" "Quit"          ""
    box_bot
    printf "\n"
    printf "  ${C_GREY}Choice  [ 1-4 / p / t / a / q ] :${R}  "
    read -r CHOICE

    case "$CHOICE" in

        1)
            printf "\n"; step "Opening My Project -- grab bar..."
            _start_admin
            ;;

        2)
            printf "\n"; step "Opening My Project -- full browser..."
            _open_full_browser
            ;;

        3)
            printf "\n"; step "Starting web server  (nginx + PHP)..."
            bash "$DIR/server/start.sh" \
                || printf "  ${C_RED}ERROR: server/start.sh failed${R}\n" >&2
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        4)
            printf "\n"; step "Stopping web server and auth..."
            [[ -n "$AUTH_BASH_PID" ]] && kill "$AUTH_BASH_PID" 2>/dev/null || true
            AUTH_BASH_PID=""
            bash "$DIR/server/stop.sh" --kill \
                || printf "  ${C_RED}ERROR: server/stop.sh failed${R}\n" >&2
            for _port in 9100; do
                _pids=$(lsof -iTCP:"$_port" -sTCP:LISTEN -t 2>/dev/null || true)
                if [[ -n "$_pids" ]]; then
                    echo "$_pids" | xargs kill 2>/dev/null || true
                    status_ok "Stopped process(es) on :$_port"
                else
                    status_info "Nothing running on :$_port"
                fi
            done
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        p|P)
            while true; do
                printf "\n"
                box_top
                box_section "PUSH REPOS"
                box_empty
                if [[ ${#PUSH_LABELS[@]} -eq 0 ]]; then
                    printf "  ${C_YELLOW}No repos found in launcher.json${R}\n"
                    printf "  ${C_GREY}Open launcher.json and add items under push_repos.${R}\n"
                else
                    _pi=1
                    while (( _pi <= ${#PUSH_LABELS[@]} )); do
                        box_item "$_pi" "${PUSH_LABELS[$_pi]}" "${PUSH_DESCS[$_pi]}"
                        (( _pi++ ))
                    done
                    box_empty
                    box_item "a" "Push all" "Push every repo listed above at once"
                fi
                box_empty
                box_item "b" "Back" "Return to main menu"
                box_bot
                printf "\n  ${C_GREY}Push  [ 1-${#PUSH_LABELS[@]} / a / b ] :${R}  "
                read -r PUSH_CHOICE

                case "$PUSH_CHOICE" in
                    a|A)
                        printf "\n"
                        step "Push All"
                        printf "\n  ${C_GREY}One commit message for all repos (blank = each uses its own default):${R}  "
                        read -r _all_msg
                        _push_errors=0
                        _pi=1
                        while (( _pi <= ${#PUSH_LABELS[@]} )); do
                            step "${_pi} / ${#PUSH_LABELS[@]}  ${PUSH_LABELS[$_pi]}..."
                            if [[ "${PUSH_PATHS[$_pi]}" == "$DIR" ]]; then
                                (
                                    cd "$DIR" || exit 1
                                    _add_out="$(git add . 2>&1)"; _add_rc=$?
                                    [[ -n "$_add_out" ]] && printf '%s\n' "$_add_out" | while IFS= read -r _l; do printf "    ${C_GREY}%s${R}\n" "$_l"; done
                                    if [[ $_add_rc -ne 0 ]]; then
                                        printf "  ${C_RED}ERROR: git add failed${R}\n" >&2; exit 1
                                    fi
                                    if git diff --cached --quiet; then
                                        printf "  ${C_YELLOW}Nothing new to commit${R}\n"
                                    else
                                        _msg="${_all_msg:-${PUSH_MSGS[$_pi]}}"
                                        git commit -m "$_msg" 2>&1 || { printf "  ${C_RED}ERROR: commit failed${R}\n" >&2; exit 1; }
                                    fi
                                    _br="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || printf 'main')"
                                    _pout="$(git push -v origin "$_br" 2>&1)"; _prc=$?
                                    printf '%s\n' "$_pout" | while IFS= read -r _l; do printf "    ${C_GREY}%s${R}\n" "$_l"; done
                                    [[ $_prc -ne 0 ]] && { printf "  ${C_RED}ERROR: push failed${R}\n" >&2; exit 1; }
                                    printf "  ${C_GREEN}Done${R}\n"
                                ) || _push_errors=$(( _push_errors + 1 ))
                            else
                                _push_repo "${PUSH_PATHS[$_pi]}" "${PUSH_STAGES[$_pi]}" \
                                    "${_all_msg:-${PUSH_MSGS[$_pi]}}" "${PUSH_LABELS[$_pi]}" "no" \
                                    || _push_errors=$(( _push_errors + 1 ))
                            fi
                            (( _pi++ ))
                        done
                        printf "\n"
                        if [[ $_push_errors -eq 0 ]]; then
                            printf "  ${C_GREEN}${BOLD}All repos pushed.${R}\n"
                        else
                            printf "  ${C_RED}${BOLD}%d repo(s) had errors -- check output above.${R}\n" "$_push_errors" >&2
                        fi
                        printf "\n"; read -r "?Press ENTER to return..."; ;;

                    b|B|"") break ;;

                    *)
                        if [[ "$PUSH_CHOICE" =~ ^[0-9]+$ ]]; then
                            _idx=$(( PUSH_CHOICE ))
                            if (( _idx >= 1 && _idx <= ${#PUSH_LABELS[@]} )); then
                                printf "\n"
                                _push_repo "${PUSH_PATHS[$_idx]}" "${PUSH_STAGES[$_idx]}" \
                                    "${PUSH_MSGS[$_idx]}" "${PUSH_LABELS[$_idx]}"
                            else
                                printf "  ${C_YELLOW}No repo at that number.  Add more repos in launcher.json.${R}\n"
                                sleep 1
                            fi
                        fi
                        printf "\n"; read -r "?Press ENTER to return..."; ;;
                esac
            done ;;

        t|T)
            while true; do
                printf "\n"
                box_top
                box_section "TOOLS"
                box_empty
                if [[ ${#TOOL_LABELS[@]} -eq 0 ]]; then
                    printf "  ${C_YELLOW}No tools found in launcher.json${R}\n"
                    printf "  ${C_GREY}Open launcher.json and add items under tools.${R}\n"
                else
                    _ti=1
                    while (( _ti <= ${#TOOL_LABELS[@]} )); do
                        box_item "$_ti" "${TOOL_LABELS[$_ti]}" "${TOOL_DESCS[$_ti]}"
                        (( _ti++ ))
                    done
                fi
                box_empty
                box_item "b" "Back" "Return to main menu"
                box_bot
                printf "\n  ${C_GREY}Tools  [ 1-${#TOOL_LABELS[@]} / b ] :${R}  "
                read -r TOOL_CHOICE

                case "$TOOL_CHOICE" in
                    b|B|"") break ;;
                    *)
                        if [[ "$TOOL_CHOICE" =~ ^[0-9]+$ ]]; then
                            _tidx=$(( TOOL_CHOICE ))
                            if (( _tidx >= 1 && _tidx <= ${#TOOL_LABELS[@]} )); then
                                _tscript="${TOOL_SCRIPTS[$_tidx]}"
                                printf "\n"; step "${TOOL_LABELS[$_tidx]}..."
                                if [[ -f "$_tscript" ]]; then
                                    bash "$_tscript" 2>&1 \
                                        || printf "  ${C_RED}ERROR: script exited with errors${R}\n" >&2
                                else
                                    printf "  ${C_RED}Script not found: %s${R}\n" "$_tscript" >&2
                                    printf "  ${C_GREY}Check the script path in launcher.json${R}\n"
                                fi
                            else
                                printf "  ${C_YELLOW}No tool at that number.  Add more tools in launcher.json.${R}\n"
                                sleep 1
                            fi
                        fi
                        printf "\n"; read -r "?Press ENTER to continue..."; ;;
                esac
            done ;;

        a|A)
            printf "\n"; step "Opening Agent Flow..."
            _open_agent_flow \
                || printf "  ${C_RED}ERROR: could not open agent-flow${R}\n" >&2
            printf "\n"; read -r "?Press ENTER to return..."; ;;

        q|Q|"")
            printf "\n  ${C_GREY}Exiting.${R}\n\n"; exit 0;;

        *)
            printf "\n  ${C_YELLOW}Unknown choice '%s' -- try again.${R}\n\n" "$CHOICE"
            sleep 1;;
    esac
done
