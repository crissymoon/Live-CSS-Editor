#pragma once
/*
 * css_parser.h  --  CSS tokenizer + rule/declaration parser
 *
 * Produces a StyleSheet (vector of CssRule) from a CSS text string.
 * Each CssRule holds:
 *   - A selector string (raw, for the style resolver to interpret)
 *   - A vector of CssDecl { property, value }
 *   - Precomputed specificity (a,b,c) packed into uint32_t
 *
 * Also handles:
 *   - At-rules (@media queries -- rules inside are included/excluded)
 *   - @import (ignored, we don't fetch)
 *   - @keyframes (stored as raw block, not used for layout)
 *   - Inline style parsing via parse_inline_style()
 *   - CSS variables (--custom-property: value)
 */

#include <string>
#include <vector>
#include <cstdint>

namespace xcm {

// -------------------------------------------------------------------------
// Specificity is packed as (id<<16 | class<<8 | element).
// Inline styles use 0xFF000000.
// -------------------------------------------------------------------------
using Specificity = uint32_t;

inline Specificity spec(uint8_t a, uint8_t b, uint8_t c) {
    return (static_cast<uint32_t>(a) << 16) |
           (static_cast<uint32_t>(b) <<  8) |
            static_cast<uint32_t>(c);
}

// -------------------------------------------------------------------------
// A single property: value declaration.
// -------------------------------------------------------------------------
struct CssDecl {
    std::string property;   // lower-cased, e.g. "background-color"
    std::string value;      // raw value string, e.g. "#ff0000" or "1px solid red"
    bool        important = false;
};

// -------------------------------------------------------------------------
// One CSS rule: selectors + declarations.
// A rule may have multiple comma-separated selectors; we split them here
// so each CssRule has exactly one selector string.
// -------------------------------------------------------------------------
struct CssRule {
    std::string  selector;     // e.g. "div.container > p:first-child"
    Specificity  specificity = 0;
    std::vector<CssDecl> decls;
    int          source_order = 0; // cascade tie-breaker
};

// -------------------------------------------------------------------------
// StyleSheet -- a parsed CSS file.
// -------------------------------------------------------------------------
struct StyleSheet {
    std::vector<CssRule> rules;
    int source_index = 0; // 0=ua, 1=user, 2=author
};

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------
StyleSheet  parse_css(const char* css, std::size_t len, int source_index = 2);
std::vector<CssDecl> parse_inline_style(const char* style, std::size_t len);
Specificity compute_specificity(const std::string& selector);

} // namespace xcm
