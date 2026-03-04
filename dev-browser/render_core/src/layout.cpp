/*
 * layout.cpp  --  Box model layout engine
 */

#include "layout.h"
#include "font_metrics.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

namespace xcm {

// =========================================================================
// LayoutEngine
// =========================================================================
LayoutEngine::LayoutEngine(float vw, float vh, Arena& ar)
    : vw_(vw), vh_(vh), layout_ar_(ar) {}

float LayoutEngine::resolve(Length l, float parent_px, float font_size) const {
    switch (l.unit) {
    case LengthUnit::PX:      return l.value;
    case LengthUnit::PERCENT: return l.value * parent_px / 100.f;
    case LengthUnit::EM:      return l.value * font_size;
    case LengthUnit::REM:     return l.value * root_font_px_;
    case LengthUnit::VW:      return l.value * vw_ / 100.f;
    case LengthUnit::VH:      return l.value * vh_ / 100.f;
    case LengthUnit::AUTO:
    case LengthUnit::NONE:
    default:                  return 0.f;
    }
}

float LayoutEngine::margin_collapse(float m1, float m2) const {
    if (m1 >= 0 && m2 >= 0) return std::max(m1, m2);
    if (m1 < 0  && m2 < 0)  return std::min(m1, m2);
    return m1 + m2;
}

float LayoutEngine::measure_text_width(std::string_view sv,
                                        const ComputedStyle* cs) const {
    if (sv.empty()) return 0.f;
    float em    = cs ? cs->font_size : 16.f;
    // Use cached advance table for this em size -- no per-char division.
    const float* adv_table = glyph_cache_.table_for(em);
    float scale = em / 7.f;   // still needed for kern_adjust and multi-byte glyphs
    float w     = 0;
    const char* text = sv.data();
    const std::size_t len = sv.size();
    for (std::size_t i = 0; i < len; ) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x80) {
            // Per-character advance from pre-built cache -- no multiply here.
            float adv = (c >= 32 && c <= 127) ? adv_table[c - 32]
                                               : adv_table[0]; // fallback to space
            // Kern with next ASCII character.
            if (i + 1 < len) {
                unsigned char nx = static_cast<unsigned char>(text[i + 1]);
                if (nx >= 32 && nx < 128) {
                    adv += xcm::kern_adjust(c, nx) * scale;
                }
            }
            w += adv;
            ++i;
        } else if (c < 0xC0) {
            ++i;  // UTF-8 continuation byte
        } else if (c < 0xE0) {
            // 2-byte sequence -- treat as en-width
            w += adv_table['n' - 32];
            i += 2;
        } else if (c < 0xF0) {
            // 3-byte (CJK): double-wide approximation
            w += adv_table['m' - 32] * 1.5f;
            i += 3;
        } else {
            // 4-byte (emoji): double-wide
            w += adv_table['m' - 32] * 2.f;
            i += 4;
        }
    }
    return w;
}

LayoutBox* LayoutEngine::make_box(Node* node) {
    LayoutBox* b = layout_ar_.make<LayoutBox>();
    b->node = node;
    if (node) node->layout_box = b;
    all_boxes.push(layout_ar_, b);
    return b;
}

LayoutBox* LayoutEngine::make_anon_block() {
    LayoutBox* b = layout_ar_.make<LayoutBox>();
    b->is_anonymous_block = true;
    all_boxes.push(layout_ar_, b);
    return b;
}

// =========================================================================
// layout() -- entry
// =========================================================================
LayoutBox* LayoutEngine::layout(Document* doc) {
    if (!doc || !doc->root) return nullptr;
    // Find body or root element.
    Node* body = doc->body ? doc->body : doc->html ? doc->html : doc->root;
    if (!body) body = doc->root;

    LayoutBox* root_box = make_box(body);
    const ComputedStyle* cs = body->computed_style ?
        static_cast<const ComputedStyle*>(body->computed_style) : nullptr;

    root_box->x = 0; root_box->y = 0;
    root_box->width = vw_;

    if (cs) {
        root_box->padding_top    = resolve(cs->padding[0], vw_, cs->font_size);
        root_box->padding_right  = resolve(cs->padding[1], vw_, cs->font_size);
        root_box->padding_bottom = resolve(cs->padding[2], vw_, cs->font_size);
        root_box->padding_left   = resolve(cs->padding[3], vw_, cs->font_size);
        root_box->margin_top    = resolve(cs->margin[0], vw_, cs->font_size);
        root_box->margin_right  = resolve(cs->margin[1], vw_, cs->font_size);
        root_box->margin_bottom = resolve(cs->margin[2], vw_, cs->font_size);
        root_box->margin_left   = resolve(cs->margin[3], vw_, cs->font_size);
    }

    layout_block(root_box, vw_);
    layout_out_of_flow(root_box);
    return root_box;
}

// =========================================================================
// layout_block  --  block formatting context
// =========================================================================
void LayoutEngine::layout_block(LayoutBox* box, float containing_width) {
    const ComputedStyle* cs = box->node && box->node->computed_style
        ? static_cast<const ComputedStyle*>(box->node->computed_style)
        : nullptr;

    float em = cs ? cs->font_size : 16.f;

    // Resolve border widths.
    if (cs) {
        box->border_top    = cs->border_width[0];
        box->border_right  = cs->border_width[1];
        box->border_bottom = cs->border_width[2];
        box->border_left   = cs->border_width[3];
        box->padding_top    = resolve(cs->padding[0], containing_width, em);
        box->padding_right  = resolve(cs->padding[1], containing_width, em);
        box->padding_bottom = resolve(cs->padding[2], containing_width, em);
        box->padding_left   = resolve(cs->padding[3], containing_width, em);
        box->margin_top    = resolve(cs->margin[0], containing_width, em);
        box->margin_right  = resolve(cs->margin[1], containing_width, em);
        box->margin_bottom = resolve(cs->margin[2], containing_width, em);
        box->margin_left   = resolve(cs->margin[3], containing_width, em);
    }

    // Compute box width.
    float avail_width = containing_width
        - box->margin_left - box->margin_right
        - box->border_left - box->border_right
        - box->padding_left - box->padding_right;

    float boxw = avail_width;
    if (cs) {
        if (!cs->width.is_auto()) {
            float w = resolve(cs->width, containing_width, em);
            if (cs->box_sizing == BoxSizing::BORDER_BOX) {
                w -= box->border_left + box->border_right + box->padding_left + box->padding_right;
            }
            boxw = std::max(w, 0.f);
        }
        // Clamp to min/max.
        if (cs->min_width.unit != LengthUnit::NONE) {
            float mn = resolve(cs->min_width, containing_width, em);
            boxw = std::max(boxw, mn);
        }
        if (cs->max_width.unit != LengthUnit::NONE) {
            float mx = resolve(cs->max_width, containing_width, em);
            boxw = std::min(boxw, mx);
        }
    }
    box->width = boxw + box->padding_left + box->padding_right + box->border_left + box->border_right;

    // Lay out children.
    if (cs && (cs->display == Display::FLEX || cs->display == Display::INLINE_FLEX)) {
        layout_flex(box, boxw);
    } else {
        layout_inline_children(box, boxw);
    }

    // After children, compute height.
    float child_height = 0;
    for (auto* ch : box->children) {
        if (!ch->out_of_flow) {
            child_height = std::max(child_height, ch->y + ch->height - box->content_y());
        }
    }

    float explicit_h = -1.f;
    if (cs && !cs->height.is_auto() && cs->height.unit != LengthUnit::NONE) {
        explicit_h = resolve(cs->height, vh_, em);
        if (cs->box_sizing == BoxSizing::BORDER_BOX) {
            explicit_h -= box->border_top + box->border_bottom + box->padding_top + box->padding_bottom;
        }
        explicit_h = std::max(explicit_h, 0.f);
    }
    float inner_h = (explicit_h >= 0) ? explicit_h : child_height;
    // min-height
    if (cs && cs->min_height.unit != LengthUnit::NONE) {
        float mn = resolve(cs->min_height, vh_, em);
        inner_h = std::max(inner_h, mn);
    }
    box->height = inner_h + box->padding_top + box->padding_bottom + box->border_top + box->border_bottom;
}

// =========================================================================
// layout_inline_children  --  block container with mixed inline/block children
// =========================================================================
// Wraps consecutive inline children into anonymous block line-boxes.
void LayoutEngine::layout_inline_children(LayoutBox* box, float avail_width) {
    if (!box->node) return;
    Node* parent_node = box->node;
    const ComputedStyle* parent_cs = parent_node->computed_style
        ? static_cast<const ComputedStyle*>(parent_node->computed_style) : nullptr;

    float cursor_y = box->content_y();
    float prev_margin_bottom = 0;

    // Process child nodes.
    for (Node* ch = parent_node->first_child; ch; ch = ch->next_sibling) {
        const ComputedStyle* ch_cs = ch->computed_style
            ? static_cast<const ComputedStyle*>(ch->computed_style) : nullptr;

        // Skip display:none.
        if (ch_cs && ch_cs->display == Display::NONE) continue;
        if (!ch_cs && ch->kind == NodeKind::TEXT) {
            // Text node has inherited style from parent.
            ch_cs = parent_cs;
        }

        // Determine if this is a block-level child.
        bool is_block = true;
        if (ch->kind == NodeKind::TEXT) {
            is_block = false;
        } else if (ch_cs) {
            Display d = ch_cs->display;
            is_block = (d == Display::BLOCK || d == Display::LIST_ITEM ||
                        d == Display::TABLE || d == Display::FLEX ||
                        d == Display::GRID  || d == Display::INLINE_FLEX);
        }

        if (ch_cs && ch_cs->position != Position::STATIC) {
            // Absolutely/fixed positioned -- will be laid out in second pass.
            LayoutBox* cb = make_box(ch);
            cb->out_of_flow = true;
            cb->parent = box;
            box->children.push(layout_ar_, cb);
            out_of_flow_.push(layout_ar_, cb);
            continue;
        }

        if (is_block) {
            LayoutBox* cb = make_box(ch);
            cb->parent = box;

            float ch_em = ch_cs ? ch_cs->font_size : 16.f;
            float mt = ch_cs ? resolve(ch_cs->margin[0], avail_width, ch_em) : 0.f;
            float mb = ch_cs ? resolve(ch_cs->margin[2], avail_width, ch_em) : 0.f;
            float ml = ch_cs ? resolve(ch_cs->margin[3], avail_width, ch_em) : 0.f;

            // Margin collapsing.
            float coll = margin_collapse(prev_margin_bottom, mt);
            cursor_y += (box->children.empty() ? 0.f : coll - prev_margin_bottom);
            prev_margin_bottom = 0;

            cb->x = box->content_x() + ml;
            cb->y = cursor_y;

            layout_block(cb, avail_width - ml);

            cursor_y += cb->height + mb;
            prev_margin_bottom = mb;
            box->children.push(layout_ar_, cb);
        } else {
            // Inline / text: accumulate into line boxes.
            std::vector<std::pair<Node*, const ComputedStyle*>> inline_nodes;
            // Collect consecutive inline children.
            Node* in_ch = ch;
            while (in_ch) {
                const ComputedStyle* in_cs = in_ch->computed_style
                    ? static_cast<const ComputedStyle*>(in_ch->computed_style)
                    : parent_cs;
                bool is_blk = false;
                if (in_ch->kind != NodeKind::TEXT && in_cs) {
                    Display d = in_cs->display;
                    is_blk = (d == Display::BLOCK || d == Display::LIST_ITEM ||
                              d == Display::TABLE || d == Display::FLEX || d == Display::GRID);
                }
                if (is_blk) break;
                if (!in_cs || in_cs->display != Display::NONE) {
                    inline_nodes.emplace_back(in_ch, in_cs ? in_cs : parent_cs);
                }
                in_ch = in_ch->next_sibling;
            }

            // Lay out line boxes with proper word-wrap and text-align.
            float line_x     = box->content_x();
            float line_y     = cursor_y;
            float line_h     = (parent_cs ? parent_cs->font_size * parent_cs->line_height : 19.2f);
            float pen_x      = line_x;
            float max_line_h = line_h;

            // text-align from parent (or inline element's) computed style.
            TextAlign ta = parent_cs ? parent_cs->text_align : TextAlign::LEFT;

            // Index into box->children marking the start of the current line.
            // Used to retroactively shift boxes on a completed line for center/right alignment.
            std::size_t line_box_start_idx = box->children.size();

            // Helper: apply text-align x-offset to all boxes on the current line.
            // line_w = pen_x - line_x (width used so far on this line).
            auto flush_line_align = [&]() {
                if (ta == TextAlign::LEFT) return;
                float line_w = pen_x - line_x;
                float offset = 0.f;
                if (ta == TextAlign::CENTER)
                    offset = std::max(0.f, (avail_width - line_w) / 2.f);
                else if (ta == TextAlign::RIGHT)
                    offset = std::max(0.f, avail_width - line_w);
                if (offset < 0.5f) return;
                for (std::size_t ci = line_box_start_idx; ci < box->children.size(); ++ci)
                    box->children[ci]->x += offset;
            };

            for (auto& [in_node, in_cs2] : inline_nodes) {
                float em = in_cs2 ? in_cs2->font_size : 16.f;
                float lh = in_cs2 ? em * in_cs2->line_height : em * 1.2f;
                max_line_h = std::max(max_line_h, lh);
                // Update text-align if this inline node overrides it.
                if (in_cs2 && in_cs2->text_align != TextAlign::LEFT)
                    ta = in_cs2->text_align;

                if (in_node->kind == NodeKind::TEXT && in_node->text) {
                    // -------------------------------------------------------
                    // Word-wrap: iterate word-by-word (unbreakable within a word).
                    // For each word: measure including leading whitespace when not
                    // at the start of a line; break to a new line when the word
                    // does not fit; apply text-align after each completed line.
                    // -------------------------------------------------------
                    const char* txt  = in_node->text;
                    std::size_t tlen = std::strlen(txt);
                    std::size_t pos  = 0;

                    while (pos < tlen) {
                        // Record start of any leading whitespace.
                        std::size_t spc_start  = pos;
                        while (pos < tlen && (txt[pos] == ' ' || txt[pos] == '\t')) ++pos;
                        std::size_t word_start = pos;
                        // Collect the unbreakable word.
                        while (pos < tlen &&
                               txt[pos] != ' ' && txt[pos] != '\t' && txt[pos] != '\n')
                            ++pos;
                        std::size_t word_end = pos;

                        if (word_end == word_start) break; // only trailing whitespace

                        // Segment: include leading space only when mid-line.
                        std::size_t seg_s = (pen_x > line_x) ? spc_start : word_start;
                        std::size_t seg_l = word_end - seg_s;
                        float seg_w = measure_text_width(
                            std::string_view(txt + seg_s, seg_l), in_cs2);

                        float avail_line = box->content_x() + avail_width - pen_x;
                        if (seg_w > avail_line && pen_x > line_x) {
                            // Current line is full -- flush alignment, start new line.
                            flush_line_align();
                            line_y          += max_line_h;
                            max_line_h       = lh;
                            pen_x            = line_x;
                            line_box_start_idx = box->children.size();
                            // Re-measure word without leading whitespace.
                            seg_s  = word_start;
                            seg_l  = word_end - word_start;
                            seg_w  = measure_text_width(
                                std::string_view(txt + seg_s, seg_l), in_cs2);
                        }

                        // Create a word-level layout box.
                        LayoutBox* wb  = make_box(in_node);
                        wb->parent     = box;
                        wb->x          = pen_x;
                        wb->y          = line_y;
                        wb->width      = seg_w;
                        wb->height     = lh;
                        wb->text_start = static_cast<uint32_t>(seg_s);
                        wb->text_len   = static_cast<uint32_t>(seg_l);
                        pen_x         += seg_w;
                        box->children.push(layout_ar_, wb);
                    }
                } else if (in_node->kind == NodeKind::ELEMENT && in_cs2) {
                    // Inline element -- recurse.
                    LayoutBox* ib = make_box(in_node);
                    ib->parent = box;
                    ib->x = pen_x;
                    ib->y = line_y;

                    float ml = resolve(in_cs2->margin[3], avail_width, em);
                    float mr = resolve(in_cs2->margin[1], avail_width, em);
                    ib->padding_left   = resolve(in_cs2->padding[3], avail_width, em);
                    ib->padding_right  = resolve(in_cs2->padding[1], avail_width, em);
                    ib->border_left    = in_cs2->border_width[3];
                    ib->border_right   = in_cs2->border_width[1];

                    // img element: use width/height attrs or defaults.
                    const char* wa = in_node->attr("width");
                    const char* ha = in_node->attr("height");
                    float iw = wa ? std::atof(wa)
                               : (in_cs2->width.is_auto()  ? 100.f
                                  : resolve(in_cs2->width,  avail_width, em));
                    float ih = ha ? std::atof(ha)
                               : (in_cs2->height.is_auto() ? 100.f
                                  : resolve(in_cs2->height, vh_, em));

                    ib->width  = iw;
                    ib->height = ih;
                    pen_x += ml + ib->width + mr;
                    max_line_h = std::max(max_line_h, ih);
                    box->children.push(layout_ar_, ib);
                }
            }

            // Flush the final (possibly incomplete) line.
            flush_line_align();
            cursor_y = line_y + max_line_h;

            // Advance outer ch past all consumed inline nodes.
            if (in_ch && in_ch != ch) {
                Node* last_inline = (!inline_nodes.empty()) ? inline_nodes.back().first : ch;
                ch = last_inline;
            }
        }
    }
}

// =========================================================================
// layout_flex  --  basic flex container layout
// =========================================================================
void LayoutEngine::layout_flex(LayoutBox* box, float avail_width) {
    const ComputedStyle* cs = box->node && box->node->computed_style
        ? static_cast<const ComputedStyle*>(box->node->computed_style)
        : nullptr;
    if (!cs) return;

    bool row_dir = (cs->flex_direction == FlexDir::ROW ||
                    cs->flex_direction == FlexDir::ROW_REV);

    float em = cs->font_size;
    float gap_main  = 0.f; // gap property not implemented yet
    float gap_cross = 0.f;

    struct FlexItem {
        LayoutBox* box;
        const ComputedStyle* cs;
        float base_size  = 0;
        float grow       = 0;
        float shrink     = 1;
        float frozen     = 0; // final main-axis size
        bool  frozen_flag = false;
    };
    std::vector<FlexItem> items;

    // Collect flex children.
    for (Node* ch = box->node->first_child; ch; ch = ch->next_sibling) {
        const ComputedStyle* ch_cs = ch->computed_style
            ? static_cast<const ComputedStyle*>(ch->computed_style)
            : nullptr;
        if (!ch_cs || ch_cs->display == Display::NONE) continue;
        LayoutBox* chb = make_box(ch);
        chb->parent = box;

        FlexItem fi;
        fi.box     = chb;
        fi.cs      = ch_cs;
        fi.grow    = ch_cs->flex_grow;
        fi.shrink  = ch_cs->flex_shrink;

        float ch_em = ch_cs->font_size;
        float ml = resolve(ch_cs->margin[3], avail_width, ch_em);
        float mr = resolve(ch_cs->margin[1], avail_width, ch_em);
        float mt = resolve(ch_cs->margin[0], row_dir ? box->height : avail_width, ch_em);
        float mb = resolve(ch_cs->margin[2], row_dir ? box->height : avail_width, ch_em);
        chb->margin_left = ml; chb->margin_right = mr;
        chb->margin_top  = mt; chb->margin_bottom = mb;
        chb->padding_left   = resolve(ch_cs->padding[3], avail_width, ch_em);
        chb->padding_right  = resolve(ch_cs->padding[1], avail_width, ch_em);
        chb->padding_top    = resolve(ch_cs->padding[0], avail_width, ch_em);
        chb->padding_bottom = resolve(ch_cs->padding[2], avail_width, ch_em);
        chb->border_left    = ch_cs->border_width[3];
        chb->border_right   = ch_cs->border_width[1];
        chb->border_top     = ch_cs->border_width[0];
        chb->border_bottom  = ch_cs->border_width[2];

        // Flex basis.
        float basis = 0;
        if (!ch_cs->flex_basis.is_auto() && ch_cs->flex_basis.unit != LengthUnit::NONE) {
            basis = resolve(ch_cs->flex_basis, avail_width, ch_em);
        } else if (!ch_cs->width.is_auto() && row_dir) {
            basis = resolve(ch_cs->width, avail_width, ch_em);
        } else if (!ch_cs->height.is_auto() && !row_dir) {
            basis = resolve(ch_cs->height, vh_, ch_em);
        } else {
            // Lay out to get a natural size.
            if (row_dir) {
                chb->x = 0; chb->y = 0;
                layout_block(chb, avail_width);
                basis = chb->width;
            } else {
                chb->x = 0; chb->y = 0;
                layout_block(chb, avail_width);
                basis = chb->height;
            }
        }
        fi.base_size = fi.frozen = basis;
        items.push_back(fi);
        box->children.push(layout_ar_, chb);
    }

    // Calculate total base size + free space.
    float main_total = gap_main * std::max(0, static_cast<int>(items.size()) - 1);
    for (auto& fi : items) {
        main_total += fi.base_size
            + (row_dir ? fi.box->margin_left + fi.box->margin_right
                       : fi.box->margin_top  + fi.box->margin_bottom);
    }

    float free_space = (row_dir ? avail_width : vh_) - main_total;

    // Distribute free space via flex-grow / flex-shrink.
    if (free_space > 0) {
        float total_grow = 0;
        for (auto& fi : items) total_grow += fi.grow;
        if (total_grow > 0) {
            for (auto& fi : items) fi.frozen += (fi.grow / total_grow) * free_space;
        }
    } else if (free_space < 0) {
        float total_shrink_scaled = 0;
        for (auto& fi : items) total_shrink_scaled += fi.shrink * fi.base_size;
        if (total_shrink_scaled > 0) {
            for (auto& fi : items) {
                fi.frozen += (fi.shrink * fi.base_size / total_shrink_scaled) * free_space;
                fi.frozen  = std::max(fi.frozen, 0.f);
            }
        }
    }

    // Place items.
    float pen  = row_dir ? box->content_x() : box->content_y();
    float cross_size = row_dir ? box->content_h() : box->content_w();
    if (cross_size <= 0) cross_size = row_dir ? vh_ : avail_width;

    // Justify-content offset.
    float offset_main = 0, between = 0;
    switch (cs->justify_content) {
    case JustContent::FLEX_END:       offset_main = free_space; break;
    case JustContent::CENTER:          offset_main = free_space / 2.f; break;
    case JustContent::SPACE_BETWEEN:
        between = items.size() > 1 ? free_space / (items.size() - 1) : 0; break;
    case JustContent::SPACE_AROUND:
        between = free_space / items.size();
        offset_main = between / 2.f; break;
    case JustContent::SPACE_EVENLY:
        between = free_space / (items.size() + 1);
        offset_main = between; break;
    default: break;
    }

    pen += offset_main;

    for (auto& fi : items) {
        float mm1 = row_dir ? fi.box->margin_left  : fi.box->margin_top;
        float mm2 = row_dir ? fi.box->margin_right : fi.box->margin_bottom;
        pen += mm1;

        if (row_dir) {
            fi.box->x     = pen;
            fi.box->width = fi.frozen;
            // Re-layout with resolved width.
            layout_block(fi.box, fi.frozen - fi.box->padding_left - fi.box->padding_right
                                           - fi.box->border_left  - fi.box->border_right);
            fi.box->width = fi.frozen;
            // Cross axis.
            switch (cs->align_items) {
            case AlignItems::CENTER:
                fi.box->y = box->content_y() + (cross_size - fi.box->height) / 2.f; break;
            case AlignItems::FLEX_END:
                fi.box->y = box->content_y() + cross_size - fi.box->height; break;
            case AlignItems::STRETCH:
                fi.box->y = box->content_y();
                fi.box->height = cross_size; break;
            default:
                fi.box->y = box->content_y(); break;
            }
        } else {
            fi.box->y      = pen;
            fi.box->height = fi.frozen;
            fi.box->x      = box->content_x();
            layout_block(fi.box, avail_width);
            fi.box->height = fi.frozen;
        }

        pen += fi.frozen + mm2 + between;
    }
}

// =========================================================================
// layout_out_of_flow  --  absolute/fixed positioning pass
// =========================================================================
void LayoutEngine::layout_out_of_flow(LayoutBox* root) {
    for (LayoutBox* box : out_of_flow_) {
        const ComputedStyle* cs = box->node && box->node->computed_style
            ? static_cast<const ComputedStyle*>(box->node->computed_style)
            : nullptr;
        if (!cs) continue;

        float em = cs->font_size;
        bool fixed = cs->position == Position::FIXED;

        // Find containing block.
        float cb_x = 0, cb_y = 0, cb_w = vw_, cb_h = vh_;
        if (!fixed && box->parent) {
            LayoutBox* pb = box->parent;
            cb_x = pb->content_x(); cb_y = pb->content_y();
            cb_w = pb->content_w(); cb_h = pb->content_h();
        }

        layout_block(box, cb_w);

        float w = box->width;
        float h = box->height;

        // Resolve left/right/top/bottom.
        if (!cs->left.is_auto())
            box->x = cb_x + resolve(cs->left, cb_w, em);
        else if (!cs->right.is_auto())
            box->x = cb_x + cb_w - w - resolve(cs->right, cb_w, em);
        else
            box->x = cb_x;

        if (!cs->top.is_auto())
            box->y = cb_y + resolve(cs->top, cb_h, em);
        else if (!cs->bottom.is_auto())
            box->y = cb_y + cb_h - h - resolve(cs->bottom, cb_h, em);
        else
            box->y = cb_y;
    }
}

} // namespace xcm
