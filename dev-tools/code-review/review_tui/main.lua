-- main.lua  –  Code Review TUI
-- Love2D entry point.  Matches Live CSS Editor dark-purple theme.

--------------------------------------------------------------------
-- Globals used by modules
--------------------------------------------------------------------
W, H = 1060, 720   -- updated each frame
MENUBAR_H  = 24
TOOLBAR_H  = 72
SUMMARY_H  = 32

-- Shell escape a single argument safely
function shell_escape(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
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
}

--------------------------------------------------------------------
-- Draw helpers
--------------------------------------------------------------------
local function gc(key)  love.graphics.setColor(C[key] or C.text) end

local function fill_rect(x, y, w, h, r)
    love.graphics.rectangle("fill", x, y, w, h, r or 0)
end

local function stroke_rect(x, y, w, h, r)
    love.graphics.rectangle("line", x, y, w, h, r or 0)
end

local function text_at(x, y, s)
    love.graphics.print(s, math.floor(x), math.floor(y))
end

local function trunc_str(s, max_w, font)
    if not s then return "" end
    if font:getWidth(s) <= max_w then return s end
    local t = s
    while #t > 1 and font:getWidth(t .. "…") > max_w do
        t = t:sub(1, -2)
    end
    return t .. "…"
end

--------------------------------------------------------------------
-- Fonts
--------------------------------------------------------------------
local font_sm, font_md, font_ui

local function load_fonts()
    local mono_candidates = {
        "/System/Library/Fonts/Menlo.ttc",
        "/Library/Fonts/JetBrainsMono-Regular.ttf",
        "/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMono-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    }
    local ui_candidates = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNSText.ttf",
    }

    local function try_font(paths, size)
        for _, p in ipairs(paths) do
            local ok, f = pcall(love.graphics.newFont, p, size)
            if ok and f then return f end
        end
        return love.graphics.newFont(size)
    end

    font_sm = try_font(mono_candidates, 11)
    font_md = try_font(mono_candidates, 13)
    font_ui = try_font(ui_candidates,   13)
end

--------------------------------------------------------------------
-- State
--------------------------------------------------------------------
local scan_path    = os.getenv("HOME") or "/"
local status       = "READY"   -- READY | RUNNING | DONE | ERROR
local results      = {}        -- {text, kind}  kind: result|error|report|dim|head|sep
local scroll_y     = 0
local max_scroll   = 0
local last_report  = nil
local path_editing = false
local path_buf     = scan_path
local about_open   = false

local counts = { critical=0, high=0, medium=0, low=0, info=0 }

--------------------------------------------------------------------
-- Modules
--------------------------------------------------------------------
local Bridge  = require "modules.bridge"
local Menu    = require "modules.menu"
local Browser = require "modules.browser"

--------------------------------------------------------------------
-- Results helpers
--------------------------------------------------------------------
local function add_line(text, kind)
    results[#results+1] = { text = text or "", kind = kind or "result" }
    -- auto-scroll
    scroll_y = 1e9  -- clamped in draw
end

local function classify_kind(text)
    local up = text:upper()
    if up:find("CRITICAL") then counts.critical = counts.critical + 1; return "critical"
    elseif up:find("%bHIGH%b") then counts.high = counts.high + 1; return "high"
    elseif up:find("MEDIUM") then counts.medium = counts.medium + 1; return "medium"
    elseif up:find("%bLOW%b" ) then counts.low  = counts.low  + 1; return "low"
    elseif up:find("INFO") or up:find("NOTE") then counts.info = counts.info + 1; return "info"
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
    add_line("──── " .. label .. " ────  " .. scan_path, "head")
    status = "RUNNING"
    reset_counts()
    Bridge.start({cmd_name, scan_path},
        function(text, kind)
            local k = kind == "error" and "error" or classify_kind(text)
            add_line(text, k)
        end,
        function(report)
            last_report = report
            status = "DONE"
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
    os.execute("open \"" .. dir .. "/reports\" 2>/dev/null || xdg-open \"" .. dir .. "/reports\" 2>/dev/null &")
end

actions.quit = function()
    love.event.quit()
end

actions.scan_security  = function() run_scan("security_scan", "Security Scan") end
actions.scan_god_funcs = function() run_scan("god_funcs",     "God Functions") end
actions.scan_lines     = function() run_scan("lines_count",   "Line Count")    end
actions.scan_py_audit  = function() run_scan("py_audit",      "Py Audit")      end
actions.scan_smells    = function() run_scan("code_smells",   "Code Smells")   end
actions.scan_all       = function() run_scan("run_all",       "Run All Scans") end

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

    Bridge.init(bridge_path())
    Menu.init(actions, C)
    Browser.init(C, function(path)
        scan_path = path
        Browser.set_root(path)
    end)

    add_line("Code Review TUI  — Live CSS Editor Suite", "head")
    add_line("Select a directory in the browser or type a path above, then choose a scan.", "dim")
    add_line("", "sep")
end

--------------------------------------------------------------------
-- love.update
--------------------------------------------------------------------
function love.update(dt)
    W = love.graphics.getWidth()
    H = love.graphics.getHeight()
    Bridge.poll()
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

    love.graphics.setFont(font_ui)

    -- Title
    gc("lavender")
    text_at(12, y + 8, "Code Review")

    -- Path label
    gc("dim")
    text_at(12, y + 32, "Path:")

    -- Path input box
    local bx, by, bw, bh = 54, y + 28, W - 300, 20
    if path_editing then
        gc("panel_bg")
        fill_rect(bx, by, bw, bh, 3)
        gc("violet")
        stroke_rect(bx, by, bw, bh, 3)
        gc("text_bright")
        love.graphics.setFont(font_sm)
        text_at(bx + 5, by + 3, trunc_str(path_buf, bw - 10, font_sm) .. (math.floor(love.timer.getTime() * 2) % 2 == 0 and "▌" or ""))
    else
        gc("panel_bg")
        fill_rect(bx, by, bw, bh, 3)
        gc("border")
        stroke_rect(bx, by, bw, bh, 3)
        gc("text")
        love.graphics.setFont(font_sm)
        text_at(bx + 5, by + 3, trunc_str(scan_path, bw - 10, font_sm))
    end

    -- Status badge
    local badge_colours = {
        READY   = C.dim,
        RUNNING = C.violet,
        DONE    = C.green,
        ERROR   = C.red,
    }
    local bx2 = W - 240
    love.graphics.setFont(font_ui)
    gc("dim")
    text_at(bx2, y + 8, "Status:")
    local sc = badge_colours[status] or C.dim
    love.graphics.setColor(sc)
    fill_rect(bx2 + 58, y + 5, 70, 18, 4)
    gc("bg")
    love.graphics.setFont(font_sm)
    text_at(bx2 + 58 + (70 - font_sm:getWidth(status)) / 2, y + 8, status)

    -- Scan buttons row
    local btns = {
        { label="Security",  action=actions.scan_security  },
        { label="God Funcs", action=actions.scan_god_funcs },
        { label="Lines",     action=actions.scan_lines     },
        { label="Py Audit",  action=actions.scan_py_audit  },
        { label="Smells",    action=actions.scan_smells    },
        { label="▶ All",     action=actions.scan_all, accent=true },
    }
    local bx3 = bx2 - 10
    local btn_h = 20
    love.graphics.setFont(font_sm)
    for i = #btns, 1, -1 do
        local b   = btns[i]
        local bw2 = font_sm:getWidth(b.label) + 16
        bx3 = bx3 - bw2 - 6
        b._x, b._y, b._w, b._h = bx3, y + 28, bw2, btn_h

        local mx, my = love.mouse.getPosition()
        local hov = mx >= bx3 and mx < bx3 + bw2 and my >= y+28 and my < y+28+btn_h

        if b.accent then
            love.graphics.setColor(hov and C.violet or C.accent)
        else
            love.graphics.setColor(hov and C.hover or C.panel_bg)
        end
        fill_rect(bx3, y+28, bw2, btn_h, 3)
        love.graphics.setColor(hov and C.text_bright or C.text)
        stroke_rect(bx3, y+28, bw2, btn_h, 3)
        gc(hov and "text_bright" or "text")
        text_at(bx3 + 8, y + 31, b.label)
    end
    -- store for click detection
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

    love.graphics.setScissor(rx, ry, rw, rh)
    love.graphics.setFont(font_sm)

    local iy = ry + 6 - scroll_y
    for _, r in ipairs(results) do
        if iy + RESULT_LINE_H >= ry and iy < ry + rh then
            love.graphics.setColor(kind_colour(r.kind))
            if r.kind == "sep" then
                love.graphics.setColor(C.dark)
                love.graphics.line(rx + 8, iy + RESULT_LINE_H/2, rx + rw - 8, iy + RESULT_LINE_H/2)
            else
                text_at(rx + 10, iy, r.text)
            end
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
    local px = 10
    for _, p in ipairs(pills) do
        local num = tostring(counts[p.key])
        local lbl = p.label .. " " .. num
        local pw  = font_sm:getWidth(lbl) + 16
        love.graphics.setColor(p.col[1], p.col[2], p.col[3], 0.25)
        fill_rect(px, sy + 6, pw, SUMMARY_H - 12, 4)
        love.graphics.setColor(p.col)
        stroke_rect(px, sy + 6, pw, SUMMARY_H - 12, 4)
        text_at(px + 8, sy + 9, lbl)
        px = px + pw + 8
    end

    -- Last report on the right
    if last_report then
        gc("dim")
        local rtext = "Report: " .. last_report
        love.graphics.setFont(font_sm)
        text_at(W - font_sm:getWidth(rtext) - 12, sy + 9, rtext)
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
    text_at(mx + 20, my + 100, "Scanners: security_ck · god_funcs · lines_count · py_audit · code_smells")
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
    draw_results()
    draw_summary()

    -- Menu on top
    Menu.draw(MENUBAR_H, font_ui)

    -- About modal on very top
    draw_about()
end

--------------------------------------------------------------------
-- Input
--------------------------------------------------------------------
function love.keypressed(key)
    if path_editing then
        if key == "return" or key == "escape" then
            if key == "return" then scan_path = path_buf end
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
    if key == "q" and love.keyboard.isDown("lgui", "rgui") then
        love.event.quit()
    end
end

function love.textinput(t)
    if path_editing then
        path_buf = path_buf .. t
    end
end

function love.mousepressed(mx, my, btn)
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
        -- Path box click
        local bx, by, bw, bh = 54, MENUBAR_H + 28, W - 300, 20
        if mx >= bx and mx < bx + bw and my >= by and my < by + bh then
            path_editing = true
            path_buf     = scan_path
            return
        end
        -- Scan buttons
        for _, b in ipairs(_toolbar_btns) do
            if b._x and mx >= b._x and mx < b._x + b._w and
               my >= b._y and my < b._y + b._h then
                if not Bridge.streaming then b.action() end
                return
            end
        end
        return
    end

    -- Browser
    if Browser.mousepressed(mx, my, btn) then return end
end

function love.mousemoved(mx, my, dx, dy)
    Menu.mousemoved(mx, my, MENUBAR_H)
end

function love.wheelmoved(wx, wy)
    local mx, my = love.mouse.getPosition()
    if Browser.wheelmoved(wx, wy, mx, my) then return end
    -- Scroll results
    if mx >= content_x() and my >= content_y() and my < H - SUMMARY_H then
        scroll_y = scroll_y - wy * 40
    end
end

function love.resize(w, h)
    W, H = w, h
end
