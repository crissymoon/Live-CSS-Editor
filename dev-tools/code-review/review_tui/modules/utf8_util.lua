-- modules/utf8_util.lua
-- Dependency-free UTF-8 helpers.
--
-- LÖVE's graphics API rejects invalid or partial byte sequences with
-- "UTF-8 decoding error", so every string passed to love.graphics.print*
-- must be cleaned and split on real character boundaries, never on bytes.

local U = {}

local function is_cont(b)
    return b ~= nil and b >= 0x80 and b <= 0xBF
end

local function three(s, i, b2_lo, b2_hi)
    if i + 2 > #s then return 0 end
    local b2, b3 = s:byte(i + 1), s:byte(i + 2)
    if b2 >= b2_lo and b2 <= b2_hi and is_cont(b3) then return 3 end
    return 0
end

local function four(s, i, b2_lo, b2_hi)
    if i + 3 > #s then return 0 end
    local b2, b3, b4 = s:byte(i + 1), s:byte(i + 2), s:byte(i + 3)
    if b2 >= b2_lo and b2 <= b2_hi and is_cont(b3) and is_cont(b4) then return 4 end
    return 0
end

-- Length of the valid UTF-8 sequence starting at byte i, or 0 when the
-- bytes there are not a complete, well-formed character.
local function seq_len(s, i)
    local b1 = s:byte(i)
    if not b1 then return 0 end
    if b1 < 0x80 then return 1 end
    if b1 >= 0xC2 and b1 <= 0xDF then
        return is_cont(s:byte(i + 1)) and 2 or 0
    end
    if b1 == 0xE0 then return three(s, i, 0xA0, 0xBF) end
    if b1 == 0xED then return three(s, i, 0x80, 0x9F) end
    if b1 >= 0xE1 and b1 <= 0xEF then return three(s, i, 0x80, 0xBF) end
    if b1 == 0xF0 then return four(s, i, 0x90, 0xBF) end
    if b1 == 0xF4 then return four(s, i, 0x80, 0x8F) end
    if b1 >= 0xF1 and b1 <= 0xF3 then return four(s, i, 0x80, 0xBF) end
    return 0
end

-- Return the next valid character at or after i plus the index after it.
-- Invalid bytes are skipped. Returns nil when the string is exhausted.
function U.next_char(s, i)
    local n = #s
    while i <= n do
        local len = seq_len(s, i)
        if len > 0 then
            return s:sub(i, i + len - 1), i + len
        end
        i = i + 1
    end
    return nil, i
end

-- Return a copy of s with every invalid byte removed. Pure ASCII strings
-- are returned untouched via a fast path.
function U.sanitize(s)
    if type(s) ~= "string" then return "" end

    local n = #s
    local i = 1
    while i <= n and s:byte(i) < 0x80 do i = i + 1 end
    if i > n then return s end

    local out = {}
    i = 1
    while i <= n do
        local len = seq_len(s, i)
        if len > 0 then
            out[#out + 1] = s:sub(i, i + len - 1)
            i = i + len
        else
            i = i + 1
        end
    end
    return table.concat(out)
end

-- Split s into a table of whole characters, skipping invalid bytes.
function U.to_chars(s)
    local out = {}
    local i = 1
    while true do
        local ch, ni = U.next_char(s, i)
        if not ch then return out end
        out[#out + 1] = ch
        i = ni
    end
end

-- Remove the final whole character, not the final byte.
function U.trim_last_char(s)
    s = U.sanitize(s)
    if s == "" then return "" end

    local start = #s
    while start > 1 and is_cont(s:byte(start)) do
        start = start - 1
    end
    return s:sub(1, start - 1)
end

return U
