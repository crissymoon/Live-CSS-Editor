#pragma once
/*
 * layout.h  --  Box model layout engine
 *
 * Builds a LayoutBox tree from the DOM + ComputedStyle tree and computes
 * exact x/y/width/height for every box.  The paint engine then traverses
 * this tree to rasterize.
 *
 * Supported formatting contexts:
 *   - Block formatting context (BFC) -- div, p, h1-h6, body, ...
 *   - Block-level replaced elements -- img (uses width/height attrs)
 *   - Inline formatting context (IFC) -- text inside block containers,
 *     inline elements, with word-wrapping and line boxes
 *   - Flex container -- row and column main axis, simple cross-axis alignment
 *   - Absolute/fixed positioning (removed from BFC, positioned separately)
 *
 * Text measurement: character-width average (no real font metrics).
 * Accurate text rendering requires a font engine (stb_truetype); this
 * approximation keeps the core dependency-free and still produces
 * correct line-wrapping and layout for most pages.
 */

#include "dom.h"
#include "style_resolver.h"
#include <vector>

namespace xcm {

// -------------------------------------------------------------------------
// LayoutBox -- one box per rendered node (not 1:1 with DOM, anonymous
// block/inline boxes can be created).
// -------------------------------------------------------------------------
struct LayoutBox {
    float x = 0, y = 0;
    float width = 0, height = 0;

    // Scroll offset (for scrollable containers).
    float scroll_x = 0, scroll_y = 0;

    // Content area (inside padding).
    float content_x() const { return x + padding_left + border_left; }
    float content_y() const { return y + padding_top  + border_top;  }
    float content_w() const { return width  - padding_left - padding_right  - border_left - border_right;  }
    float content_h() const { return height - padding_top  - padding_bottom - border_top  - border_bottom; }

    float padding_top = 0, padding_right = 0, padding_bottom = 0, padding_left = 0;
    float border_top  = 0, border_right  = 0, border_bottom  = 0, border_left  = 0;
    float margin_top  = 0, margin_right  = 0, margin_bottom  = 0, margin_left  = 0;

    // Link back to source node.
    Node* node = nullptr;

    // Children (in paint order, z-order not yet applied).
    ArenaVec<LayoutBox*> children;
    LayoutBox* parent = nullptr;

    // For inline line boxes.
    bool is_line_box = false;
    float baseline = 0;

    // Flags.
    bool is_anonymous_block = false;
    bool is_anonymous_inline = false;
    bool out_of_flow = false; // absolute/fixed
};

// -------------------------------------------------------------------------
// LayoutEngine
// -------------------------------------------------------------------------
class LayoutEngine {
public:
    LayoutEngine(float viewport_width, float viewport_height, Arena& ar);
    ~LayoutEngine() = default;

    // Build layout tree from document root, returns root LayoutBox.
    LayoutBox* layout(Document* doc);

    // All boxes tracked here (arena-backed, no heap ownership).
    ArenaVec<LayoutBox*> all_boxes;

private:
    float vw_, vh_;
    Arena& layout_ar_;     // reference to xcm_ctx::layout_arena -- persists between renders

    // Root font size for rem resolution.
    float root_font_px_ = 16.f;

    LayoutBox* make_box(Node* node);
    LayoutBox* make_anon_block();

    // Main recursive layout pass.
    void layout_block(LayoutBox* box, float containing_width);
    void layout_flex (LayoutBox* box, float containing_width);
    void layout_inline_children(LayoutBox* box, float containing_width);

    // Measure a text run width for a given style.
    float measure_text_width(const char* text, std::size_t len,
                             const ComputedStyle* cs) const;

    // Resolve a Length to px given containing block and style context.
    float resolve(Length l, float parent_px,
                  float font_size = 16.f) const;

    // Collect absolute/fixed boxes for second pass.
    ArenaVec<LayoutBox*> out_of_flow_;
    void layout_out_of_flow(LayoutBox* root);

    // Margin collapsing.
    float margin_collapse(float m1, float m2) const;
};

} // namespace xcm
