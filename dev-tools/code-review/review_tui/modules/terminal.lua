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

local TAB_H = 24
local INPUT_H = 24
local PAD_X = 8
local SCROLL_STEP = 4
local MAX_LINES = 3000

local _tab_geom = {}
local _plus_geom = nil
local _close_geom = nil

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

local function normalize_dir(path)
    local p = trim(path)
    if p == "" then return default_cwd end
    if IS_WIN then
        p = p:gsub("/", "\\")
        -- relative: not a drive letter path and not UNC
        if not p:match("^%a:\\\\") and not p:match("^\\\\\\\\") then
            p = default_cwd:gsub("\\\\+$", "") .. "\\" .. p
        end
        p = p:gsub("\\\\+$", "")
        if p == "" then p = "C:\\" end
        return p
    else
        if p == "~" then return os.getenv("HOME") or default_cwd end
        if p:sub(1, 1) ~= "/" then
            p = default_cwd:gsub("/+$", "") .. "/" .. p
        end
        p = p:gsub("/+$", "")
        if p == "" then p = "/" end
        return p
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
    }
    tab_seq = tab_seq + 1
    return t
end

local function active_tab()
    return tabs[cur_idx]
end

local function add_line(t, line)
    t.lines[#t.lines + 1] = tostring(line or "")
    if #t.lines > MAX_LINES then
        local drop = #t.lines - MAX_LINES
        for _ = 1, drop do table.remove(t.lines, 1) end
    end
end

local function set_scroll_bottom(t, h, font)
    local line_h = font:getHeight()
    local out_h = h - TAB_H - INPUT_H - 2
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
        -- absolute: starts with drive letter or UNC
        if target:match("^%a:\\\\") or target:match("^\\\\\\\\") then
            return target
        end
        return base:gsub("\\\\+$", "") .. "\\" .. target
    else
        local home = os.getenv("HOME") or base
        if target == "" or target == "~" then return home end
        if target:sub(1, 1) == "/" then return target end
        if target:sub(1, 2) == "~/" then return home .. "/" .. target:sub(3) end
        return base:gsub("/+$", "") .. "/" .. target
    end
end

local function run_command(t, command, h, font)
    local cmd = trim(command)
    if cmd == "" then return end

    t.history[#t.history + 1] = cmd
    t.hist_idx = #t.history + 1
    add_line(t, t.cwd .. " $ " .. cmd)

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

    local shell_cmd
    if IS_WIN then
        -- PowerShell: change to cwd (single-quoted, ' escaped as '') then run cmd
        local safe_cwd = t.cwd:gsub("'", "''")
        shell_cmd = "powershell -NoProfile -NonInteractive -Command "
                 .. '"Set-Location \'' .. safe_cwd .. "\'; " .. cmd:gsub('"', '\\"') .. '"'
    else
        shell_cmd = "cd " .. shell_escape(t.cwd) .. " && " .. cmd .. " 2>&1"
    end
    local has_output = false
    for line in WP.lines_live(shell_cmd) or (function() return nil end) do
        add_line(t, line)
        has_output = true
    end
    if not has_output then add_line(t, "") end

    set_scroll_bottom(t, h, font)
    save_state()
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
    local tx = x + 6
    love.graphics.setFont(font)
    for i, tab in ipairs(tabs) do
        local label = tab.name
        local tw = font:getWidth(label) + 24
        local hov = false
        local active = i == cur_idx
        love.graphics.setColor(active and C.accent or C.panel_bg)
        love.graphics.rectangle("fill", tx, y + 2, tw, TAB_H - 4, 3)
        love.graphics.setColor(active and C.text_bright or C.text)
        love.graphics.rectangle("line", tx, y + 2, tw, TAB_H - 4, 3)
        love.graphics.print(label, tx + 8, y + 6)
        _tab_geom[#_tab_geom + 1] = { x = tx, y = y + 2, w = tw, h = TAB_H - 4 }
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

    -- cwd line
    love.graphics.setColor(C.dim)
    love.graphics.print("cwd: " .. t.cwd, x + PAD_X, y + TAB_H + 4)

    -- output
    local out_y = y + TAB_H + 20
    local out_h = h - TAB_H - INPUT_H - 20
    local line_h = font:getHeight()
    local vis = math.max(1, math.floor(out_h / line_h))
    local max_scr = math.max(0, #t.lines - vis)
    if t.scroll > max_scr then t.scroll = max_scr end
    if t.scroll < 0 then t.scroll = 0 end

    love.graphics.setScissor(x + 2, out_y, w - 4, out_h)
    local start = t.scroll + 1
    local stop = math.min(#t.lines, t.scroll + vis)
    local py = out_y
    for i = start, stop do
        local line = tostring(t.lines[i] or "")
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

return M