-- modules/menu.lua
-- Menubar with dropdown menus.
-- Actions table is injected by main.lua so this module stays stateless.

local M = {}

-- Menubar height is controlled by MENUBAR_H global in main.lua
local ITEMS = nil
local C = nil  -- colour table reference, set by M.init()

local open_idx  = nil  -- currently open dropdown index (nil = all closed)
local hover_top = nil  -- hovered top-level index
local hover_sub = nil  -- hovered sub-item index inside open_idx

-- Build the menu item definition table.
-- `actions` is a plain table the caller fills:
--   actions.set_path, actions.quit, actions.scan_security, ...
function M.init(actions, colours)
    C = colours
    ITEMS = {
        {
            label = "File",
            items = {
                { label = "Set Path...",     action = actions.set_path },
                { label = "Open Reports...", action = actions.open_reports },
                { separator = true },
                { label = "Quit",          action = actions.quit },
            },
        },
        {
            label = "Scan",
            items = {
                { label = "Security Scan",  action = actions.scan_security },
                { label = "God Functions",  action = actions.scan_god_funcs },
                { label = "Line Count",     action = actions.scan_lines },
                { label = "Py Audit",       action = actions.scan_py_audit },
                { label = "Code Smells",    action = actions.scan_smells },
                { separator = true },
                { label = "Run All >",    action = actions.scan_all },
            },
        },
        {
            label = "View",
            items = {
                { label = "Toggle Browser",      action = actions.toggle_browser },
                { label = "Scroll to Bottom",    action = actions.scroll_bottom },
                { label = "Clear Results",       action = actions.clear_results },
            },
        },
        {
            label = "Help",
            items = {
                { label = "About", action = actions.about },
            },
        },
    }
end

local function item_count(menu_idx)
    local n = 0
    for _, it in ipairs(ITEMS[menu_idx].items) do
        if not it.separator then n = n + 1 end
    end
    return #ITEMS[menu_idx].items
end

local PAD_H = 12   -- horizontal padding per top item
local PAD_V = 4    -- vertical padding text vs bar top
local DROP_W = 180
local DROP_ITEM_H = 22
local DROP_PAD_X = 10

-- Call from love.draw.  bar_h = MENUBAR_H global.
function M.draw(bar_h, font)
    if not ITEMS then return end
    local W = love.graphics.getWidth()

    -- Background bar
    love.graphics.setColor(C.menu_bg)
    love.graphics.rectangle("fill", 0, 0, W, bar_h)
    -- Bottom edge separator
    love.graphics.setColor(C.border)
    love.graphics.line(0, bar_h, W, bar_h)

    local x = PAD_H
    for idx, menu in ipairs(ITEMS) do
        local tw = font:getWidth(menu.label)
        local active = open_idx == idx
        local hov    = hover_top == idx

        if active or hov then
            love.graphics.setColor(active and C.accent or C.hover)
            love.graphics.rectangle("fill", x - 6, 1, tw + 12, bar_h - 2, 3, 3)
        end

        love.graphics.setColor(active and C.text_bright or C.text)
        love.graphics.setFont(font)
        love.graphics.print(menu.label, x, PAD_V)

        -- Dropdown
        if active then
            local dy = bar_h
            local n  = #menu.items
            local total_h = (n * DROP_ITEM_H) + 6
            love.graphics.setColor(C.menu_bg)
            love.graphics.rectangle("fill", x - 6, dy, DROP_W, total_h, 3, 3)
            love.graphics.setColor(C.border)
            love.graphics.rectangle("line", x - 6, dy, DROP_W, total_h, 3, 3)

            local iy = dy + 4
            for si, item in ipairs(menu.items) do
                if item.separator then
                    love.graphics.setColor(C.border)
                    love.graphics.line(x - 2, iy + DROP_ITEM_H/2 - 1,
                                       x - 6 + DROP_W - 4, iy + DROP_ITEM_H/2 - 1)
                    iy = iy + DROP_ITEM_H
                else
                    if hover_sub == si then
                        love.graphics.setColor(C.accent)
                        love.graphics.rectangle("fill", x - 4, iy, DROP_W - 4, DROP_ITEM_H, 2, 2)
                    end
                    love.graphics.setColor(item.action and C.text or C.dim)
                    love.graphics.print(item.label, x + DROP_PAD_X, iy + 3)
                    iy = iy + DROP_ITEM_H
                end
            end
        end

        menu._x   = x - 6
        menu._w   = tw + 12
        x = x + tw + PAD_H * 2
    end
end

-- Returns the pixel x start of menu at index idx
local function top_x(idx)
    return ITEMS[idx] and ITEMS[idx]._x or 0
end

function M.mousemoved(mx, my, bar_h)
    if not ITEMS then return end
    hover_top = nil
    hover_sub = nil
    local x = PAD_H
    for idx, menu in ipairs(ITEMS) do
        if menu._x and mx >= menu._x and mx <= menu._x + menu._w and my >= 0 and my < bar_h then
            hover_top = idx
            if open_idx and open_idx ~= idx then
                open_idx = idx   -- slide to adjacent menu
            end
        end
        -- Sub items
        if open_idx == idx and menu._x then
            local dy = bar_h + 4
            for si, item in ipairs(menu.items) do
                local iy = dy + (si - 1) * DROP_ITEM_H
                local ix = menu._x - 6
                if mx >= ix and mx <= ix + DROP_W and my >= iy and my < iy + DROP_ITEM_H then
                    hover_sub = si
                end
            end
        end
    end
end

function M.mousepressed(mx, my, btn, bar_h)
    if not ITEMS or btn ~= 1 then return false end

    -- Click on top-level item
    for idx, menu in ipairs(ITEMS) do
        if menu._x and mx >= menu._x and mx <= menu._x + menu._w and my >= 0 and my < bar_h then
            open_idx = (open_idx == idx) and nil or idx
            hover_sub = nil
            return true
        end
    end

    -- Click inside open dropdown
    if open_idx then
        local menu = ITEMS[open_idx]
        local dy   = bar_h + 4
        for si, item in ipairs(menu.items) do
            local iy = dy + (si - 1) * DROP_ITEM_H
            local ix = menu._x - 6
            if mx >= ix and mx <= ix + DROP_W and my >= iy and my < iy + DROP_ITEM_H then
                if item.action then item.action() end
                open_idx  = nil
                hover_sub = nil
                return true
            end
        end
        -- Click outside dropdown = close
        open_idx  = nil
        hover_sub = nil
        return false
    end

    return false
end

function M.close()
    open_idx  = nil
    hover_sub = nil
end

function M.is_open()
    return open_idx ~= nil
end

return M
