#pragma once
/*
 * style_resolver.h  --  CSS cascade, selector matching, computed style
 *
 * Walks the DOM tree and assigns a ComputedStyle to every element by:
 *   1. Collecting all matching CssRule objects (from UA + author sheets)
 *   2. Sorting by (source, specificity, source_order)
 *   3. Applying declarations in order (later wins, !important wins always)
 *   4. Inheriting inheritable properties from parent
 *   5. Resolving relative values (em -> px, % -> px where parent is known)
 *
 * The ComputedStyle struct holds every property needed by the layout and
 * paint engines as typed C fields -- no string lookups at layout time.
 */

#include "dom.h"
#include "css_parser.h"
#include <vector>
#include <cstdint>

namespace xcm {

// -------------------------------------------------------------------------
// Colour -- RGBA8888 unpacked
// -------------------------------------------------------------------------
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    static Color transparent() { return {0,0,0,0}; }
    static Color black()       { return {0,0,0,255}; }
    static Color white()       { return {255,255,255,255}; }
};

// -------------------------------------------------------------------------
// Length unit
// -------------------------------------------------------------------------
enum class LengthUnit : uint8_t { PX, PERCENT, AUTO, EM, REM, VW, VH, NONE };

struct Length {
    float       value = 0.f;
    LengthUnit  unit  = LengthUnit::AUTO;
    bool is_auto()    const { return unit == LengthUnit::AUTO; }
    bool is_percent() const { return unit == LengthUnit::PERCENT; }
    float resolve(float parent_px, float font_size_px = 16.f, float root_font_px = 16.f,
                  float vw = 0.f, float vh = 0.f) const;
};

// -------------------------------------------------------------------------
// Display value
// -------------------------------------------------------------------------
enum class Display : uint8_t {
    BLOCK, INLINE, INLINE_BLOCK, FLEX, INLINE_FLEX,
    GRID, INLINE_GRID, TABLE, TABLE_ROW, TABLE_CELL,
    LIST_ITEM, NONE
};

enum class Position : uint8_t { STATIC, RELATIVE, ABSOLUTE, FIXED, STICKY };
enum class Overflow : uint8_t { VISIBLE, HIDDEN, SCROLL, AUTO };
enum class FlexDir  : uint8_t { ROW, ROW_REV, COLUMN, COLUMN_REV };
enum class FlexWrap : uint8_t { NOWRAP, WRAP, WRAP_REV };
enum class JustContent : uint8_t { FLEX_START, FLEX_END, CENTER, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY };
enum class AlignItems   : uint8_t { FLEX_START, FLEX_END, CENTER, STRETCH, BASELINE };
enum class TextAlign    : uint8_t { LEFT, RIGHT, CENTER, JUSTIFY };
enum class FontWeight   : uint8_t { NORMAL=0, BOLD };
enum class FontStyle    : uint8_t { NORMAL=0, ITALIC, OBLIQUE };
enum class WhiteSpace   : uint8_t { NORMAL, PRE, PRE_WRAP, PRE_LINE, NOWRAP };
enum class BoxSizing    : uint8_t { CONTENT_BOX, BORDER_BOX };

// -------------------------------------------------------------------------
// ComputedStyle -- all properties as typed C fields.
// -------------------------------------------------------------------------
struct ComputedStyle {
    // Box model
    Display   display    = Display::BLOCK;
    Position  position   = Position::STATIC;
    BoxSizing box_sizing = BoxSizing::CONTENT_BOX;

    Length width {0, LengthUnit::AUTO};
    Length height{0, LengthUnit::AUTO};
    Length min_width {0, LengthUnit::NONE};
    Length min_height{0, LengthUnit::NONE};
    Length max_width {0, LengthUnit::NONE};
    Length max_height{0, LengthUnit::NONE};

    // Margin (top, right, bottom, left)
    Length margin[4] = {{0,LengthUnit::PX},{0,LengthUnit::PX},{0,LengthUnit::PX},{0,LengthUnit::PX}};
    // Padding
    Length padding[4]= {{0,LengthUnit::PX},{0,LengthUnit::PX},{0,LengthUnit::PX},{0,LengthUnit::PX}};
    // Border width
    float  border_width[4] = {0,0,0,0};
    Color  border_color[4];
    uint8_t border_style[4] = {0,0,0,0}; // 0=none,1=solid,2=dashed,3=dotted

    // Positioning
    Length left  {0, LengthUnit::AUTO};
    Length right {0, LengthUnit::AUTO};
    Length top   {0, LengthUnit::AUTO};
    Length bottom{0, LengthUnit::AUTO};

    int z_index = 0;

    // Colours
    Color color           = Color::black();
    Color background_color= Color::transparent();

    // Background image (raw URL string from arena, may be nullptr)
    const char* background_image = nullptr;

    // Typography
    float      font_size   = 16.f;  // px
    FontWeight font_weight = FontWeight::NORMAL;
    FontStyle  font_style  = FontStyle::NORMAL;
    TextAlign  text_align  = TextAlign::LEFT;
    WhiteSpace white_space = WhiteSpace::NORMAL;
    float      line_height = 1.2f;   // multiplier
    float      letter_spacing = 0.f; // px
    const char* font_family = nullptr;

    // Overflow
    Overflow overflow_x = Overflow::VISIBLE;
    Overflow overflow_y = Overflow::VISIBLE;

    // Flex
    FlexDir    flex_direction  = FlexDir::ROW;
    FlexWrap   flex_wrap       = FlexWrap::NOWRAP;
    JustContent justify_content = JustContent::FLEX_START;
    AlignItems  align_items    = AlignItems::STRETCH;
    float flex_grow   = 0.f;
    float flex_shrink = 1.f;
    Length flex_basis{0, LengthUnit::AUTO};

    // Opacity / visibility
    float   opacity   = 1.f;
    bool    visible   = true;

    // Transform (stored as raw string for now; paint engine handles simple translate/scale)
    const char* transform = nullptr;

    // Cursor
    const char* cursor = nullptr;

    // border-radius (4 corners, px)
    float border_radius[4] = {0,0,0,0};

    // outline
    float outline_width = 0.f;
    Color outline_color;

    // list-style
    bool list_style_none = false;
};

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------
void resolve_styles(Document* doc,
                    const std::vector<StyleSheet>& sheets,
                    float viewport_width,
                    float viewport_height);

// Parse a CSS colour string to Color.
Color parse_color(const std::string& val);

// Parse a CSS length string (e.g. "12px", "1.5em", "50%", "auto") to Length.
Length parse_length(const std::string& val);

} // namespace xcm
