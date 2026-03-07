-- modules/editor.lua
-- Multi-tab file editor for the Code Review TUI.
--
-- Globals expected from main.lua:
--   W, H, MENUBAR_H, TOOLBAR_H, SUMMARY_H
--   C (colour table), font_sm, font_ui
--   fill_rect, stroke_rect, text_at, gc
--
-- Public API
--   M.open_file(path)              open or switch to a file tab
--   M.draw(x, y, w, h)             draw tab bar + editor content
--   M.keypressed(key)              returns true if consumed
--   M.textinput(t)                 returns true if consumed
--   M.mousepressed(mx, my, btn)    returns true if consumed
--   M.wheelmoved(wx, wy, mx, my)   returns true if consumed
--   M.active_tab()                 returns current tab object or nil
--   M.has_tabs()                   true when at least one file is open

local M = {}

-- ─────────────────────────────────────────────
-- Constants
-- ─────────────────────────────────────────────
local CHAR_H    = 16
local _last_h   = 480   -- cache of last draw height for use in textinput
local GUTTER    = 50    -- line-number column width
local TAB_H     = 26    -- tab bar height
local CTX_W     = 178
local CTX_ITEM_H= 22

local CTX_ITEMS = {
    { label = "Select All",  hint = "Ctrl+A" },
    { label = "Copy",        hint = "Ctrl+C" },
    { label = "Cut",         hint = "Ctrl+X" },
    { label = "Paste",       hint = "Ctrl+V" },
}

-- ─────────────────────────────────────────────
-- Tab state
-- ─────────────────────────────────────────────
M.tabs    = {}   -- array of tab objects (see new_tab)
M.cur_idx = 0    -- 1..#tabs; 0 means no tab focused (should not occur when tabs exist)

local function new_tab(path, lines)
    return {
        filepath = path,
        lines    = lines,
        cursor   = { line = 1, col = 1 },
        scroll   = 0,
        modified = false,
        sel      = nil,   -- { s={line,col}, e={line,col} }
        _ctx     = nil,   -- right-click context menu position
    }
end

function M.has_tabs()
    return #M.tabs > 0
end

function M.active_tab()
    return M.tabs[M.cur_idx]
end

-- ─────────────────────────────────────────────
-- UTF-8 safety (copied pattern from reference)
-- ─────────────────────────────────────────────
local function _seq_len(b)
    if b < 0x80 then return 1
    elseif b < 0xC0 then return nil
    elseif b < 0xE0 then return 2
    elseif b < 0xF0 then return 3
    elseif b < 0xF5 then return 4
    else return nil end
end

local function _valid_seq(s, i, len)
    for j = i + 1, i + len - 1 do
        local c = s:byte(j)
        if not c or c < 0x80 or c > 0xBF then return false end
    end
    return true
end

local function _utf8_safe(s)
    if type(s) ~= "string" then return tostring(s or "") end
    if not s:find("[\x80-\xFF]") then return s end
    local out = {}
    local i, n = 1, #s
    while i <= n do
        local b   = s:byte(i)
        local len = _seq_len(b)
        if len and i + len - 1 <= n and _valid_seq(s, i, len) then
            out[#out + 1] = s:sub(i, i + len - 1)
            i = i + len
        else
            i = i + 1
        end
    end
    return table.concat(out)
end

-- ─────────────────────────────────────────────
-- Simple syntax highlighter
-- Returns list of { text, col_key } tokens
-- ─────────────────────────────────────────────
local KEYWORDS = {
    lua     = { "and","break","do","else","elseif","end","false","for","function","goto","if",
                "in","local","nil","not","or","repeat","return","then","true","until","while" },
    python  = { "False","None","True","and","as","assert","async","await","break","class","continue",
                "def","del","elif","else","except","finally","for","from","global","if","import",
                "in","is","lambda","nonlocal","not","or","pass","raise","return","try","while","with","yield" },
    js      = { "break","case","catch","class","const","continue","debugger","default","delete","do",
                "else","export","extends","false","finally","for","function","if","import","in",
                "instanceof","let","new","null","return","static","super","switch","this","throw",
                "true","try","typeof","undefined","var","void","while","with","yield","async","await" },
    php     = { "echo","print","if","else","elseif","while","for","foreach","break","continue",
                "return","function","class","new","extends","implements","namespace","use","true",
                "false","null","public","private","protected","static","abstract","final","try",
                "catch","throw","interface","trait" },
}
local KW_SETS = {}
for lang, words in pairs(KEYWORDS) do
    KW_SETS[lang] = {}
    for _, w in ipairs(words) do KW_SETS[lang][w] = true end
end

local EXT_LANG = {
    lua="lua", py="python", js="js", ts="js", jsx="js", tsx="js", php="php",
    c="js", h="js", cpp="js", cs="js",
    sh="python", bash="python",
}

local function ext_of(path)
    return (path:match("%.([^%.]+)$") or ""):lower()
end

local function tokenize_line(line, ext)
    local lang  = EXT_LANG[ext] or "js"
    local kw    = KW_SETS[lang] or {}
    local out   = {}
    local i     = 1
    local n     = #line

    -- comment prefixes per lang
    local lc = (lang == "lua")    and "--"
            or (lang == "python") and "#"
            or "//"

    while i <= n do
        -- line comment
        if line:sub(i, i + #lc - 1) == lc then
            out[#out+1] = { text = line:sub(i), col = "grey" }
            break
        end

        -- string
        local q = line:sub(i, i)
        if q == '"' or q == "'" then
            local j = i + 1
            while j <= n do
                if line:sub(j, j) == "\\" then j = j + 2
                elseif line:sub(j, j) == q then j = j + 1; break
                else j = j + 1 end
            end
            out[#out+1] = { text = line:sub(i, j - 1), col = "green" }
            i = j

        -- number
        elseif line:sub(i, i):match("[0-9]") then
            local j = i
            while j <= n and line:sub(j, j):match("[0-9%.x]") do j = j + 1 end
            out[#out+1] = { text = line:sub(i, j - 1), col = "cyan" }
            i = j

        -- word / keyword
        elseif line:sub(i, i):match("[%a_]") then
            local j = i
            while j <= n and line:sub(j, j):match("[%w_]") do j = j + 1 end
            local word = line:sub(i, j - 1)
            local col  = kw[word] and "violet" or "text"
            out[#out+1] = { text = word, col = col }
            i = j

        else
            -- single char punctuation / space
            local char = line:sub(i, i)
            local col  = "dim"
            if char:match("[(){}%[%]<>]") then col = "lavender" end
            out[#out+1] = { text = char, col = col }
            i = i + 1
        end
    end
    return out
end

-- ─────────────────────────────────────────────
-- File I/O
-- ─────────────────────────────────────────────
function M.open_file(path)
    -- already open? switch to it
    for i, t in ipairs(M.tabs) do
        if t.filepath == path then
            M.cur_idx = i
            return
        end
    end
    local f = io.open(path, "r")
    if not f then return end
    local lines = {}
    for line in f:lines() do lines[#lines+1] = line end
    f:close()
    if #lines == 0 then lines = { "" } end
    local tab = new_tab(path, lines)
    M.tabs[#M.tabs+1] = tab
    M.cur_idx = #M.tabs
end

local function save_tab(t)
    local f = io.open(t.filepath, "w")
    if not f then return end
    for i, ln in ipairs(t.lines) do
        f:write(ln)
        if i < #t.lines then f:write("\n") end
    end
    f:close()
    t.modified = false
end

local function close_tab(idx)
    table.remove(M.tabs, idx)
    if #M.tabs == 0 then
        M.cur_idx = 0
    elseif M.cur_idx > #M.tabs then
        M.cur_idx = #M.tabs
    end
end

-- ─────────────────────────────────────────────
-- Selection helpers
-- ─────────────────────────────────────────────
local function sel_norm(t)
    if not t.sel then return nil end
    local s, e = t.sel.s, t.sel.e
    if s.line < e.line or (s.line == e.line and s.col <= e.col) then
        return s.line, s.col, e.line, e.col
    else
        return e.line, e.col, s.line, s.col
    end
end

local function sel_text(t)
    local sl, sc, el, ec = sel_norm(t)
    if not sl then return nil end
    if sl == el then return t.lines[sl]:sub(sc, ec - 1) end
    local parts = { t.lines[sl]:sub(sc) }
    for li = sl + 1, el - 1 do parts[#parts+1] = t.lines[li] end
    parts[#parts+1] = t.lines[el]:sub(1, ec - 1)
    return table.concat(parts, "\n")
end

local function delete_sel(t)
    local sl, sc, el, ec = sel_norm(t)
    if not sl then return end
    if sl == el then
        t.lines[sl] = t.lines[sl]:sub(1, sc - 1) .. t.lines[sl]:sub(ec)
    else
        local new_line = t.lines[sl]:sub(1, sc - 1) .. t.lines[el]:sub(ec)
        for _ = sl + 1, el do table.remove(t.lines, sl + 1) end
        t.lines[sl] = new_line
    end
    t.cursor.line = sl
    t.cursor.col  = sc
    t.sel         = nil
    t.modified    = true
end

-- ─────────────────────────────────────────────
-- Cursor helpers
-- ─────────────────────────────────────────────
local function clamp_col(t)
    local max_col = #t.lines[t.cursor.line] + 1
    if t.cursor.col > max_col then t.cursor.col = max_col end
    if t.cursor.col < 1       then t.cursor.col = 1 end
end

local function visible_lines(h)
    return math.floor((h - TAB_H) / CHAR_H)
end

local function ensure_scroll(t, h)
    local vis = visible_lines(h)
    if t.cursor.line < t.scroll + 1 then
        t.scroll = t.cursor.line - 1
    elseif t.cursor.line > t.scroll + vis then
        t.scroll = t.cursor.line - vis
    end
    if t.scroll < 0 then t.scroll = 0 end
end

local function col_px(line_str, col)
    return GUTTER + font_sm:getWidth(_utf8_safe(line_str:sub(1, col - 1)))
end

local function paste_into(t, clip, h)
    if not clip or clip == "" then return end
    if t.sel then delete_sel(t) end
    local paste_lines = {}
    for ln in (clip .. "\n"):gmatch("([^\n]*)\n") do
        paste_lines[#paste_lines+1] = ln
    end
    local L, CO = t.cursor.line, t.cursor.col
    if #paste_lines == 1 then
        t.lines[L] = t.lines[L]:sub(1, CO-1) .. paste_lines[1] .. t.lines[L]:sub(CO)
        t.cursor.col = CO + #paste_lines[1]
    else
        local after = t.lines[L]:sub(CO)
        t.lines[L]  = t.lines[L]:sub(1, CO-1) .. paste_lines[1]
        for pi = 2, #paste_lines - 1 do
            table.insert(t.lines, L + pi - 1, paste_lines[pi])
        end
        local last = paste_lines[#paste_lines]
        table.insert(t.lines, L + #paste_lines - 1, last .. after)
        t.cursor.line = L + #paste_lines - 1
        t.cursor.col  = #last + 1
    end
    t.modified = true
    ensure_scroll(t, h)
end

-- ─────────────────────────────────────────────
-- DRAW
-- ─────────────────────────────────────────────

-- tab button geometry cache: { { x, w, close_x } ... }
local _tab_geom = {}

function M.draw(x, y, w, h)
    if not M.has_tabs() then return end
    _last_h = h

    local t = M.active_tab()

    -- ---- tab bar ----
    love.graphics.setColor(C.menu_bg)
    fill_rect(x, y, w, TAB_H)
    love.graphics.setColor(C.border)
    love.graphics.line(x, y + TAB_H, x + w, y + TAB_H)

    _tab_geom = {}
    local tx = x + 4
    love.graphics.setFont(font_sm)
    for i, tab in ipairs(M.tabs) do
        local name   = (tab.filepath:match("([^/]+)$") or tab.filepath)
        local label  = (tab.modified and "*" or "") .. name
        local lw     = font_sm:getWidth(label)
        local close_w= font_sm:getWidth("x") + 8
        local tab_w  = lw + close_w + 20
        local active = (i == M.cur_idx)

        _tab_geom[i] = { x = tx, w = tab_w, close_x = tx + lw + 16 }

        if active then
            love.graphics.setColor(C.panel_bg)
            fill_rect(tx, y + 1, tab_w, TAB_H - 1, 3)
            love.graphics.setColor(C.accent)
            fill_rect(tx, y, tab_w, 2)
        else
            love.graphics.setColor(C.bg)
            fill_rect(tx, y + 3, tab_w, TAB_H - 3, 3)
        end

        love.graphics.setColor(active and C.text_bright or C.dim)
        love.graphics.setFont(font_sm)
        love.graphics.print(label, tx + 8, y + 5)

        -- close X
        love.graphics.setColor(active and C.grey or C.dark)
        local cx = tx + lw + 16
        love.graphics.print("x", cx, y + 5)

        tx = tx + tab_w + 2
    end

    -- ---- editor area ----
    local ey    = y + TAB_H
    local eh    = h - TAB_H
    local vis   = math.floor(eh / CHAR_H)
    local ext   = ext_of(t and t.filepath or "")

    love.graphics.setColor(C.bg)
    fill_rect(x, ey, w, eh)

    if not t then return end

    -- clamp scroll
    local max_scr = math.max(0, #t.lines - vis)
    if t.scroll > max_scr then t.scroll = max_scr end

    love.graphics.setScissor(x, ey, w, eh)
    love.graphics.setFont(font_sm)

    local draw_y = ey + 4
    for li = t.scroll + 1, math.min(t.scroll + vis + 1, #t.lines) do
        local line   = t.lines[li] or ""
        local is_cur = (li == t.cursor.line)

        -- current line highlight
        if is_cur then
            love.graphics.setColor(0.12, 0.06, 0.22, 1)
            fill_rect(x, draw_y - 2, w, CHAR_H + 2)
        end

        -- selection highlight
        local sl, sc, el, ec = sel_norm(t)
        if sl and li >= sl and li <= el then
            local px_s = (li == sl) and col_px(line, sc) or (x + GUTTER)
            local px_e = (li == el) and col_px(line, ec)
                         or (x + GUTTER + font_sm:getWidth(_utf8_safe(line)) + font_sm:getWidth(" "))
            if px_e > px_s then
                love.graphics.setColor(0.33, 0.0, 0.8, 0.3)
                fill_rect(px_s, draw_y - 2, px_e - px_s, CHAR_H + 2)
            end
        end

        -- gutter
        love.graphics.setColor(C.panel_bg[1] or 0.065, C.panel_bg[2] or 0.04, C.panel_bg[3] or 0.14, 1)
        fill_rect(x, draw_y - 2, GUTTER - 4, CHAR_H + 2)
        local ln_str = tostring(li)
        local ln_w   = font_sm:getWidth(ln_str)
        love.graphics.setColor(is_cur and C.lavender or C.grey)
        love.graphics.setFont(font_sm)
        love.graphics.print(ln_str, x + GUTTER - 6 - ln_w, draw_y + 1)

        -- syntax tokens
        local toks = tokenize_line(line, ext)
        local text_x = x + GUTTER
        for _, tok in ipairs(toks) do
            local safe = _utf8_safe(tok.text)
            if safe ~= "" then
                love.graphics.setColor(C[tok.col] or C.text)
                love.graphics.setFont(font_sm)
                love.graphics.print(safe, text_x, draw_y)
                text_x = text_x + font_sm:getWidth(safe)
            end
        end

        -- cursor caret
        if is_cur then
            local before = _utf8_safe(line:sub(1, t.cursor.col - 1))
            local cx2    = x + GUTTER + font_sm:getWidth(before)
            local blink  = math.floor(love.timer.getTime() * 2) % 2 == 0
            if blink then
                love.graphics.setColor(C.text_bright)
                fill_rect(cx2, draw_y, 2, CHAR_H)
            end
        end

        draw_y = draw_y + CHAR_H
    end

    love.graphics.setScissor()

    -- footer bar: filename + position
    love.graphics.setColor(C.menu_bg)
    fill_rect(x, y + h - 20, w, 20)
    love.graphics.setColor(C.border)
    love.graphics.line(x, y + h - 20, x + w, y + h - 20)
    love.graphics.setFont(font_sm)
    love.graphics.setColor(C.dim)
    local pos = "Ln " .. t.cursor.line .. "  Col " .. t.cursor.col
             .. "  |  " .. (ext ~= "" and ext:upper() or "TXT")
             .. "  |  " .. t.filepath
    love.graphics.print(pos, x + 8, y + h - 16)

    -- context menu
    if t._ctx then
        local cmx     = math.min(t._ctx.x, x + w - CTX_W - 4)
        local cmy     = math.min(t._ctx.y, y + h - #CTX_ITEMS * CTX_ITEM_H - 4)
        local total_h = #CTX_ITEMS * CTX_ITEM_H + 6
        local mmx, mmy = love.mouse.getPosition()
        love.graphics.setColor(C.menu_bg)
        fill_rect(cmx, cmy, CTX_W, total_h, 4)
        love.graphics.setColor(C.border)
        stroke_rect(cmx, cmy, CTX_W, total_h, 4)
        love.graphics.setFont(font_sm)
        for ci, item in ipairs(CTX_ITEMS) do
            local ity = cmy + 3 + (ci - 1) * CTX_ITEM_H
            local hov = mmx >= cmx and mmx < cmx + CTX_W and mmy >= ity and mmy < ity + CTX_ITEM_H
            if hov then
                love.graphics.setColor(C.accent)
                fill_rect(cmx + 2, ity, CTX_W - 4, CTX_ITEM_H, 3)
            end
            love.graphics.setColor(hov and C.text_bright or C.text)
            love.graphics.print(item.label, cmx + 10, ity + 4)
            local hw = font_sm:getWidth(item.hint)
            love.graphics.setColor(C.dim)
            love.graphics.print(item.hint, cmx + CTX_W - hw - 10, ity + 4)
        end
    end
end

-- ─────────────────────────────────────────────
-- Shift-selection helpers
-- ─────────────────────────────────────────────
-- Called before a movement when shift is held: anchors sel.s at current cursor
local function sel_start(t)
    if not t.sel then
        t.sel = {
            s = { line = t.cursor.line, col = t.cursor.col },
            e = { line = t.cursor.line, col = t.cursor.col },
        }
    end
end

-- Called after a movement: extend sel.e to cursor if shift held, else clear sel
local function sel_finish(t, shift)
    if shift then
        if t.sel then
            t.sel.e = { line = t.cursor.line, col = t.cursor.col }
        end
    else
        t.sel = nil
    end
end

-- ─────────────────────────────────────────────
-- KEYPRESSED
-- ─────────────────────────────────────────────
function M.keypressed(key, h)
    if not M.has_tabs() then return false end
    local t    = M.active_tab()
    if not t   then return false end

    local cmd   = love.keyboard.isDown("lgui","rgui")
    local ctrl  = love.keyboard.isDown("lctrl","rctrl") or cmd
    local shift = love.keyboard.isDown("lshift","rshift")
    local L     = t.cursor.line
    local CO    = t.cursor.col

    -- close context menu on any key
    if t._ctx then t._ctx = nil end

    -- Cmd+Left / Cmd+Right: switch tabs
    if cmd and not love.keyboard.isDown("lctrl","rctrl") then
        if key == "left" then
            if M.cur_idx > 1 then M.cur_idx = M.cur_idx - 1 end
            return true
        elseif key == "right" then
            if M.cur_idx < #M.tabs then M.cur_idx = M.cur_idx + 1 end
            return true
        end
    end

    if key == "escape" then
        if t.sel then t.sel = nil return true end
        return false  -- let main handle

    elseif key == "s" and ctrl then
        save_tab(t); return true

    elseif key == "w" and ctrl then
        close_tab(M.cur_idx); return true

    elseif key == "a" and ctrl then
        t.sel = { s = { line = 1, col = 1 },
                  e = { line = #t.lines, col = #t.lines[#t.lines] + 1 } }
        return true

    elseif key == "c" and ctrl then
        local txt = sel_text(t) or t.lines[t.cursor.line]
        if txt then love.system.setClipboardText(txt) end
        return true

    elseif key == "x" and ctrl then
        local txt = sel_text(t)
        if txt then
            love.system.setClipboardText(txt)
            delete_sel(t)
            ensure_scroll(t, h)
        end
        return true

    elseif key == "v" and ctrl then
        paste_into(t, love.system.getClipboardText(), h); return true

    elseif key == "return" or key == "kpenter" then
        if t.sel then delete_sel(t) end
        local before  = t.lines[L]:sub(1, CO - 1)
        local after   = t.lines[L]:sub(CO)
        local indent  = before:match("^(%s*)") or ""
        t.lines[L]    = before
        table.insert(t.lines, L + 1, indent .. after)
        t.cursor.line = L + 1
        t.cursor.col  = #indent + 1
        t.modified    = true
        ensure_scroll(t, h); return true

    elseif key == "backspace" then
        if t.sel then delete_sel(t); ensure_scroll(t, h); return true end
        if ctrl then
            local b     = t.lines[L]:sub(1, CO - 1)
            local new_b = b:match("^(.*%s)%S+%s*$") or b:match("^(.-)%S+$") or ""
            t.lines[L]   = new_b .. t.lines[L]:sub(CO)
            t.cursor.col = #new_b + 1
        elseif CO > 1 then
            t.lines[L]   = t.lines[L]:sub(1, CO - 2) .. t.lines[L]:sub(CO)
            t.cursor.col = CO - 1
        elseif L > 1 then
            local prev   = #t.lines[L - 1]
            t.lines[L-1] = t.lines[L-1] .. t.lines[L]
            table.remove(t.lines, L)
            t.cursor.line = L - 1
            t.cursor.col  = prev + 1
        end
        t.modified = true
        ensure_scroll(t, h); return true

    elseif key == "delete" then
        if t.sel then delete_sel(t); ensure_scroll(t, h); return true end
        if CO <= #t.lines[L] then
            t.lines[L] = t.lines[L]:sub(1, CO-1) .. t.lines[L]:sub(CO+1)
            t.modified = true
        elseif L < #t.lines then
            t.lines[L] = t.lines[L] .. t.lines[L+1]
            table.remove(t.lines, L+1)
            t.modified = true
        end
        return true

    elseif key == "tab" then
        if t.sel then delete_sel(t) end
        local spaces = "    "
        t.lines[L]   = t.lines[L]:sub(1, CO-1) .. spaces .. t.lines[L]:sub(CO)
        t.cursor.col = CO + #spaces
        t.modified   = true; return true

    elseif key == "up" then
        -- shift = turbo: jump 8 lines; ctrl = jump 8 lines; plain = 1 line
        local step = (shift or ctrl) and 8 or 1
        t.cursor.line = math.max(1, t.cursor.line - step)
        clamp_col(t); ensure_scroll(t, h); return true

    elseif key == "down" then
        local step = (shift or ctrl) and 8 or 1
        t.cursor.line = math.min(#t.lines, t.cursor.line + step)
        clamp_col(t); ensure_scroll(t, h); return true

    elseif key == "left" then
        -- shift or ctrl = word jump left
        if shift or ctrl then
            local line = t.lines[t.cursor.line]
            local col  = t.cursor.col - 1
            if col < 1 then
                if t.cursor.line > 1 then
                    t.cursor.line = t.cursor.line - 1
                    t.cursor.col  = #t.lines[t.cursor.line] + 1
                end
            else
                while col >= 1 and line:sub(col, col):match("%s") do col = col - 1 end
                while col >= 1 and not line:sub(col, col):match("%s") do col = col - 1 end
                t.cursor.col = col + 1
            end
        else
            if t.cursor.col > 1 then
                t.cursor.col = t.cursor.col - 1
            elseif t.cursor.line > 1 then
                t.cursor.line = t.cursor.line - 1
                t.cursor.col  = #t.lines[t.cursor.line] + 1
            end
        end
        ensure_scroll(t, h); return true

    elseif key == "right" then
        -- shift or ctrl = word jump right
        if shift or ctrl then
            local line = t.lines[t.cursor.line]
            local len  = #line
            local col  = t.cursor.col
            if col > len then
                if t.cursor.line < #t.lines then
                    t.cursor.line = t.cursor.line + 1
                    t.cursor.col  = 1
                end
            else
                while col <= len and not line:sub(col, col):match("%s") do col = col + 1 end
                while col <= len and line:sub(col, col):match("%s") do col = col + 1 end
                t.cursor.col = col
            end
        else
            if t.cursor.col <= #t.lines[t.cursor.line] then
                t.cursor.col = t.cursor.col + 1
            elseif t.cursor.line < #t.lines then
                t.cursor.line = t.cursor.line + 1
                t.cursor.col  = 1
            end
        end
        ensure_scroll(t, h); return true

    elseif key == "home" then
        -- shift = jump to very first line; plain = smart home
        if shift then
            t.cursor.line = 1
            t.cursor.col  = 1
        else
            local indent_end = (t.lines[t.cursor.line]:match("^%s*()") or 1)
            t.cursor.col = (t.cursor.col == indent_end) and 1 or indent_end
        end
        ensure_scroll(t, h); return true

    elseif key == "end" then
        -- shift = jump to very last line; plain = end of current line
        if shift then
            t.cursor.line = #t.lines
            t.cursor.col  = #t.lines[#t.lines] + 1
        else
            t.cursor.col = #t.lines[t.cursor.line] + 1
        end
        ensure_scroll(t, h); return true

    elseif key == "pageup" then
        local vis = visible_lines(h)
        t.cursor.line = math.max(1, t.cursor.line - vis)
        clamp_col(t); ensure_scroll(t, h); return true

    elseif key == "pagedown" then
        local vis = visible_lines(h)
        t.cursor.line = math.min(#t.lines, t.cursor.line + vis)
        clamp_col(t); ensure_scroll(t, h); return true
    end

    return false
end

-- ─────────────────────────────────────────────
-- TEXTINPUT
-- ─────────────────────────────────────────────
function M.textinput(char)
    if not M.has_tabs() then return false end
    local t = M.active_tab()
    if not t then return false end
    if t.sel then delete_sel(t) end
    local L, CO = t.cursor.line, t.cursor.col
    t.lines[L]   = t.lines[L]:sub(1, CO-1) .. char .. t.lines[L]:sub(CO)
    t.cursor.col = CO + #char
    t.modified   = true
    ensure_scroll(t, _last_h)
    return true
end

-- ─────────────────────────────────────────────
-- MOUSE
-- ─────────────────────────────────────────────
function M.mousepressed(mx, my, btn, px, py, pw, ph)
    -- px,py,pw,ph = editor panel position/size from main
    if not M.has_tabs() then return false end

    -- ---- tab bar clicks ----
    if my >= py and my < py + TAB_H then
        for i, geom in ipairs(_tab_geom) do
            if mx >= geom.x and mx < geom.x + geom.w then
                -- close button
                local xbtn_x = geom.close_x
                if btn == 1 and mx >= xbtn_x and mx < xbtn_x + 14 then
                    close_tab(i)
                    return true
                end
                -- switch tab
                if btn == 1 then
                    M.cur_idx = i
                    return true
                end
            end
        end
        return false
    end

    local t = M.active_tab()
    if not t then return false end

    local ey = py + TAB_H

    -- outside editor area
    if my < ey or my >= py + ph - 20 then return false end

    -- right-click: context menu
    if btn == 2 then
        t._ctx = { x = mx, y = my }
        return true
    end

    -- left-click: handle context menu or move cursor
    if btn == 1 then
        if t._ctx then
            local cmx = math.min(t._ctx.x, px + pw - CTX_W - 4)
            local cmy = math.min(t._ctx.y, py + ph - #CTX_ITEMS * CTX_ITEM_H - 4)
            if mx >= cmx and mx < cmx + CTX_W then
                local ci = math.floor((my - cmy - 3) / CTX_ITEM_H) + 1
                if ci >= 1 and ci <= #CTX_ITEMS then
                    t._ctx = nil
                    if ci == 1 then  -- Select All
                        t.sel = { s={line=1,col=1}, e={line=#t.lines, col=#t.lines[#t.lines]+1} }
                    elseif ci == 2 then  -- Copy
                        local txt = sel_text(t) or t.lines[t.cursor.line]
                        if txt then love.system.setClipboardText(txt) end
                    elseif ci == 3 then  -- Cut
                        local txt = sel_text(t)
                        if txt then love.system.setClipboardText(txt); delete_sel(t) end
                    elseif ci == 4 then  -- Paste
                        paste_into(t, love.system.getClipboardText(), ph)
                    end
                    return true
                end
            end
            t._ctx = nil
            -- fall through to cursor placement
        end

        -- map click to line
        local clicked_line = t.scroll + 1 + math.floor((my - ey - 4) / CHAR_H)
        clicked_line = math.max(1, math.min(#t.lines, clicked_line))
        t.cursor.line = clicked_line

        -- map click to column
        local line    = t.lines[clicked_line] or ""
        local safe    = _utf8_safe(line)
        local rel_x   = mx - (px + GUTTER)
        if rel_x <= 0 then
            t.cursor.col = 1
        else
            local best = #safe + 1
            local acc  = 0
            local ci   = 1
            while ci <= #safe do
                local b   = safe:byte(ci)
                local len = _seq_len(b) or 1
                if ci + len - 1 > #safe then break end
                local ch = safe:sub(ci, ci + len - 1)
                local cw = font_sm:getWidth(ch)
                if rel_x < acc + cw / 2 then best = ci; break end
                acc = acc + cw
                ci  = ci + len
            end
            t.cursor.col = best
        end
        t.sel = nil
        clamp_col(t)
        ensure_scroll(t, ph)
        return true
    end

    return false
end

function M.wheelmoved(wy, px, py, pw, ph)
    if not M.has_tabs() then return false end
    local t   = M.active_tab()
    if not t  then return false end
    local vis = visible_lines(ph)
    local max_scr = math.max(0, #t.lines - vis)
    t.scroll  = math.max(0, math.min(max_scr, t.scroll - wy * 3))
    return true
end

return M
