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

local History = require "modules.history"

-- ---------------------------------------------
-- File-watch helper (LuaJIT FFI _stat64)
-- ---------------------------------------------
local _ffi_ok, ffi = pcall(require, "ffi")
if _ffi_ok then
    pcall(function()
        ffi.cdef[[
            struct __stat64 {
                unsigned __int64 st_dev;
                unsigned __int64 st_ino;
                unsigned short   st_mode;
                short            st_nlink;
                short            st_uid;
                short            st_gid;
                unsigned __int64 st_rdev;
                __int64          st_size;
                __int64          st_atime;
                __int64          st_mtime;
                __int64          st_ctime;
            };
            int _stat64(const char *path, struct __stat64 *buf);
        ]]
    end)
end

local function get_mtime(path)
    if not _ffi_ok then return nil end
    local ok, st = pcall(function()
        local s = ffi.new("struct __stat64")
        if ffi.C._stat64(path, s) == 0 then
            return tonumber(s.st_mtime)
        end
        return nil
    end)
    return ok and st or nil
end

local _watch_accum = 0
local WATCH_INTERVAL = 1.5  -- seconds between disk-mtime polls

-- ---------------------------------------------
-- Constants
-- ---------------------------------------------
local CHAR_H    = 16
local _last_h   = 480   -- cache of last draw height for use in textinput
local GUTTER    = 50    -- line-number column width
local TAB_H     = 26    -- tab bar height
local FOOTER_H  = 20    -- editor footer/status strip
local TOP_PAD   = 4     -- top inset inside editor viewport
local BOTTOM_PAD= 12    -- bottom breathing room above footer
local OVER_ROWS = 2     -- allow scrolling past EOF a little
local CTX_W     = 178
local CTX_ITEM_H= 22
local ARROW_W   = 18   -- width of the left/right scroll-arrow buttons on the tab bar

-- drag-to-select state
local _drag              = false
local _drag_anchor       = nil   -- { line, col }
local _drag_ctx          = nil   -- { px, py, pw, ph }
local _drag_mx           = 0     -- last mouse position during drag (scaled coords)
local _drag_my           = 0
local _drag_scroll_accum = 0     -- fractional line accumulator for smooth auto-scroll
local _request_browser_focus = false

-- tab drag-to-reorder state
local _tab_drag_idx    = nil  -- index of tab currently being dragged (nil = none)
local _tab_drag_origin = 0    -- mouse x when the tab drag started

-- find bar state (shared across all tabs)
local _find = {
    active  = false,
    query   = "",
    matches = {},  -- { line, col_s, col_e }  col_e is one-past-end
    idx     = 0,   -- current highlighted match (1-based); 0 = none
}
local FIND_BAR_W = 264
local FIND_BAR_H = 28

local CTX_ITEMS = {
    { label = "Select All",  hint = "Ctrl+A" },
    { label = "Copy",        hint = "Ctrl+C" },
    { label = "Cut",         hint = "Ctrl+X" },
    { label = "Paste",       hint = "Ctrl+V" },
}

-- ---------------------------------------------
-- Tab state
-- ---------------------------------------------
M.tabs    = {}   -- array of tab objects (see new_tab)
M.cur_idx = 0    -- 1..#tabs; 0 means no tab focused (should not occur when tabs exist)

local function new_tab(path, lines)
    return {
        filepath     = path,
        lines        = lines,
        cursor       = { line = 1, col = 1 },
        scroll       = 0,
        modified     = false,
        disk_changed = false,
        mtime        = nil,
        sel          = nil,
        _ctx         = nil,
        -- History (time-travel snapshots)
        hist_pos     = nil,
        hist_list    = nil,
        last_snap_t  = 0,
        -- Per-keystroke undo/redo
        undo_stack   = {},
        redo_stack   = {},
        last_undo_t  = 0,
    }
end

local UNDO_LIMIT = 200

local function _copy_lines(lines)
    local c = {}
    for i, ln in ipairs(lines) do c[i] = ln end
    return c
end

-- Push a checkpoint onto the undo stack.
-- force=true  : always push (major edits: paste, indent, line-break, etc.)
-- force=false : coalesce rapid same-direction edits (single char, single backspace)
local function push_undo(t, force)
    local now = love.timer.getTime()
    if not force and (now - t.last_undo_t) < 0.5 then return end
    t.redo_stack = {}  -- any new edit discards the redo history
    t.undo_stack[#t.undo_stack + 1] = {
        lines  = _copy_lines(t.lines),
        cursor = { line = t.cursor.line, col = t.cursor.col },
        sel    = t.sel and {
            s = { line = t.sel.s.line, col = t.sel.s.col },
            e = { line = t.sel.e.line, col = t.sel.e.col },
        } or nil,
    }
    if #t.undo_stack > UNDO_LIMIT then
        table.remove(t.undo_stack, 1)
    end
    t.last_undo_t = now
end

function M.has_tabs()
    return #M.tabs > 0
end

function M.active_tab()
    return M.tabs[M.cur_idx]
end

function M.consume_browser_focus_request()
    local v = _request_browser_focus
    _request_browser_focus = false
    return v
end

-- ---------------------------------------------
-- UTF-8 safety
-- ---------------------------------------------
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

-- U+FFFD REPLACEMENT CHARACTER (UTF-8: EF BF BD).
-- Used in place of bytes that are not valid UTF-8, so they appear as a
-- visible stand-in rather than being silently dropped.  This handles files
-- saved in cp1252 / latin-1 where e.g. em dash is byte 0x97.
local _REPLACEMENT = "\xef\xbf\xbd"

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
            -- Undecodable byte: emit the Unicode replacement character so the
            -- character is visible rather than silently absent.
            out[#out + 1] = _REPLACEMENT
            i = i + 1
        end
    end
    return table.concat(out)
end

-- ---------------------------------------------
-- Simple syntax highlighter
-- Returns list of { text, col_key } tokens
-- ---------------------------------------------
-- keyword → color-key map per language (each word maps to its color key string)
local KW_MAP = {
    lua = {
        ["if"]="kw_ctrl",["then"]="kw_ctrl",["else"]="kw_ctrl",["elseif"]="kw_ctrl",
        ["end"]="kw_ctrl",["for"]="kw_ctrl",["while"]="kw_ctrl",["do"]="kw_ctrl",
        ["repeat"]="kw_ctrl",["until"]="kw_ctrl",["break"]="kw_ctrl",
        ["return"]="kw_ctrl",["goto"]="kw_ctrl",
        ["function"]="kw_decl",["local"]="kw_decl",["in"]="kw_decl",
        ["true"]="literal",["false"]="literal",["nil"]="literal",
        ["and"]="violet",["or"]="violet",["not"]="violet",
    },
    python = {
        ["if"]="kw_ctrl",["elif"]="kw_ctrl",["else"]="kw_ctrl",["for"]="kw_ctrl",
        ["while"]="kw_ctrl",["break"]="kw_ctrl",["continue"]="kw_ctrl",
        ["return"]="kw_ctrl",["yield"]="kw_ctrl",["raise"]="kw_ctrl",
        ["try"]="kw_ctrl",["except"]="kw_ctrl",["finally"]="kw_ctrl",
        ["with"]="kw_ctrl",["pass"]="kw_ctrl",
        ["def"]="kw_decl",["class"]="kw_decl",["lambda"]="kw_decl",
        ["import"]="kw_decl",["from"]="kw_decl",["as"]="kw_decl",
        ["global"]="kw_decl",["nonlocal"]="kw_decl",["del"]="kw_decl",
        ["async"]="kw_decl",["await"]="kw_decl",
        ["True"]="literal",["False"]="literal",["None"]="literal",
        ["and"]="violet",["or"]="violet",["not"]="violet",
        ["is"]="violet",["in"]="violet",["assert"]="violet",
    },
    js = {
        ["if"]="kw_ctrl",["else"]="kw_ctrl",["for"]="kw_ctrl",["while"]="kw_ctrl",
        ["do"]="kw_ctrl",["switch"]="kw_ctrl",["case"]="kw_ctrl",["default"]="kw_ctrl",
        ["break"]="kw_ctrl",["continue"]="kw_ctrl",["return"]="kw_ctrl",
        ["throw"]="kw_ctrl",["try"]="kw_ctrl",["catch"]="kw_ctrl",["finally"]="kw_ctrl",
        ["yield"]="kw_ctrl",["debugger"]="kw_ctrl",
        ["function"]="kw_decl",["class"]="kw_decl",["const"]="kw_decl",
        ["let"]="kw_decl",["var"]="kw_decl",["new"]="kw_decl",
        ["import"]="kw_decl",["export"]="kw_decl",["extends"]="kw_decl",
        ["static"]="kw_decl",["super"]="kw_decl",["async"]="kw_decl",["await"]="kw_decl",
        ["typeof"]="kw_decl",["instanceof"]="kw_decl",["delete"]="kw_decl",
        ["void"]="kw_decl",["in"]="kw_decl",["with"]="kw_decl",
        ["true"]="literal",["false"]="literal",["null"]="literal",
        ["undefined"]="literal",["this"]="literal",
    },
    php = {
        ["if"]="kw_ctrl",["else"]="kw_ctrl",["elseif"]="kw_ctrl",
        ["while"]="kw_ctrl",["for"]="kw_ctrl",["foreach"]="kw_ctrl",
        ["break"]="kw_ctrl",["continue"]="kw_ctrl",["return"]="kw_ctrl",
        ["try"]="kw_ctrl",["catch"]="kw_ctrl",["throw"]="kw_ctrl",
        ["function"]="kw_decl",["class"]="kw_decl",["new"]="kw_decl",
        ["extends"]="kw_decl",["implements"]="kw_decl",["namespace"]="kw_decl",
        ["use"]="kw_decl",["public"]="kw_decl",["private"]="kw_decl",
        ["protected"]="kw_decl",["static"]="kw_decl",["abstract"]="kw_decl",
        ["final"]="kw_decl",["interface"]="kw_decl",["trait"]="kw_decl",
        ["echo"]="kw_decl",["print"]="kw_decl",
        ["true"]="literal",["false"]="literal",["null"]="literal",
    },
    c = {
        ["if"]="kw_ctrl",["else"]="kw_ctrl",["for"]="kw_ctrl",["while"]="kw_ctrl",
        ["do"]="kw_ctrl",["switch"]="kw_ctrl",["case"]="kw_ctrl",["default"]="kw_ctrl",
        ["break"]="kw_ctrl",["continue"]="kw_ctrl",["return"]="kw_ctrl",["goto"]="kw_ctrl",
        ["int"]="kw_decl",["float"]="kw_decl",["double"]="kw_decl",["void"]="kw_decl",
        ["char"]="kw_decl",["long"]="kw_decl",["short"]="kw_decl",["unsigned"]="kw_decl",
        ["signed"]="kw_decl",["const"]="kw_decl",["struct"]="kw_decl",["enum"]="kw_decl",
        ["union"]="kw_decl",["typedef"]="kw_decl",["extern"]="kw_decl",["static"]="kw_decl",
        ["inline"]="kw_decl",["volatile"]="kw_decl",["register"]="kw_decl",["auto"]="kw_decl",
        ["true"]="literal",["false"]="literal",["NULL"]="literal",
        ["sizeof"]="violet",
    },
    cpp = {
        ["if"]="kw_ctrl",["else"]="kw_ctrl",["for"]="kw_ctrl",["while"]="kw_ctrl",
        ["do"]="kw_ctrl",["switch"]="kw_ctrl",["case"]="kw_ctrl",["default"]="kw_ctrl",
        ["break"]="kw_ctrl",["continue"]="kw_ctrl",["return"]="kw_ctrl",["goto"]="kw_ctrl",
        ["throw"]="kw_ctrl",["try"]="kw_ctrl",["catch"]="kw_ctrl",
        ["int"]="kw_decl",["float"]="kw_decl",["double"]="kw_decl",["void"]="kw_decl",
        ["char"]="kw_decl",["long"]="kw_decl",["short"]="kw_decl",["unsigned"]="kw_decl",
        ["signed"]="kw_decl",["const"]="kw_decl",["struct"]="kw_decl",["enum"]="kw_decl",
        ["union"]="kw_decl",["typedef"]="kw_decl",["extern"]="kw_decl",["static"]="kw_decl",
        ["inline"]="kw_decl",["volatile"]="kw_decl",["auto"]="kw_decl",["constexpr"]="kw_decl",
        ["class"]="kw_decl",["template"]="kw_decl",["typename"]="kw_decl",
        ["namespace"]="kw_decl",["using"]="kw_decl",["operator"]="kw_decl",
        ["public"]="kw_decl",["private"]="kw_decl",["protected"]="kw_decl",
        ["virtual"]="kw_decl",["override"]="kw_decl",["final"]="kw_decl",
        ["explicit"]="kw_decl",["friend"]="kw_decl",["mutable"]="kw_decl",
        ["new"]="kw_decl",["delete"]="kw_decl",["decltype"]="kw_decl",
        ["true"]="literal",["false"]="literal",["nullptr"]="literal",
        ["NULL"]="literal",["this"]="literal",
        ["sizeof"]="violet",["typeid"]="violet",["alignof"]="violet",
        ["noexcept"]="violet",["and"]="violet",["or"]="violet",["not"]="violet",
        ["static_assert"]="kw_ctrl",
    },
}

-- symbol characters that should receive the operator color
local OP_SET = {
    ["="] = true, ["+"] = true, ["-"] = true, ["*"] = true, ["/"] = true,
    ["!"] = true, ["&"] = true, ["|"] = true, ["~"] = true, ["?"] = true,
    ["@"] = true, ["%"] = true, ["^"] = true, ["\\"]= true,
    ["<"] = true, [">"] = true, [":"] = true, ["#"] = true,
}

local EXT_LANG = {
    lua="lua", py="python", js="js", ts="js", jsx="js", tsx="js", php="php",
    c="c", h="cpp", cpp="cpp", cs="js",
    sh="python", bash="python",
    css="css",
}

local function ext_of(path)
    return (path:match("%.([^%.]+)$") or ""):lower()
end

-- Return just the filename from a path that may use / or \ as separator.
local function basename(path)
    return path:match("[^/\\]+$") or path
end

-- CSS-specific tokenizer.
-- Colors:
--   .class    → yellow     (#id → literal/amber, both distinct in the dark purple theme)
--   property  → lavender   (light purple/blueish, same hue used for brackets elsewhere)
--   :pseudo   → violet
--   @at-rule  → kw_ctrl
--   strings   → green,  hex/numbers → cyan,  comments → comment
local function tokenize_line_css(line)
    local out = {}
    local i   = 1
    local n   = #line

    -- Fast path: multi-line comment body (starts with * or */)
    local stripped = line:match("^%s*(.*)")
    if stripped:match("^/%*") or (stripped:match("^%*") and not stripped:match("^%*/")) then
        out[#out+1] = { text = line, col = "comment" }
        return out
    end

    -- Decide initial declaration context for lines that have no braces.
    -- Lines with braces toggle in_decl as we scan them.
    local has_open  = line:find("{",  1, true) ~= nil
    local has_close = line:find("}",  1, true) ~= nil
    local in_decl
    if has_open or has_close then
        in_decl = false   -- toggled dynamically below
    elseif stripped == "" or stripped:match("^@") or stripped:match("^[.#:*%[]") then
        in_decl = false
    else
        -- Heuristic: identifier followed by ":" then whitespace/digit/#/quote → declaration.
        -- "a:hover" has a letter after ":" so it stays as selector context.
        -- Indented lines that are ambiguous (word:word) default to declaration.
        local after = stripped:match("^%-?[%a][%w%-]*%s*:(.?)")
        if after == nil then
            in_decl = false
        elseif after == "" or after:match("[%s%d#\"']") then
            in_decl = true
        elseif after:match("[%a]") then
            in_decl = line:match("^%s") ~= nil   -- indented → declaration
        else
            in_decl = false
        end
    end

    while i <= n do
        local c = line:sub(i, i)

        -- block comment
        if c == "/" and line:sub(i, i+1) == "/*" then
            local j = line:find("*/", i+2, true)
            if j then
                out[#out+1] = { text = line:sub(i, j+1), col = "comment" }
                i = j + 2
            else
                out[#out+1] = { text = line:sub(i), col = "comment" }
                i = n + 1
            end

        elseif c == "{" then
            out[#out+1] = { text = c, col = "lavender" }
            in_decl = true
            i = i + 1

        elseif c == "}" then
            out[#out+1] = { text = c, col = "lavender" }
            in_decl = false
            i = i + 1

        elseif c == ";" then
            out[#out+1] = { text = c, col = "op" }
            i = i + 1

        elseif not in_decl then
            -- Selector context
            if c == "@" then
                local j = i + 1
                while j <= n and line:sub(j,j):match("[%a%-]") do j = j + 1 end
                out[#out+1] = { text = line:sub(i, j-1), col = "kw_ctrl" }
                i = j
            elseif c == "." then
                local j = i + 1
                while j <= n and line:sub(j,j):match("[%w%-_]") do j = j + 1 end
                out[#out+1] = { text = line:sub(i, j-1), col = "yellow" }
                i = j
            elseif c == "#" then
                local j = i + 1
                while j <= n and line:sub(j,j):match("[%w%-_]") do j = j + 1 end
                out[#out+1] = { text = line:sub(i, j-1), col = "literal" }
                i = j
            elseif c == ":" then
                -- :pseudo or ::pseudo-element
                local j = i + 1
                if line:sub(j,j) == ":" then j = j + 1 end
                while j <= n and line:sub(j,j):match("[%w%-]") do j = j + 1 end
                out[#out+1] = { text = line:sub(i, j-1), col = "violet" }
                i = j
            elseif c:match("[%a_]") then
                local j = i
                while j <= n and line:sub(j,j):match("[%w%-_]") do j = j + 1 end
                out[#out+1] = { text = line:sub(i, j-1), col = "text" }
                i = j
            elseif c:match("[>+~,]") then
                out[#out+1] = { text = c, col = "op" }
                i = i + 1
            elseif c == "[" or c == "]" or c == "(" or c == ")" then
                out[#out+1] = { text = c, col = "lavender" }
                i = i + 1
            elseif c:match("%s") then
                out[#out+1] = { text = c, col = "dim" }
                i = i + 1
            else
                out[#out+1] = { text = c, col = "text" }
                i = i + 1
            end

        else
            -- Declaration context (inside {})
            if c == '"' or c == "'" then
                local j = i + 1
                while j <= n do
                    if line:sub(j,j) == "\\" then j = j + 2
                    elseif line:sub(j,j) == c then j = j + 1; break
                    else j = j + 1 end
                end
                out[#out+1] = { text = line:sub(i, j-1), col = "green" }
                i = j
            elseif c == "#" then
                -- hex color value
                local j = i + 1
                while j <= n and line:sub(j,j):match("[%x]") do j = j + 1 end
                out[#out+1] = { text = line:sub(i, j-1), col = "cyan" }
                i = j
            elseif c:match("[0-9]") or (c == "-" and line:sub(i+1,i+1):match("[0-9]")) then
                -- number + optional CSS unit (px, em, rem, vh, vw, %, etc.)
                local j = i
                if c == "-" then j = j + 1 end
                while j <= n and line:sub(j,j):match("[0-9%.]") do j = j + 1 end
                while j <= n and line:sub(j,j):match("[%a%%]") do j = j + 1 end
                out[#out+1] = { text = line:sub(i, j-1), col = "cyan" }
                i = j
            elseif c:match("[%a%-]") then
                -- Scan word; look ahead past optional whitespace for ":" to detect property names.
                local j = i
                while j <= n and line:sub(j,j):match("[%w%-]") do j = j + 1 end
                local word = line:sub(i, j-1)
                local k = j
                while k <= n and line:sub(k,k):match("%s") do k = k + 1 end
                -- single colon (not "::") right after → property name
                if line:sub(k,k) == ":" and line:sub(k+1,k+1) ~= ":" then
                    out[#out+1] = { text = word, col = "lavender" }
                else
                    out[#out+1] = { text = word, col = "text" }
                end
                i = j
            elseif c == ":" then
                out[#out+1] = { text = c, col = "op" }
                i = i + 1
            elseif c == "," then
                out[#out+1] = { text = c, col = "op" }
                i = i + 1
            elseif c:match("[()%[%]]") then
                out[#out+1] = { text = c, col = "lavender" }
                i = i + 1
            elseif c == "!" then
                out[#out+1] = { text = c, col = "op" }
                i = i + 1
            elseif c:match("%s") then
                out[#out+1] = { text = c, col = "dim" }
                i = i + 1
            else
                out[#out+1] = { text = c, col = "text" }
                i = i + 1
            end
        end
    end
    return out
end

local function tokenize_line(line, ext)
    if ext == "css" then return tokenize_line_css(line) end
    local lang  = EXT_LANG[ext] or "js"
    local kw    = KW_MAP[lang] or {}
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
            out[#out+1] = { text = line:sub(i), col = "comment" }
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
            local col
            -- namespace/type qualifier: word immediately followed by :: (e.g. std::, MyClass::)
            if line:sub(j, j+1) == "::" then
                col = "ns"
            else
                col = kw[word] or "text"
            end
            out[#out+1] = { text = word, col = col }
            i = j

        else
            -- single char or multi-byte UTF-8 sequence
            local b = line:byte(i)
            local slen = _seq_len(b)
            if slen and slen > 1 and i + slen - 1 <= n and _valid_seq(line, i, slen) then
                -- Multi-byte UTF-8 char (em-dash, curly quotes, emoji, CJK, etc.).
                -- Use full text brightness so these characters are clearly visible.
                out[#out+1] = { text = line:sub(i, i + slen - 1), col = "text" }
                i = i + slen
            else
                local char = line:sub(i, i)
                local col
                if char:match("[(){}%[%]]") then
                    col = "lavender"
                elseif OP_SET[char] then
                    col = "op"
                elseif char:match("%S") then
                    col = "text"
                else
                    col = "dim"
                end
                out[#out+1] = { text = char, col = col }
                i = i + 1
            end
        end
    end
    return out
end

-- Forward declaration so open_file (below) can call it before the body is defined.
local ensure_active_tab_visible

-- ---------------------------------------------
-- Encoding helpers for file I/O
-- ---------------------------------------------
-- Windows-1252 special range 0x80-0x9F → Unicode codepoints.
-- Entries absent from this table are undefined in cp1252 and map to U+FFFD.
-- Bytes 0xA0-0xFF are identical to Latin-1 (same codepoint as byte value).
local _CP1252 = {
    [0x80]=0x20AC,           [0x82]=0x201A,[0x83]=0x0192,[0x84]=0x201E,
    [0x85]=0x2026,[0x86]=0x2020,[0x87]=0x2021,[0x88]=0x02C6,
    [0x89]=0x2030,[0x8A]=0x0160,[0x8B]=0x2039,[0x8C]=0x0152,
                  [0x8E]=0x017D,
    [0x91]=0x2018,[0x92]=0x2019,[0x93]=0x201C,[0x94]=0x201D,
    [0x95]=0x2022,[0x96]=0x2013,[0x97]=0x2014,
    [0x98]=0x02DC,[0x99]=0x2122,[0x9A]=0x0161,[0x9B]=0x203A,
    [0x9C]=0x0153,            [0x9E]=0x017E,[0x9F]=0x0178,
}

-- Encode a Unicode codepoint to a UTF-8 byte string (U+0000..U+10FFFF).
local function _utf8_chr(cp)
    if cp < 0x80 then
        return string.char(cp)
    elseif cp < 0x800 then
        return string.char(0xC0 + math.floor(cp / 64),
                           0x80 + (cp % 64))
    elseif cp < 0x10000 then
        return string.char(0xE0 + math.floor(cp / 4096),
                           0x80 + math.floor(cp / 64) % 64,
                           0x80 + (cp % 64))
    else
        return string.char(0xF0 + math.floor(cp / 262144),
                           0x80 + math.floor(cp / 4096) % 64,
                           0x80 + math.floor(cp / 64) % 64,
                           0x80 + (cp % 64))
    end
end

-- Return true if s is entirely valid UTF-8.
local function _is_utf8(s)
    local i, n = 1, #s
    while i <= n do
        local b   = s:byte(i)
        local len = _seq_len(b)
        if not len or i + len - 1 > n or not _valid_seq(s, i, len) then
            return false
        end
        i = i + len
    end
    return true
end

-- Convert a raw byte string from cp1252/Latin-1 to UTF-8.
-- Bytes 0x00-0x7F pass through unchanged (same as ASCII/UTF-8).
-- Bytes 0x80-0x9F use the cp1252 table above; undefined entries → U+FFFD.
-- Bytes 0xA0-0xFF use their Latin-1 value (identical to the Unicode codepoint).
local function _cp1252_to_utf8(s)
    local out, n = {}, #s
    for i = 1, n do
        local b = s:byte(i)
        if b < 0x80 then
            out[#out+1] = string.char(b)
        elseif b >= 0xA0 then
            out[#out+1] = _utf8_chr(b)          -- Latin-1 supplement
        else
            out[#out+1] = _utf8_chr(_CP1252[b] or 0xFFFD)
        end
    end
    return table.concat(out)
end

-- ---------------------------------------------
-- File I/O
-- ---------------------------------------------
function M.open_file(path)
    -- already open? switch to it
    for i, t in ipairs(M.tabs) do
        if t.filepath == path then
            M.cur_idx = i
            ensure_active_tab_visible()
            return
        end
    end
    -- Read as raw bytes so no OS-level encoding conversion happens.
    local f = io.open(path, "rb")
    if not f then return end
    local content = f:read("*a")
    f:close()

    local lines = { "" }
    if content and content ~= "" then
        -- Auto-detect encoding: if the bytes are not valid UTF-8 treat the file
        -- as cp1252/Latin-1 and recode to UTF-8 so all characters display correctly.
        if not _is_utf8(content) then
            content = _cp1252_to_utf8(content)
        end
        -- Strip UTF-8 BOM (EF BB BF / U+FEFF) if present at the start of the file.
        if content:sub(1, 3) == "\xEF\xBB\xBF" then
            content = content:sub(4)
        end
        -- Normalise line endings (CRLF, CR, LF all → LF) then split.
        content = content:gsub("\r\n", "\n"):gsub("\r", "\n")
        lines = {}
        for line in (content .. "\n"):gmatch("([^\n]*)\n") do
            lines[#lines+1] = line
        end
        -- Drop the spurious trailing empty line the gmatch adds.
        if #lines > 1 and lines[#lines] == "" then
            lines[#lines] = nil
        end
        if #lines == 0 then lines = { "" } end
    end

    local tab = new_tab(path, lines)
    tab.mtime = get_mtime(path)
    M.tabs[#M.tabs+1] = tab
    M.cur_idx = #M.tabs
    ensure_active_tab_visible()
    -- Capture the on-disk state as a baseline snapshot.
    History.snapshot(path, lines)
    tab.last_snap_t = love.timer.getTime()
end

local function save_tab(t)
    local f = io.open(t.filepath, "w")
    if not f then return end
    for i, ln in ipairs(t.lines) do
        f:write(ln)
        if i < #t.lines then f:write("\n") end
    end
    f:close()
    t.modified     = false
    t.disk_changed = false
    t.mtime        = get_mtime(t.filepath)
    -- Snapshot after save so Ctrl+Z can return to this saved state.
    History.snapshot(t.filepath, t.lines)
    t.last_snap_t = love.timer.getTime()
    t.hist_pos    = nil
    t.hist_list   = nil
end

-- Reload a tab from disk, preserving cursor and scroll as much as possible.
local function reload_tab(t)
    local f = io.open(t.filepath, "rb")
    if not f then return end
    local content = f:read("*a")
    f:close()
    if not content then return end
    if not _is_utf8(content) then content = _cp1252_to_utf8(content) end
    if content:sub(1, 3) == "\xEF\xBB\xBF" then content = content:sub(4) end
    content = content:gsub("\r\n", "\n"):gsub("\r", "\n")
    local lines = {}
    for line in (content .. "\n"):gmatch("([^\n]*)\n") do lines[#lines+1] = line end
    if #lines > 1 and lines[#lines] == "" then lines[#lines] = nil end
    if #lines == 0 then lines = { "" } end
    t.lines        = lines
    t.modified     = false
    t.disk_changed = false
    t.mtime        = get_mtime(t.filepath)
    t.hist_pos     = nil
    t.hist_list    = nil
    -- Clamp cursor and scroll to the new line count.
    t.cursor.line = math.min(t.cursor.line, #t.lines)
    t.cursor.col  = math.min(t.cursor.col,  #t.lines[t.cursor.line] + 1)
    t.scroll      = math.min(t.scroll, math.max(0, #t.lines - 1))
    t.sel         = nil
end

local function close_tab(idx)
    table.remove(M.tabs, idx)
    if #M.tabs == 0 then
        M.cur_idx = 0
    elseif M.cur_idx > #M.tabs then
        M.cur_idx = #M.tabs
    end
end

-- ---------------------------------------------
-- Selection helpers
-- ---------------------------------------------
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

-- ---------------------------------------------
-- Cursor helpers
-- ---------------------------------------------
local function clamp_col(t)
    local max_col = #t.lines[t.cursor.line] + 1
    if t.cursor.col > max_col then t.cursor.col = max_col end
    if t.cursor.col < 1       then t.cursor.col = 1 end
end

local function visible_lines(h)
    local inner_h = h - TAB_H - FOOTER_H - TOP_PAD - BOTTOM_PAD
    return math.max(1, math.floor(inner_h / CHAR_H))
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
    push_undo(t, true)
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

-- ---------------------------------------------
-- Find / search helpers
-- ---------------------------------------------
local function find_rebuild(t, query)
    _find.matches = {}
    _find.idx     = 0
    if not t or query == "" then return end
    local q  = query:lower()
    local ql = #q
    for li, line in ipairs(t.lines) do
        local s   = line:lower()
        local col = 1
        while col <= #s do
            local a = s:find(q, col, true)
            if not a then break end
            _find.matches[#_find.matches + 1] = { line = li, col_s = a, col_e = a + ql }
            col = a + 1
        end
    end
end

local function find_goto(t, h, step)
    if #_find.matches == 0 then return end
    _find.idx     = ((_find.idx - 1 + step) % #_find.matches) + 1
    local m       = _find.matches[_find.idx]
    t.cursor.line = m.line
    t.cursor.col  = m.col_s
    ensure_scroll(t, h)
end

-- ---------------------------------------------
-- DRAW
-- ---------------------------------------------

-- tab button geometry cache: { { x, w, close_x } ... }
local _tab_geom     = {}
local _tab_scroll_x = 0   -- horizontal scroll offset for the tab bar
local _tab_bar_x    = 0   -- updated each draw; used by scroll helpers
local _tab_bar_w    = 0
local _total_tabs_w = 0   -- total pixel width of all tabs (cached after each draw)
local TAB_MAX_W     = 200  -- max pixel width of a single tab

-- Truncate a label to fit inside max_px pixels (using font_sm).
local function truncate_label(lbl, max_px)
    if font_sm:getWidth(lbl) <= max_px then return lbl, font_sm:getWidth(lbl) end
    local ellipsis = "\xe2\x80\xa6"   -- UTF-8 ellipsis character
    local ew = font_sm:getWidth(ellipsis)
    local avail = max_px - ew
    local i = #lbl
    while i > 0 and font_sm:getWidth(lbl:sub(1, i)) > avail do i = i - 1 end
    local s = lbl:sub(1, i) .. ellipsis
    return s, font_sm:getWidth(s)
end

-- Scroll _tab_scroll_x so the active tab is fully in view,
-- leaving ARROW_W margin on each side so it never hides behind an arrow.
ensure_active_tab_visible = function()
    if #M.tabs == 0 or _tab_bar_w == 0 then return end
    local tx = 4
    for i, tab in ipairs(M.tabs) do
        local name  = basename(tab.filepath)
        local label = (tab.disk_changed and "!" or "") .. (tab.modified and "*" or "") .. name
        local lw    = font_sm:getWidth(label)
        local cw    = font_sm:getWidth("x") + 8
        local tw    = math.min(lw + cw + 20, TAB_MAX_W)
        if i == M.cur_idx then
            if tx < _tab_scroll_x + ARROW_W then
                _tab_scroll_x = math.max(0, tx - ARROW_W)
            elseif tx + tw > _tab_scroll_x + _tab_bar_w - ARROW_W then
                _tab_scroll_x = tx + tw - _tab_bar_w + ARROW_W
            end
            break
        end
        tx = tx + tw + 2
    end
end

function M.draw(x, y, w, h)
    if not M.has_tabs() then return end
    _last_h = h

    local t = M.active_tab()

    -- ---- tab bar ----
    love.graphics.setColor(C.menu_bg)
    fill_rect(x, y, w, TAB_H)
    love.graphics.setColor(C.border)
    love.graphics.line(x, y + TAB_H, x + w, y + TAB_H)

    _tab_bar_x = x
    _tab_bar_w = w
    _tab_geom  = {}
    love.graphics.setFont(font_sm)

    -- Compute per-tab widths and total scrollable width.
    local tab_widths = {}
    local total_tabs_w = 4
    for i, tab in ipairs(M.tabs) do
        local name  = basename(tab.filepath)
        local label = (tab.disk_changed and "!" or "") .. (tab.modified and "*" or "") .. name
        local lw    = font_sm:getWidth(label)
        local cw    = font_sm:getWidth("x") + 8
        local tw    = math.min(lw + cw + 20, TAB_MAX_W)
        tab_widths[i] = tw
        total_tabs_w  = total_tabs_w + tw + 2
    end

    -- Clamp scroll.
    local max_scroll = math.max(0, total_tabs_w - w)
    _tab_scroll_x = math.max(0, math.min(_tab_scroll_x, max_scroll))
    _total_tabs_w = total_tabs_w

    -- Determine which scroll-arrow buttons are needed.
    local show_left  = _tab_scroll_x > 0
    local show_right = total_tabs_w > _tab_scroll_x + w
    local lpad = show_left  and ARROW_W or 0
    local rpad = show_right and ARROW_W or 0

    -- Clip rendering to the visible tab area (between arrow zones).
    love.graphics.setScissor(x + lpad, y, w - lpad - rpad, TAB_H)

    local tx = x + 4 - _tab_scroll_x
    for i, tab in ipairs(M.tabs) do
        local name   = basename(tab.filepath)
        local label  = (tab.disk_changed and "!" or "") .. (tab.modified and "*" or "") .. name
        local cw     = font_sm:getWidth("x") + 8
        local tw     = tab_widths[i]
        local active = (i == M.cur_idx)

        -- Label area is everything left of the close button.
        local label_max_px = tw - cw - 20
        local disp_label, dlw = truncate_label(label, label_max_px)
        local close_x = tx + dlw + 16

        _tab_geom[i] = { x = tx, w = tw, close_x = close_x }

        -- Only draw if at least partially visible.
        if tx + tw > x and tx < x + w then
            local dragging = (_tab_drag_idx == i)
            if active then
                love.graphics.setColor(C.panel_bg)
                fill_rect(tx, y + 1, tw, TAB_H - 1, 3)
                -- Orange stripe when disk changed with local edits; normal accent otherwise.
                love.graphics.setColor(tab.disk_changed and {1, 0.6, 0.1, 1} or C.accent)
                fill_rect(tx, y, tw, 2)
            elseif dragging then
                -- Lifted appearance: matches the active style but with the
                -- accent stripe on the bottom to signal it is being moved.
                love.graphics.setColor(C.panel_bg)
                fill_rect(tx, y + 2, tw, TAB_H - 2, 3)
                love.graphics.setColor(C.accent)
                fill_rect(tx, y + TAB_H - 2, tw, 2)
            else
                love.graphics.setColor(C.bg)
                fill_rect(tx, y + 3, tw, TAB_H - 3, 3)
            end

            love.graphics.setColor((active or dragging) and C.text_bright or C.dim)
            love.graphics.setFont(font_sm)
            love.graphics.print(disp_label, tx + 8, y + 5)

            love.graphics.setColor((active or dragging) and C.grey or C.dark)
            love.graphics.print("x", close_x, y + 5)
        end

        tx = tx + tw + 2
    end

    love.graphics.setScissor()

    -- Draw scroll-arrow buttons on top of the tab bar (over any partially-visible tabs).
    if show_left then
        love.graphics.setColor(C.menu_bg)
        fill_rect(x, y, ARROW_W, TAB_H)
        love.graphics.setColor(C.dim)
        love.graphics.setFont(font_sm)
        love.graphics.print("<", x + 4, y + 5)
    end
    if show_right then
        love.graphics.setColor(C.menu_bg)
        fill_rect(x + w - ARROW_W, y, ARROW_W, TAB_H)
        love.graphics.setColor(C.dim)
        love.graphics.setFont(font_sm)
        love.graphics.print(">", x + w - ARROW_W + 4, y + 5)
    end

    -- ---- editor area ----
    local ey    = y + TAB_H
    local eh    = h - TAB_H - FOOTER_H
    local vis   = visible_lines(h)
    local ext   = ext_of(t and t.filepath or "")

    love.graphics.setColor(C.bg)
    fill_rect(x, ey, w, eh)

    if not t then return end

    -- clamp scroll (allow slight overscroll below EOF for breathing room)
    local max_scr = math.max(0, #t.lines - vis + OVER_ROWS)
    if t.scroll > max_scr then t.scroll = max_scr end

    -- strict content rect: draw only full rows between top and bottom padding
    local scissor_y = ey + TOP_PAD
    local scissor_h = vis * CHAR_H
    love.graphics.setScissor(x, scissor_y, w, scissor_h)
    love.graphics.setFont(font_sm)

    local draw_y = ey + TOP_PAD
    for li = t.scroll + 1, math.min(t.scroll + vis, #t.lines) do
        local line   = t.lines[li] or ""
        local is_cur = (li == t.cursor.line)

        -- current line highlight
        if is_cur then
            love.graphics.setColor(0.12, 0.06, 0.22, 1)
            fill_rect(x, draw_y, w, CHAR_H)
        end

        -- selection highlight
        local sl, sc, el, ec = sel_norm(t)
        if sl and li >= sl and li <= el then
            local px_s = (li == sl) and (x + col_px(line, sc)) or (x + GUTTER)
            local px_e = (li == el) and (x + col_px(line, ec))
                         or (x + GUTTER + font_sm:getWidth(_utf8_safe(line)) + font_sm:getWidth(" "))
            if px_e > px_s then
                love.graphics.setColor(0.55, 0.28, 1.0, 0.82)
                fill_rect(px_s, draw_y, px_e - px_s, CHAR_H)
            end
        end

        -- find match highlights (behind text, in front of selection)
        if _find.active and _find.query ~= "" then
            for mi, m in ipairs(_find.matches) do
                if m.line == li then
                    local px_s = x + GUTTER + font_sm:getWidth(_utf8_safe(line:sub(1, m.col_s - 1)))
                    local px_e = x + GUTTER + font_sm:getWidth(_utf8_safe(line:sub(1, m.col_e - 1)))
                    if mi == _find.idx then
                        love.graphics.setColor(1.0, 0.82, 0.0, 0.72)  -- current match: gold
                    else
                        love.graphics.setColor(0.9, 0.7, 0.2, 0.32)   -- other matches: dim gold
                    end
                    fill_rect(px_s, draw_y, math.max(4, px_e - px_s), CHAR_H)
                end
            end
        end

        -- gutter
        love.graphics.setColor(C.panel_bg[1] or 0.065, C.panel_bg[2] or 0.04, C.panel_bg[3] or 0.14, 1)
        fill_rect(x, draw_y, GUTTER - 4, CHAR_H)
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
    fill_rect(x, y + h - FOOTER_H, w, FOOTER_H)
    love.graphics.setColor(C.border)
    love.graphics.line(x, y + h - FOOTER_H, x + w, y + h - FOOTER_H)
    love.graphics.setFont(font_sm)
    love.graphics.setColor(C.dim)
    -- Build history indicator when browsing snapshots.
    local hist_str = ""
    if t.hist_pos and t.hist_list then
        local entry = t.hist_list[t.hist_pos]
        local ts    = entry and os.date("%Y-%m-%d %H:%M", entry.saved_at) or "?"
        hist_str = "  |  Hist " .. t.hist_pos .. "/" .. #t.hist_list .. "  " .. ts
    elseif History.enabled() then
        hist_str = ""
    end
    local pos = "Ln " .. t.cursor.line .. "  Col " .. t.cursor.col
             .. "  |  " .. (ext ~= "" and ext:upper() or "TXT")
             .. hist_str
             .. "  |  " .. t.filepath
    love.graphics.print(pos, x + 8, y + h - FOOTER_H + 4)

    -- find bar overlay (floats over editor content, top-right)
    if _find.active then
        local fbx    = x + w - FIND_BAR_W - 6
        local fby    = y + TAB_H + 4
        local lbl    = "Find:"
        local lbl_w  = font_sm:getWidth(lbl) + 10
        local cnt_str
        if #_find.matches == 0 and _find.query ~= "" then
            cnt_str = "0/0"
        elseif #_find.matches > 0 then
            cnt_str = tostring(_find.idx) .. "/" .. tostring(#_find.matches)
        else
            cnt_str = ""
        end
        local cnt_w  = cnt_str ~= "" and (font_sm:getWidth(cnt_str) + 8) or 0
        local ifw    = FIND_BAR_W - lbl_w - cnt_w - 8

        -- background + border
        love.graphics.setColor(C.menu_bg)
        fill_rect(fbx, fby, FIND_BAR_W, FIND_BAR_H, 4)
        love.graphics.setColor(C.border)
        stroke_rect(fbx, fby, FIND_BAR_W, FIND_BAR_H, 4)
        love.graphics.setFont(font_sm)

        -- label
        love.graphics.setColor(C.dim)
        love.graphics.print(lbl, fbx + 6, fby + 6)

        -- input field background
        love.graphics.setColor(C.bg)
        fill_rect(fbx + lbl_w, fby + 4, ifw, FIND_BAR_H - 8, 3)
        love.graphics.setColor(C.border)
        stroke_rect(fbx + lbl_w, fby + 4, ifw, FIND_BAR_H - 8, 3)

        -- query text (right-align so the end is always visible while typing)
        local disp_q = _find.query
        while #disp_q > 0 and font_sm:getWidth(disp_q) > ifw - 8 do
            disp_q = disp_q:sub(2)
        end
        love.graphics.setColor(C.text)
        love.graphics.print(disp_q, fbx + lbl_w + 4, fby + 6)

        -- blinking cursor inside input
        local qcw   = font_sm:getWidth(disp_q)
        local blink = math.floor(love.timer.getTime() * 2) % 2 == 0
        if blink then
            love.graphics.setColor(C.text_bright)
            fill_rect(fbx + lbl_w + 4 + qcw, fby + 5, 1, FIND_BAR_H - 10)
        end

        -- match count
        if cnt_str ~= "" then
            love.graphics.setColor(_find.idx > 0 and C.text or C.dim)
            love.graphics.print(cnt_str, fbx + FIND_BAR_W - cnt_w - 2, fby + 6)
        end
    end

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

-- ---------------------------------------------
-- Shift-selection helpers
-- ---------------------------------------------
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

-- ---------------------------------------------
-- KEYPRESSED
-- ---------------------------------------------
function M.keypressed(key, h)
    if not M.has_tabs() then return false end
    local t    = M.active_tab()
    if not t   then return false end

    local cmd        = love.keyboard.isDown("lgui","rgui")
    local pure_ctrl  = love.keyboard.isDown("lctrl","rctrl")
    local ctrl       = pure_ctrl or cmd
    local shift      = love.keyboard.isDown("lshift","rshift")
    local alt        = love.keyboard.isDown("lalt","ralt")
    local L          = t.cursor.line
    local CO         = t.cursor.col

    -- close context menu on any key
    if t._ctx then t._ctx = nil end

    -- Cmd+Left/Right (macOS/Win key) or Alt+Left/Right (Windows): switch tabs
    -- Cmd+T / Alt+T: new empty tab
    if (cmd and not pure_ctrl) or (alt and not pure_ctrl and not cmd) then
        if key == "left" then
            if M.cur_idx > 1 then
                M.cur_idx = M.cur_idx - 1
                ensure_active_tab_visible()
            else
                _request_browser_focus = true
            end
            return true
        elseif key == "right" then
            if M.cur_idx < #M.tabs then M.cur_idx = M.cur_idx + 1 end
            ensure_active_tab_visible()
            return true
        elseif key == "t" then
            local tab = new_tab("untitled", { "" })
            M.tabs[#M.tabs + 1] = tab
            M.cur_idx = #M.tabs
            ensure_active_tab_visible()
            return true
        end
    end

    if key == "escape" then
        if _find.active then _find.active = false; return true end
        if t.sel then t.sel = nil return true end
        return false  -- let main handle

    elseif key == "f" and ctrl then
        _find.active = true
        find_rebuild(t, _find.query)
        return true

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
            push_undo(t, true)
            love.system.setClipboardText(txt)
            delete_sel(t)
            ensure_scroll(t, h)
        end
        return true

    elseif key == "v" and ctrl then
        paste_into(t, love.system.getClipboardText(), h); return true

    -- Ctrl+Z = undo, Ctrl+Y = redo, Ctrl+Shift+Z = snapshot history back
    elseif key == "z" and ctrl and not shift then
        local state = table.remove(t.undo_stack, #t.undo_stack)
        if state then
            t.redo_stack[#t.redo_stack + 1] = {
                lines  = _copy_lines(t.lines),
                cursor = { line = t.cursor.line, col = t.cursor.col },
                sel    = nil,
            }
            t.lines       = state.lines
            t.cursor      = state.cursor
            t.sel         = state.sel
            t.modified    = true
            t.hist_pos    = nil
            t.hist_list   = nil
            ensure_scroll(t, h)
        end
        return true

    elseif key == "y" and ctrl and not shift then
        local state = table.remove(t.redo_stack, #t.redo_stack)
        if state then
            t.undo_stack[#t.undo_stack + 1] = {
                lines  = _copy_lines(t.lines),
                cursor = { line = t.cursor.line, col = t.cursor.col },
                sel    = nil,
            }
            t.lines       = state.lines
            t.cursor      = state.cursor
            t.sel         = state.sel
            t.modified    = true
            ensure_scroll(t, h)
        end
        return true

    -- Ctrl+Shift+Z / Ctrl+Shift+Y = snapshot time-travel (file-level history)
    elseif key == "z" and ctrl and shift then
        if not History.enabled() then return true end
        if not t.hist_pos then
            History.snapshot(t.filepath, t.lines)
            t.last_snap_t = love.timer.getTime()
            t.hist_list   = nil
        end
        if not t.hist_list then
            t.hist_list = History.list(t.filepath)
        end
        local list = t.hist_list
        if #list < 2 then return true end
        local new_pos = (t.hist_pos or 0) + 1
        if new_pos > #list then new_pos = #list end
        if new_pos == (t.hist_pos or 0) then return true end
        local snap_lines = History.load(list[new_pos].id)
        if snap_lines then
            t.lines     = snap_lines
            t.hist_pos  = new_pos
            t.modified  = true
            t.sel       = nil
            t.cursor.line = math.min(t.cursor.line, #t.lines)
            t.cursor.col  = math.min(t.cursor.col, #t.lines[t.cursor.line] + 1)
            t.scroll      = math.min(t.scroll, math.max(0, #t.lines - 1))
        end
        return true

    elseif key == "y" and ctrl and shift then
        if not History.enabled() or not t.hist_pos then return true end
        if not t.hist_list then
            t.hist_list = History.list(t.filepath)
        end
        local list    = t.hist_list
        local new_pos = t.hist_pos - 1
        if new_pos < 1 then
            t.hist_pos = nil
            return true
        end
        local snap_lines = History.load(list[new_pos].id)
        if snap_lines then
            t.lines     = snap_lines
            t.hist_pos  = new_pos
            t.modified  = true
            t.sel       = nil
            t.cursor.line = math.min(t.cursor.line, #t.lines)
            t.cursor.col  = math.min(t.cursor.col, #t.lines[t.cursor.line] + 1)
            t.scroll      = math.min(t.scroll, math.max(0, #t.lines - 1))
        end
        return true

    -- Cmd+Shift+I/J/K/L (macOS/Win key) or Ctrl+Shift+I/J/K/L (Windows): turbo movement
    elseif key == "i" and ctrl and shift then
        t.cursor.line = math.max(1, t.cursor.line - 8)
        clamp_col(t); ensure_scroll(t, h); return true

    elseif key == "k" and ctrl and shift then
        t.cursor.line = math.min(#t.lines, t.cursor.line + 8)
        clamp_col(t); ensure_scroll(t, h); return true

    elseif key == "j" and ctrl and shift then
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
        ensure_scroll(t, h); return true

    elseif key == "l" and ctrl and shift then
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
        ensure_scroll(t, h); return true

    elseif key == "return" or key == "kpenter" then
        if _find.active then
            find_goto(t, h, shift and -1 or 1)
            return true
        end
        push_undo(t, true)
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
        if _find.active then
            _find.query = _find.query:sub(1, -2)
            find_rebuild(t, _find.query)
            return true
        end
        if t.sel then push_undo(t, true); delete_sel(t); ensure_scroll(t, h); return true end
        push_undo(t, ctrl)  -- ctrl+backspace = word delete, always checkpoint
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
        if t.sel then push_undo(t, true); delete_sel(t); ensure_scroll(t, h); return true end
        push_undo(t, false)
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
        local sl, sc, el, ec = sel_norm(t)
        local INDENT = "    "

        if shift then
            -- Shift+Tab: unindent selected lines (or current line if no selection).
            push_undo(t, true)
            local first = (sl and el and el > sl) and sl or L
            local last  = (sl and el and el > sl) and el  or L
            for li = first, last do
                local line    = t.lines[li]
                local removed = 0
                -- Remove up to 4 leading spaces (one indent level).
                for i = 1, 4 do
                    if line:sub(i, i) == " " then removed = i
                    else break end
                end
                if removed > 0 then
                    t.lines[li] = line:sub(removed + 1)
                end
            end
            -- Adjust selection / cursor after unindent.
            if sl and el and el > sl then
                t.sel = {
                    s = { line = sl, col = math.max(1, sc - 4) },
                    e = { line = el, col = math.max(1, ec - 4) },
                }
                t.cursor.col = math.max(1, ec - 4)
            else
                t.cursor.col = math.max(1, CO - 4)
            end
            t.modified = true; return true
        end

        if sl and el and el > sl then
            -- Tab with multi-line selection: indent all selected lines.
            push_undo(t, true)
            for li = sl, el do
                t.lines[li] = INDENT .. t.lines[li]
            end
            t.sel = {
                s = { line = sl, col = sc + #INDENT },
                e = { line = el, col = ec + #INDENT },
            }
            t.cursor.line = el
            t.cursor.col  = ec + #INDENT
        else
            -- Tab with no / single-line selection: insert spaces at cursor.
            push_undo(t, true)
            if t.sel then delete_sel(t) end
            t.lines[L]   = t.lines[L]:sub(1, CO-1) .. INDENT .. t.lines[L]:sub(CO)
            t.cursor.col = CO + #INDENT
        end
        t.modified = true; return true

    elseif key == "up" then
        -- ctrl = jump 8 lines; shift = extend selection; plain = 1 line
        local step = ctrl and 8 or 1
        if shift then sel_start(t) end
        t.cursor.line = math.max(1, t.cursor.line - step)
        clamp_col(t)
        sel_finish(t, shift)
        ensure_scroll(t, h); return true

    elseif key == "down" then
        local step = ctrl and 8 or 1
        if shift then sel_start(t) end
        t.cursor.line = math.min(#t.lines, t.cursor.line + step)
        clamp_col(t)
        sel_finish(t, shift)
        ensure_scroll(t, h); return true

    elseif key == "left" then
        -- ctrl = word jump; shift = extend selection; ctrl+shift = word jump with selection
        if shift then sel_start(t) end
        if ctrl then
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
        sel_finish(t, shift)
        ensure_scroll(t, h); return true

    elseif key == "right" then
        -- ctrl = word jump; shift = extend selection; ctrl+shift = word jump with selection
        if shift then sel_start(t) end
        if ctrl then
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
        sel_finish(t, shift)
        ensure_scroll(t, h); return true

    elseif key == "home" then
        -- shift = smart home with selection; ctrl = jump to first line
        if shift then sel_start(t) end
        if ctrl then
            t.cursor.line = 1
            t.cursor.col  = 1
        else
            local indent_end = (t.lines[t.cursor.line]:match("^%s*()") or 1)
            t.cursor.col = (t.cursor.col == indent_end) and 1 or indent_end
        end
        sel_finish(t, shift)
        ensure_scroll(t, h); return true

    elseif key == "end" then
        -- shift = end of line with selection; ctrl = jump to last line
        if shift then sel_start(t) end
        if ctrl then
            t.cursor.line = #t.lines
            t.cursor.col  = #t.lines[#t.lines] + 1
        else
            t.cursor.col = #t.lines[t.cursor.line] + 1
        end
        sel_finish(t, shift)
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

-- ---------------------------------------------
-- TEXTINPUT
-- ---------------------------------------------
function M.textinput(char)
    if not M.has_tabs() then return false end
    local t = M.active_tab()
    if not t then return false end
    if _find.active then
        _find.query = _find.query .. char
        find_rebuild(t, _find.query)
        return true
    end
    -- Any real typing exits history-browse mode (keeps position but re-anchors to live).
    if t.hist_pos then t.hist_pos = nil; t.hist_list = nil end
    if t.sel then
        push_undo(t, true)   -- replacing a selection = always checkpoint
        delete_sel(t)
    else
        push_undo(t, false)  -- coalesced: only checkpoint if > 0.5s since last
    end
    local L, CO = t.cursor.line, t.cursor.col
    t.lines[L]   = t.lines[L]:sub(1, CO-1) .. char .. t.lines[L]:sub(CO)
    t.cursor.col = CO + #char
    t.modified   = true
    ensure_scroll(t, _last_h)
    return true
end

-- ---------------------------------------------
-- Mouse position helpers
-- ---------------------------------------------
local function _pos_from_mouse(mx, my, t, px, py)
    local ey = py + TAB_H
    if my < ey then return nil, nil end
    local row = t.scroll + 1 + math.floor((my - ey - TOP_PAD) / CHAR_H)
    row = math.max(1, math.min(#t.lines, row))
    local line  = t.lines[row] or ""
    local safe  = _utf8_safe(line)
    local rel_x = mx - (px + GUTTER)
    local best  = #safe + 1
    if rel_x > 0 then
        local acc = 0
        local ci  = 1
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
    else
        best = 1
    end
    return row, best
end

-- ---------------------------------------------
-- MOUSE
-- ---------------------------------------------
function M.mousepressed(mx, my, btn, px, py, pw, ph)
    -- px,py,pw,ph = editor panel position/size from main
    if not M.has_tabs() then return false end

    -- ---- tab bar clicks ----
    if my >= py and my < py + TAB_H then
        -- Left scroll-arrow button.
        if _tab_scroll_x > 0 and mx >= px and mx < px + ARROW_W then
            _tab_scroll_x = math.max(0, _tab_scroll_x - TAB_MAX_W)
            return true
        end
        -- Right scroll-arrow button.
        local show_right = _total_tabs_w > _tab_scroll_x + pw
        if show_right and mx >= px + pw - ARROW_W and mx < px + pw then
            local max_scroll = math.max(0, _total_tabs_w - pw)
            _tab_scroll_x = math.min(max_scroll, _tab_scroll_x + TAB_MAX_W)
            return true
        end

        -- Visible tab area (between the arrow zones).
        local vis_left  = px + (_tab_scroll_x > 0 and ARROW_W or 0)
        local vis_right = px + pw - (show_right and ARROW_W or 0)

        for i, geom in ipairs(_tab_geom) do
            -- Guard: only accept clicks within the visible (non-arrow) region so
            -- tabs scrolled off the left edge can't be hit by accident.
            if mx >= geom.x and mx < geom.x + geom.w
               and mx >= vis_left and mx < vis_right then
                -- close button
                local xbtn_x = geom.close_x
                if btn == 1 and mx >= xbtn_x and mx < xbtn_x + 14 then
                    close_tab(i)
                    return true
                end
                -- switch tab and begin drag tracking
                if btn == 1 then
                    M.cur_idx        = i
                    _tab_drag_idx    = i
                    _tab_drag_origin = mx
                    ensure_active_tab_visible()
                    return true
                end
                -- middle-click to close
                if btn == 3 then
                    close_tab(i)
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
    if my < ey or my >= py + ph - FOOTER_H then return false end

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

        -- map click to line and column
        local row, col = _pos_from_mouse(mx, my, t, px, py)
        if row then
            t.cursor.line = row
            t.cursor.col  = col
        end
        t.sel          = nil
        clamp_col(t)
        ensure_scroll(t, ph)
        -- begin drag-to-select
        _drag        = true
        _drag_anchor = { line = t.cursor.line, col = t.cursor.col }
        _drag_ctx    = { px = px, py = py, pw = pw, ph = ph }
        return true
    end

    return false
end

function M.mousemoved(mx, my)
    -- Tab drag-to-reorder: active anywhere once a tab drag has begun.
    if _tab_drag_idx and math.abs(mx - _tab_drag_origin) > 4 then
        if _tab_drag_idx > 1 then
            local prev = _tab_geom[_tab_drag_idx - 1]
            if prev and mx < prev.x + prev.w / 2 then
                M.tabs[_tab_drag_idx], M.tabs[_tab_drag_idx - 1] =
                    M.tabs[_tab_drag_idx - 1], M.tabs[_tab_drag_idx]
                _tab_drag_idx = _tab_drag_idx - 1
                M.cur_idx     = _tab_drag_idx
            end
        end
        if _tab_drag_idx < #M.tabs then
            local nxt = _tab_geom[_tab_drag_idx + 1]
            if nxt and mx > nxt.x + nxt.w / 2 then
                M.tabs[_tab_drag_idx], M.tabs[_tab_drag_idx + 1] =
                    M.tabs[_tab_drag_idx + 1], M.tabs[_tab_drag_idx]
                _tab_drag_idx = _tab_drag_idx + 1
                M.cur_idx     = _tab_drag_idx
            end
        end
        return true
    end

    if not _drag or not _drag_anchor then return false end
    local t = M.active_tab()
    if not t or not _drag_ctx then return false end

    -- Track mouse position so M.update(dt) can auto-scroll each frame.
    _drag_mx = mx
    _drag_my = my

    local ctx  = _drag_ctx
    local ey   = ctx.py + TAB_H
    local vis  = visible_lines(ctx.ph)
    local row, col

    if my < ey then
        -- Mouse above the editor (in tab bar or beyond): pin selection to the
        -- first visible line so dragging upward feels responsive.
        row = math.max(1, t.scroll + 1)
        col = 1
    elseif my >= ey + TOP_PAD + vis * CHAR_H then
        -- Mouse below the visible rows: pin to last visible line so the
        -- update-loop can scroll further while selection keeps extending.
        row = math.min(#t.lines, t.scroll + vis)
        col = #t.lines[row] + 1
    else
        row, col = _pos_from_mouse(mx, my, t, ctx.px, ctx.py)
        if not row then return false end
    end

    t.cursor.line = row
    t.cursor.col  = col
    clamp_col(t)
    t.sel = {
        s = { line = _drag_anchor.line, col = _drag_anchor.col },
        e = { line = t.cursor.line,     col = t.cursor.col },
    }
    return true
end

-- Auto-scroll the editor while a drag-to-select is in progress and the
-- mouse is outside the visible row area.  Called every frame from love.update.
function M.update(dt, mx, my)
    -- ---- File-watch polling ----
    _watch_accum = _watch_accum + dt
    if _watch_accum >= WATCH_INTERVAL then
        _watch_accum = 0
        for _, t in ipairs(M.tabs) do
            if t.filepath and t.mtime then
                local m = get_mtime(t.filepath)
                if m and m ~= t.mtime then
                    if not t.modified then
                        -- No local edits: silently reload.
                        reload_tab(t)
                    else
                        -- User has unsaved edits: flag the conflict, don't clobber.
                        t.disk_changed = true
                        t.mtime        = m
                    end
                end
            end
        end
    end

    -- ---- Periodic auto-snapshot (background history) ----
    if History.enabled() then
        local now = love.timer.getTime()
        for _, t in ipairs(M.tabs) do
            if t.modified and t.filepath
               and (now - t.last_snap_t) >= AUTO_SNAP_INTERVAL then
                History.snapshot(t.filepath, t.lines)
                t.last_snap_t = now
                t.hist_list   = nil  -- invalidate cached list
            end
        end
    end

    -- ---- Drag-to-select auto-scroll ----
    if not _drag or not _drag_ctx then return end
    local t = M.active_tab()
    if not t then return end

    -- Use the passed-in position (already DPI-scaled by main.lua).
    _drag_mx = mx or _drag_mx
    _drag_my = my or _drag_my

    local ctx    = _drag_ctx
    local ey     = ctx.py + TAB_H
    local vis    = visible_lines(ctx.ph)
    local top    = ey + TOP_PAD
    local bottom = top + vis * CHAR_H

    -- Compute scroll velocity: proportional to distance outside the viewport.
    -- At exactly one CHAR_H outside we get ~6 lines/sec; further = faster.
    local speed = 0
    if _drag_my < top then
        speed = (_drag_my - top) / CHAR_H   -- negative = scroll up
    elseif _drag_my >= bottom then
        speed = (_drag_my - bottom + 1) / CHAR_H  -- positive = scroll down
    end

    if speed == 0 then
        _drag_scroll_accum = 0
        return
    end

    _drag_scroll_accum = _drag_scroll_accum + speed * dt * 6
    local lines = math.floor(math.abs(_drag_scroll_accum) + 0.5)
    if lines < 1 then return end

    local dir     = _drag_scroll_accum > 0 and 1 or -1
    local max_scr = math.max(0, #t.lines - vis + OVER_ROWS)
    local new_scr = math.max(0, math.min(max_scr, t.scroll + dir * lines))
    if new_scr == t.scroll then
        _drag_scroll_accum = 0
        return
    end
    t.scroll           = new_scr
    _drag_scroll_accum = _drag_scroll_accum - dir * lines

    -- Extend the selection to the newly revealed edge row.
    if dir > 0 then
        t.cursor.line = math.min(#t.lines, t.scroll + vis)
        t.cursor.col  = #t.lines[t.cursor.line] + 1
    else
        t.cursor.line = math.max(1, t.scroll + 1)
        t.cursor.col  = 1
    end
    t.sel = {
        s = { line = _drag_anchor.line, col = _drag_anchor.col },
        e = { line = t.cursor.line,     col = t.cursor.col },
    }
end

function M.mousereleased(mx, my, btn)
    if btn == 1 then
        _tab_drag_idx      = nil     -- clear any tab drag
        _drag              = false
        _drag_anchor       = nil
        _drag_ctx          = nil
        _drag_scroll_accum = 0   -- reset so residual doesn't fire on next drag
    end
    return false
end

function M.wheelmoved(wx, wy, px, py, pw, ph, mx, my)
    if not M.has_tabs() then return false end
    local t   = M.active_tab()
    if not t  then return false end

    -- Horizontal scroll of the tab bar when mouse hovers over it.
    -- Prefer the horizontal trackpad axis (wx); fall back to the vertical
    -- wheel axis (wy) so a regular scroll wheel also works over the tab bar.
    if mx and my and my >= py and my < py + TAB_H then
        local tab_widths_total = 4
        for _, tab in ipairs(M.tabs) do
            local name  = basename(tab.filepath)
            local label = (tab.disk_changed and "!" or "") .. (tab.modified and "*" or "") .. name
            local lw    = font_sm:getWidth(label)
            local cw    = font_sm:getWidth("x") + 8
            local tw    = math.min(lw + cw + 20, TAB_MAX_W)
            tab_widths_total = tab_widths_total + tw + 2
        end
        local max_scroll = math.max(0, tab_widths_total - pw)
        -- wx > 0 = swipe/scroll right → show tabs further right → increase offset.
        -- wy > 0 = scroll up  → show earlier (left) tabs → decrease offset.
        local delta = (wx ~= 0) and (wx * 40) or (-wy * 40)
        _tab_scroll_x = math.max(0, math.min(max_scroll, _tab_scroll_x + delta))
        return true
    end

    local vis = visible_lines(ph)
    local max_scr = math.max(0, #t.lines - vis + OVER_ROWS)
    t.scroll  = math.max(0, math.min(max_scr, t.scroll - wy * 3))
    return true
end

return M
