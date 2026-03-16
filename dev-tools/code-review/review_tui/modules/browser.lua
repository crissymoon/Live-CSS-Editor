-- modules/browser.lua
-- Folder-browser sidebar.  Lets the user navigate the filesystem and
-- double-click any directory to set it as the active scan path.

local M = {}

local WP     = require "modules.winpipe"
local IS_WIN = love.system.getOS() == "Windows"
local C = nil

local W_PANEL   = 240
local ITEM_H    = 22
local INDENT_W  = 14
local ICON_DIR  = "+ "
local ICON_DIR_O= "- "
local ICON_FILE = "  "
local ICON_RECENT = "R "
local SCROLL_SPEED = 40
local RECENT_MAX = 6
local FINDER_H = 22

local entries   = {}      -- tree entries only
local display_entries = {} -- recents + tree entries
local recent_paths = {}
local finder = { active = false, query = "" }
local scroll_y  = 0
local max_scroll= 0
local hover_idx = nil
local visible   = true
local top_y     = 0  -- set by draw (menubar_h + toolbar_h)
local viewport_h = 0 -- set by draw (rows area under root bar)
local kb_focus  = false
local kb_idx    = 1

-- on_select(path) callback when user picks a directory
local _on_select = nil
-- on_edit(path) callback when user chooses "Edit file" from context menu
local _on_edit   = nil

-- Right-click context menu state
local bctx        = nil   -- { x, y, entry_idx } or nil
local BCTX_ITEMS  = { "Edit file", "Copy path", "Set as scan path" }
local BCTX_W      = 160
local BCTX_ITEM_H = 22

local function fuzzy_match(text, query)
    local t = (text or ""):lower()
    local q = (query or ""):lower()
    if q == "" then return true end
    local qi = 1
    for i = 1, #t do
        if t:sub(i, i) == q:sub(qi, qi) then
            qi = qi + 1
            if qi > #q then return true end
        end
    end
    return false
end

local function get_visible_entries()
    if not finder.active or finder.query == "" then
        return display_entries
    end

    local out = {}
    for _, e in ipairs(display_entries) do
        local hay = (e.is_recent and e.path) or (e.name .. " " .. e.path)
        if fuzzy_match(hay, finder.query) then
            out[#out + 1] = e
        end
    end
    return out
end

local function draw_fuzzy_text(x, y, text, query, base_col, hit_col)
    local s = tostring(text or "")
    local q = (query or ""):lower()
    local qi = 1
    local cx = x

    for i = 1, #s do
        local ch = s:sub(i, i)
        local is_hit = false
        if q ~= "" and qi <= #q and ch:lower() == q:sub(qi, qi) then
            is_hit = true
            qi = qi + 1
        end

        if is_hit then
            love.graphics.setColor(hit_col)
        else
            love.graphics.setColor(base_col)
        end
        love.graphics.print(ch, cx, y)
        cx = cx + font_sm:getWidth(ch)
    end
end

local function normalize_path(path)
    local p = tostring(path or "")
    p = p:gsub("%s+$", "")
    if IS_WIN then
        p = p:gsub("[/\\]+$", "")
        if p == "" then p = "C:\\" end
    else
        if p ~= "/" then p = p:gsub("/+$", "") end
        if p == "" then p = "/" end
    end
    return p
end

local function recents_file_path()
    local base = os.getenv("CODE_REVIEW_DIR") or "."
    return base .. "/.browser_recent_paths"
end

local function load_recent_paths()
    recent_paths = {}
    local fh = io.open(recents_file_path(), "r")
    if not fh then return end
    for line in fh:lines() do
        local p = normalize_path(line)
        if p ~= "" then
            recent_paths[#recent_paths + 1] = p
            if #recent_paths >= RECENT_MAX then break end
        end
    end
    fh:close()
end

local function save_recent_paths()
    local fh = io.open(recents_file_path(), "w")
    if not fh then return end
    for _, p in ipairs(recent_paths) do
        fh:write(p, "\n")
    end
    fh:close()
end

local function add_recent_path(path)
    local p = normalize_path(path)
    if p == "" then return end
    local keep = { p }
    for _, existing in ipairs(recent_paths) do
        if existing ~= p and #keep < RECENT_MAX then
            keep[#keep + 1] = existing
        end
    end
    recent_paths = keep
    save_recent_paths()
end

local function rebuild_display_entries()
    display_entries = {}
    for _, p in ipairs(recent_paths) do
        display_entries[#display_entries + 1] = {
            name = p,
            path = p,
            is_dir = true,
            depth = 0,
            open = false,
            is_recent = true,
        }
    end
    for _, e in ipairs(entries) do
        display_entries[#display_entries + 1] = e
    end
end

function M.init(colours, on_select, on_edit)
    C          = colours
    _on_select = on_select
    _on_edit   = on_edit
    load_recent_paths()
    rebuild_display_entries()
end

function M.set_visible(v)
    visible = v
end

function M.toggle()
    visible = not visible
end

function M.is_visible()
    return visible
end

function M.width()
    return visible and W_PANEL or 0
end

function M.open_finder()
    finder.active = true
    finder.query = ""
    kb_focus = true
    kb_idx = 1
    clamp_kb_idx()
end

local function clamp_kb_idx()
    local visible_entries = get_visible_entries()
    if #visible_entries <= 0 then
        kb_idx = 1
        return
    end
    if kb_idx < 1 then kb_idx = 1 end
    if kb_idx > #visible_entries then kb_idx = #visible_entries end
end

local function ensure_kb_visible()
    local visible_entries = get_visible_entries()
    if #visible_entries <= 0 then return end
    local y0 = (kb_idx - 1) * ITEM_H
    local y1 = y0 + ITEM_H
    local view_h = math.max(ITEM_H, viewport_h)
    local top = scroll_y
    local bot = scroll_y + view_h
    if y0 < top then
        scroll_y = y0
    elseif y1 > bot then
        scroll_y = y1 - view_h
    end
    if scroll_y < 0 then scroll_y = 0 end
    if scroll_y > max_scroll then scroll_y = max_scroll end
end

local function find_idx_by_path(path)
    for i, e in ipairs(display_entries) do
        if e.path == path then return i end
    end
    return nil
end

-- Build a flat entry list for directory at `path` at given depth.
-- Only expands dirs that are marked open.
local function build_entries(path, depth, out)
    local names = {}
    if IS_WIN then
        local safe = path:gsub('"', '')
        -- directories first (append "/" to match ls -1p convention used below)
        for name in WP.lines('cmd /C "chcp 65001 >nul 2>&1 & dir /B /AD "' .. safe .. '" 2>nul"') do
            if name ~= "" and name:sub(1,1) ~= "." then
                names[#names+1] = name .. "/"
            end
        end
        for name in WP.lines('cmd /C "dir /B /A-D "' .. safe .. '" 2>nul"') do
            if name ~= "" and name:sub(1,1) ~= "." then
                names[#names+1] = name
            end
        end
    else
        for name in WP.lines("ls -1p \"" .. path:gsub('"', '\\"') .. "\" 2>/dev/null") do
            names[#names+1] = name
        end
    end

    table.sort(names, function(a, b)
        local ad = a:sub(-1) == "/"
        local bd = b:sub(-1) == "/"
        if ad ~= bd then return ad end  -- dirs first
        return a:lower() < b:lower()
    end)

    for _, raw in ipairs(names) do
        local is_dir = raw:sub(-1) == "/"
        local name   = is_dir and raw:sub(1, -2) or raw
        -- skip hidden
        if name:sub(1,1) ~= "." then
            local sep = IS_WIN and "\\" or "/"
            local full = path:gsub("[/\\]?$", sep) .. name
            local entry = { name=name, path=full, is_dir=is_dir, depth=depth, open=false }
            out[#out+1] = entry
            if is_dir and entry.open then
                build_entries(full, depth + 1, out)
            end
        end
    end
end

-- Initial root: code-review directory until main.lua applies configured path
local root_path = os.getenv("CODE_REVIEW_DIR") or "."
local root_open = true

function M.set_root(path)
    root_path = normalize_path(path)
    add_recent_path(root_path)
    entries   = {}
    scroll_y  = 0
    M.refresh()
    kb_idx = 1
end

function M.go_up()
    if IS_WIN then
        local parent = root_path:gsub("[/\\]+$", ""):match("^(.*)[/\\][^/\\]+$")
        if parent and parent ~= "" then
            M.set_root(parent)
        else
            -- already at drive root: stay put
            local drive = root_path:match("^(%a:\\)")
            if drive then M.set_root(drive) end
        end
    else
        -- strip trailing slash then take the parent segment
        local parent = root_path:gsub("/?$", ""):match("^(.*)/[^/]+$")
        if parent and parent ~= "" then
            M.set_root(parent)
        else
            M.set_root("/")
        end
    end
end

function M.refresh()
    entries = {}
    if root_open then
        build_entries(root_path, 0, entries)
    end
    rebuild_display_entries()
    clamp_kb_idx()
end

M.refresh()

local function refresh_open_dirs()
    local new = {}
    local function recurse(path, depth)
        local names = {}
        if IS_WIN then
            local safe = path:gsub('"', '')
            for name in WP.lines('cmd /C "chcp 65001 >nul 2>&1 & dir /B /AD "' .. safe .. '" 2>nul"') do
                if name ~= "" and name:sub(1,1) ~= "." then names[#names+1] = name .. "/" end
            end
            for name in WP.lines('cmd /C "dir /B /A-D "' .. safe .. '" 2>nul"') do
                if name ~= "" and name:sub(1,1) ~= "." then names[#names+1] = name end
            end
        else
            for name in WP.lines("ls -1p \"" .. path:gsub('"', '\\"') .. "\" 2>/dev/null") do
                names[#names+1] = name
            end
        end
        table.sort(names, function(a,b)
            local ad = a:sub(-1)=="/" local bd=b:sub(-1)=="/"
            if ad~=bd then return ad end
            return a:lower()<b:lower()
        end)
        for _, raw in ipairs(names) do
            local is_dir = raw:sub(-1)=="/"
            local name   = is_dir and raw:sub(1,-2) or raw
            if name:sub(1,1)~="." then
                local sep = IS_WIN and "\\" or "/"
                local full = path:gsub("[/\\]?$", sep)..name
                -- preserve open state
                local prev_open = false
                for _, e in ipairs(entries) do
                    if e.path==full then prev_open=e.open break end
                end
                local entry = {name=name,path=full,is_dir=is_dir,depth=depth,open=prev_open}
                new[#new+1]=entry
                if is_dir and prev_open then recurse(full, depth+1) end
            end
        end
    end
    recurse(root_path, 0)
    entries = new
    rebuild_display_entries()
end

function M.draw(x, y, h, font)
    if not visible then return end
    top_y = y

    local visible_entries = get_visible_entries()
    local list_top = y + ITEM_H

    if finder.active then
        list_top = list_top + FINDER_H
    end

    local total_h    = #visible_entries * ITEM_H
    viewport_h = h - (list_top - y) - 4
    max_scroll = math.max(0, total_h - viewport_h)
    scroll_y   = math.max(0, math.min(scroll_y, max_scroll))

    -- panel background
    love.graphics.setColor(C.panel_bg)
    love.graphics.rectangle("fill", x, y, W_PANEL, h)
    -- right border
    love.graphics.setColor(C.border)
    love.graphics.line(x + W_PANEL, y, x + W_PANEL, y + h)

    -- root label bar
    love.graphics.setColor(C.accent)
    love.graphics.rectangle("fill", x, y, W_PANEL, ITEM_H)
    love.graphics.setColor(C.text_bright)
    love.graphics.setFont(font)
    local rname = IS_WIN
        and (root_path:match("([^/\\]+)[/\\]?$") or root_path)
        or  (root_path:match("([^/]+)$") or root_path)
    love.graphics.print("~ " .. rname, x + 6, y + 3)
    -- "up" button on the right of the root bar
    local up_label = "^ up"
    local up_w     = font:getWidth(up_label) + 10
    local mx0, my0 = love.mouse.getPosition()
    local up_hov   = (mx0 >= x + W_PANEL - up_w - 4 and mx0 < x + W_PANEL - 4
                      and my0 >= y and my0 < y + ITEM_H)
    if up_hov then
        love.graphics.setColor(0, 0, 0, 0.25)
        love.graphics.rectangle("fill", x + W_PANEL - up_w - 4, y + 2, up_w, ITEM_H - 4, 3)
    end
    love.graphics.setColor(up_hov and C.text_bright or C.lavender)
    love.graphics.print(up_label, x + W_PANEL - up_w - 2, y + 3)

    if finder.active then
        local fy = y + ITEM_H
        love.graphics.setColor(C.menu_bg)
        love.graphics.rectangle("fill", x, fy, W_PANEL, FINDER_H)
        love.graphics.setColor(C.border)
        love.graphics.rectangle("line", x, fy, W_PANEL, FINDER_H)
        love.graphics.setColor(C.text_bright)
        local q = finder.query ~= "" and finder.query or ""
        love.graphics.print("Find: " .. q .. ((math.floor(love.timer.getTime() * 2) % 2 == 0) and "|" or ""), x + 6, fy + 3)
    end

    love.graphics.setScissor(x, list_top, W_PANEL, h - (list_top - y))

    local iy = list_top - scroll_y
    hover_idx = nil
    local mx, my = love.mouse.getPosition()

    for i, e in ipairs(visible_entries) do
        local item_y = iy + (i - 1) * ITEM_H

        if item_y + ITEM_H >= list_top and item_y < y + h then
            -- hover detection
            local hov = (mx >= x and mx < x + W_PANEL and my >= item_y and my < item_y + ITEM_H)
            if hov then hover_idx = i end

            if kb_focus and i == kb_idx then
                love.graphics.setColor(C.accent[1], C.accent[2], C.accent[3], 0.28)
                love.graphics.rectangle("fill", x, item_y, W_PANEL, ITEM_H)
            end

            if hov then
                love.graphics.setColor(C.hover)
                love.graphics.rectangle("fill", x, item_y, W_PANEL, ITEM_H)
            end

            local indent = x + 6 + e.depth * INDENT_W
            if e.is_recent then
                love.graphics.setColor(C.violet)
                love.graphics.print(ICON_RECENT, indent, item_y + 3)
                draw_fuzzy_text(
                    indent + font:getWidth(ICON_RECENT),
                    item_y + 3,
                    e.name,
                    finder.active and finder.query or "",
                    C.text,
                    C.text_bright
                )
            elseif e.is_dir then
                love.graphics.setColor(e.open and C.violet or C.lavender)
                local icon = e.open and ICON_DIR_O or ICON_DIR
                love.graphics.print(icon, indent, item_y + 3)
                draw_fuzzy_text(
                    indent + font:getWidth(icon),
                    item_y + 3,
                    e.name,
                    finder.active and finder.query or "",
                    C.text,
                    C.text_bright
                )
            else
                love.graphics.setColor(C.dim)
                love.graphics.print(ICON_FILE, indent, item_y + 3)
                draw_fuzzy_text(
                    indent + font:getWidth(ICON_FILE),
                    item_y + 3,
                    e.name,
                    finder.active and finder.query or "",
                    C.dim,
                    C.text_bright
                )
            end
        end
    end

    love.graphics.setScissor()

    -- context menu over browser panel
    if bctx then
        local e    = get_visible_entries()[bctx.entry_idx]
        local cmx  = math.min(bctx.x, x + W_PANEL - BCTX_W - 4)
        local cmy  = math.min(bctx.y, y + h - #BCTX_ITEMS * BCTX_ITEM_H - 6)
        local mh   = #BCTX_ITEMS * BCTX_ITEM_H + 6
        local mmx, mmy = love.mouse.getPosition()
        love.graphics.setColor(C.menu_bg)
        love.graphics.rectangle("fill", cmx, cmy, BCTX_W, mh, 4)
        love.graphics.setColor(C.border)
        love.graphics.rectangle("line", cmx, cmy, BCTX_W, mh, 4)
        love.graphics.setFont(font)
        for ci, label in ipairs(BCTX_ITEMS) do
            -- hide "Edit file" for directories
            if label == "Edit file" and e and e.is_dir then
                -- skip
            else
                local ity = cmy + 3 + (ci - 1) * BCTX_ITEM_H
                local hov = mmx >= cmx and mmx < cmx + BCTX_W
                         and mmy >= ity and mmy < ity + BCTX_ITEM_H
                if hov then
                    love.graphics.setColor(C.accent)
                    love.graphics.rectangle("fill", cmx + 2, ity, BCTX_W - 4, BCTX_ITEM_H, 3)
                end
                love.graphics.setColor(hov and C.text_bright or C.text)
                love.graphics.print(label, cmx + 10, ity + 4)
            end
        end
    end
end

function M.wheelmoved(wx, wy, mx, my)
    if not visible then return false end
    if mx < M.width() then
        scroll_y = math.max(0, math.min(scroll_y - wy * SCROLL_SPEED, max_scroll))
        return true
    end
    return false
end

function M.mousepressed(mx, my, btn)
    if not visible then return false end
    if mx >= W_PANEL then return false end

    -- right-click: open context menu (requires a hovered entry)
    if btn == 2 then
        if hover_idx then
            bctx = { x = mx, y = my, entry_idx = hover_idx }
            return true
        end
        return false
    end

    -- left-click: handle context menu or normal action
    if btn == 1 then
        -- click on root bar: "up" zone or full-bar up
        if my >= top_y and my < top_y + ITEM_H then
            bctx = nil
            M.go_up()
            return true
        end

        if finder.active and my >= top_y + ITEM_H and my < top_y + ITEM_H + FINDER_H then
            return true
        end

        if bctx then
            local visible_entries = get_visible_entries()
            local e   = visible_entries[bctx.entry_idx]
            local cmx = math.min(bctx.x, W_PANEL - BCTX_W - 4)
            local cmy = bctx.y
            if mx >= cmx and mx < cmx + BCTX_W then
                local ci = math.floor((my - cmy - 3) / BCTX_ITEM_H) + 1
                if ci >= 1 and ci <= #BCTX_ITEMS then
                    local label = BCTX_ITEMS[ci]
                    bctx = nil
                    if label == "Edit file" and e and not e.is_dir then
                        if _on_edit then _on_edit(e.path) end
                    elseif label == "Copy path" and e then
                        love.system.setClipboardText(e.path)
                    elseif label == "Set as scan path" and e then
                        if _on_select then _on_select(e.is_dir and e.path or (e.path:match("^(.-/)[^/]+$") or e.path)) end
                    end
                    return true
                end
            end
            bctx = nil
            return true
        end

        if not hover_idx then return false end
        local visible_entries = get_visible_entries()
        local e = visible_entries[hover_idx]
        if not e then return false end
        kb_idx = hover_idx

        if e.is_recent then
            M.set_root(e.path)
            if _on_select then _on_select(e.path) end
        elseif e.is_dir then
            e.open = not e.open
            refresh_open_dirs()
            local ni = find_idx_by_path(e.path)
            if ni then kb_idx = ni end
            if _on_select then _on_select(e.path) end
        elseif _on_edit then
            -- single-click on file opens it in the editor
            _on_edit(e.path)
            kb_focus = false
        end
        return true
    end

    return false
end

function M.set_keyboard_focus(v)
    kb_focus = not not v
    if kb_focus then
        clamp_kb_idx()
        ensure_kb_visible()
    end
end

function M.has_keyboard_focus()
    return kb_focus
end

function M.keypressed(key)
    if not visible or not kb_focus then return false end

    if key == "p" and love.keyboard.isDown("lctrl", "rctrl", "lgui", "rgui") then
        M.open_finder()
        return true
    end

    if key == "/" then
        finder.active = true
        finder.query = ""
        kb_idx = 1
        clamp_kb_idx()
        return true
    end

    if finder.active then
        if key == "backspace" then
            finder.query = finder.query:sub(1, -2)
            kb_idx = 1
            clamp_kb_idx()
            return true
        elseif key == "escape" then
            if finder.query ~= "" then
                finder.query = ""
                kb_idx = 1
                clamp_kb_idx()
                return true
            end
            finder.active = false
            return true
        end
    end

    if key == "escape" then
        kb_focus = false
        finder.active = false
        finder.query = ""
        return true
    end

    if key == "up" then
        kb_idx = kb_idx - 1
        clamp_kb_idx()
        ensure_kb_visible()
        return true
    elseif key == "down" then
        kb_idx = kb_idx + 1
        clamp_kb_idx()
        ensure_kb_visible()
        return true
    elseif key == "left" then
        local visible_entries = get_visible_entries()
        local e = visible_entries[kb_idx]
        if e and e.is_recent then
            M.go_up()
            clamp_kb_idx()
            ensure_kb_visible()
            return true
        end
        if e and e.is_dir and e.open then
            e.open = false
            local p = e.path
            refresh_open_dirs()
            local ni = find_idx_by_path(p)
            if ni then kb_idx = ni end
        else
            M.go_up()
        end
        clamp_kb_idx()
        ensure_kb_visible()
        return true
    elseif key == "right" then
        local visible_entries = get_visible_entries()
        local e = visible_entries[kb_idx]
        if e and e.is_dir then
            if e.is_recent then
                M.set_root(e.path)
                if _on_select then _on_select(e.path) end
            elseif not e.open then
                e.open = true
                refresh_open_dirs()
                local ni = find_idx_by_path(e.path)
                if ni then kb_idx = ni end
            end
            if _on_select then _on_select(e.path) end
        end
        clamp_kb_idx()
        ensure_kb_visible()
        return true
    elseif key == "return" or key == "kpenter" then
        local visible_entries = get_visible_entries()
        local e = visible_entries[kb_idx]
        if not e then return true end
        if e.is_recent then
            M.set_root(e.path)
            if _on_select then _on_select(e.path) end
        elseif e.is_dir then
            e.open = not e.open
            refresh_open_dirs()
            local ni = find_idx_by_path(e.path)
            if ni then kb_idx = ni end
            if _on_select then _on_select(e.path) end
        elseif _on_edit then
            _on_edit(e.path)
            kb_focus = false
        end
        clamp_kb_idx()
        ensure_kb_visible()
        return true
    end

    return false
end

function M.textinput(t)
    if not visible or not kb_focus or not finder.active then return false end
    if not t or t == "" then return false end
    finder.query = finder.query .. t
    kb_idx = 1
    clamp_kb_idx()
    return true
end

return M
