-- main.lua  -  Code Review TUI
-- Love2D entry point.  Matches Live CSS Editor dark-purple theme.

--------------------------------------------------------------------
-- Globals used by modules
--------------------------------------------------------------------
W, H = 1060, 720   -- updated each frame
MENUBAR_H  = 24
TOOLBAR_H  = 96
SUMMARY_H  = 32
CHAR_H     = 16
TABS_H     = 26

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

function fill_rect(x, y, w, h, r)
    love.graphics.rectangle("fill", x, y, w, h, r or 0)
end

function stroke_rect(x, y, w, h, r)
    love.graphics.rectangle("line", x, y, w, h, r or 0)
end

function text_at(x, y, s)
    love.graphics.print(s, math.floor(x), math.floor(y))
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

--------------------------------------------------------------------
-- Modules
--------------------------------------------------------------------
local Bridge  = require "modules.bridge"
local Menu    = require "modules.menu"
local Browser = require "modules.browser"
local Editor  = require "modules.editor"

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
    -- plain = true avoids pattern errors from special chars in scanner output
    if up:find("CRITICAL", 1, true) then
        counts.critical = counts.critical + 1; return "critical"
    elseif up:find("[HIGH]", 1, true) or up:find(": HIGH", 1, true) or up:find("HIGH]", 1, true) then
        counts.high = counts.high + 1; return "high"
    elseif up:find("MEDIUM", 1, true) then
        counts.medium = counts.medium + 1; return "medium"
    elseif up:find("[LOW]", 1, true) or up:find(": LOW", 1, true) or up:find("LOW]", 1, true) then
        counts.low = counts.low + 1; return "low"
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
    Browser.init(C,
        function(path)
            scan_path = path
            Browser.set_root(path)
        end,
        function(path)
            Editor.open_file(path)
        end
    )
    CHAR_H = font_sm:getHeight()

    add_line("Code Review TUI  - Live CSS Editor Suite", "head")
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

    local mx, my = love.mouse.getPosition()

    -- Row 1: title on left, status badge on right
    love.graphics.setFont(font_ui)
    gc("lavender")
    text_at(12, y + 6, "Code Review")

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
    text_at(sl_x, y + 6, status_label)
    love.graphics.setColor(sc)
    fill_rect(badge_x, y + 3, badge_w, 20, 4)
    gc("bg")
    love.graphics.setFont(font_sm)
    text_at(badge_x + (badge_w - font_sm:getWidth(status)) / 2, y + 7, status)

    -- Row 2: path label + input spanning available width
    love.graphics.setFont(font_ui)
    gc("dim")
    local path_label = "Path:"
    text_at(12, y + 34, path_label)
    local plw = font_ui:getWidth(path_label)
    local px, pw = 12 + plw + 8, W - 12 - 12 - plw - 8
    local ph     = 20
    local py     = y + 30
    if path_editing then
        gc("panel_bg")
        fill_rect(px, py, pw, ph, 3)
        gc("violet")
        stroke_rect(px, py, pw, ph, 3)
        gc("text_bright")
        love.graphics.setFont(font_sm)
        text_at(px + 5, py + 3, trunc_str(path_buf, pw - 10, font_sm) .. (math.floor(love.timer.getTime() * 2) % 2 == 0 and "|" or ""))
    else
        gc("panel_bg")
        fill_rect(px, py, pw, ph, 3)
        gc("border")
        stroke_rect(px, py, pw, ph, 3)
        gc("text")
        love.graphics.setFont(font_sm)
        text_at(px + 5, py + 3, trunc_str(scan_path, pw - 10, font_sm))
    end

    -- Row 3: scan buttons left to right
    local btns = {
        { label="Security",  action=actions.scan_security  },
        { label="God Funcs", action=actions.scan_god_funcs },
        { label="Lines",     action=actions.scan_lines     },
        { label="Py Audit",  action=actions.scan_py_audit  },
        { label="Smells",    action=actions.scan_smells    },
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

    local iy = ry + 6 - scroll_y
    for i, r in ipairs(results) do
        if iy + RESULT_LINE_H >= ry and iy < ry + rh then
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
    text_at(mx + 20, my + 100, "Scanners: security_ck / god_funcs / lines_count / py_audit / code_smells")
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
    if Editor.has_tabs() then
        Editor.draw(content_x(), content_y(), content_w(), content_h())
    else
        draw_results()
    end
    draw_summary()

    -- Menu on top
    Menu.draw(MENUBAR_H, font_ui)

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
    -- Route to editor when tabs are open
    if Editor.has_tabs() and not path_editing then
        if key == "escape" and Editor.active_tab() and not Editor.active_tab()._ctx then
            -- escape with no ctx: let editor clear selection; if nothing more, close top tab
        end
        if Editor.keypressed(key, content_h()) then return end
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
    if Editor.has_tabs() and not path_editing then
        if Editor.textinput(t) then return end
    end
    if path_editing then
        path_buf = path_buf .. t
    end
end

function love.mousepressed(mx, my, btn)
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
        -- Route to editor if open
        if Editor.has_tabs() then
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
    if Browser.mousepressed(mx, my, btn) then return end
end

function love.mousereleased(mx, my, btn)
    if btn == 1 then
        sel_dragging = false
    end
end

function love.mousemoved(mx, my, dx, dy)
    Menu.mousemoved(mx, my, MENUBAR_H)
    -- extend selection while dragging
    if sel_dragging and in_results(mx, my) then
        sel_cur = line_idx_at(my)
    end
end

function love.wheelmoved(wx, wy)
    local mx, my = love.mouse.getPosition()
    if Browser.wheelmoved(wx, wy, mx, my) then return end
    -- Route to editor if open
    if Editor.has_tabs() and mx >= content_x() then
        Editor.wheelmoved(wy, content_x(), content_y(), content_w(), content_h())
        return
    end
    -- Scroll results
    if mx >= content_x() and my >= content_y() and my < H - SUMMARY_H then
        scroll_y = scroll_y - wy * 40
    end
end

function love.resize(w, h)
    W, H = w, h
end
