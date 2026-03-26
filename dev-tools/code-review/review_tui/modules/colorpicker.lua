-- modules/colorpicker.lua
-- Floating color picker: HSV square, hue bar, harmony modes, quick-copy.

local M = {}

local C = nil

-- ─────────────────────────────────────────────────────────────────────────────
-- Color math
-- ─────────────────────────────────────────────────────────────────────────────

local function clamp(x, lo, hi)
    return math.max(lo, math.min(hi, x))
end

local function hsv_to_rgb(h, s, v)
    if s == 0 then return v, v, v end
    h = h * 6
    local i = math.floor(h) % 6
    local f = h - math.floor(h)
    local p = v * (1 - s)
    local q = v * (1 - s * f)
    local t = v * (1 - s * (1 - f))
    if     i == 0 then return v, t, p
    elseif i == 1 then return q, v, p
    elseif i == 2 then return p, v, t
    elseif i == 3 then return p, q, v
    elseif i == 4 then return t, p, v
    else               return v, p, q
    end
end

local function hsv_to_hsl(h, s, v)
    local l  = v * (1 - s * 0.5)
    local sl = (l == 0 or l == 1) and 0 or (v - l) / math.min(l, 1 - l)
    return h, sl, l
end

local function fmt_hex(r, g, b)
    return string.format("#%02X%02X%02X",
        math.floor(r * 255 + 0.5),
        math.floor(g * 255 + 0.5),
        math.floor(b * 255 + 0.5))
end

local function fmt_rgb(r, g, b)
    return string.format("rgb(%d, %d, %d)",
        math.floor(r * 255 + 0.5),
        math.floor(g * 255 + 0.5),
        math.floor(b * 255 + 0.5))
end

local function fmt_hsl(h, s, v)
    local hh, ss, ll = hsv_to_hsl(h, s, v)
    return string.format("hsl(%d, %d%%, %d%%)",
        math.floor(hh * 360 + 0.5),
        math.floor(ss * 100 + 0.5),
        math.floor(ll * 100 + 0.5))
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Harmony generation
-- ─────────────────────────────────────────────────────────────────────────────

local HARMONY_NAMES  = { "mono",  "comp",   "analog",   "triad",   "split",  "tetrad"  }
local HARMONY_LABELS = { "Mono",  "Comp",   "Analog",   "Triadic", "Split",  "Tetrad"  }

local function harmony_colors(h, s, v, mode)
    local function mk(hh, ss, vv) return { hh % 1, clamp(ss, 0, 1), clamp(vv, 0, 1) } end
    if mode == "mono" then
        return {
            mk(h, s * 0.15, clamp(v + 0.15, 0, 1)),
            mk(h, s * 0.45, v),
            mk(h, s,        v),
            mk(h, s,        v * 0.65),
            mk(h, s * 0.8,  v * 0.35),
        }
    elseif mode == "comp" then
        local c = h + 0.5
        return {
            mk(h, s * 0.4, clamp(v + 0.1, 0, 1)),
            mk(h, s,       v),
            mk(c, s * 0.4, clamp(v + 0.1, 0, 1)),
            mk(c, s,       v),
        }
    elseif mode == "analog" then
        return {
            mk(h - 2/12, s, v),
            mk(h - 1/12, s, v),
            mk(h,        s, v),
            mk(h + 1/12, s, v),
            mk(h + 2/12, s, v),
        }
    elseif mode == "triad" then
        return {
            mk(h,         s,       v),
            mk(h,         s * 0.5, v),
            mk(h + 1/3,   s,       v),
            mk(h + 1/3,   s * 0.5, v),
            mk(h + 2/3,   s,       v),
        }
    elseif mode == "split" then
        return {
            mk(h,         s,       v),
            mk(h,         s * 0.5, v),
            mk(h + 5/12,  s,       v),
            mk(h + 7/12,  s,       v),
            mk(h + 7/12,  s * 0.5, v),
        }
    elseif mode == "tetrad" then
        return {
            mk(h,        s,       v),
            mk(h + 0.25, s,       v),
            mk(h + 0.5,  s,       v),
            mk(h + 0.75, s,       v),
        }
    end
    return { mk(h, s, v) }
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Panel geometry (all Y offsets relative to panel top)
-- ─────────────────────────────────────────────────────────────────────────────

local PW = 300
-- Rows (all relative to panel top-left)
local HDR_H           = 28
local SV_X, SV_Y      = 20,  HDR_H + 10
local SV_W, SV_H      = 260, 162
local HB_X, HB_Y      = 20,  SV_Y + SV_H + 22   -- bar top; pointer arrow lives HB_Y-12..HB_Y
local HB_W, HB_H      = 260, 14
local CP_Y            = HB_Y + HB_H + 16          -- copy/swatch section top
local HR_Y            = CP_Y + 62                  -- harmony tabs top
local SW_Y            = HR_Y + 36                  -- harmony swatches top
local PH              = SW_Y + 52 + 14             -- swatch(36)+hex(14)+pad

-- ─────────────────────────────────────────────────────────────────────────────
-- State
-- ─────────────────────────────────────────────────────────────────────────────

local visible      = false
local hue          = 0.02
local sat          = 0.80
local val          = 0.90
local harm_mode    = "comp"

local PX           = 200     -- panel screen position (draggable)
local PY           = 80

local drag_panel   = false
local drag_sv      = false
local drag_hue     = false
local drag_ox, drag_oy = 0, 0

-- Hit areas populated each draw
local _close_btn   = nil
local _harm_btns   = {}
local _swatch_btns = {}
local _copy_btns   = {}

-- ─────────────────────────────────────────────────────────────────────────────
-- Shader + canvas
-- ─────────────────────────────────────────────────────────────────────────────

local _sv_shader = nil
local _white_px   = nil    -- 1x1 white canvas used to give the shader proper 0..1 UV coords
local _hue_canvas, _hue_canvas_w = nil, 0

-- GLSL pixel shader: renders the saturation/value square for a given hue.
local SV_SHADER = [[
extern float u_hue;

vec3 hsv2rgb(float h, float s, float v) {
    float c  = v * s;
    float h6 = h * 6.0;
    float x  = c * (1.0 - abs(mod(h6, 2.0) - 1.0));
    float m  = v - c;
    vec3 rgb;
    if      (h6 < 1.0) rgb = vec3(c, x, 0.0);
    else if (h6 < 2.0) rgb = vec3(x, c, 0.0);
    else if (h6 < 3.0) rgb = vec3(0.0, c, x);
    else if (h6 < 4.0) rgb = vec3(0.0, x, c);
    else if (h6 < 5.0) rgb = vec3(x, 0.0, c);
    else               rgb = vec3(c, 0.0, x);
    return rgb + vec3(m);
}

vec4 effect(vec4 color, Image tex, vec2 tc, vec2 sc) {
    return vec4(hsv2rgb(u_hue, tc.x, 1.0 - tc.y), 1.0);
}
]]

local function ensure_shader()
    if _sv_shader ~= nil then return end
    local ok, result = pcall(love.graphics.newShader, SV_SHADER)
    _sv_shader = ok and result or false
    -- 1x1 white canvas: drawing it scaled gives proper 0..1 UV to the shader
    _white_px = love.graphics.newCanvas(2, 2)
    local prev = love.graphics.getCanvas()
    love.graphics.setCanvas(_white_px)
    love.graphics.clear(1, 1, 1, 1)
    love.graphics.setCanvas(prev)
    love.graphics.setColor(1, 1, 1, 1)
end

local function ensure_hue_canvas(w, h)
    if _hue_canvas and _hue_canvas_w == w then return end
    _hue_canvas_w = w
    _hue_canvas = love.graphics.newCanvas(w, h)
    local prev_canvas = love.graphics.getCanvas()
    love.graphics.setCanvas(_hue_canvas)
    love.graphics.clear(0, 0, 0, 0)
    love.graphics.setBlendMode("replace")
    for xi = 0, w - 1 do
        local r, g, b = hsv_to_rgb(xi / (w - 1), 1, 1)
        love.graphics.setColor(r, g, b, 1)
        love.graphics.rectangle("fill", xi, 0, 1, h)
    end
    love.graphics.setBlendMode("alpha")
    love.graphics.setCanvas(prev_canvas)
    love.graphics.setColor(1, 1, 1, 1)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Drawing helpers
-- ─────────────────────────────────────────────────────────────────────────────

local function fill_r(x, y, w, h, r)
    love.graphics.rectangle("fill", x, y, w, h, r or 0, r or 0)
end
local function line_r(x, y, w, h, r)
    love.graphics.rectangle("line", x, y, w, h, r or 0, r or 0)
end
local function gc(k)
    love.graphics.setColor(C[k] or C.text)
end
local function gca(k, a)
    local col = C[k] or C.text
    love.graphics.setColor(col[1], col[2], col[3], a)
end

local function draw_sv_square(ax, ay, aw, ah)
    ensure_shader()
    if _sv_shader and _white_px then
        _sv_shader:send("u_hue", hue)
        love.graphics.setShader(_sv_shader)
        love.graphics.setColor(1, 1, 1, 1)
        -- Draw the 2x2 white canvas scaled to fill the square.
        -- Scaling a canvas keeps UV coords in 0..1 so the shader receives
        -- proper saturation (x) and value (1-y) across the whole area.
        love.graphics.draw(_white_px, ax, ay, 0, aw / 2, ah / 2)
        love.graphics.setShader()
    else
        local r, g, b = hsv_to_rgb(hue, sat, val)
        love.graphics.setColor(r, g, b, 1)
        fill_r(ax, ay, aw, ah, 0)
    end
    gc("border")
    line_r(ax, ay, aw, ah, 4)
    -- Crosshair
    local mx2 = ax + sat * aw
    local my2 = ay + (1 - val) * ah
    love.graphics.setLineWidth(2)
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.circle("line", mx2, my2, 6)
    love.graphics.setLineWidth(1)
    love.graphics.setColor(0, 0, 0, 0.55)
    love.graphics.circle("line", mx2, my2, 7)
end

local function draw_hue_bar(ax, ay, aw, ah)
    ensure_hue_canvas(aw, ah)
    if _hue_canvas then
        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.draw(_hue_canvas, ax, ay)
    end
    gc("border")
    line_r(ax, ay, aw, ah, 2)
    -- Cursor line + triangle pointer above
    local cx = ax + hue * aw
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.setLineWidth(2)
    love.graphics.line(cx, ay, cx, ay + ah)
    love.graphics.setLineWidth(1)
    love.graphics.setColor(1, 1, 1, 0.92)
    love.graphics.polygon("fill", cx, ay - 4, cx - 5, ay - 12, cx + 5, ay - 12)
    gc("border")
    love.graphics.polygon("line", cx, ay - 4, cx - 5, ay - 12, cx + 5, ay - 12)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Public API
-- ─────────────────────────────────────────────────────────────────────────────

function M.init(colours)
    C = colours
    ensure_shader()
end

function M.toggle()
    visible = not visible
end

function M.is_visible()
    return visible
end

function M.draw(font_sm, font_ui)
    if not visible then return end

    local x, y = math.floor(PX), math.floor(PY)

    _harm_btns   = {}
    _swatch_btns = {}
    _copy_btns   = {}

    -- Drop shadow
    love.graphics.setColor(0, 0, 0, 0.45)
    fill_r(x + 5, y + 5, PW, PH, 8)

    -- Panel background + border
    gc("panel_bg")
    fill_r(x, y, PW, PH, 6)
    gc("border")
    line_r(x, y, PW, PH, 6)

    -- Header
    gc("menu_bg")
    fill_r(x, y, PW, HDR_H, 6)
    love.graphics.setColor(C.menu_bg[1], C.menu_bg[2], C.menu_bg[3], 1)
    fill_r(x, y + HDR_H - 6, PW, 6)   -- square bottom edge of header
    gc("border")
    love.graphics.line(x, y + HDR_H, x + PW, y + HDR_H)
    love.graphics.setFont(font_ui)
    gc("lavender")
    love.graphics.print("Color Picker", x + 10, y + 7)

    -- Close button
    local cbx, cby = x + PW - 22, y + 6
    _close_btn = { x = cbx, y = cby, w = 16, h = 16 }
    gc("dim")
    love.graphics.print("x", cbx + 3, cby + 1)

    -- ── SV square ────────────────────────────────────────────────────────────
    draw_sv_square(x + SV_X, y + SV_Y, SV_W, SV_H)

    -- ── Hue bar ───────────────────────────────────────────────────────────────
    draw_hue_bar(x + HB_X, y + HB_Y, HB_W, HB_H)

    -- ── Copy section: swatch + hex/rgb/hsl rows ───────────────────────────────
    love.graphics.setFont(font_sm)
    local cp_ay = y + CP_Y
    local sr, sg, sb = hsv_to_rgb(hue, sat, val)

    -- Selected color swatch
    love.graphics.setColor(sr, sg, sb, 1)
    fill_r(x + 20, cp_ay, 40, 40, 4)
    gc("border")
    line_r(x + 20, cp_ay, 40, 40, 4)

    -- Three copy rows
    local row_x   = x + 68
    local btn_x   = x + PW - 46
    local btn_w   = 38
    local btn_h   = 14

    local hex_str = fmt_hex(sr, sg, sb)
    local rgb_str = fmt_rgb(sr, sg, sb)
    local hsl_str = fmt_hsl(hue, sat, val)

    local rows = {
        { y = cp_ay + 2,  label = "hex",  text = hex_str },
        { y = cp_ay + 20, label = "rgb",  text = rgb_str },
        { y = cp_ay + 38, label = "hsl",  text = hsl_str },
    }

    for _, row in ipairs(rows) do
        -- label
        gc("dim")
        love.graphics.print(row.label, row_x, row.y)
        local lw = font_sm:getWidth(row.label)
        -- value
        gc("text")
        love.graphics.print(row.text, row_x + lw + 5, row.y)
        -- copy button
        love.graphics.setColor(C.accent[1], C.accent[2], C.accent[3], 0.65)
        fill_r(btn_x, row.y, btn_w, btn_h, 3)
        gc("text_bright")
        love.graphics.print("copy", btn_x + 5, row.y)
        _copy_btns[#_copy_btns + 1] = {
            x = btn_x, y = row.y, w = btn_w, h = btn_h, text = row.text
        }
    end

    -- ── Harmony mode tabs ─────────────────────────────────────────────────────
    local hr_ay   = y + HR_Y
    love.graphics.setFont(font_sm)
    gc("dim")
    love.graphics.print("Harmony", x + 20, hr_ay)

    local tab_total = PW - 40
    local tab_w     = math.floor(tab_total / #HARMONY_NAMES)
    local tab_y     = hr_ay + 16

    for i, mode in ipairs(HARMONY_NAMES) do
        local tx     = x + 20 + (i - 1) * tab_w
        local active = mode == harm_mode
        if active then
            love.graphics.setColor(C.accent[1], C.accent[2], C.accent[3], 0.9)
        else
            gc("dark")
        end
        fill_r(tx, tab_y, tab_w - 2, 18, 3)
        if active then gc("text_bright") else gc("dim") end
        local lbl = HARMONY_LABELS[i]
        local lw  = font_sm:getWidth(lbl)
        love.graphics.print(lbl, tx + math.floor((tab_w - 2 - lw) / 2), tab_y + 3)
        _harm_btns[#_harm_btns + 1] = { x = tx, y = tab_y, w = tab_w - 2, h = 18, mode = mode }
    end

    -- ── Harmony swatches ──────────────────────────────────────────────────────
    local sw_ay   = y + SW_Y
    local colors  = harmony_colors(hue, sat, val, harm_mode)
    local n       = #colors
    local sw_full = PW - 40
    local sw_w    = math.floor(sw_full / n) - 4

    love.graphics.setFont(font_sm)
    for i, hsv3 in ipairs(colors) do
        local cr, cg, cb = hsv_to_rgb(hsv3[1], hsv3[2], hsv3[3])
        local sx = x + 20 + (i - 1) * (sw_w + 4)

        love.graphics.setColor(cr, cg, cb, 1)
        fill_r(sx, sw_ay, sw_w, 34, 4)
        gc("border")
        line_r(sx, sw_ay, sw_w, 34, 4)

        -- hex label under swatch
        gc("dim")
        local hx = fmt_hex(cr, cg, cb)
        local hw = font_sm:getWidth(hx)
        love.graphics.print(hx, sx + math.floor((sw_w - hw) / 2), sw_ay + 36)

        _swatch_btns[#_swatch_btns + 1] = {
            x = sx, y = sw_ay, w = sw_w, h = 34,
            h_ = hsv3[1], s_ = hsv3[2], v_ = hsv3[3],
            text = fmt_hex(cr, cg, cb),
        }
    end

    love.graphics.setColor(1, 1, 1, 1)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- Input
-- ─────────────────────────────────────────────────────────────────────────────

function M.mousepressed(mx, my, btn)
    if not visible then return false end
    local x, y = math.floor(PX), math.floor(PY)

    -- Close
    if _close_btn and mx >= _close_btn.x and mx < _close_btn.x + _close_btn.w
                  and my >= _close_btn.y and my < _close_btn.y + _close_btn.h then
        visible = false
        return true
    end

    -- Drag header
    if my >= y and my < y + HDR_H and mx >= x and mx < x + PW then
        drag_panel = true
        drag_ox    = mx - PX
        drag_oy    = my - PY
        return true
    end

    -- SV square
    local sv_ax, sv_ay = x + SV_X, y + SV_Y
    if mx >= sv_ax and mx < sv_ax + SV_W and my >= sv_ay and my < sv_ay + SV_H then
        drag_sv = true
        sat = clamp((mx - sv_ax) / SV_W, 0, 1)
        val = clamp(1 - (my - sv_ay) / SV_H, 0, 1)
        return true
    end

    -- Hue bar (including the triangle pointer area above it)
    local hb_ax, hb_ay = x + HB_X, y + HB_Y
    if mx >= hb_ax and mx < hb_ax + HB_W and my >= hb_ay - 14 and my < hb_ay + HB_H then
        drag_hue = true
        hue = clamp((mx - hb_ax) / HB_W, 0, 1)
        return true
    end

    -- Copy buttons
    for _, cb in ipairs(_copy_btns) do
        if mx >= cb.x and mx < cb.x + cb.w and my >= cb.y and my < cb.y + cb.h then
            love.system.setClipboardText(cb.text)
            return true
        end
    end

    -- Harmony tabs
    for _, tb in ipairs(_harm_btns) do
        if mx >= tb.x and mx < tb.x + tb.w and my >= tb.y and my < tb.y + tb.h then
            harm_mode = tb.mode
            return true
        end
    end

    -- Harmony swatches: click applies that color; double-click logic not needed, single click copies hex
    for _, sw in ipairs(_swatch_btns) do
        if mx >= sw.x and mx < sw.x + sw.w and my >= sw.y and my < sw.y + sw.h then
            if btn == 1 then
                hue = sw.h_
                sat = sw.s_
                val = sw.v_
            elseif btn == 2 then
                love.system.setClipboardText(sw.text)
            end
            return true
        end
    end

    -- Consume all clicks on the panel
    if mx >= x and mx < x + PW and my >= y and my < y + PH then
        return true
    end
    return false
end

function M.mousemoved(mx, my)
    if not visible then return end
    if drag_panel then
        PX = mx - drag_ox
        PY = my - drag_oy
    elseif drag_sv then
        local ax = math.floor(PX) + SV_X
        local ay = math.floor(PY) + SV_Y
        sat = clamp((mx - ax) / SV_W, 0, 1)
        val = clamp(1 - (my - ay) / SV_H, 0, 1)
    elseif drag_hue then
        local ax = math.floor(PX) + HB_X
        hue = clamp((mx - ax) / HB_W, 0, 1)
    end
end

function M.mousereleased()
    drag_panel = false
    drag_sv    = false
    drag_hue   = false
end

function M.keypressed(key)
    if not visible then return false end
    if key == "escape" then
        visible = false
        return true
    end
    return false
end

return M
