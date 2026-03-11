/*
 * paint.cpp  --  Software rasterizer
 */

#include "paint.h"
#include "font_metrics.h"
#include "xcm_rgba_raster.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace xcm {

// =========================================================================
// 5x7 bitmap font (ASCII 32-127)
// Each glyph is 5 columns; each byte is one column, bit0=top row, bit6=bottom.
// =========================================================================
static const uint8_t FONT5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x5F,0x00,0x00,0x00}, // '!'
    {0x07,0x00,0x07,0x00,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
    {0x23,0x13,0x08,0x64,0x62}, // '%'
    {0x36,0x49,0x55,0x22,0x50}, // '&'
    {0x00,0x05,0x03,0x00,0x00}, // '\''
    {0x00,0x1C,0x22,0x41,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00}, // ')'
    {0x14,0x08,0x3E,0x08,0x14}, // '*'
    {0x08,0x08,0x3E,0x08,0x08}, // '+'
    {0x00,0x50,0x30,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08}, // '-'
    {0x00,0x60,0x60,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // '0'
    {0x00,0x42,0x7F,0x40,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46}, // '2'
    {0x21,0x41,0x45,0x4B,0x31}, // '3'
    {0x18,0x14,0x12,0x7F,0x10}, // '4'
    {0x27,0x45,0x45,0x45,0x39}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // '6'
    {0x01,0x71,0x09,0x05,0x03}, // '7'
    {0x36,0x49,0x49,0x49,0x36}, // '8'
    {0x06,0x49,0x49,0x29,0x1E}, // '9'
    {0x00,0x36,0x36,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00}, // ';'
    {0x08,0x14,0x22,0x41,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14}, // '='
    {0x00,0x41,0x22,0x14,0x08}, // '>'
    {0x02,0x01,0x51,0x09,0x06}, // '?'
    {0x32,0x49,0x79,0x41,0x3E}, // '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 'L'
    {0x7F,0x02,0x04,0x02,0x7F}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // '['
    {0x02,0x04,0x08,0x10,0x20}, // '\'
    {0x00,0x41,0x41,0x7F,0x00}, // ']'
    {0x04,0x02,0x01,0x02,0x04}, // '^'
    {0x40,0x40,0x40,0x40,0x40}, // '_'
    {0x00,0x01,0x02,0x04,0x00}, // '`'
    {0x20,0x54,0x54,0x54,0x78}, // 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 'z'
    {0x00,0x08,0x36,0x41,0x00}, // '{'
    {0x00,0x00,0x7F,0x00,0x00}, // '|'
    {0x00,0x41,0x36,0x08,0x00}, // '}'
    {0x02,0x01,0x02,0x04,0x02}, // '~'
    {0x7F,0x7F,0x7F,0x7F,0x7F}, // DEL
};

// =========================================================================
// PaintEngine
// =========================================================================
PaintEngine::PaintEngine(int width, int height)
    : w_(width), h_(height), pixels_(static_cast<std::size_t>(width * height * 4), 255)
{
    clear(Color::white());
}

void PaintEngine::clear(Color c) {
    simd::rgba_clear(pixels_.data(), w_, h_, w_ * 4, c.r, c.g, c.b, c.a);
}

// -------------------------------------------------------------------------
void PaintEngine::push_clip(float fx, float fy, float fw, float fh) {
    int x0 = static_cast<int>(fx);
    int y0 = static_cast<int>(fy);
    int x1 = static_cast<int>(std::ceil(fx + fw));
    int y1 = static_cast<int>(std::ceil(fy + fh));
    x0 = std::max(x0, 0); y0 = std::max(y0, 0);
    x1 = std::min(x1, w_); y1 = std::min(y1, h_);
    ClipRect clip{x0, y0, x1, y1};
    if (!clip_stack_.empty()) clip = clip.intersect(clip_stack_.back());
    clip_stack_.push_back(clip);
}
void PaintEngine::pop_clip() {
    if (!clip_stack_.empty()) clip_stack_.pop_back();
}

// -------------------------------------------------------------------------
void PaintEngine::blend_pixel(int x, int y, Color c, float alpha) {
    ClipRect clip = current_clip();
    if (x < clip.x0 || x >= clip.x1 || y < clip.y0 || y >= clip.y1) return;
    uint8_t* p = pixels_.data() + (y * w_ + x) * 4;
    float a = (c.a / 255.f) * alpha;
    float oma = 1.f - a;
    p[0] = static_cast<uint8_t>(c.r * a + p[0] * oma);
    p[1] = static_cast<uint8_t>(c.g * a + p[1] * oma);
    p[2] = static_cast<uint8_t>(c.b * a + p[2] * oma);
    p[3] = static_cast<uint8_t>(255.f  * a + p[3] * oma);
}

// -------------------------------------------------------------------------
void PaintEngine::fill_rect(float fx, float fy, float fw, float fh, Color c, float opacity) {
    if (c.a == 0 || opacity <= 0.f) return;
    int x0 = static_cast<int>(fx);
    int y0 = static_cast<int>(fy);
    int x1 = static_cast<int>(std::ceil(fx + fw));
    int y1 = static_cast<int>(std::ceil(fy + fh));
    ClipRect clip = current_clip();
    x0 = std::max(x0, clip.x0); y0 = std::max(y0, clip.y0);
    x1 = std::min(x1, clip.x1); y1 = std::min(y1, clip.y1);
    if (x0 >= x1 || y0 >= y1) return;
    float a = (c.a / 255.f) * opacity;

    // Fast path: fully opaque fill -- write packed uint32_t per pixel.
    // Skips all per-channel float multiply/add; std::fill lets the compiler
    // autovectorize to 4-pixel-wide WASM SIMD stores.  Background fill is the
    // hottest paint operation so this has the largest impact on frame rate.
    if (a >= 0.9999f) {
        simd::rgba_fill_rect_opaque(
            pixels_.data(),
            w_,
            h_,
            w_ * 4,
            x0,
            y0,
            x1 - x0,
            y1 - y0,
            c.r,
            c.g,
            c.b,
            c.a);
        return;
    }

    const uint8_t alpha_u8 = static_cast<uint8_t>(std::clamp(a * 255.f, 0.f, 255.f));
    simd::rgba_fill_rect_alpha(
        pixels_.data(),
        w_,
        h_,
        w_ * 4,
        x0,
        y0,
        x1 - x0,
        y1 - y0,
        c.r,
        c.g,
        c.b,
        alpha_u8);
    return;

}

// -------------------------------------------------------------------------
void PaintEngine::stroke_rect(float x, float y, float w, float h,
                              Color c, float lw, float opacity) {
    if (lw <= 0 || c.a == 0) return;
    fill_rect(x,         y,         w,  lw, c, opacity); // top
    fill_rect(x,         y + h - lw, w, lw, c, opacity); // bottom
    fill_rect(x,         y,         lw, h,  c, opacity); // left
    fill_rect(x + w - lw, y,        lw, h,  c, opacity); // right
}

// -------------------------------------------------------------------------
float PaintEngine::circle_coverage(float px, float py, float cx, float cy, float r) const {
    float dx = px - cx, dy = py - cy;
    float dist = std::sqrt(dx*dx + dy*dy);
    return std::clamp(r - dist + 0.5f, 0.f, 1.f); // simple 1px anti-alias
}

// -------------------------------------------------------------------------
void PaintEngine::fill_rounded_rect(float x, float y, float w, float h,
                                    const float radius[4], Color c, float opacity) {
    if (c.a == 0 || opacity <= 0.f) return;
    // Fast path: no radius.
    bool has_radius = false;
    for (int i=0;i<4;++i) if (radius[i] > 0) { has_radius = true; break; }
    if (!has_radius) { fill_rect(x, y, w, h, c, opacity); return; }

    // Corner definitions: TL, TR, BR, BL.
    // radius[0]=TL, [1]=TR, [2]=BR, [3]=BL.
    struct Corner { float cx, cy, r; };
    Corner corners[4] = {
        {x + radius[0],     y + radius[0],     radius[0]},  // TL
        {x + w - radius[1], y + radius[1],     radius[1]},  // TR
        {x + w - radius[2], y + h - radius[2], radius[2]},  // BR
        {x + radius[3],     y + h - radius[3], radius[3]},  // BL
    };
    // Region definitions for scanline render:
    // For each pixel, decide inside/outside rounded bounds.
    int x0 = static_cast<int>(x), y0 = static_cast<int>(y);
    int x1 = static_cast<int>(std::ceil(x+w)), y1 = static_cast<int>(std::ceil(y+h));
    ClipRect clip = current_clip();
    x0 = std::max(x0, clip.x0); y0 = std::max(y0, clip.y0);
    x1 = std::min(x1, clip.x1); y1 = std::min(y1, clip.y1);

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            float fpx = px + 0.5f, fpy = py + 0.5f;
            // Determine which corner region this pixel is in.
            float cover = 1.f;
            float mid_x = x + w/2, mid_y = y + h/2;
            if (fpx < mid_x && fpy < mid_y) {
                // TL corner.
                if (radius[0] > 0 && fpx < corners[0].cx && fpy < corners[0].cy) {
                    cover = circle_coverage(fpx, fpy, corners[0].cx, corners[0].cy, radius[0]);
                }
            } else if (fpx >= mid_x && fpy < mid_y) {
                // TR corner.
                if (radius[1] > 0 && fpx > corners[1].cx && fpy < corners[1].cy) {
                    cover = circle_coverage(fpx, fpy, corners[1].cx, corners[1].cy, radius[1]);
                }
            } else if (fpx >= mid_x && fpy >= mid_y) {
                // BR corner.
                if (radius[2] > 0 && fpx > corners[2].cx && fpy > corners[2].cy) {
                    cover = circle_coverage(fpx, fpy, corners[2].cx, corners[2].cy, radius[2]);
                }
            } else {
                // BL corner.
                if (radius[3] > 0 && fpx < corners[3].cx && fpy > corners[3].cy) {
                    cover = circle_coverage(fpx, fpy, corners[3].cx, corners[3].cy, radius[3]);
                }
            }
            if (cover > 0) blend_pixel(px, py, c, cover * opacity);
        }
    }
}

// -------------------------------------------------------------------------
void PaintEngine::stroke_rounded_rect(float x, float y, float w, float h,
                                      const float radius[4], Color c, float lw, float opacity) {
    if (lw <= 0 || c.a == 0) return;
    // Simple approach: fill outer rounded rect, then fill inner with transparent to subtract.
    // Actually easier: just draw border segments with radius masking.
    // For now fall back to stroke_rect for simplicity at lw <= 1.
    if (lw <= 1.0f) {
        stroke_rect(x, y, w, h, c, lw, opacity);
        return;
    }
    // Outer - inner with inset lw.
    fill_rounded_rect(x, y, w, h, radius, c, opacity);
    float inner_radius[4];
    for (int i=0;i<4;++i) inner_radius[i] = std::max(0.f, radius[i] - lw);
    // Fill interior with background (we don't know bg here so just use 0 alpha cutout).
    // Instead just draw 4 filled border strips.
    fill_rect(x, y, w, lw, c, opacity);              // top
    fill_rect(x, y+h-lw, w, lw, c, opacity);          // bottom
    fill_rect(x, y+lw, lw, h-lw*2, c, opacity);       // left
    fill_rect(x+w-lw, y+lw, lw, h-lw*2, c, opacity);  // right
}

// -------------------------------------------------------------------------
void PaintEngine::stroke_segment(float x0, float y0, float x1, float y1,
                                 Color c, float lw, uint8_t style, float opacity) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len < 1) return;
    float dot_len = lw * 3.f;
    float gap_len = lw * 3.f;
    if (style == 3) { dot_len = lw; gap_len = lw * 2.f; } // dotted
    float unit_x = dx / len, unit_y = dy / len;
    float t = 0;
    bool draw = true;
    while (t < len) {
        float seg = draw ? dot_len : gap_len;
        if (style == 1) seg = len; // solid = one stroke
        float tend = std::min(t + seg, len);
        if (draw) {
            float sx = x0 + unit_x * t,  sy = y0 + unit_y * t;
            float ex = x0 + unit_x * tend, ey = y0 + unit_y * tend;
            // Draw a thick line via fill_rect (horizontal or vertical only).
            if (std::abs(dy) < 0.5f) {
                fill_rect(sx, sy - lw/2, ex - sx, lw, c, opacity);
            } else {
                fill_rect(sx - lw/2, sy, lw, ey - sy, c, opacity);
            }
        }
        t += seg;
        draw = !draw;
    }
}

// -------------------------------------------------------------------------
void PaintEngine::draw_text(float fx, float fy, const char* text, std::size_t len,
                            const ComputedStyle* cs, float opacity) {
    if (!text || len == 0) return;
    Color col  = cs ? cs->color : Color::black();
    float em   = cs ? cs->font_size : 16.f;
    float scale = em / 7.f;   // font is 7px tall; scale = px per bitmap row
    float pen_x = fx, pen_y = fy;
    const uint8_t col_ch[3] = { col.r, col.g, col.b };

    for (std::size_t gi = 0; gi < len; ++gi) {
        unsigned char c = static_cast<unsigned char>(text[gi]);
        if (c == '\n') { pen_x = fx; pen_y += em * 1.2f; continue; }
        if (c < 32 || c > 127) { pen_x += xcm::char_advance_px(c, scale); continue; }
        const uint8_t* bm = FONT5x7[c - 32];

        // Per-char advance with kerning against next character.
        float adv = xcm::char_advance_px(c, scale);
        if (gi + 1 < len) {
            unsigned char nx = static_cast<unsigned char>(text[gi + 1]);
            if (nx >= 32 && nx < 128) {
                adv += xcm::kern_adjust(c, nx) * scale;
            }
        }

        for (int row = 0; row < 7; ++row) {
            float vy0 = pen_y + row * scale;
            float vy1 = pen_y + (row + 1) * scale;
            int   py0 = static_cast<int>(vy0);
            int   py1 = static_cast<int>(std::ceil(vy1));

            // SDF-style horizontal coverage sample: interpolates between adjacent
            // bitmap columns so edges anti-alias smoothly across scale boundaries.
            // offset is in output-pixel units (can be sub-pixel, e.g. +/-0.333).
            auto sp_sample = [&](float sx_off) -> float {
                float cf = sx_off / scale;
                // floor for negative values (truncation is ceiling for negatives)
                int ci = static_cast<int>(cf) - (cf < 0.f && cf != static_cast<float>(static_cast<int>(cf)) ? 1 : 0);
                float fr = cf - static_cast<float>(ci);
                float v0 = (ci     >= 0 && ci     < 5) ? ((bm[ci]     >> row) & 1 ? 1.f : 0.f) : 0.f;
                float v1 = (ci + 1 >= 0 && ci + 1 < 5) ? ((bm[ci + 1] >> row) & 1 ? 1.f : 0.f) : 0.f;
                return v0 * (1.f - fr) + v1 * fr;
            };

            ClipRect clip = current_clip();
            // Pixel x range: glyph cols 0..5 plus 1 extra for sub-pixel bleed.
            int px0 = std::max(static_cast<int>(pen_x) - 1,     clip.x0);
            int px1 = std::min(static_cast<int>(pen_x + 5.f * scale) + 2, clip.x1);

            for (int py = py0; py < py1; ++py) {
                if (py < clip.y0 || py >= clip.y1) continue;
                // Vertical coverage: fractional row-edge anti-aliasing.
                float vcov = std::min(static_cast<float>(py + 1), vy1)
                           - std::max(static_cast<float>(py),      vy0);
                vcov = std::max(0.f, std::min(1.f, vcov));

                uint8_t* row_ptr = pixels_.data() + py * w_ * 4;
                for (int px = px0; px < px1; ++px) {
                    float sx = static_cast<float>(px) - pen_x;
                    // Sub-pixel R/G/B offsets (RGB-stripe LCD: R=left, G=center, B=right).
                    float rc = sp_sample(sx - 0.333f) * vcov;
                    float gc = sp_sample(sx          ) * vcov;
                    float bc = sp_sample(sx + 0.333f) * vcov;
                    if (rc < 0.004f && gc < 0.004f && bc < 0.004f) continue;

                    uint8_t* p = row_ptr + px * 4;
                    float cov3[3] = { rc, gc, bc };
                    for (int ch = 0; ch < 3; ++ch) {
                        float a = cov3[ch] * opacity;
                        p[ch] = static_cast<uint8_t>(col_ch[ch] * a + p[ch] * (1.f - a));
                    }
                    // Alpha: max channel coverage drives opacity.
                    float mc = rc > gc ? (rc > bc ? rc : bc) : (gc > bc ? gc : bc);
                    p[3] = static_cast<uint8_t>(std::min(255.f, p[3] + mc * opacity * 255.f));
                }
            }
        }
        pen_x += adv;
    }
}

// =========================================================================
// paint_borders
// =========================================================================
void PaintEngine::paint_borders(LayoutBox* box, float bx, float by, float bw, float bh,
                                const ComputedStyle* cs, float opacity) {
    if (!cs) return;
    auto paint_edge = [&](float ex, float ey, float ew, float eh, int k) {
        if (cs->border_width[k] <= 0 || cs->border_style[k] == 0) return;
        float lw = cs->border_width[k];
        if (cs->border_style[k] == 1) {
            fill_rect(ex, ey, ew, eh, cs->border_color[k], opacity);
        } else {
            stroke_segment(ex, ey, ex + ew, ey + eh, cs->border_color[k], lw,
                           cs->border_style[k], opacity);
        }
    };
    paint_edge(bx, by,             bw,                box->border_top,    0); // top
    paint_edge(bx, by + bh - box->border_bottom, bw, box->border_bottom, 2); // bottom
    paint_edge(bx, by,             box->border_left,  bh,                 3); // left
    paint_edge(bx + bw - box->border_right, by, box->border_right, bh,   1); // right
}

// =========================================================================
// paint_box  --  recursive paint of one layout box
// =========================================================================
void PaintEngine::paint_box(LayoutBox* box, float ox, float oy, float parent_opacity) {
    if (!box) return;
    const ComputedStyle* cs = box->node && box->node->computed_style
        ? static_cast<const ComputedStyle*>(box->node->computed_style)
        : nullptr;

    if (cs && !cs->visible) return;
    if (cs && cs->display == Display::NONE) return;

    float opacity = parent_opacity * (cs ? cs->opacity : 1.f);
    float bx = box->x + ox, by = box->y + oy;
    float bw = box->width, bh = box->height;

    // -----------------------------------------------------------------------
    // Viewport culling with overscan band.
    //
    // Visible screen band: [-overscan_, h_ + overscan_).
    // Extra overscan_ pixels above and below (1-2 line heights) are rendered
    // proactively so fast scrolls never reveal an unrendered region.
    //
    // Skip the cull for out-of-flow (absolute/fixed) boxes because their
    // screen coordinates are independent of their parent offset.
    // Also skip if the box has out-of-flow children (OOF children may overlap
    // a visible area even when their static parent is off-screen).
    // -----------------------------------------------------------------------
    if (!box->out_of_flow) {
        bool has_oof_child = false;
        for (auto* ch : box->children)
            if (ch->out_of_flow) { has_oof_child = true; break; }
        if (!has_oof_child) {
            float vis_top    = -overscan_;
            float vis_bottom = static_cast<float>(h_) + overscan_;
            if (by + bh <= vis_top || by >= vis_bottom) return;
        }
    }

    // Background.
    if (cs && (cs->background_color.a > 0)) {
        bool has_radius = false;
        for (int i=0;i<4;++i) if (cs->border_radius[i] > 0) { has_radius = true; break; }
        if (has_radius) {
            fill_rounded_rect(bx, by, bw, bh, cs->border_radius,
                              cs->background_color, opacity);
        } else {
            fill_rect(bx, by, bw, bh, cs->background_color, opacity);
        }
    } else if (cs && cs->background_image && cs->background_image[0] != '\0'
               && cs->background_color.a == 0 && bw > 1 && bh > 1) {
        // The element has a background-image that the software renderer cannot
        // display.  Paint a dark neutral placeholder so that white/light text
        // remains readable instead of being invisible on the default white canvas.
        Color placeholder{28, 32, 48, 255};
        fill_rect(bx, by, bw, bh, placeholder, opacity);
    }

    // Borders.
    if (cs) paint_borders(box, bx, by, bw, bh, cs, opacity);

    // Outline.
    if (cs && cs->outline_width > 0) {
        float ow = cs->outline_width;
        stroke_rect(bx - ow, by - ow, bw + ow*2, bh + ow*2, cs->outline_color, ow, opacity);
    }

    // Clip overflow:hidden.
    bool clipped = cs && (cs->overflow_x == Overflow::HIDDEN ||
                          cs->overflow_y == Overflow::HIDDEN);
    if (clipped) push_clip(bx, by, bw, bh);

    // Text content for text nodes.
    // Respects text_start / text_len for word-level inline boxes produced by
    // the word-wrap pass in layout.cpp.  text_len == 0 means paint the whole
    // node text (backward-compat for any box created without splitting).
    if (box->node && box->node->kind == NodeKind::TEXT && box->node->text) {
        const char* full_txt  = box->node->text;
        std::size_t full_len  = std::strlen(full_txt);
        const char* draw_start;
        std::size_t draw_len;
        if (box->text_len > 0) {
            // Word-level box: paint only the assigned slice.
            std::size_t off = static_cast<std::size_t>(box->text_start);
            std::size_t cnt = static_cast<std::size_t>(box->text_len);
            if (off >= full_len) cnt = 0;
            else cnt = std::min(cnt, full_len - off);
            draw_start = full_txt + off;
            draw_len   = cnt;
        } else {
            draw_start = full_txt;
            draw_len   = full_len;
        }
        if (draw_len > 0)
            draw_text(bx, by, draw_start, draw_len, cs, opacity);
    }

    // Paint children recursively with a simple z-index ordering pass.
    // This keeps DOM order for equal z-index values.
    float scroll_dx = 0, scroll_dy = 0; // per-box scroll NYI
    std::vector<LayoutBox*> ordered;
    ordered.reserve(box->children.size());
    for (auto* ch : box->children) ordered.push_back(ch);
    std::stable_sort(ordered.begin(), ordered.end(), [](LayoutBox* a, LayoutBox* b) {
        const ComputedStyle* ca = (a && a->node && a->node->computed_style)
            ? static_cast<const ComputedStyle*>(a->node->computed_style)
            : nullptr;
        const ComputedStyle* cb = (b && b->node && b->node->computed_style)
            ? static_cast<const ComputedStyle*>(b->node->computed_style)
            : nullptr;
        int za = ca ? ca->z_index : 0;
        int zb = cb ? cb->z_index : 0;
        return za < zb;
    });
    for (auto* ch : ordered) {
        paint_box(ch, ox + scroll_dx, oy + scroll_dy, opacity);
    }

    if (clipped) pop_clip();
}

// =========================================================================
// paint  --  public entry point
// =========================================================================
void PaintEngine::paint(LayoutBox* root) {
    if (!root) return;
    clear(Color::white());
    // Apply scroll offset as a vertical translation so every box is drawn at
    // its screen position (document_y - scroll_y_).  paint_box adds oy to
    // every box->y, so passing -scroll_y_ here propagates through the tree.
    paint_box(root, 0.f, -scroll_y_, 1.f);
}

} // namespace xcm
