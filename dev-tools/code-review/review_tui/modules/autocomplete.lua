-- modules/autocomplete.lua
-- Word-based popup autocomplete + optional AI ghost text for the editor.
--
-- Integration in editor.lua:
--   local AC = require "modules.autocomplete"
--   M.update  -> AC.poll()
--   M.draw    -> store _ac_cx/_ac_cy during cursor draw; call AC.draw(...) at end
--   M.keypressed -> AC.keypressed(key, ctrl, shift, t, h)  (before other handling)
--   M.textinput  -> AC.on_text(t, char)  (after char is inserted)
--   M.mousepressed -> AC.dismiss()       (when cursor moves via click)
--
-- Integration in main.lua:
--   AC.init(base_dir)    in love.load
--   actions.toggle_autocomplete = function() AC.toggle() end
--   actions.toggle_ac_ai        = function() AC.toggle_ai() end
--
-- Keys (when popup is visible):
--   Tab / Enter  = accept selected item
--   Up / Down    = navigate list
--   Escape       = dismiss
--   Ctrl+Space   = trigger AI ghost text (or cycle popup when open)
--
-- Keys (when ghost text is visible):
--   Tab          = accept ghost text
--   Escape       = dismiss

local M = {}

-- ---- State ------------------------------------------------------------------
local _enabled  = true    -- master on/off switch
local _ai_on    = true    -- AI sub-toggle
local _popup    = nil     -- { items={}, sel=1, prefix="" }  or nil
local _ghost    = nil     -- { text="" }  or nil  (AI inline suggestion)
local _ai_busy  = false
local _ai_thread = nil
local _ai_chan   = nil
local _ai_job    = 0
local _state_path = nil

-- ---- AI worker (runs in a Love2D worker thread) -----------------------------
local _AI_WORKER = [[
local context_file, chan_name = ...
local chan = love.thread.getChannel(chan_name)
local ok, err = pcall(function()
    local base = os.getenv("CODE_REVIEW_DIR") or "."
    local script = base .. "/complete.py"
    local function sq(s) return "'" .. (tostring(s):gsub("'", "'\\''")) .. "'" end
    local p = io.popen("python3 " .. sq(script) .. " --context " .. sq(context_file) .. " 2>&1", "r")
    if not p then chan:push("ERR:popen failed"); return end
    local result = p:read("*a")
    p:close()
    os.remove(context_file)
    result = result and result:match("^%s*(.-)%s*$") or ""
    if result ~= "" then
        chan:push("OK:" .. result)
    else
        chan:push("ERR:empty response")
    end
end)
if not ok then chan:push("ERR:" .. tostring(err)) end
chan:push("AI_DONE")
]]

-- ---- Persistence ------------------------------------------------------------
local function save_state()
    if not _state_path then return end
    local ok, f = pcall(io.open, _state_path, "w")
    if not ok or not f then return end
    f:write("return{enabled=" .. tostring(_enabled)
         .. ",ai_on=" .. tostring(_ai_on) .. "}\n")
    f:close()
end

local function load_state()
    if not _state_path then return end
    local ok, chunk = pcall(loadfile, _state_path)
    if not ok or not chunk then return end
    local ok2, data = pcall(chunk)
    if not ok2 or type(data) ~= "table" then return end
    if type(data.enabled) == "boolean" then _enabled = data.enabled end
    if type(data.ai_on)   == "boolean" then _ai_on   = data.ai_on   end
end

-- ---- Word helpers -----------------------------------------------------------
local MIN_PREFIX  = 2   -- minimum prefix length before showing popup
local MAX_ITEMS   = 8   -- max popup rows
local MIN_WORD    = 3   -- minimum word length to include in word list

-- Scan backwards from col-1 to find the current word prefix.
local function current_prefix(line, col)
    local s = col - 1
    while s >= 1 and line:sub(s, s):match("[%w_]") do s = s - 1 end
    return line:sub(s + 1, col - 1)
end

-- Collect unique words from all lines, sorted alphabetically.
local function collect_words(lines)
    local seen = {}
    for _, ln in ipairs(lines) do
        for w in ln:gmatch("[%a_][%w_]*") do
            if #w >= MIN_WORD then seen[w] = true end
        end
    end
    local out = {}
    for w in pairs(seen) do out[#out + 1] = w end
    table.sort(out)
    return out
end

-- Filter word list to those starting with prefix, excluding exact match.
local function filter_words(words, prefix)
    if #prefix < MIN_PREFIX then return {} end
    local plen = #prefix
    local out  = {}
    for _, w in ipairs(words) do
        if #w > plen and w:sub(1, plen) == prefix then
            out[#out + 1] = w
            if #out >= MAX_ITEMS then break end
        end
    end
    return out
end

-- ---- UTF-8 safe print helper ------------------------------------------------
local function utf8_safe(s)
    if type(s) ~= "string" then s = tostring(s or "") end
    local out, i = {}, 1
    while i <= #s do
        local b = s:byte(i)
        local n = b < 0x80 and 1 or b < 0xE0 and 2 or b < 0xF0 and 3 or 4
        local seq = s:sub(i, i + n - 1)
        out[#out + 1] = (#seq == n) and seq or "\xEF\xBF\xBD"
        i = i + n
    end
    return table.concat(out)
end

-- ---- Accept helpers ---------------------------------------------------------
local function accept_popup(t)
    if not _popup then return false end
    local item   = _popup.items[_popup.sel]
    local suffix = item:sub(#_popup.prefix + 1)
    local L, CO  = t.cursor.line, t.cursor.col
    t.lines[L]   = t.lines[L]:sub(1, CO - 1) .. suffix .. t.lines[L]:sub(CO)
    t.cursor.col = CO + #suffix
    t.modified   = true
    _popup = nil
    return true
end

local function accept_ghost(t)
    if not _ghost then return false end
    -- Accept only the first line of a multi-line suggestion.
    local first = _ghost.text:match("^([^\n]*)") or _ghost.text
    local L, CO = t.cursor.line, t.cursor.col
    t.lines[L]   = t.lines[L]:sub(1, CO - 1) .. first .. t.lines[L]:sub(CO)
    t.cursor.col = CO + #first
    t.modified   = true
    _ghost = nil
    return true
end

-- ---- Public API -------------------------------------------------------------

function M.init(base_dir)
    local base = os.getenv("CODE_REVIEW_DIR") or base_dir or "."
    _state_path = base .. "/.autocomplete_state.lua"
    load_state()
end

function M.is_enabled()   return _enabled end
function M.is_ai_on()     return _ai_on end
function M.has_api_key()
    return (os.getenv("OPENAI_API_KEY") or os.getenv("ANTHROPIC_API_KEY")) ~= nil
end

function M.toggle()
    _enabled = not _enabled
    if not _enabled then _popup = nil; _ghost = nil end
    save_state()
end

function M.toggle_ai()
    _ai_on = not _ai_on
    if not _ai_on then _ghost = nil end
    save_state()
end

function M.dismiss()
    _popup = nil
    _ghost = nil
end

-- Called from editor.textinput after each character is inserted.
function M.on_text(t, char)
    if not _enabled then return end
    _ghost = nil  -- any typing clears ghost text

    if not char:match("[%w_]") then
        _popup = nil
        return
    end

    local line   = t.lines[t.cursor.line] or ""
    local prefix = current_prefix(line, t.cursor.col)
    if #prefix < MIN_PREFIX then _popup = nil; return end

    local words = collect_words(t.lines)
    local items = filter_words(words, prefix)
    if #items == 0 then _popup = nil; return end

    _popup = { items = items, sel = 1, prefix = prefix }
end

-- Trigger an AI completion request for the current cursor context.
function M.trigger_ai(t)
    if not _enabled or not _ai_on then return end
    if _ai_busy then return end
    if not M.has_api_key() then return end

    local L, CO = t.cursor.line, t.cursor.col

    -- Build prefix context (up to 20 lines before cursor).
    local pre_parts = {}
    for i = math.max(1, L - 20), L do
        local ln = t.lines[i] or ""
        pre_parts[#pre_parts + 1] = (i == L) and ln:sub(1, CO - 1) or ln
    end

    -- Build suffix context (up to 10 lines after cursor).
    local suf_parts = { (t.lines[L] or ""):sub(CO) }
    for i = L + 1, math.min(#t.lines, L + 10) do
        suf_parts[#suf_parts + 1] = t.lines[i] or ""
    end

    local prefix_text = table.concat(pre_parts, "\n")
    local suffix_text = table.concat(suf_parts, "\n")
    local ext = (t.filepath or ""):match("%.([^%.]+)$") or "txt"

    -- Write context JSON to a temp file.
    local tmp = os.tmpname() .. "_ac.json"
    local f   = io.open(tmp, "w")
    if not f then return end
    -- Simple manual JSON (avoids dependency on a JSON library).
    local function json_str(s)
        return '"' .. s:gsub('\\', '\\\\'):gsub('"', '\\"')
                       :gsub('\n', '\\n'):gsub('\r', '\\r')
                       :gsub('\t', '\\t') .. '"'
    end
    f:write('{"prefix":' .. json_str(prefix_text)
         .. ',"suffix":' .. json_str(suffix_text)
         .. ',"ext":'    .. json_str(ext) .. '}')
    f:close()

    _ai_job  = _ai_job + 1
    local chan_name = "ac_ai_" .. tostring(_ai_job)
    _ai_chan   = love.thread.getChannel(chan_name)
    _ai_chan:clear()
    _ai_thread = love.thread.newThread(_AI_WORKER)
    _ai_thread:start(tmp, chan_name)
    _ai_busy  = true
end

-- Poll AI worker for results. Call every frame from editor.update or love.update.
function M.poll()
    if not _ai_chan then return end
    while true do
        local msg = _ai_chan:pop()
        if not msg then
            if _ai_thread and not _ai_thread:isRunning() then
                _ai_busy = false; _ai_chan = nil; _ai_thread = nil
            end
            break
        end
        if msg == "AI_DONE" then
            _ai_busy = false; _ai_chan = nil; _ai_thread = nil
        elseif msg:sub(1, 3) == "OK:" then
            local text = msg:sub(4)
            if text ~= "" then _ghost = { text = text } end
        end
        -- ERR messages: ghost just doesn't appear; dismiss spinner silently.
    end
end

-- Handle keys before the editor does. Returns true if the key was consumed.
-- Pass ctrl, shift booleans; t = active tab; h = editor height.
function M.keypressed(key, ctrl, shift, t, h)
    if not _enabled then return false end
    if not t        then return false end

    -- Ctrl+Space: trigger AI, or cycle popup selection if popup is open.
    if ctrl and key == "space" then
        if _popup and #_popup.items > 0 then
            _popup.sel = (_popup.sel % #_popup.items) + 1
        else
            M.trigger_ai(t)
        end
        return true
    end

    -- Tab (no shift): accept popup or ghost text; fall through for normal indent.
    if key == "tab" and not shift and not ctrl then
        if _popup  then return accept_popup(t) end
        if _ghost  then return accept_ghost(t) end
        return false
    end

    -- Enter: accept popup item.
    if key == "return" and _popup then
        return accept_popup(t)
    end

    -- Up / Down: navigate popup list.
    if key == "up" and _popup then
        _popup.sel = math.max(1, _popup.sel - 1)
        return true
    end
    if key == "down" and _popup then
        _popup.sel = math.min(#_popup.items, _popup.sel + 1)
        return true
    end

    -- Escape: dismiss popup or ghost (but only if something is showing).
    if key == "escape" and (_popup or _ghost) then
        _popup = nil; _ghost = nil
        return true
    end

    -- Any cursor-movement key dismisses popup (but not ghost — ghost stays until Tab/Esc).
    local clears_popup = { left=true, right=true, home=true, ["end"]=true,
                           pageup=true, pagedown=true, backspace=true, delete=true }
    if clears_popup[key] then
        _popup = nil
        -- Ghost text persists through navigation so the user can still Tab-accept.
    end

    return false
end

-- Draw autocomplete overlays. Call at the end of editor.draw (scissor already cleared).
-- cx, cy = cursor pixel position calculated during the draw loop.
-- ex, ey = editor panel top-left; ew = editor panel width.
function M.draw(cx, cy, ex, ey, ew)
    if not _enabled then return end

    -- ---- Ghost text: shown inline after the cursor caret ----
    if _ghost and _ghost.text ~= "" then
        local first = _ghost.text:match("^([^\n]*)") or _ghost.text
        if first ~= "" then
            love.graphics.setFont(font_sm)
            -- Dim purple — clearly distinct from real code.
            love.graphics.setColor(0.55, 0.40, 0.85, 0.55)
            love.graphics.print(utf8_safe(first), cx + 2, cy)
            -- Small "Tab" hint.
            love.graphics.setColor(0.4, 0.35, 0.6, 0.5)
            local hint_x = cx + 2 + font_sm:getWidth(utf8_safe(first)) + 4
            love.graphics.print("[Tab]", hint_x, cy)
        end
    end

    -- ---- Popup dropdown ----
    if not _popup or #_popup.items == 0 then return end

    local ITEM_H  = 18
    local PAD_X   = 7
    local POPUP_W = 190
    local n       = #_popup.items
    local POPUP_H = n * ITEM_H + 6

    -- Position: just below cursor; flip upward if near bottom edge.
    local px = cx
    local py = cy + 17   -- one line below cursor

    -- Clamp to right edge of editor.
    if px + POPUP_W > ex + ew - 4 then
        px = ex + ew - POPUP_W - 4
    end

    -- Background + border.
    love.graphics.setColor(C.menu_bg)
    love.graphics.rectangle("fill", px, py, POPUP_W, POPUP_H, 3, 3)
    love.graphics.setColor(C.border)
    love.graphics.rectangle("line", px + 0.5, py + 0.5, POPUP_W - 1, POPUP_H - 1, 3, 3)

    love.graphics.setFont(font_sm)

    local prefix = _popup.prefix
    local plen   = #prefix

    for i, item in ipairs(_popup.items) do
        local iy = py + 3 + (i - 1) * ITEM_H

        -- Row highlight for selected item.
        if i == _popup.sel then
            love.graphics.setColor(0.32, 0.12, 0.60, 1)
            love.graphics.rectangle("fill", px + 2, iy, POPUP_W - 4, ITEM_H, 2, 2)
        end

        -- Prefix portion (dim) + suffix portion (bright).
        local text_x = px + PAD_X
        love.graphics.setColor(i == _popup.sel and C.dim or C.grey)
        love.graphics.print(utf8_safe(prefix), text_x, iy + 1)
        text_x = text_x + font_sm:getWidth(utf8_safe(prefix))

        local suffix_text = item:sub(plen + 1)
        -- Truncate suffix if it overflows.
        local max_w = POPUP_W - PAD_X - font_sm:getWidth(utf8_safe(prefix)) - PAD_X
        while #suffix_text > 1 and font_sm:getWidth(utf8_safe(suffix_text)) > max_w do
            suffix_text = suffix_text:sub(1, -2)
        end
        love.graphics.setColor(i == _popup.sel and C.text_bright or C.text)
        love.graphics.print(utf8_safe(suffix_text), text_x, iy + 1)
    end

    -- Key hint: drawn just below the popup box.
    local key_str = "Tab=accept  \xE2\x86\x91\xE2\x86\x93=nav  Esc=close"
    if M.has_api_key() and _ai_on then
        key_str = key_str .. "  C+Space=AI"
    end
    love.graphics.setFont(font_sm)
    love.graphics.setColor(C.dim)
    love.graphics.print(key_str, px, py + POPUP_H + 2)
end

return M
