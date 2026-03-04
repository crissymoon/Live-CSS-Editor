#pragma once
/*
 * paint.h  --  Software rasterizer (RGBA8 pixel buffer)
 *
 * Traverses the layout tree and paints boxes into a flat RGBA8 pixel buffer.
 *
 * Capabilities:
 *   - Filled rectangles with alpha blending
 *   - Border drawing (solid, dashed, dotted) per-edge
 *   - Rounded corners (border-radius)
 *   - Text rendering via a built-in 5x7 bitmap font (ASCII 32-127)
 *   - Clip stack (overflow:hidden)
 *   - Opacity compositing
 *   - Outline drawing
 *
 * The pixel format is RGBA8888, row-major, top-left origin.
 * The buffer is owned by PaintEngine and can be read out via pixels().
 *
 * Text rendering at this level is intentionally low-resolution; the engine
 * is designed to handle layout and structural correctness, with a browser
 * engine providing the actual glyph rasterization in production.
 */

#include "layout.h"
#include <cstdint>
#include <vector>

namespace xcm {

// -------------------------------------------------------------------------
// ClipRect
// -------------------------------------------------------------------------
struct ClipRect {
    int x0, y0, x1, y1; // inclusive min, exclusive max
    bool empty() const { return x0 >= x1 || y0 >= y1; }
    ClipRect intersect(ClipRect o) const {
        return {std::max(x0, o.x0), std::max(y0, o.y0),
                std::min(x1, o.x1), std::min(y1, o.y1)};
    }
};

// -------------------------------------------------------------------------
// PaintEngine
// -------------------------------------------------------------------------
class PaintEngine {
public:
    PaintEngine(int width, int height);
    ~PaintEngine() = default;

    // Paint the full layout tree into the buffer.
    void paint(LayoutBox* root);

    // Access the final pixel buffer (RGBA8, row-major).
    const uint8_t* pixels() const { return pixels_.data(); }
    uint8_t*       pixels()       { return pixels_.data(); }
    int            width()  const { return w_; }
    int            height() const { return h_; }
    std::size_t    byte_size() const { return pixels_.size(); }

    // Clear to a given colour.
    void clear(Color c = Color::white());

private:
    int w_, h_;
    std::vector<uint8_t> pixels_;   // RGBA8, w*h*4 bytes
    std::vector<ClipRect> clip_stack_;

    ClipRect current_clip() const {
        return clip_stack_.empty() ? ClipRect{0, 0, w_, h_} : clip_stack_.back();
    }

    void push_clip(float x, float y, float width, float height);
    void pop_clip();

    // Core draw primitives.
    void blend_pixel(int x, int y, Color c, float alpha = 1.f);
    void fill_rect  (float x, float y, float w, float h, Color c, float opacity = 1.f);
    void stroke_rect(float x, float y, float w, float h, Color c, float lw, float opacity = 1.f);
    void fill_rounded_rect(float x, float y, float w, float h,
                           const float radius[4], Color c, float opacity = 1.f);
    void stroke_rounded_rect(float x, float y, float w, float h,
                              const float radius[4], Color c, float lw, float opacity = 1.f);
    void draw_text(float x, float y, const char* text, std::size_t len,
                   const ComputedStyle* cs, float opacity = 1.f);

    // Draw dashed/dotted horizontal or vertical segment.
    void stroke_segment(float x0, float y0, float x1, float y1, Color c, float lw,
                        uint8_t style, float opacity);

    // Per-box paint.
    void paint_box(LayoutBox* box, float offset_x, float offset_y, float opacity);

    // Draw borders for a box.
    void paint_borders(LayoutBox* box, float x, float y, float w, float h,
                       const ComputedStyle* cs, float opacity);

    // Circle sampling for rounded rect (alpha antialiasing).
    float circle_coverage(float px, float py, float cx, float cy, float r) const;
};

} // namespace xcm
