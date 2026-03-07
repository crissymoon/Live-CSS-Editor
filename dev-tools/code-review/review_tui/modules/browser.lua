-- modules/browser.lua
-- Folder-browser sidebar.  Lets the user navigate the filesystem and
-- double-click any directory to set it as the active scan path.

local M = {}

local C = nil

local W_PANEL   = 240
local ITEM_H    = 22
local INDENT_W  = 14
local ICON_DIR  = "▸ "
local ICON_DIR_O= "▾ "
local ICON_FILE = "  "
local SCROLL_SPEED = 40

local entries   = {}      -- {name, path, is_dir, depth, open}
local scroll_y  = 0
local max_scroll= 0
local hover_idx = nil
local visible   = true
local top_y     = 0  -- set by draw (menubar_h + toolbar_h)

-- on_select(path) callback when user picks a directory
local _on_select = nil

function M.init(colours, on_select)
    C          = colours
    _on_select = on_select
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

-- Build a flat entry list for directory at `path` at given depth.
-- Only expands dirs that are marked open.
local function build_entries(path, depth, out)
    local handle = io.popen("ls -1p \"" .. path:gsub('"', '\\"') .. "\" 2>/dev/null")
    if not handle then return end
    local names = {}
    for name in handle:lines() do
        names[#names+1] = name
    end
    handle:close()

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
            local full = path:gsub("/?$", "/") .. name
            local entry = { name=name, path=full, is_dir=is_dir, depth=depth, open=false }
            out[#out+1] = entry
            if is_dir and entry.open then
                build_entries(full, depth + 1, out)
            end
        end
    end
end

-- Initial root: home directory
local root_path = os.getenv("HOME") or "/"
local root_open = true

function M.set_root(path)
    root_path = path
    entries   = {}
    scroll_y  = 0
    M.refresh()
end

function M.refresh()
    entries = {}
    if root_open then
        build_entries(root_path, 0, entries)
    end
end

M.refresh()

local function refresh_open_dirs()
    local new = {}
    local function recurse(path, depth)
        local handle = io.popen("ls -1p \"" .. path:gsub('"', '\\"') .. "\" 2>/dev/null")
        if not handle then return end
        local names = {}
        for name in handle:lines() do names[#names+1] = name end
        handle:close()
        table.sort(names, function(a,b)
            local ad = a:sub(-1)=="/" local bd=b:sub(-1)=="/"
            if ad~=bd then return ad end
            return a:lower()<b:lower()
        end)
        for _, raw in ipairs(names) do
            local is_dir = raw:sub(-1)=="/"
            local name   = is_dir and raw:sub(1,-2) or raw
            if name:sub(1,1)~="." then
                local full = path:gsub("/?$","/")..name
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
end

function M.draw(x, y, h, font)
    if not visible then return end
    top_y = y

    local total_h = #entries * ITEM_H
    max_scroll = math.max(0, total_h - h + 8)
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
    local rname = root_path:match("([^/]+)$") or root_path
    love.graphics.print("⌂  " .. rname, x + 6, y + 3)

    love.graphics.setScissor(x, y + ITEM_H, W_PANEL, h - ITEM_H)

    local iy = y + ITEM_H - scroll_y
    hover_idx = nil
    local mx, my = love.mouse.getPosition()

    for i, e in ipairs(entries) do
        local item_y = iy + (i - 1) * ITEM_H

        if item_y + ITEM_H >= y + ITEM_H and item_y < y + h then
            -- hover detection
            local hov = (mx >= x and mx < x + W_PANEL and my >= item_y and my < item_y + ITEM_H)
            if hov then hover_idx = i end

            if hov then
                love.graphics.setColor(C.hover)
                love.graphics.rectangle("fill", x, item_y, W_PANEL, ITEM_H)
            end

            local indent = x + 6 + e.depth * INDENT_W
            if e.is_dir then
                love.graphics.setColor(e.open and C.violet or C.lavender)
                local icon = e.open and ICON_DIR_O or ICON_DIR
                love.graphics.print(icon, indent, item_y + 3)
                love.graphics.setColor(C.text)
                love.graphics.print(e.name, indent + font:getWidth(icon), item_y + 3)
            else
                love.graphics.setColor(C.dim)
                love.graphics.print(ICON_FILE .. e.name, indent, item_y + 3)
            end
        end
    end

    love.graphics.setScissor()
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
    if mx >= W_PANEL or btn ~= 1 then return false end

    if not hover_idx then return false end

    local e = entries[hover_idx]
    if not e then return false end

    if e.is_dir then
        -- single click: expand/collapse
        e.open = not e.open
        refresh_open_dirs()
        -- double-click logic via timer in main (simplified here: single click sets path)
        if _on_select then _on_select(e.path) end
    end
    return true
end

return M
