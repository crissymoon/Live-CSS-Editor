-- modules/terminal.lua
-- In-app terminal panel with persistent tabs and per-tab cwd.

local M = {}

local WP     = require "modules.winpipe"
local IS_WIN = love.system.getOS() == "Windows"
local C = nil
local visible = false
local default_cwd = IS_WIN
    and (os.getenv("USERPROFILE") or os.getenv("HOME") or "C:\\")
    or  (os.getenv("HOME") or "/")

local tabs = {}
local cur_idx = 1
local tab_seq = 1

-- Async command state (one command at a time across all tabs).
local _term_job_id = 0
local _term_chan    = nil
local _term_thread  = nil
local _term_tab     = nil   -- tab that owns the running command

local TERM_WORKER = [[
local cmd, chan_name = ...
local chan = love.thread.getChannel(chan_name)
local WP   = require "modules.winpipe"
local iter = WP.lines_live(cmd)
if not iter then
    chan:push("ERR:Failed to start: " .. tostring(cmd))
    chan:push("__DONE__")
    return
end
for line in iter do
    chan:push(line)
end
chan:push("__DONE__")
]]

local TAB_H = 24
local INPUT_H = 24
local PAD_X = 8
local SCROLL_STEP = 4
local MAX_LINES = 3000

local _tab_geom       = {}
local _tab_close_geom = {}   -- per-tab × button hit areas
local _plus_geom  = nil
local _close_geom = nil

-- Text selection state for copy-from-terminal.
-- l1/c1 = anchor, l2/c2 = drag end (either may come first in reading order).
-- char offsets are 1-based Lua string indices.
local _sel = { tab_idx = 0, l1 = 0, c1 = 0, l2 = 0, c2 = 0, dragging = false }
-- Geometry captured during M.draw so mouse handlers can do hit-testing.
local _draw_state = nil

local function state_path()
    local base = os.getenv("CODE_REVIEW_DIR") or "."
    return base .. "/.terminal_state.lua"
end

local function shell_escape(s)
    local str = tostring(s or "")
    if IS_WIN then
        return '"' .. str:gsub('"', '\\"') .. '"'
    else
        return "'" .. str:gsub("'", "'\\''" ) .. "'"
    end
end

local function trim(s)
    return tostring(s or ""):gsub("^%s+", ""):gsub("%s+$", "")
end

-- Collapse . and .. segments in an already-absolute path.
local function collapse_dots(p)
    if IS_WIN then
        local drive = p:match("^%a:\\") or p:match("^\\\\[^\\]+\\[^\\]+\\")
        if not drive then return p end
        local rest = p:sub(#drive + 1)
        local parts = {}
        for seg in rest:gmatch("[^\\]+") do
            if seg == ".." then
                if #parts > 0 then parts[#parts] = nil end
            elseif seg ~= "." then
                parts[#parts + 1] = seg
            end
        end
        local result = drive .. table.concat(parts, "\\")
        -- Remove trailing backslash unless it is the drive root itself.
        result = result:gsub("\\+$", "")
        return result ~= "" and result or drive:sub(1, -2)
    else
        local is_abs = p:sub(1, 1) == "/"
        local parts = {}
        for seg in p:gmatch("[^/]+") do
            if seg == ".." then
                if #parts > 0 then parts[#parts] = nil end
            elseif seg ~= "." then
                parts[#parts + 1] = seg
            end
        end
        local result = table.concat(parts, "/")
        if is_abs then return "/" .. result end
        return result ~= "" and result or "."
    end
end

-- Return the 1-based char index in `line` at or just after pixel offset `rel_x`.
local function char_at_x(font, line, rel_x)
    local n = #line
    if rel_x <= 0 then return 1 end
    for i = 1, n do
        if font:getWidth(line:sub(1, i)) > rel_x then return i end
    end
    return n + 1
end

-- Return selection bounds in reading order: sl,sc,el,ec.
local function sel_normalize()
    if _sel.l1 < _sel.l2 or (_sel.l1 == _sel.l2 and _sel.c1 <= _sel.c2) then
        return _sel.l1, _sel.c1, _sel.l2, _sel.c2
    end
    return _sel.l2, _sel.c2, _sel.l1, _sel.c1
end

-- Detect a venv activation command and return the venv root path, or nil.
-- Handles: `source path`, `. path`, `call path`, bare path.
-- The activate script must be named `activate*` inside Scripts/ or bin/.
local function detect_venv_activation(cmd, cwd)
    local raw = cmd:match("^[Ss]ource%s+(.+)$")
             or cmd:match("^%.%s+(.+)$")
             or cmd:match("^[Cc]all%s+(.+)$")
             or cmd
    raw = trim(raw)
    raw = raw:match('^"(.-)"$') or raw:match("^'(.-)'$") or raw
    local fname = raw:match("[/\\]([^/\\]+)$") or raw
    if not fname:lower():match("^activate") then return nil end
    -- Resolve to absolute path.
    local full
    if IS_WIN then
        raw = raw:gsub("/", "\\")
        if raw:match("^%a:\\") or raw:match("^\\\\") then
            full = raw
        else
            full = cwd:gsub("\\+$", "") .. "\\" .. raw
        end
    else
        if raw:sub(1, 1) == "/" then
            full = raw
        elseif raw:sub(1, 2) == "~/" then
            full = (os.getenv("HOME") or cwd) .. "/" .. raw:sub(3)
        else
            full = cwd:gsub("/+$", "") .. "/" .. raw
        end
    end
    full = collapse_dots(full)
    -- Venv root = parent of Scripts/ or bin/ directory.
    return full:match("^(.+)[/\\][Ss]cripts[/\\][^/\\]+$")
        or full:match("^(.+)[/\\][Bb]in[/\\][^/\\]+$")
end

local function normalize_dir(path)
    local p = trim(path)
    if p == "" then return default_cwd end
    if IS_WIN then
        p = p:gsub("/", "\\")
        -- relative: not a drive letter path and not UNC
        if not p:match("^%a:\\") and not p:match("^\\\\") then
            p = default_cwd:gsub("\\\\+$", "") .. "\\" .. p
        end
        p = p:gsub("\\\\+$", "")
        if p == "" then p = "C:\\" end
        return collapse_dots(p)
    else
        if p == "~" then return os.getenv("HOME") or default_cwd end
        if p:sub(1, 1) ~= "/" then
            p = default_cwd:gsub("/+$", "") .. "/" .. p
        end
        p = p:gsub("/+$", "")
        if p == "" then p = "/" end
        return collapse_dots(p)
    end
end

local function new_tab(cwd)
    local t = {
        name = "Term " .. tostring(tab_seq),
        cwd = normalize_dir(cwd or default_cwd),
        lines = {},
        input = "",
        scroll = 0,
        history = {},
        hist_idx = 0,
        active_venv = nil,
    }
    tab_seq = tab_seq + 1
    return t
end

local function active_tab()
    return tabs[cur_idx]
end

-- Strip ANSI/VT100 escape sequences from a terminal output line,
-- collapse carriage-return overwrite sequences, and drop any invalid
-- UTF-8 bytes so love.graphics.print never crashes on subprocess output.
local function strip_ansi(s)
    -- CSI sequences: ESC [ <params> <final-letter>  (colours, cursor moves, etc.)
    s = s:gsub("\027%[[^%a]*%a", "")
    -- Remaining two-char ESC sequences (ESC c, ESC M, etc.)
    s = s:gsub("\027.", "")
    -- Carriage-return overwrite: keep only text after the last \r
    s = s:gsub("^.*\r", "")

    -- Drop invalid UTF-8 bytes (fast-path: skip if pure ASCII)
    local n = #s
    local i = 1
    while i <= n do
        if s:byte(i) >= 0x80 then break end
        i = i + 1
    end
    if i <= n then
        local out = {}
        i = 1
        while i <= n do
            local b = s:byte(i)
            if b < 0x80 then
                out[#out+1] = s:sub(i,i); i = i+1
            elseif b >= 0xC2 and b <= 0xDF and i+1 <= n then
                local b2 = s:byte(i+1)
                if b2 >= 0x80 and b2 <= 0xBF then out[#out+1]=s:sub(i,i+1); i=i+2
                else i=i+1 end
            elseif b >= 0xE0 and b <= 0xEF and i+2 <= n then
                local b2,b3 = s:byte(i+1),s:byte(i+2)
                if b2>=0x80 and b2<=0xBF and b3>=0x80 and b3<=0xBF then
                    out[#out+1]=s:sub(i,i+2); i=i+3
                else i=i+1 end
            elseif b >= 0xF0 and b <= 0xF4 and i+3 <= n then
                local b2,b3,b4 = s:byte(i+1),s:byte(i+2),s:byte(i+3)
                if b2>=0x80 and b2<=0xBF and b3>=0x80 and b3<=0xBF and b4>=0x80 and b4<=0xBF then
                    out[#out+1]=s:sub(i,i+3); i=i+4
                else i=i+1 end
            else
                i=i+1
            end
        end
        s = table.concat(out)
    end
    return s
end

local function add_line(t, line)
    t.lines[#t.lines + 1] = strip_ansi(tostring(line or ""))
    if #t.lines > MAX_LINES then
        local drop = #t.lines - MAX_LINES
        for _ = 1, drop do table.remove(t.lines, 1) end
    end
end

local function set_scroll_bottom(t, h, font)
    local line_h = font:getHeight()
    -- Must match the output-area height used in M.draw:
    --   out_h = h - TAB_H - INPUT_H - 20  (20 px for the cwd line below the tab bar)
    local out_h = h - TAB_H - INPUT_H - 20
    local vis = math.max(1, math.floor(out_h / line_h))
    local max_scr = math.max(0, #t.lines - vis)
    t.scroll = max_scr
end

local function serialize_string(s)
    return string.format("%q", tostring(s or ""))
end

local function save_state()
    local ok, fh = pcall(io.open, state_path(), "w")
    if not ok or not fh then return end

    fh:write("return {\n")
    fh:write("  visible = ", visible and "true" or "false", ",\n")
    fh:write("  cur_idx = ", tostring(cur_idx), ",\n")
    fh:write("  tab_seq = ", tostring(tab_seq), ",\n")
    fh:write("  tabs = {\n")
    for _, t in ipairs(tabs) do
        fh:write("    {\n")
        fh:write("      name = ", serialize_string(t.name), ",\n")
        fh:write("      cwd = ", serialize_string(t.cwd), ",\n")
        fh:write("      input = ", serialize_string(t.input), ",\n")
        fh:write("      scroll = ", tostring(t.scroll or 0), ",\n")
        fh:write("      hist_idx = ", tostring(t.hist_idx or 0), ",\n")
        fh:write("      lines = {\n")
        for _, ln in ipairs(t.lines or {}) do
            fh:write("        ", serialize_string(ln), ",\n")
        end
        fh:write("      },\n")
        fh:write("      history = {\n")
        for _, cmd in ipairs(t.history or {}) do
            fh:write("        ", serialize_string(cmd), ",\n")
        end
        fh:write("      },\n")
        fh:write("      active_venv = ", t.active_venv and serialize_string(t.active_venv) or "nil", ",\n")
        fh:write("    },\n")
    end
    fh:write("  },\n")
    fh:write("}\n")
    fh:close()
end

local function load_state()
    local chunk = loadfile(state_path())
    if not chunk then return false end
    local ok, data = pcall(chunk)
    if not ok or type(data) ~= "table" then return false end

    tabs = {}
    visible = data.visible and true or false
    cur_idx = tonumber(data.cur_idx) or 1
    tab_seq = tonumber(data.tab_seq) or 1

    if type(data.tabs) == "table" then
        for _, src in ipairs(data.tabs) do
            if type(src) == "table" then
                tabs[#tabs + 1] = {
                    name = tostring(src.name or ("Term " .. tostring(#tabs + 1))),
                    cwd = normalize_dir(src.cwd or default_cwd),
                    lines = type(src.lines) == "table" and src.lines or {},
                    input = tostring(src.input or ""),
                    scroll = tonumber(src.scroll) or 0,
                    history = type(src.history) == "table" and src.history or {},
                    hist_idx = tonumber(src.hist_idx) or 0,
                    active_venv = type(src.active_venv) == "string" and src.active_venv or nil,
                }
            end
        end
    end

    if #tabs == 0 then
        tabs = { new_tab(default_cwd) }
        cur_idx = 1
    end
    if cur_idx < 1 then cur_idx = 1 end
    if cur_idx > #tabs then cur_idx = #tabs end
    return true
end

local function resolve_cd(base, arg)
    local target = trim(arg)
    if IS_WIN then
        if target == "" then return base end
        target = target:gsub("/", "\\")
        local raw
        -- absolute: starts with drive letter or UNC
        if target:match("^%a:\\") or target:match("^\\\\") then
            raw = target
        else
            raw = base:gsub("\\\\+$", "") .. "\\" .. target
        end
        return collapse_dots(raw)
    else
        local home = os.getenv("HOME") or base
        if target == "" or target == "~" then return home end
        local raw
        if target:sub(1, 1) == "/" then
            raw = target
        elseif target:sub(1, 2) == "~/" then
            raw = home .. "/" .. target:sub(3)
        else
            raw = base:gsub("/+$", "") .. "/" .. target
        end
        return collapse_dots(raw)
    end
end

local function run_command(t, command, h, font)
    local cmd = trim(command)
    if cmd == "" then return end

    t.history[#t.history + 1] = cmd
    t.hist_idx = #t.history + 1
    local venv_prompt = t.active_venv and "(venv) " or ""
    add_line(t, venv_prompt .. t.cwd .. " $ " .. cmd)

    -- cd handling.
    local cd_target = cmd:match("^cd%s*(.*)$")
    if cd_target ~= nil then
        local target = resolve_cd(t.cwd, cd_target)
        local dir_ok
        if IS_WIN then
            dir_ok = WP.is_dir(target)
        else
            local rc = os.execute("[ -d " .. shell_escape(target) .. " ]")
            dir_ok = (rc == 0 or rc == true)
        end
        if dir_ok then
            t.cwd = target
        else
            add_line(t, "cd: no such directory: " .. target)
        end
        set_scroll_bottom(t, h, font)
        save_state()
        return
    end

    -- deactivate virtual environment.
    if cmd:lower() == "deactivate" then
        if t.active_venv then
            add_line(t, "deactivated: " .. t.active_venv)
            t.active_venv = nil
        else
            add_line(t, "no virtual environment is active")
        end
        set_scroll_bottom(t, h, font)
        save_state()
        return
    end

    -- Virtual environment activation (source/./call + activate script path).
    local venv_root = detect_venv_activation(cmd, t.cwd)
    if venv_root then
        t.active_venv = venv_root
        add_line(t, "activated: " .. venv_root)
        set_scroll_bottom(t, h, font)
        save_state()
        return
    end

    -- Reject if a command is already running.
    if _term_chan then
        add_line(t, "busy: a command is already running")
        set_scroll_bottom(t, h, font)
        return
    end

    -- Strip leading `call` prefix (CMD.exe built-in — not needed in PowerShell).
    local exec_cmd = cmd
    if IS_WIN then
        exec_cmd = exec_cmd:match("^[Cc]all%s+(.+)$") or exec_cmd
    end

    local shell_cmd
    if IS_WIN then
        local safe_cwd = t.cwd:gsub("'", "''")
        -- Prepend venv Scripts to PATH when a venv is active.
        local venv_prefix = ""
        if t.active_venv then
            local vscripts = t.active_venv:gsub("'", "''") .. "\\Scripts"
            venv_prefix = "$env:VIRTUAL_ENV='" .. t.active_venv:gsub("'","''") .. "'; "
                       .. "$env:PATH='" .. vscripts .. ";' + $env:PATH; "
        end
        shell_cmd = "powershell -NoProfile -NonInteractive -Command "
                 .. '"Set-Location \'' .. safe_cwd .. "\'; "
                 .. venv_prefix:gsub('"', '\\"')
                 .. exec_cmd:gsub('"', '\\"') .. '"'
    else
        local venv_prefix = ""
        if t.active_venv then
            local vbin = t.active_venv .. "/bin"
            venv_prefix = "export VIRTUAL_ENV=" .. shell_escape(t.active_venv)
                       .. "; export PATH=" .. shell_escape(vbin) .. ":$PATH; "
        end
        shell_cmd = "cd " .. shell_escape(t.cwd) .. " && " .. venv_prefix .. exec_cmd .. " 2>&1"
    end

    -- Spawn command in a worker thread so the main thread stays responsive.
    _term_job_id = _term_job_id + 1
    local chan_name = "code_review_term_" .. tostring(_term_job_id)
    _term_chan   = love.thread.getChannel(chan_name)
    _term_chan:clear()
    _term_tab    = t
    _term_thread = love.thread.newThread(TERM_WORKER)
    _term_thread:start(shell_cmd, chan_name)
    -- Output is collected by M.poll() called each frame from love.update.
end

function M.init(colours, cwd)
    C = colours
    default_cwd = normalize_dir(cwd or default_cwd)
    if not load_state() then
        tabs = { new_tab(default_cwd) }
        cur_idx = 1
        visible = false
        save_state()
    end
end

function M.set_default_cwd(path)
    default_cwd = normalize_dir(path)
end

function M.toggle()
    visible = not visible
    save_state()
end

function M.set_visible(v)
    visible = not not v
    save_state()
end

function M.is_visible()
    return visible
end

function M.save()
    save_state()
end

function M.draw(x, y, w, h, font)
    if not visible then return end
    local t = active_tab()
    if not t then return end

    love.graphics.setColor(C.bg)
    love.graphics.rectangle("fill", x, y, w, h)

    -- tab bar
    love.graphics.setColor(C.menu_bg)
    love.graphics.rectangle("fill", x, y, w, TAB_H)
    love.graphics.setColor(C.border)
    love.graphics.line(x, y + TAB_H, x + w, y + TAB_H)

    _tab_geom = {}
    _tab_close_geom = {}
    local tx = x + 6
    love.graphics.setFont(font)
    local CLOSE_W = 16   -- width reserved for the × button inside each tab
    for i, tab in ipairs(tabs) do
        local label = tab.name
        local lw = font:getWidth(label)
        local tw = lw + 24 + CLOSE_W
        local active = i == cur_idx
        love.graphics.setColor(active and C.accent or C.panel_bg)
        love.graphics.rectangle("fill", tx, y + 2, tw, TAB_H - 4, 3)
        love.graphics.setColor(active and C.text_bright or C.text)
        love.graphics.rectangle("line", tx, y + 2, tw, TAB_H - 4, 3)
        love.graphics.print(label, tx + 8, y + 6)
        -- × close button
        local cx = tx + tw - CLOSE_W
        love.graphics.setColor(active and C.text_bright or C.dim)
        love.graphics.print("x", cx + 2, y + 6)
        _tab_geom[#_tab_geom + 1]       = { x = tx, y = y + 2, w = tw - CLOSE_W, h = TAB_H - 4 }
        _tab_close_geom[#_tab_close_geom + 1] = { x = cx, y = y + 2, w = CLOSE_W, h = TAB_H - 4 }
        tx = tx + tw + 6
    end

    _plus_geom = { x = tx, y = y + 2, w = 24, h = TAB_H - 4 }
    love.graphics.setColor(C.panel_bg)
    love.graphics.rectangle("fill", _plus_geom.x, _plus_geom.y, _plus_geom.w, _plus_geom.h, 3)
    love.graphics.setColor(C.text)
    love.graphics.rectangle("line", _plus_geom.x, _plus_geom.y, _plus_geom.w, _plus_geom.h, 3)
    love.graphics.print("+", _plus_geom.x + 8, _plus_geom.y + 4)

    _close_geom = { x = x + w - 34, y = y + 2, w = 28, h = TAB_H - 4 }
    love.graphics.setColor(C.panel_bg)
    love.graphics.rectangle("fill", _close_geom.x, _close_geom.y, _close_geom.w, _close_geom.h, 3)
    love.graphics.setColor(C.text)
    love.graphics.rectangle("line", _close_geom.x, _close_geom.y, _close_geom.w, _close_geom.h, 3)
    love.graphics.print("Hide", _close_geom.x + 3, _close_geom.y + 4)

    -- cwd line (show venv tag when active)
    love.graphics.setColor(C.dim)
    local venv_tag = t.active_venv and "(venv) " or ""
    love.graphics.print(venv_tag .. "cwd: " .. t.cwd, x + PAD_X, y + TAB_H + 4)

    -- output
    local out_y = y + TAB_H + 20
    local out_h = h - TAB_H - INPUT_H - 20
    local line_h = font:getHeight()
    local vis = math.max(1, math.floor(out_h / line_h))
    local max_scr = math.max(0, #t.lines - vis)
    if t.scroll > max_scr then t.scroll = max_scr end
    if t.scroll < 0 then t.scroll = 0 end

    -- Capture geometry so mouse handlers can hit-test without needing extra args.
    _draw_state = { x = x, y = y, w = w, h = h,
                    out_y = out_y, out_h = out_h,
                    line_h = line_h, font = font,
                    tab_idx = cur_idx }

    love.graphics.setScissor(x + 2, out_y, w - 4, out_h)
    local start = t.scroll + 1
    local stop = math.min(#t.lines, t.scroll + vis)
    local py = out_y

    -- Determine if there is an active selection on this tab.
    local has_sel = _sel.tab_idx == cur_idx
                 and (_sel.l1 ~= _sel.l2 or _sel.c1 ~= _sel.c2)
    local sl, sc, el, ec = 0, 0, 0, 0
    if has_sel then sl, sc, el, ec = sel_normalize() end

    for i = start, stop do
        local line = tostring(t.lines[i] or "")
        -- Draw selection highlight behind text.
        if has_sel and i >= sl and i <= el then
            local hx1, hx2
            if i == sl and i == el then
                hx1 = font:getWidth(line:sub(1, sc - 1))
                hx2 = font:getWidth(line:sub(1, ec - 1))
            elseif i == sl then
                hx1 = font:getWidth(line:sub(1, sc - 1))
                hx2 = math.max(font:getWidth(line), 4)
            elseif i == el then
                hx1 = 0
                hx2 = font:getWidth(line:sub(1, ec - 1))
            else
                hx1 = 0
                hx2 = math.max(font:getWidth(line), 4)
            end
            if hx2 > hx1 then
                love.graphics.setColor(C.accent[1], C.accent[2], C.accent[3], 0.35)
                love.graphics.rectangle("fill", x + PAD_X + hx1, py, hx2 - hx1, line_h)
            end
        end
        love.graphics.setColor(C.text)
        love.graphics.print(line, x + PAD_X, py)
        py = py + line_h
    end
    love.graphics.setScissor()

    if max_scr > 0 then
        local sb_w = 5
        local sb_x = x + w - sb_w - 3
        local thumb_h = math.max(20, out_h * out_h / math.max(out_h, #t.lines * line_h))
        local ratio = t.scroll / max_scr
        local thumb_y = out_y + (out_h - thumb_h) * ratio
        love.graphics.setColor(C.border)
        love.graphics.rectangle("fill", sb_x, out_y, sb_w, out_h, 2)
        love.graphics.setColor(C.violet)
        love.graphics.rectangle("fill", sb_x, thumb_y, sb_w, thumb_h, 2)
    end

    -- input
    local iy = y + h - INPUT_H
    love.graphics.setColor(C.menu_bg)
    love.graphics.rectangle("fill", x, iy, w, INPUT_H)
    love.graphics.setColor(C.border)
    love.graphics.line(x, iy, x + w, iy)
    love.graphics.setColor(C.text_bright)
    local caret = (math.floor(love.timer.getTime() * 2) % 2 == 0) and "|" or ""
    love.graphics.print("> " .. t.input .. caret, x + PAD_X, iy + 4)
end

function M.mousepressed(mx, my, btn, x, y, w, h)
    if not visible then return false end
    if mx < x or mx >= x + w or my < y or my >= y + h then return false end

    if btn ~= 1 then return true end

    -- Check per-tab × close buttons first.
    for i, g in ipairs(_tab_close_geom) do
        if mx >= g.x and mx < g.x + g.w and my >= g.y and my < g.y + g.h then
            -- Cancel async command if it belongs to this tab.
            if _term_tab == tabs[i] then
                _term_chan   = nil
                _term_thread = nil
                _term_tab    = nil
            end
            if #tabs > 1 then
                table.remove(tabs, i)
                if cur_idx > #tabs then cur_idx = #tabs end
            else
                tabs[1] = new_tab(default_cwd)
                cur_idx = 1
            end
            save_state()
            return true
        end
    end

    for i, g in ipairs(_tab_geom) do
        if mx >= g.x and mx < g.x + g.w and my >= g.y and my < g.y + g.h then
            cur_idx = i
            save_state()
            return true
        end
    end

    if _plus_geom and mx >= _plus_geom.x and mx < _plus_geom.x + _plus_geom.w and my >= _plus_geom.y and my < _plus_geom.y + _plus_geom.h then
        tabs[#tabs + 1] = new_tab(default_cwd)
        cur_idx = #tabs
        save_state()
        return true
    end

    if _close_geom and mx >= _close_geom.x and mx < _close_geom.x + _close_geom.w and my >= _close_geom.y and my < _close_geom.y + _close_geom.h then
        visible = false
        save_state()
        return true
    end

    -- Start a text selection if the click is in the output area.
    if _draw_state then
        local ds = _draw_state
        if my >= ds.out_y and my < ds.out_y + ds.out_h then
            local t2 = tabs[cur_idx]
            if t2 then
                local li = t2.scroll + math.floor((my - ds.out_y) / ds.line_h) + 1
                li = math.max(1, math.min(#t2.lines, li))
                local line = tostring(t2.lines[li] or "")
                local col = char_at_x(ds.font, line, mx - (x + PAD_X))
                _sel = { tab_idx = cur_idx, l1 = li, c1 = col,
                         l2 = li, c2 = col, dragging = true }
            end
        end
    end

    return true
end

function M.keypressed(key, h, font)
    if not visible then return false end
    local t = active_tab()
    if not t then return true end

    local cmd = love.keyboard.isDown("lgui", "rgui")
    local ctrl = love.keyboard.isDown("lctrl", "rctrl") or cmd

    if key == "escape" then
        visible = false
        save_state()
        return true
    elseif ctrl and key == "t" then
        tabs[#tabs + 1] = new_tab(default_cwd)
        cur_idx = #tabs
        save_state()
        return true
    elseif ctrl and key == "w" then
        if #tabs > 1 then
            table.remove(tabs, cur_idx)
            if cur_idx > #tabs then cur_idx = #tabs end
        else
            tabs[1] = new_tab(default_cwd)
            cur_idx = 1
        end
        save_state()
        return true
    elseif ctrl and key == "left" then
        if cur_idx > 1 then cur_idx = cur_idx - 1 end
        save_state()
        return true
    elseif ctrl and key == "right" then
        if cur_idx < #tabs then cur_idx = cur_idx + 1 end
        save_state()
        return true
    elseif ctrl and key == "c" then
        -- Copy selected text, or cancel a running command when nothing is selected.
        local has_sel = _sel.tab_idx == cur_idx
                     and (_sel.l1 ~= _sel.l2 or _sel.c1 ~= _sel.c2)
        if has_sel then
            local sl, sc, el, ec = sel_normalize()
            local parts = {}
            for li = sl, el do
                local line = tostring(t.lines[li] or "")
                if li == sl and li == el then
                    parts[#parts + 1] = line:sub(sc, ec - 1)
                elseif li == sl then
                    parts[#parts + 1] = line:sub(sc)
                elseif li == el then
                    parts[#parts + 1] = line:sub(1, ec - 1)
                else
                    parts[#parts + 1] = line
                end
            end
            love.system.setClipboardText(table.concat(parts, "\n"))
        elseif _term_chan and _term_tab == t then
            _term_chan   = nil
            _term_thread = nil
            _term_tab    = nil
            add_line(t, "^C")
            set_scroll_bottom(t, h, font)
        end
        return true
    elseif key == "return" or key == "kpenter" then
        local cmdline = t.input
        t.input = ""
        run_command(t, cmdline, h, font)
        return true
    elseif key == "backspace" then
        t.input = t.input:sub(1, -2)
        save_state()
        return true
    elseif key == "up" then
        if #t.history > 0 then
            if t.hist_idx <= 1 then t.hist_idx = 1 else t.hist_idx = t.hist_idx - 1 end
            t.input = t.history[t.hist_idx] or ""
        else
            t.scroll = math.max(0, t.scroll - SCROLL_STEP)
        end
        return true
    elseif key == "down" then
        if #t.history > 0 then
            if t.hist_idx < #t.history then
                t.hist_idx = t.hist_idx + 1
                t.input = t.history[t.hist_idx] or ""
            else
                t.hist_idx = #t.history + 1
                t.input = ""
            end
        else
            t.scroll = t.scroll + SCROLL_STEP
        end
        return true
    elseif key == "pageup" then
        t.scroll = math.max(0, t.scroll - 20)
        return true
    elseif key == "pagedown" then
        t.scroll = t.scroll + 20
        return true
    end

    return true
end

-- Extend the current drag selection as the mouse moves.
function M.mousemoved(mx, my)
    if not visible or not _sel.dragging then return end
    if not _draw_state then return end
    local ds = _draw_state
    local t = tabs[cur_idx]
    if not t then return end
    local li = t.scroll + math.floor((my - ds.out_y) / ds.line_h) + 1
    li = math.max(1, math.min(#t.lines, li))
    local line = tostring(t.lines[li] or "")
    local col = char_at_x(ds.font, line, mx - (ds.x + PAD_X))
    _sel.l2 = li
    _sel.c2 = col
end

-- End drag selection.
function M.mousereleased(mx, my, btn)
    if _sel.dragging then _sel.dragging = false end
end

function M.textinput(txt)
    if not visible then return false end
    local t = active_tab()
    if not t then return true end
    if txt and txt ~= "" then
        t.input = t.input .. txt
        save_state()
    end
    return true
end

function M.wheelmoved(wy)
    if not visible then return false end
    local t = active_tab()
    if not t then return true end
    t.scroll = math.max(0, t.scroll - wy * SCROLL_STEP)
    return true
end

function M.poll()
    if not _term_chan then return end
    local t = _term_tab
    local processed = 0
    local max_per_frame = 100
    while processed < max_per_frame do
        local line = _term_chan:pop()
        if not line then break end
        processed = processed + 1
        if line == "__DONE__" then
            _term_chan   = nil
            _term_thread = nil
            _term_tab    = nil
            if t then
                t.scroll = 1e9   -- clamped to max in draw
                save_state()
            end
            return
        elseif line:sub(1, 4) == "ERR:" then
            if t then add_line(t, line:sub(5)) end
        else
            if t then add_line(t, line) end
        end
    end
    -- Auto-scroll to bottom while output is still arriving.
    if t then t.scroll = 1e9 end
end

function M.is_busy()
    return _term_chan ~= nil
end

return M