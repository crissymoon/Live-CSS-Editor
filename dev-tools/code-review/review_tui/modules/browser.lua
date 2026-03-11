-- modules/browser.lua
-- Folder-browser sidebar.  Lets the user navigate the filesystem and
-- double-click any directory to set it as the active scan path.

local M = {}

local C = nil

local W_PANEL   = 240
local ITEM_H    = 22
local INDENT_W  = 14
local ICON_DIR  = "+ "
local ICON_DIR_O= "- "
local ICON_FILE = "  "
local SCROLL_SPEED = 40

local entries   = {}      -- {name, path, is_dir, depth, open}
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

function M.init(colours, on_select, on_edit)
    C          = colours
    _on_select = on_select
    _on_edit   = on_edit
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

local function clamp_kb_idx()
    if #entries <= 0 then
        kb_idx = 1
        return
    end
    if kb_idx < 1 then kb_idx = 1 end
    if kb_idx > #entries then kb_idx = #entries end
end

local function ensure_kb_visible()
    if #entries <= 0 then return end
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
    for i, e in ipairs(entries) do
        if e.path == path then return i end
    end
    return nil
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
    kb_idx = 1
end

function M.go_up()
    -- strip trailing slash then take the parent segment
    local parent = root_path:gsub("/?$", ""):match("^(.*)/[^/]+$")
    if parent and parent ~= "" then
        M.set_root(parent)
    else
        M.set_root("/")
    end
end

function M.refresh()
    entries = {}
    if root_open then
        build_entries(root_path, 0, entries)
    end
    clamp_kb_idx()
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

    local total_h    = #entries * ITEM_H
    viewport_h = h - ITEM_H - 4   -- subtract root label bar + small bottom padding
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
    local rname = root_path:match("([^/]+)$") or root_path
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

            if kb_focus and i == kb_idx then
                love.graphics.setColor(C.accent[1], C.accent[2], C.accent[3], 0.28)
                love.graphics.rectangle("fill", x, item_y, W_PANEL, ITEM_H)
            end

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

    -- context menu over browser panel
    if bctx then
        local e    = entries[bctx.entry_idx]
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

        if bctx then
            local e   = entries[bctx.entry_idx]
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
        local e = entries[hover_idx]
        if not e then return false end
        kb_idx = hover_idx

        if e.is_dir then
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
    if key == "escape" then
        kb_focus = false
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
        local e = entries[kb_idx]
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
        local e = entries[kb_idx]
        if e and e.is_dir then
            if not e.open then
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
        local e = entries[kb_idx]
        if not e then return true end
        if e.is_dir then
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

return M
