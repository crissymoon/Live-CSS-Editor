-- main.lua  -  Code Review TUI
-- Love2D entry point.  Matches Live CSS Editor dark-purple theme.

--------------------------------------------------------------------
-- Globals used by modules
--------------------------------------------------------------------
W, H = 1060, 720   -- updated each frame
local _dpi = 1     -- physical-to-logical scale; set in love.load and love.resize
MENUBAR_H  = 24
TOOLBAR_H  = 96
SUMMARY_H  = 32
CHAR_H     = 16
TABS_H     = 26

-- Platform detection (available before love.load)
IS_WINDOWS = love.system.getOS() == "Windows"

-- Python command: honour env override set by run.ps1, then fall back per OS
PYTHON_CMD = os.getenv("CODE_REVIEW_PYTHON")
          or (IS_WINDOWS and "py -3" or "python3")

-- Shell escape a single argument safely (OS-aware)
function shell_escape(s)
    local str = tostring(s)
    if IS_WINDOWS then
        -- cmd.exe: wrap in double quotes, escape embedded double quotes
        return '"' .. str:gsub('"', '\\"') .. '"'
    else
        return "'" .. str:gsub("'", "'\\''" ) .. "'"
    end
end

--------------------------------------------------------------------
-- Colour table
--------------------------------------------------------------------
C = {
    bg          = {0.047, 0.027, 0.109, 1},   -- #0c071c
    panel_bg    = {0.065, 0.040, 0.140, 1},
    menu_bg     = {0.055, 0.032, 0.125, 1},
    toolbar_bg  = {0.055, 0.032, 0.125, 1},
    border      = {0.22,  0.10,  0.45,  1},
    hover       = {0.18,  0.07,  0.35,  0.7},
    accent      = {0.33,  0.0,   0.8,   1},    -- purple 38;5;99
    text        = {0.925, 0.918, 0.965, 1},    -- #eceaf6
    text_bright = {1.0,   1.0,   1.0,   1},
    dim         = {0.59,  0.59,  0.59,  1},
    violet      = {0.67,  0.33,  1.0,   1},    -- 38;5;141
    lavender    = {0.8,   0.7,   1.0,   1},    -- 38;5;189
    dark        = {0.2,   0.0,   0.5,   1},    -- 38;5;54
    green       = {0.4,   0.8,   0.4,   1},    -- 38;5;114
    red         = {1.0,   0.3,   0.3,   1},    -- 38;5;203
    yellow      = {1.0,   0.87,  0.5,   1},    -- 38;5;222
    grey        = {0.59,  0.59,  0.59,  1},    -- 38;5;244
    cyan        = {0.4,   0.8,   0.87,  1},    -- 38;5;116
    orange      = {1.0,   0.55,  0.15,  1},
    kw_ctrl     = {0.95, 0.45, 0.70, 1},   -- rose-pink:  control flow (if/for/while/return)
    kw_decl     = {0.38, 0.75, 1.0,  1},   -- sky blue:   declarations (function/class/const)
    literal     = {1.0,  0.70, 0.28, 1},   -- amber:      value literals (true/false/null/nil)
    op          = {0.76, 0.88, 1.0,  1},   -- ice blue:   operators (= + - * / ! < > & | ~)
    comment     = {0.40, 0.50, 0.44, 1},   -- sage:       line comments
    ns          = {0.95, 0.82, 0.48, 1},   -- warm gold:  namespace/type qualifiers (std::, MyClass::)
}

--------------------------------------------------------------------
-- Draw helpers
--------------------------------------------------------------------
local function gc(key)  love.graphics.setColor(C[key] or C.text) end

function fill_rect(x, y, w, h, r)
    love.graphics.rectangle("fill", x, y, w, h, r or 0)
end

function stroke_rect(x, y, w, h, r)
    love.graphics.rectangle("line", x, y, w, h, r or 0)
end

-- Sanitize a string so love.graphics.print never sees invalid UTF-8.
-- Walks the raw bytes and drops any byte that is not part of a valid
-- UTF-8 sequence.  Fast path: returns the string unchanged if it is
-- already valid (most lines will be plain ASCII).
local function sanitize_utf8(s)
    if type(s) ~= "string" then return "" end
    -- Quick scan: if every byte is ASCII the string is always valid.
    local n = #s
    local i = 1
    while i <= n do
        if s:byte(i) >= 0x80 then break end
        i = i + 1
    end
    if i > n then return s end  -- pure ASCII — skip full decode

    local out = {}
    i = 1
    while i <= n do
        local b = s:byte(i)
        if b < 0x80 then
            out[#out+1] = s:sub(i, i); i = i + 1
        elseif b >= 0xC2 and b <= 0xDF and i+1 <= n then
            local b2 = s:byte(i+1)
            if b2 >= 0x80 and b2 <= 0xBF then
                out[#out+1] = s:sub(i, i+1); i = i + 2
            else i = i + 1 end
        elseif b >= 0xE0 and b <= 0xEF and i+2 <= n then
            local b2, b3 = s:byte(i+1), s:byte(i+2)
            if b2 >= 0x80 and b2 <= 0xBF and b3 >= 0x80 and b3 <= 0xBF then
                out[#out+1] = s:sub(i, i+2); i = i + 3
            else i = i + 1 end
        elseif b >= 0xF0 and b <= 0xF4 and i+3 <= n then
            local b2, b3, b4 = s:byte(i+1), s:byte(i+2), s:byte(i+3)
            if b2 >= 0x80 and b2 <= 0xBF and
               b3 >= 0x80 and b3 <= 0xBF and
               b4 >= 0x80 and b4 <= 0xBF then
                out[#out+1] = s:sub(i, i+3); i = i + 4
            else i = i + 1 end
        else
            i = i + 1  -- drop invalid byte
        end
    end
    return table.concat(out)
end

function text_at(x, y, s)
    love.graphics.print(sanitize_utf8(s), math.floor(x), math.floor(y))
end

local function trunc_str(s, max_w, font)
    if not s then return "" end
    if font:getWidth(s) <= max_w then return s end
    local t = s
    while #t > 1 and font:getWidth(t .. "...") > max_w do
        t = t:sub(1, -2)
    end
    return t .. "..."
end

--------------------------------------------------------------------
-- Fonts (globals so editor.lua and other modules can access them)
--------------------------------------------------------------------
font_sm, font_md, font_ui = nil, nil, nil

local function load_fonts()
    -- Use the Windows font directory from the environment so this works
    -- regardless of which drive Windows is installed on.
    local wf = (os.getenv("WINDIR") or os.getenv("SystemRoot") or "C:/Windows") .. "/Fonts/"

    local mono_candidates = {
        wf .. "CascadiaMono.ttf",
        wf .. "CascadiaCode.ttf",
        wf .. "consola.ttf",
        wf .. "lucon.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/SFMono-Regular.otf",
        "/Library/Fonts/JetBrainsMono-Regular.ttf",
        "/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMono-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    }
    local ui_candidates = {
        wf .. "segoeui.ttf",
        wf .. "arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNSText.ttf",
        "/System/Library/Fonts/SFCompactText.ttf",
    }

    -- Fallback fonts for glyphs not in the primary.
    --
    -- Priority order:
    --   1. Comprehensive Unicode BMP coverage (em-dash, curly quotes, arrows, math…)
    --   2. Broad script/block coverage (CJK, Indic, Arabic, Hebrew, …)
    --   3. Symbol / emoji fonts (colour bitmap glyphs)
    --
    -- Lucida Sans Unicode (l_10646.ttf) is first on Windows because it covers
    -- the entire Basic Multilingual Plane reliably, including General Punctuation
    -- (U+2000–U+206F) where em-dash, en-dash, curly quotes, and ellipsis live.
    local fallback_candidates = {
        -- Windows — BMP coverage
        wf .. "l_10646.ttf",    -- Lucida Sans Unicode: widest BMP coverage
        wf .. "segoeui.ttf",
        wf .. "seguisym.ttf",   -- Segoe UI Symbol: mathematical & miscellaneous symbols
        wf .. "arial.ttf",
        wf .. "sylfaen.ttf",    -- Sylfaen: Georgian, Armenian, Cyrillic, Greek
        -- Windows — broad script/language coverage
        wf .. "msyh.ttc",       -- Microsoft YaHei (CJK)
        wf .. "msgothic.ttc",   -- MS Gothic (Japanese)
        wf .. "malgun.ttf",     -- Malgun Gothic (Korean)
        wf .. "Nirmala.ttc",    -- Nirmala UI (South Asian scripts)
        wf .. "ebrima.ttf",     -- Ebrima (African scripts)
        wf .. "gadugi.ttf",     -- Gadugi (Cherokee, Unified Canadian Aboriginal)
        wf .. "seguihis.ttf",   -- Segoe UI Historic (ancient scripts)
        wf .. "cambria.ttc",    -- Cambria (additional Latin + Greek + Cyrillic)
        wf .. "calibri.ttf",    -- Calibri (extended Latin)
        -- Windows — emoji / colour symbols (checked last)
        wf .. "seguiemj.ttf",   -- Segoe UI Emoji (colour emoji)
        -- macOS
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/Apple Symbols.ttf",
        "/System/Library/Fonts/Supplemental/Lucida Grande.ttf",
        "/System/Library/Fonts/Supplemental/AppleGothic.ttf",
        "/System/Library/Fonts/Apple Color Emoji.ttc",
        -- Linux — broad coverage
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/opentype/noto/NotoSansMono-Regular.otf",
        "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansSymbols-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansSymbols2-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf",
        "/usr/share/fonts/truetype/unifont/unifont.ttf",
    }

    -- LuaJIT (Love2D) exposes unpack as a global in 5.1 mode and as
    -- table.unpack in 5.2-compat mode.  Support both.
    local _unpack = table.unpack or unpack

    -- Load a font from an absolute OS path by reading raw bytes with io.open,
    -- then handing the data to Love2D as FileData.  This fully bypasses Love2D's
    -- sandboxed VFS (PhysFS), which does not reliably resolve absolute system
    -- paths on all platforms.  Returns a Font object, or nil on any failure.
    local function font_from_path(path, size)
        local fh = io.open(path, "rb")
        if not fh then return nil end
        local bytes = fh:read("*a")
        fh:close()
        if not bytes or #bytes < 16 then return nil end
        local ok, result = pcall(function()
            local fd = love.filesystem.newFileData(bytes, "font")
            return love.graphics.newFont(fd, size)
        end)
        return (ok and result) or nil
    end

    local function try_font(paths, size)
        for _, p in ipairs(paths) do
            local f = font_from_path(p, size)
            if f then return f end
        end
        return love.graphics.newFont(size)   -- Love2D built-in as last resort
    end

    local function add_fallbacks(primary, size)
        local fbs = {}
        for _, p in ipairs(fallback_candidates) do
            local f = font_from_path(p, size)
            if f then fbs[#fbs + 1] = f end
        end
        if #fbs > 0 then
            -- setFallbacks was added in Love2D 11.0; pcall guards older builds.
            pcall(function() primary:setFallbacks(_unpack(fbs)) end)
        end
    end

    font_sm = try_font(mono_candidates, 11)
    font_md = try_font(mono_candidates, 13)
    font_ui = try_font(ui_candidates,   13)

    add_fallbacks(font_sm, 11)
    add_fallbacks(font_md, 13)
    add_fallbacks(font_ui, 13)
end

--------------------------------------------------------------------
-- Subprocess helper (no-window on Windows)
--------------------------------------------------------------------
local WP = require "modules.winpipe"

--------------------------------------------------------------------
-- State
--------------------------------------------------------------------
local function load_default_scan_path()
    local base   = os.getenv("CODE_REVIEW_DIR") or "."
    local script = base .. "/scan_config.py"
    local cmd    = PYTHON_CMD .. " " .. shell_escape(script) .. " default-scan-path"
    local iter   = WP.lines_live(cmd)
    if iter then
        local line = iter()  -- read first line; ignore rest
        if line and line ~= "" then return line end
    end
    return base .. "/../.."
end

local scan_path    = load_default_scan_path()
local status       = "READY"   -- READY | RUNNING | DONE | ERROR
local results      = {}        -- {text, kind}  kind: result|error|report|dim|head|sep
local MAX_RESULT_LINES = 5000
local scroll_y     = 0
local max_scroll   = 0
local last_report  = nil
local path_editing = false
local path_buf     = scan_path
local about_open   = false
local browser_nav_focus = true

-- Which main panel to show when editor has tabs open.
-- "results" = show code review results, "editor" = show editor.
-- Opening a file auto-switches to "editor"; user can switch back via toolbar or Ctrl+E.
local _panel = "results"

-- cached pixel width of the perf text slot; set once on first draw so the
-- right-anchored perf string never shifts as fps/scan values change
local _perf_slot_w = nil

-- text selection in results panel
local sel_anchor   = nil   -- line index where drag started
local sel_cur      = nil   -- line index at current mouse position
local sel_dragging = false

-- right-click context menu
local ctx_menu     = { open=false, x=0, y=0 }
local CTX_ITEMS    = {
    { label="Copy selection",  key="copy_sel"  },
    { label="Copy all",        key="copy_all"  },
    { label="Clear results",   key="clear"     },
}

local counts = { critical=0, high=0, medium=0, low=0, info=0 }
local perf = {
    fps = 0,
    frame_ms = 0,
    worst_ms = 0,
    scan_start = nil,
    last_scan_s = 0,
}

--------------------------------------------------------------------
-- Modules
--------------------------------------------------------------------
local Bridge  = require "modules.bridge"
local Menu    = require "modules.menu"
local Browser = require "modules.browser"
local Editor  = require "modules.editor"
local Terminal = require "modules.terminal"
local ColorPicker = require "modules.colorpicker"
local History = require "modules.history"
local AC      = require "modules.autocomplete"

--------------------------------------------------------------------
-- Results helpers
--------------------------------------------------------------------
local function add_line(text, kind)
    results[#results+1] = { text = text or "", kind = kind or "result" }
    if #results > MAX_RESULT_LINES then
        local drop = #results - MAX_RESULT_LINES
        for _ = 1, drop do table.remove(results, 1) end
    end
    -- auto-scroll
    scroll_y = 1e9  -- clamped in draw
end

local function classify_kind(text)
    local up = text:upper()
    -- plain = true avoids pattern errors from special chars in scanner output
    if up:find("CRITICAL", 1, true) then
        counts.critical = counts.critical + 1; return "critical"
    elseif up:find("[HIGH]", 1, true) or up:find(": HIGH", 1, true) or up:find("HIGH]", 1, true) then
        counts.high = counts.high + 1; return "high"
    elseif up:find("MEDIUM", 1, true) then
        counts.medium = counts.medium + 1; return "medium"
    elseif up:find("[LOW]", 1, true) or up:find(": LOW", 1, true) or up:find("LOW]", 1, true) then
        counts.low = counts.low + 1; return "low"
    -- "ERROR " / "WARNING " (trailing space avoids matching names like error_handler)
    elseif up:find("ERROR ", 1, true) then
        return "error"
    elseif up:find("WARNING ", 1, true) or up:find(": WARN", 1, true) then
        counts.medium = counts.medium + 1; return "medium"
    elseif up:find("INFO", 1, true) or up:find("NOTE", 1, true) then
        counts.info = counts.info + 1; return "info"
    end
    return "result"
end

local function kind_colour(kind)
    if kind == "critical" then return C.red
    elseif kind == "high"  then return C.orange
    elseif kind == "medium"then return C.yellow
    elseif kind == "low"   then return C.green
    elseif kind == "info"  then return C.cyan
    elseif kind == "error" then return C.red
    elseif kind == "report"then return C.violet
    elseif kind == "dim"   then return C.grey
    elseif kind == "head"  then return C.lavender
    elseif kind == "sep"   then return C.dark
    end
    return C.text
end

local function reset_counts()
    counts = { critical=0, high=0, medium=0, low=0, info=0 }
end

--------------------------------------------------------------------
-- Bridge path (resolve relative to this script)
--------------------------------------------------------------------
local function bridge_path()
    local info = love.filesystem.getInfo and love.filesystem.getInfo("") or nil
    -- resolve via the working dir set by run.sh (passed as arg or env)
    local base = os.getenv("CODE_REVIEW_DIR") or "."
    return base .. "/bridge.py"
end

--------------------------------------------------------------------
-- Actions (injected into Menu)
--------------------------------------------------------------------
local actions = {}

local function run_scan(cmd_name, label)
    if Bridge.streaming then return end
    add_line("", "sep")
    add_line("---- " .. label .. " ----  " .. scan_path, "head")
    status = "RUNNING"
    perf.scan_start = love.timer.getTime()
    reset_counts()
    Bridge.start({cmd_name, scan_path},
        function(text, kind)
            local k = kind == "error" and "error" or classify_kind(text)
            add_line(text, k)
        end,
        function(report)
            last_report = report
            status = "DONE"
            if perf.scan_start then
                perf.last_scan_s = love.timer.getTime() - perf.scan_start
                perf.scan_start = nil
            end
            add_line("", "sep")
            add_line("Finished.  " .. (report and ("Report: " .. report) or "No report written."), "report")
        end
    )
end

actions.set_path = function()
    path_editing = true
    path_buf     = scan_path
end

actions.open_reports = function()
    local dir = os.getenv("CODE_REVIEW_DIR") or "."
    if IS_WINDOWS then
        local wdir = (dir .. "\\reports"):gsub("/", "\\")
        os.execute('explorer "' .. wdir .. '"')
    else
        os.execute("open \"" .. dir .. "/reports\" 2>/dev/null || xdg-open \"" .. dir .. "/reports\" 2>/dev/null &")
    end
end

actions.quit = function()
    love.event.quit()
end

actions.scan_security  = function() run_scan("security_scan", "Security Scan") end
actions.scan_god_funcs = function() run_scan("god_funcs",     "God Functions") end
actions.scan_lines     = function() run_scan("lines_count",   "Line Count")    end
actions.scan_py_audit  = function() run_scan("py_audit",      "Py Audit")      end
actions.scan_smells    = function() run_scan("code_smells",   "Code Smells")   end
actions.scan_orphans   = function() run_scan("orphaned_code", "Orphaned Code") end
actions.scan_c_memsafe = function() run_scan("c_memory_safety", "C Memory Safety") end
actions.scan_c_lint    = function() run_scan("c_lint",        "C/C++ Lint")    end
actions.scan_all       = function() run_scan("run_all",       "Run All Scans") end
actions.toggle_terminal = function() Terminal.toggle() end
actions.toggle_color_picker = function() ColorPicker.toggle() end
actions.toggle_autocomplete = function() AC.toggle() end
actions.toggle_ac_ai        = function() AC.toggle_ai() end

actions.toggle_browser = function() Browser.toggle() end
actions.scroll_bottom  = function() scroll_y = 1e9 end

actions.clear_results  = function()
    results      = {}
    last_report  = nil
    reset_counts()
    status       = "READY"
    scroll_y     = 0
end

actions.about = function() about_open = true end

--------------------------------------------------------------------
-- love.load
--------------------------------------------------------------------
function love.load()
    W = love.graphics.getWidth()
    H = love.graphics.getHeight()

    load_fonts()

    -- Enable OS-level key repeat so held keys (backspace, arrows, etc.)
    -- fire love.keypressed repeatedly at the system repeat rate.
    love.keyboard.setKeyRepeat(true)

    -- DPI normalisation: keep mouse event coordinates in logical pixels
    -- (guards against SDL2 reporting physical pixels on high-DPI Windows displays)
    _dpi = (love.window.getDPIScale and love.window.getDPIScale()) or 1
    if _dpi ~= 1 then
        local _orig_get = love.mouse.getPosition
        love.mouse.getPosition = function()
            local x, y = _orig_get()
            return x / _dpi, y / _dpi
        end
    end

    Bridge.init(bridge_path())
    Menu.init(actions, C)
    Browser.init(C,
        function(path)
            scan_path = path
            Browser.set_root(path)
            Terminal.set_default_cwd(path)
        end,
        function(path)
            Editor.open_file(path)
            _panel = "editor"
            browser_nav_focus = false
            Browser.set_keyboard_focus(false)
        end
    )
    Terminal.init(C, scan_path)
    ColorPicker.init(C)
    History._open()   -- open (or create) the snapshot DB in Documents
    AC.init(os.getenv("CODE_REVIEW_DIR") or scan_path)
    Browser.set_root(scan_path)
    Browser.set_keyboard_focus(true)
    CHAR_H = font_sm:getHeight()

    add_line("Code Review TUI  - Live CSS Editor Suite", "head")
    add_line("Select a directory in the browser or type a path above, then choose a scan.", "dim")
    add_line("Startup path and skipped folders come from dev-tools/code-review/scan_config.json.", "dim")
    add_line("", "sep")
end

--------------------------------------------------------------------
-- love.update
--------------------------------------------------------------------
function love.update(dt)
    W = love.graphics.getWidth()
    H = love.graphics.getHeight()

    local ms = dt * 1000
    if perf.frame_ms == 0 then
        perf.frame_ms = ms
    else
        perf.frame_ms = perf.frame_ms * 0.9 + ms * 0.1
    end
    perf.fps = dt > 0 and (1 / dt) or 0
    perf.worst_ms = math.max(ms, perf.worst_ms * 0.98)

    -- Auto-scroll the editor while the user is drag-selecting.
    local umx, umy = love.mouse.getPosition()
    if _dpi ~= 1 then umx = umx / _dpi; umy = umy / _dpi end
    Editor.update(dt, umx, umy)

    Bridge.poll()
    Terminal.poll()
end

--------------------------------------------------------------------
-- Layout helpers
--------------------------------------------------------------------
local function content_x() return Browser.width() end
local function content_y() return MENUBAR_H + TOOLBAR_H end
local function content_w() return W - Browser.width() end
local function content_h() return H - MENUBAR_H - TOOLBAR_H - SUMMARY_H end

local RESULT_LINE_H = 16

--------------------------------------------------------------------
-- Draw toolbar
--------------------------------------------------------------------
local function draw_toolbar()
    local y = MENUBAR_H
    gc("toolbar_bg")
    fill_rect(0, y, W, TOOLBAR_H)
    gc("border")
    love.graphics.line(0, y + TOOLBAR_H, W, y + TOOLBAR_H)

    local mx, my = love.mouse.getPosition()

    -- Row 1: title on left, status badge on right
    -- Badge occupies y+3 .. y+23 (height 20). Center all Row 1 text within that box.
    local row1_box_y = y + 3
    local row1_box_h = 20
    local ui_oy  = math.floor((row1_box_h - font_ui:getHeight()) / 2)
    local sm_oy  = math.floor((row1_box_h - font_sm:getHeight()) / 2)

    love.graphics.setFont(font_ui)
    gc("lavender")
    text_at(12, row1_box_y + ui_oy, "Code Review")

    local badge_colours = {
        READY   = C.dim,
        RUNNING = C.violet,
        DONE    = C.green,
        ERROR   = C.red,
    }
    local sc = badge_colours[status] or C.dim
    love.graphics.setFont(font_ui)
    gc("dim")
    local status_label = "Status:"
    local slw = font_ui:getWidth(status_label)
    local badge_w = 72
    local badge_x = W - badge_w - 12
    local sl_x    = badge_x - slw - 8
    text_at(sl_x, row1_box_y + ui_oy, status_label)
    love.graphics.setColor(sc)
    fill_rect(badge_x, row1_box_y, badge_w, row1_box_h, 4)
    gc("bg")
    love.graphics.setFont(font_sm)
    text_at(badge_x + math.floor((badge_w - font_sm:getWidth(status)) / 2),
            row1_box_y + sm_oy, status)

    -- Row 2: path label + input — both centered within the input box height
    local ph  = 20
    local py  = y + 30
    local ui_label_oy = math.floor((ph - font_ui:getHeight()) / 2)
    local sm_text_oy  = math.floor((ph - font_sm:getHeight()) / 2)

    love.graphics.setFont(font_ui)
    gc("dim")
    local path_label = "Path:"
    text_at(12, py + ui_label_oy, path_label)
    local plw = font_ui:getWidth(path_label)
    local px, pw = 12 + plw + 8, W - 12 - 12 - plw - 8
    if path_editing then
        gc("panel_bg")
        fill_rect(px, py, pw, ph, 3)
        gc("violet")
        stroke_rect(px, py, pw, ph, 3)
        gc("text_bright")
        love.graphics.setFont(font_sm)
        text_at(px + 5, py + sm_text_oy, trunc_str(path_buf, pw - 10, font_sm) .. (math.floor(love.timer.getTime() * 2) % 2 == 0 and "|" or ""))
    else
        gc("panel_bg")
        fill_rect(px, py, pw, ph, 3)
        gc("border")
        stroke_rect(px, py, pw, ph, 3)
        gc("text")
        love.graphics.setFont(font_sm)
        text_at(px + 5, py + sm_text_oy, trunc_str(scan_path, pw - 10, font_sm))
    end

    -- Row 3: scan buttons left to right (fixed positions — no prepend/append)
    local btns = {
        { label="Security",  action=actions.scan_security  },
        { label="God Funcs", action=actions.scan_god_funcs },
        { label="Lines",     action=actions.scan_lines     },
        { label="Py Audit",  action=actions.scan_py_audit  },
        { label="Smells",    action=actions.scan_smells    },
        { label="Orphans",   action=actions.scan_orphans   },
        { label="C Safe",    action=actions.scan_c_memsafe },
        { label="C Lint",    action=actions.scan_c_lint    },
        { label="Terminal",  action=actions.toggle_terminal },
        { label="Colors",    action=actions.toggle_color_picker },
        { label=AC.is_enabled() and "AC:on" or "AC:off", action=actions.toggle_autocomplete },
        { label="> All",     action=actions.scan_all, accent=true },
    }
    local btn_h  = 20
    local btn_y  = y + 58
    local btn_cx = 12
    love.graphics.setFont(font_sm)
    for _, b in ipairs(btns) do
        local bw2 = font_sm:getWidth(b.label) + 20
        b._x, b._y, b._w, b._h = btn_cx, btn_y, bw2, btn_h

        local hov = mx >= btn_cx and mx < btn_cx + bw2 and my >= btn_y and my < btn_y + btn_h

        if b.accent then
            love.graphics.setColor(hov and C.violet or C.accent)
        else
            love.graphics.setColor(hov and C.hover or C.panel_bg)
        end
        fill_rect(btn_cx, btn_y, bw2, btn_h, 3)
        love.graphics.setColor(hov and C.text_bright or C.text)
        stroke_rect(btn_cx, btn_y, bw2, btn_h, 3)
        gc(hov and "text_bright" or "text")
        text_at(btn_cx + 10, btn_y + 3, b.label)
        btn_cx = btn_cx + bw2 + 6
    end

    -- Panel toggle: fixed right-anchored, never shifts the scan buttons above
    if Editor.has_tabs() then
        -- reserve a fixed width wide enough for both labels so the button never resizes
        local tog_label  = (_panel == "editor") and "< Review" or "Editor >"
        local tog_action = (_panel == "editor") and function() _panel="results" end
                                                 or function() _panel="editor"  end
        local tog_accent = (_panel == "editor")
        -- fixed slot: pick the wider of the two labels once
        local slot_w = math.max(font_sm:getWidth("< Review"), font_sm:getWidth("Editor >")) + 20
        local tog_x  = W - slot_w - 12
        local hov    = mx >= tog_x and mx < tog_x + slot_w and my >= btn_y and my < btn_y + btn_h
        love.graphics.setColor(tog_accent and (hov and C.violet or C.accent)
                                          or  (hov and C.hover  or C.panel_bg))
        fill_rect(tog_x, btn_y, slot_w, btn_h, 3)
        love.graphics.setColor(hov and C.text_bright or C.text)
        stroke_rect(tog_x, btn_y, slot_w, btn_h, 3)
        gc(hov and "text_bright" or "text")
        -- center label within the fixed slot
        local lw = font_sm:getWidth(tog_label)
        text_at(tog_x + math.floor((slot_w - lw) / 2), btn_y + 3, tog_label)
        table.insert(btns, { label=tog_label, action=tog_action,
                              _x=tog_x, _y=btn_y, _w=slot_w, _h=btn_h })
    end

    _toolbar_btns = btns
end

_toolbar_btns = {}

--------------------------------------------------------------------
-- Draw results panel
--------------------------------------------------------------------
local function draw_results()
    local rx = content_x()
    local ry = content_y()
    local rw = content_w()
    local rh = content_h()

    gc("bg")
    fill_rect(rx, ry, rw, rh)

    -- Compute total needed height
    local total_h = #results * RESULT_LINE_H + 8
    max_scroll = math.max(0, total_h - rh)
    scroll_y   = math.max(0, math.min(scroll_y, max_scroll))

    -- selection range (normalised)
    local sel_lo, sel_hi
    if sel_anchor and sel_cur then
        sel_lo = math.min(sel_anchor, sel_cur)
        sel_hi = math.max(sel_anchor, sel_cur)
    end

    love.graphics.setScissor(rx, ry, rw, rh)
    love.graphics.setFont(font_sm)

    local first_idx = math.max(1, math.floor(scroll_y / RESULT_LINE_H) + 1)
    local visible_count = math.ceil(rh / RESULT_LINE_H) + 2
    local last_idx = math.min(#results, first_idx + visible_count)

    local iy = ry + 6 - scroll_y + (first_idx - 1) * RESULT_LINE_H
    for i = first_idx, last_idx do
        local r = results[i]
        -- highlight selected lines
        if sel_lo and i >= sel_lo and i <= sel_hi then
            love.graphics.setColor(0.33, 0.0, 0.8, 0.28)
            fill_rect(rx, iy, rw - 8, RESULT_LINE_H)
        end
        love.graphics.setColor(kind_colour(r.kind))
        if r.kind == "sep" then
            love.graphics.setColor(C.dark)
            love.graphics.line(rx + 8, iy + RESULT_LINE_H/2, rx + rw - 8, iy + RESULT_LINE_H/2)
        else
            text_at(rx + 10, iy, r.text)
        end
        iy = iy + RESULT_LINE_H
    end

    love.graphics.setScissor()

    -- Scrollbar
    if max_scroll > 0 then
        local sb_w     = 5
        local sb_x     = rx + rw - sb_w - 2
        local ratio    = scroll_y / max_scroll
        local thumb_h  = math.max(20, rh * rh / total_h)
        local thumb_y  = ry + (rh - thumb_h) * ratio
        love.graphics.setColor(C.border)
        fill_rect(sb_x, ry, sb_w, rh, 2)
        love.graphics.setColor(C.violet)
        fill_rect(sb_x, thumb_y, sb_w, thumb_h, 2)
    end

    -- Context menu
    if ctx_menu.open then
        local item_h = 22
        local mw     = 160
        local mh     = #CTX_ITEMS * item_h + 6
        local cmx    = math.min(ctx_menu.x, W - mw - 4)
        local cmy    = math.min(ctx_menu.y, H - mh - 4)
        local mmx, mmy = love.mouse.getPosition()
        love.graphics.setColor(C.menu_bg)
        fill_rect(cmx, cmy, mw, mh, 4)
        love.graphics.setColor(C.border)
        stroke_rect(cmx, cmy, mw, mh, 4)
        love.graphics.setFont(font_sm)
        for ci, item in ipairs(CTX_ITEMS) do
            local ity = cmy + 3 + (ci - 1) * item_h
            local hov = mmx >= cmx and mmx < cmx + mw and mmy >= ity and mmy < ity + item_h
            if hov then
                love.graphics.setColor(C.accent)
                fill_rect(cmx + 2, ity, mw - 4, item_h, 3)
            end
            gc(hov and "text_bright" or "text")
            text_at(cmx + 10, ity + 4, item.label)
        end
    end
end

--------------------------------------------------------------------
-- Draw summary bar
--------------------------------------------------------------------
local function draw_summary()
    local sy = H - SUMMARY_H
    gc("menu_bg")
    fill_rect(0, sy, W, SUMMARY_H)
    gc("border")
    love.graphics.line(0, sy, W, sy)

    local pills = {
        { label="CRITICAL", key="critical", col=C.red    },
        { label="HIGH",     key="high",     col=C.orange },
        { label="MEDIUM",   key="medium",   col=C.yellow },
        { label="LOW",      key="low",      col=C.green  },
        { label="INFO",     key="info",     col=C.cyan   },
    }

    love.graphics.setFont(font_sm)
    -- reserve a fixed minimum width for the count so pills don't resize as numbers grow
    local count_slot = font_sm:getWidth("9999")
    local px = 10
    for _, p in ipairs(pills) do
        local base_w = font_sm:getWidth(p.label .. " ") + count_slot + 16
        local pw     = math.max(base_w, font_sm:getWidth(p.label .. " " .. tostring(counts[p.key])) + 16)
        love.graphics.setColor(p.col[1], p.col[2], p.col[3], 0.25)
        fill_rect(px, sy + 6, pw, SUMMARY_H - 12, 4)
        love.graphics.setColor(p.col)
        stroke_rect(px, sy + 6, pw, SUMMARY_H - 12, 4)
        text_at(px + 8, sy + 9, p.label .. " " .. tostring(counts[p.key]))
        px = px + pw + 8
    end

    local qd = Bridge.queue_depth and Bridge.queue_depth() or 0
    -- Build perf string with fixed-width fields so the right-anchored text never shifts.
    -- fps: 3-digit, frame_ms: 5 chars (xx.x), q: 3-digit, optional suffix: fixed 12 chars
    local perf_base   = string.format("fps:%3d  frame:%4.1fms  q:%-3d",
                                      math.floor(perf.fps + 0.5), perf.frame_ms, qd)
    local perf_suffix = ""
    if status == "RUNNING" and perf.scan_start then
        perf_suffix = string.format("  scan:%6.1fs", love.timer.getTime() - perf.scan_start)
    elseif perf.last_scan_s > 0 then
        perf_suffix = string.format("  last:%6.1fs", perf.last_scan_s)
    else
        perf_suffix = "              "   -- same char count as "  last:XXXX.Xs"
    end
    local perf_text = perf_base .. perf_suffix

    gc("dim")
    love.graphics.setFont(font_sm)
    -- anchor to the right edge using a fixed reserved width so perf_x never drifts
    if not _perf_slot_w then
        _perf_slot_w = font_sm:getWidth(perf_text)
    end
    local perf_x = W - _perf_slot_w - 12
    text_at(perf_x, sy + 9, perf_text)

    -- Last report left of perf diagnostics
    if last_report then
        gc("dim")
        local rtext = "Report: " .. last_report
        love.graphics.setFont(font_sm)
        local max_w = math.max(0, perf_x - px - 16)
        text_at(px, sy + 9, trunc_str(rtext, max_w, font_sm))
    end
end

--------------------------------------------------------------------
-- Draw about modal
--------------------------------------------------------------------
local function draw_about()
    if not about_open then return end
    local mw, mh = 440, 200
    local mx = (W - mw) / 2
    local my = (H - mh) / 2
    gc("menu_bg")
    fill_rect(mx, my, mw, mh, 6)
    gc("border")
    stroke_rect(mx, my, mw, mh, 6)
    love.graphics.setFont(font_md)
    gc("lavender")
    text_at(mx + 20, my + 20, "Code Review TUI")
    love.graphics.setFont(font_sm)
    gc("text")
    text_at(mx + 20, my + 50, "Part of the Live CSS Editor Suite.")
    text_at(mx + 20, my + 70, "Built with Love2D + Lua + Python.")
    gc("dim")
    text_at(mx + 20, my + 100, "Scanners: security_ck / god_funcs / lines_count / py_audit / code_smells / orphaned_code / c_memory_safety / c_linter")
    text_at(mx + 20, my + 118, "Reports written to:  dev-tools/code-review/reports/")
    gc("violet")
    local cls = "[ Close ]"
    local cw  = font_sm:getWidth(cls) + 24
    fill_rect(mx + mw - cw - 16, my + mh - 36, cw, 24, 4)
    gc("text_bright")
    text_at(mx + mw - cw - 4, my + mh - 30, cls)
end

--------------------------------------------------------------------
-- love.draw
--------------------------------------------------------------------
function love.draw()
    gc("bg")
    love.graphics.clear(C.bg)

    -- Browser sidebar
    Browser.draw(0, MENUBAR_H + TOOLBAR_H, H - MENUBAR_H - TOOLBAR_H - SUMMARY_H, font_sm)

    -- Main panels
    draw_toolbar()
    if Terminal.is_visible() then
        Terminal.draw(content_x(), content_y(), content_w(), content_h(), font_sm)
    elseif Editor.has_tabs() and _panel == "editor" then
        Editor.draw(content_x(), content_y(), content_w(), content_h())
    else
        draw_results()
    end
    draw_summary()

    -- Menu on top
    Menu.draw(MENUBAR_H, font_ui)

    -- Color picker overlay (above content, below menu dropdowns handled by menu)
    ColorPicker.draw(font_sm, font_ui)

    -- About modal on very top
    draw_about()
end

--------------------------------------------------------------------
-- Copy helpers
--------------------------------------------------------------------
local function lines_to_text(lo, hi)
    local t = {}
    lo = math.max(1, lo or 1)
    hi = math.min(#results, hi or #results)
    for i = lo, hi do
        t[#t+1] = results[i].text
    end
    return table.concat(t, "\n")
end

local function line_idx_at(my)
    local ry = content_y()
    local offset = my - ry - 6 + scroll_y
    local idx = math.floor(offset / RESULT_LINE_H) + 1
    if idx < 1 then idx = 1 end
    if idx > #results then idx = #results end
    return idx
end

local function in_results(mx, my)
    return mx >= content_x() and mx < content_x() + content_w()
        and my >= content_y() and my < content_y() + content_h()
end

--------------------------------------------------------------------
-- Input
--------------------------------------------------------------------
function love.keypressed(key)
    local cmd = love.keyboard.isDown("lgui", "rgui")
    local ctrl = love.keyboard.isDown("lctrl", "rctrl") or cmd

    if ColorPicker.keypressed(key) then return end

    -- Ctrl+E: toggle between editor and review results when editor has tabs open.
    if ctrl and key == "e" and Editor.has_tabs() then
        _panel = (_panel == "editor") and "results" or "editor"
        return
    end

    if Terminal.is_visible() then
        if Terminal.keypressed(key, content_h(), font_sm) then
            return
        end
    end

    if not path_editing and key == "p" and ctrl then
        browser_nav_focus = true
        Browser.set_keyboard_focus(true)
        if Browser.open_finder then Browser.open_finder() end
        return
    end

    -- Tab (browser focus, no path editing) → jump to editor.
    -- Cmd/Ctrl+Right while browser has focus → jump to editor (Mac: Cmd+Right,
    -- Windows/Linux: Ctrl+Right).  This is the reverse of Cmd+Left-at-first-tab
    -- in the editor which sends focus to the browser.
    if not path_editing and browser_nav_focus and Editor.has_tabs() then
        if key == "tab" or (ctrl and key == "right") then
            browser_nav_focus = false
            Browser.set_keyboard_focus(false)
            _panel = "editor"
            return
        end
    end

    if not path_editing and (browser_nav_focus or not Editor.has_tabs()) then
        if not browser_nav_focus then
            browser_nav_focus = true
            Browser.set_keyboard_focus(true)
        end
        if Browser.keypressed(key) then
            -- Browser may have cleared its own kb_focus internally (e.g. on Escape
            -- or after opening a file).  Sync the main-level flag so that subsequent
            -- key events are routed correctly.
            if not Browser.has_keyboard_focus() then
                browser_nav_focus = false
            end
            return
        end
        if key == "escape" then
            browser_nav_focus = false
            Browser.set_keyboard_focus(false)
            return
        end
    end

    -- Route to editor when tabs are open and editor panel is active
    if Editor.has_tabs() and _panel == "editor" and not path_editing and not browser_nav_focus then
        if key == "escape" and Editor.active_tab() and not Editor.active_tab()._ctx then
            -- escape with no ctx: let editor clear selection; if nothing more, close top tab
        end
        if Editor.keypressed(key, content_h()) then
            if Editor.consume_browser_focus_request() then
                browser_nav_focus = true
                Browser.set_keyboard_focus(true)
            end
            return
        end
    end

    -- Cmd/Ctrl+C  copy selection
    if (key == "c") and love.keyboard.isDown("lgui", "rgui", "lctrl", "rctrl") then
        if sel_anchor and sel_cur then
            local lo = math.min(sel_anchor, sel_cur)
            local hi = math.max(sel_anchor, sel_cur)
            love.system.setClipboardText(lines_to_text(lo, hi))
        end
        return
    end
    if path_editing then
        if key == "return" or key == "escape" then
            if key == "return" then
                scan_path = path_buf
                Browser.set_root(scan_path)
                Terminal.set_default_cwd(scan_path)
            end
            path_editing = false
        elseif key == "backspace" then
            path_buf = path_buf:sub(1, -2)
        end
        return
    end
    if key == "escape" then
        if about_open   then about_open = false; return end
        if Menu.is_open() then Menu.close(); return end
    end
    if key == "q" and (love.keyboard.isDown("lgui", "rgui") or love.keyboard.isDown("lctrl", "rctrl")) then
        love.event.quit()
    end
end

function love.textinput(t)
    if Terminal.is_visible() and Terminal.textinput(t) then
        return
    end

    if browser_nav_focus and not path_editing and Browser.textinput and Browser.textinput(t) then
        return
    end

    if Editor.has_tabs() and not path_editing then
        if Editor.textinput(t) then return end
    end
    if path_editing then
        path_buf = path_buf .. t
    end
end

function love.mousepressed(mx, my, btn)
    if _dpi ~= 1 then mx = mx / _dpi; my = my / _dpi end
    -- Color picker gets first priority when visible (it's a floating overlay)
    if ColorPicker.mousepressed(mx, my, btn) then return end
    if Terminal.is_visible() then
        if Terminal.mousepressed(mx, my, btn, content_x(), content_y(), content_w(), content_h()) then
            return
        end
    end

    if browser_nav_focus and mx >= content_x() then
        browser_nav_focus = false
        Browser.set_keyboard_focus(false)
    end

    -- Close context menu on any click
    if ctx_menu.open then
        if btn == 1 then
            -- check if clicked on a context menu item
            local item_h = 22
            local mw     = 160
            local cmx    = math.min(ctx_menu.x, W - mw - 4)
            local cmy    = math.min(ctx_menu.y, H - (#CTX_ITEMS * item_h + 6) - 4)
            for ci, item in ipairs(CTX_ITEMS) do
                local ity = cmy + 3 + (ci - 1) * item_h
                if mx >= cmx and mx < cmx + mw and my >= ity and my < ity + item_h then
                    if item.key == "copy_sel" then
                        if sel_anchor and sel_cur then
                            local lo = math.min(sel_anchor, sel_cur)
                            local hi = math.max(sel_anchor, sel_cur)
                            love.system.setClipboardText(lines_to_text(lo, hi))
                        end
                    elseif item.key == "copy_all" then
                        love.system.setClipboardText(lines_to_text(1, #results))
                    elseif item.key == "clear" then
                        actions.clear_results()
                    end
                    break
                end
            end
        end
        ctx_menu.open = false
        return
    end

    -- About modal close
    if about_open then
        about_open = false
        return
    end

    -- Menu
    if my < MENUBAR_H then
        Menu.mousepressed(mx, my, btn, MENUBAR_H)
        return
    end
    if Menu.is_open() then
        Menu.mousepressed(mx, my, btn, MENUBAR_H)
        Menu.close()
        return
    end

    -- Toolbar buttons
    if my >= MENUBAR_H and my < MENUBAR_H + TOOLBAR_H then
        local _plw = font_ui:getWidth("Path:")
        local bx, by = 12 + _plw + 8, MENUBAR_H + 30
        local bw, bh = W - 12 - 12 - _plw - 8, 20
        if mx >= bx and mx < bx + bw and my >= by and my < by + bh then
            path_editing = true
            path_buf     = scan_path
            return
        end
        for _, b in ipairs(_toolbar_btns) do
            if b._x and mx >= b._x and mx < b._x + b._w and
               my >= b._y and my < b._y + b._h then
                if not Bridge.streaming then b.action() end
                return
            end
        end
        return
    end

    -- Results panel: left-click starts selection, right-click opens context menu
    if in_results(mx, my) then
        -- Route to editor only when currently showing the editor panel.
        if Editor.has_tabs() and _panel == "editor" then
            Editor.mousepressed(mx, my, btn, content_x(), content_y(), content_w(), content_h())
            return
        end
        if btn == 1 then
            local idx = line_idx_at(my)
            sel_anchor   = idx
            sel_cur      = idx
            sel_dragging = true
            return
        elseif btn == 2 then
            ctx_menu.open = true
            ctx_menu.x    = mx
            ctx_menu.y    = my
            return
        end
    end

    -- Browser
    if Browser.mousepressed(mx, my, btn) then
        browser_nav_focus = true
        Browser.set_keyboard_focus(true)
        return
    end
end

function love.mousereleased(mx, my, btn)
    if _dpi ~= 1 then mx = mx / _dpi; my = my / _dpi end
    ColorPicker.mousereleased()
    Terminal.mousereleased(mx, my, btn)
    if btn == 1 then
        sel_dragging = false
    end
    Editor.mousereleased(mx, my, btn)
end

function love.mousemoved(mx, my, dx, dy)
    if _dpi ~= 1 then mx = mx / _dpi; my = my / _dpi end
    Menu.mousemoved(mx, my, MENUBAR_H)
    ColorPicker.mousemoved(mx, my)
    Terminal.mousemoved(mx, my)
    -- extend drag selection in editor (only when editor panel is active)
    if Editor.has_tabs() and _panel == "editor" then
        Editor.mousemoved(mx, my)
    end
    -- extend selection while dragging in results panel
    if sel_dragging and in_results(mx, my) then
        sel_cur = line_idx_at(my)
    end
end

function love.wheelmoved(wx, wy)
    local mx, my = love.mouse.getPosition()
    if Terminal.is_visible() and mx >= content_x() and my >= content_y() and my < H - SUMMARY_H then
        if Terminal.wheelmoved(wy) then return end
    end
    if Browser.wheelmoved(wx, wy, mx, my) then return end
    -- Route to editor only when the editor panel is active.
    if Editor.has_tabs() and _panel == "editor" and mx >= content_x() then
        Editor.wheelmoved(wx, wy, content_x(), content_y(), content_w(), content_h(), mx, my)
        return
    end
    -- Scroll results
    if mx >= content_x() and my >= content_y() and my < H - SUMMARY_H then
        scroll_y = scroll_y - wy * 40
    end
end

function love.resize(w, h)
    W, H = w, h
    _dpi = (love.window.getDPIScale and love.window.getDPIScale()) or 1
end

function love.quit()
    if Terminal and Terminal.save then
        Terminal.save()
    end
end
