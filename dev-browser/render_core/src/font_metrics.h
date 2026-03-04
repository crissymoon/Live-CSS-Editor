#pragma once
/*
 * font_metrics.h  --  Per-character advance widths and kern pairs for FONT5x7
 *
 * Used by both layout.cpp (measure_text_width) and paint.cpp (draw_text) so
 * that text measurement and rendering agree on every character's advance.
 *
 * ADVANCE5x7[c - 32]: advance in "column units" for ASCII 32-127.
 *   6 = full-width  (5 bitmap cols + 1-col gap, maps to 5*scale+1 px)
 *   5 = one col narrower  (maps to 5*scale px -- saves 1 gap pixel)
 *   4 = two cols narrower
 *   3 = narrow (space, ! etc.)
 *
 * Derived from the rightmost non-zero column in FONT5x7 + 2 (col+gap).
 * Full-width chars (adv=6) use the original "5*scale+1" formula to preserve
 * layout continuity; narrow chars use adv*scale directly.
 */

#include <cstdint>

namespace xcm {

// 96 entries: indices 0-95 map to ASCII 32-127.
static const uint8_t ADVANCE5x7[96] = {
    3,      // [0]  ' '  space
    3,      // [1]  '!'
    4,      // [2]  '"'
    6,      // [3]  '#'
    6,      // [4]  '$'
    6,      // [5]  '%'
    6,      // [6]  '&'
    4,      // [7]  '\''
    5,      // [8]  '('
    5,      // [9]  ')'
    6,      // [10] '*'
    6,      // [11] '+'
    4,      // [12] ','
    6,      // [13] '-'
    4,      // [14] '.'
    6,      // [15] '/'
    6,      // [16] '0'
    5,      // [17] '1'
    6,      // [18] '2'
    6,6,6,6,6,6,6,  // [19-25] '3'-'9'
    4,      // [26] ':'
    4,      // [27] ';'
    5,      // [28] '<'
    6,      // [29] '='
    5,      // [30] '>'
    6,      // [31] '?'
    6,      // [32] '@'
    // 'A'-'Z' (indices 33-58)
    6,6,6,6,6,6,6,6,  // A-H
    5,      // [41] 'I'
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,  // J-Z
    5,      // [59] '['
    6,      // [60] '\'
    5,      // [61] ']'
    6,      // [62] '^'
    6,      // [63] '_'
    5,      // [64] '`'
    // 'a'-'z' (indices 65-90)
    6,6,6,6,6,6,6,6,  // a-h
    5,      // [73] 'i'
    5,      // [74] 'j'
    5,      // [75] 'k'
    5,      // [76] 'l'
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,  // m-z
    5,      // [91] '{'
    4,      // [92] '|'
    5,      // [93] '}'
    6,      // [94] '~'
    6,      // [95] DEL
};

// Returns advance in pixels for a single ASCII character.
// scale = em / 7.0f  (7px is the bitmap font height).
// Full-width chars (adv==6) use the exact "5*scale + 1" formula that the
// original renderer used, preserving any existing layout that depends on it.
inline float char_advance_px(unsigned char c, float scale) {
    if (c < 32 || c > 127) return 3.f * scale;
    uint8_t adv = ADVANCE5x7[static_cast<int>(c) - 32];
    if (adv == 6) return 5.f * scale + 1.f;  // full-width: original formula
    return static_cast<float>(adv) * scale;
}

// -------------------------------------------------------------------------
// Kern pair table.
// adjust is in column units (negative = tighten).
// Applied as: pen_x += kern_adjust(prev, cur) * scale
// -------------------------------------------------------------------------
struct KernEntry { unsigned char a, b; signed char adj; };

static const KernEntry KERN_TABLE[] = {
    // Capital diagonal pairs
    { 'A', 'V', -1 }, { 'V', 'A', -1 },
    { 'A', 'W', -1 }, { 'W', 'A', -1 },
    { 'A', 'T', -1 }, { 'T', 'A', -1 },
    { 'A', 'Y', -1 }, { 'Y', 'A', -1 },
    // L followed by tall cap with diagonal
    { 'L', 'T', -1 }, { 'L', 'V', -1 },
    { 'L', 'W', -1 }, { 'L', 'Y', -1 },
    // T overhang over lowercase
    { 'T', 'a', -1 }, { 'T', 'e', -1 },
    { 'T', 'o', -1 }, { 'T', 'r', -1 },
    { 'T', 'y', -1 }, { 'T', 'u', -1 },
    { 'T', 'i', -1 }, { 'T', 'c', -1 },
    // V/W diagonal over lowercase
    { 'V', 'a', -1 }, { 'V', 'e', -1 }, { 'V', 'o', -1 },
    { 'W', 'a', -1 }, { 'W', 'e', -1 }, { 'W', 'o', -1 },
    // Y over lowercase
    { 'Y', 'a', -1 }, { 'Y', 'e', -1 }, { 'Y', 'o', -1 },
    // Punctuation after overhanging serifs
    { 'F', '.', -1 }, { 'F', ',', -1 },
    { 'P', '.', -1 }, { 'P', ',', -1 },
    { 'r', '.', -1 }, { 'r', ',', -1 },
    { 0, 0, 0 }  // sentinel
};

inline float kern_adjust(unsigned char a, unsigned char b) {
    for (const KernEntry* k = KERN_TABLE; k->a != 0; ++k) {
        if (k->a == a && k->b == b) return static_cast<float>(k->adj);
    }
    return 0.f;
}

} // namespace xcm
