/*
 * xcm_tui.cpp  --  TUI renderer, input engine, and app loop implementation
 *
 * Rendering pipeline:
 *   RGBA pixels -> cell mapping (half-block / braille / monochrome)
 *              -> luminance shading pass (fills flat areas with texture)
 *              -> Sobel edge pass (overlays box/block glyphs along edges)
 *              -> color serializer (truecolor / 256-color / mono ANSI)
 *              -> diff-update writer (only re-draws changed cells)
 *
 * Input pipeline:
 *   stdin raw bytes -> VT escape parser -> TuiEvent structs
 *   Mouse tracking: SGR 1006 preferred, X10 legacy fallback
 *   Resize: SIGWINCH handler + TIOCGWINSZ query
 */

#include "xcm_tui.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#ifndef _WIN32
#  include <fcntl.h>
#  include <poll.h>
#  include <sys/ioctl.h>
#endif

namespace xcm {

// ==========================================================================
// Unicode utilities
// ==========================================================================
static void utf8_encode(uint32_t cp, char out[5]) {
    if (cp < 0x80) {
        out[0] = static_cast<char>(cp); out[1] = 0;
    } else if (cp < 0x800) {
        out[0] = static_cast<char>(0xC0 | (cp >> 6));
        out[1] = static_cast<char>(0x80 | (cp & 0x3F));
        out[2] = 0;
    } else if (cp < 0x10000) {
        out[0] = static_cast<char>(0xE0 | (cp >> 12));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
        out[3] = 0;
    } else {
        out[0] = static_cast<char>(0xF0 | (cp >> 18));
        out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((cp >> 6)  & 0x3F));
        out[3] = static_cast<char>(0x80 | (cp & 0x3F));
        out[4] = 0;
    }
}

// ==========================================================================
// Pixel sampling helpers
// ==========================================================================
static inline uint8_t luma(uint8_t r, uint8_t g, uint8_t b) {
    // BT.709 coefficients scaled to integers; avoids float per pixel.
    return static_cast<uint8_t>((r * 2126u + g * 7152u + b * 722u) / 10000u);
}

static inline int clampi2(int v, int lo, int hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

// Nearest-neighbor sample from RGBA buffer.
static inline const uint8_t* sample(const uint8_t* rgba, int w, int h,
                                    int x, int y) {
    x = clampi2(x, 0, w - 1);
    y = clampi2(y, 0, h - 1);
    return rgba + (y * w + x) * 4;
}

// Average color of a pixel rectangle.
static void avg_color(const uint8_t* rgba, int src_w, int src_h,
                      int x0, int y0, int x1, int y1,
                      uint8_t& r, uint8_t& g, uint8_t& b) {
    x0 = clampi2(x0, 0, src_w - 1);
    y0 = clampi2(y0, 0, src_h - 1);
    x1 = clampi2(x1, 0, src_w);
    y1 = clampi2(y1, 0, src_h);
    int cnt = (x1 - x0) * (y1 - y0);
    if (cnt <= 0) { r = g = b = 0; return; }
    uint32_t sr = 0, sg = 0, sb = 0;
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const uint8_t* p = rgba + (py * src_w + px) * 4;
            sr += p[0]; sg += p[1]; sb += p[2];
        }
    }
    r = static_cast<uint8_t>(sr / static_cast<unsigned>(cnt));
    g = static_cast<uint8_t>(sg / static_cast<unsigned>(cnt));
    b = static_cast<uint8_t>(sb / static_cast<unsigned>(cnt));
}

// ==========================================================================
// Shading palette  --  ASCII-safe chars, densest-to-lightest.
// Used in MONOCHROME and SHADING_PASS modes to avoid missing-glyph squares.
// ==========================================================================
static const char SHADE_PALETTE[] = "@%#*+=-:. ";
static const int SHADE_LEN = 10;

static char shade_for_luma(uint8_t lum) {
    // Map 0-255 luminance to ASCII density characters.
    // lum 0 (dark) -> '@', lum 255 (bright) -> ' '.
    int idx = static_cast<int>(lum * (SHADE_LEN - 1) / 255);
    return SHADE_PALETTE[clampi2(idx, 0, SHADE_LEN - 1)];
}

// ==========================================================================
// Braille dot mapping
// ==========================================================================
// Braille block: 2 columns x 4 rows of dots per cell.
// Unicode base: U+2800.  Dot bit positions: col0 = bits 0,1,2,6
//                                            col1 = bits 3,4,5,7
static const uint8_t BRAILLE_BIT[4][2] = {
    {0x01, 0x08},  // row0: dot1, dot4
    {0x02, 0x10},  // row1: dot2, dot5
    {0x04, 0x20},  // row2: dot3, dot6
    {0x40, 0x80},  // row3: dot7, dot8
};

// ==========================================================================
// Edge glyph palette  --  used for Sobel edge overlay.
// Direction-indexed: the 8 compass directions + cross.
// ==========================================================================
static const uint32_t EDGE_GLYPH[] = {
    0x2500, // HORIZONTAL
    0x2502, // VERTICAL
    0x250C, // TOP-LEFT corner
    0x2510, // TOP-RIGHT corner
    0x2514, // BOT-LEFT corner
    0x2518, // BOT-RIGHT corner
    0x253C, // CROSS
    0x2524, // RIGHT
    0x251C, // LEFT
    0x2534, // BOTTOM
    0x252C, // TOP
    0x25A0, // fallback filled square
};

// ==========================================================================
// TuiRenderer  --  RGBA -> cell grid
// ==========================================================================

// --------------------------------------------------------------------------
// HALF_BLOCK mode
//   Each terminal row covers 2 source pixel rows.
//   Use U+2580 UPPER HALF BLOCK with fg=top pixel, bg=bottom pixel.
// --------------------------------------------------------------------------
void TuiRenderer::build_half_block(const uint8_t* rgba, int src_w, int src_h,
                                   TuiFramebuffer& fb) const {
    for (int row = 0; row < fb.rows; ++row) {
        // Each row in the cell grid covers 2 rows of source pixels.
        int py_top = (row * 2 * src_h) / (fb.rows * 2);
        int py_bot = ((row * 2 + 1) * src_h) / (fb.rows * 2);
        py_top = clampi2(py_top, 0, src_h - 1);
        py_bot = clampi2(py_bot, 0, src_h - 1);

        for (int col = 0; col < fb.cols; ++col) {
            int px = (col * src_w) / fb.cols;
            px = clampi2(px, 0, src_w - 1);

            const uint8_t* pt = rgba + (py_top * src_w + px) * 4;
            const uint8_t* pb = rgba + (py_bot * src_w + px) * 4;

            TuiCell& cell = fb.at(col, row);
            cell.fg_r = pt[0]; cell.fg_g = pt[1]; cell.fg_b = pt[2];
            cell.bg_r = pb[0]; cell.bg_g = pb[1]; cell.bg_b = pb[2];

            // U+2580 UPPER HALF BLOCK
            const uint32_t UPPER_HALF = 0x2580;
            utf8_encode(UPPER_HALF, cell.glyph);
        }
    }
}

// --------------------------------------------------------------------------
// BRAILLE mode
//   Each terminal cell covers 2 columns x 4 rows of source pixels.
//   Dots are lit when the pixel luminance exceeds a threshold.
// --------------------------------------------------------------------------
void TuiRenderer::build_braille(const uint8_t* rgba, int src_w, int src_h,
                                TuiFramebuffer& fb) const {
    const int BTHRESH = 128;
    for (int row = 0; row < fb.rows; ++row) {
        for (int col = 0; col < fb.cols; ++col) {
            uint8_t bits = 0;
            uint8_t avg_r = 0, avg_g = 0, avg_b = 0;
            uint32_t sr = 0, sg = 0, sb = 0;
            int cnt = 0;

            for (int dr = 0; dr < 4; ++dr) {
                int py = ((row * 4 + dr) * src_h) / (fb.rows * 4);
                py = clampi2(py, 0, src_h - 1);
                for (int dc = 0; dc < 2; ++dc) {
                    int px = ((col * 2 + dc) * src_w) / (fb.cols * 2);
                    px = clampi2(px, 0, src_w - 1);
                    const uint8_t* p = rgba + (py * src_w + px) * 4;
                    uint8_t lum = luma(p[0], p[1], p[2]);
                    if (lum < BTHRESH) bits |= BRAILLE_BIT[dr][dc];
                    sr += p[0]; sg += p[1]; sb += p[2];
                    ++cnt;
                }
            }
            if (cnt > 0) {
                avg_r = static_cast<uint8_t>(sr / static_cast<unsigned>(cnt));
                avg_g = static_cast<uint8_t>(sg / static_cast<unsigned>(cnt));
                avg_b = static_cast<uint8_t>(sb / static_cast<unsigned>(cnt));
            }

            TuiCell& cell = fb.at(col, row);
            cell.fg_r = avg_r; cell.fg_g = avg_g; cell.fg_b = avg_b;
            cell.bg_r = 0;     cell.bg_g = 0;     cell.bg_b = 0;

            uint32_t cp = 0x2800u | bits;
            utf8_encode(cp, cell.glyph);
        }
    }
}

// --------------------------------------------------------------------------
// MONOCHROME mode
//   No color.  Maps each cell's average luminance to a shading character.
// --------------------------------------------------------------------------
void TuiRenderer::build_monochrome(const uint8_t* rgba, int src_w, int src_h,
                                   TuiFramebuffer& fb) const {
    int px_per_col = clampi2(src_w / std::max(fb.cols, 1), 1, src_w);
    int px_per_row = clampi2(src_h / std::max(fb.rows, 1), 1, src_h);

    for (int row = 0; row < fb.rows; ++row) {
        for (int col = 0; col < fb.cols; ++col) {
            int x0 = col * px_per_col;
            int y0 = row * px_per_row;
            uint8_t r, g, b;
            avg_color(rgba, src_w, src_h,
                      x0, y0, x0 + px_per_col, y0 + px_per_row,
                      r, g, b);
            uint8_t lum = luma(r, g, b);

            TuiCell& cell = fb.at(col, row);
            cell.fg_r = 200; cell.fg_g = 200; cell.fg_b = 200;
            cell.bg_r = 0;   cell.bg_g = 0;   cell.bg_b = 0;
            cell.glyph[0] = shade_for_luma(lum);
            cell.glyph[1] = 0;
        }
    }
}

// --------------------------------------------------------------------------
// Shading pass  --  refines flat-color areas using luminance palette
// --------------------------------------------------------------------------
void TuiRenderer::apply_shading_pass(const uint8_t* rgba, int src_w, int src_h,
                                     TuiFramebuffer& fb) const {
    // Only rewrite cells that have not been tagged as edge cells.
    // For each cell, sample average luminance and if the area is near-flat
    // (low gradient variance) add a sub-pixel texture character.
    int px_per_col = std::max(src_w / std::max(fb.cols, 1), 1);
    int px_per_row = std::max(src_h / std::max(fb.rows, 1), 1);

    for (int row = 0; row < fb.rows; ++row) {
        for (int col = 0; col < fb.cols; ++col) {
            TuiCell& cell = fb.at(col, row);
            if (cell.is_edge) continue;

            int x0 = col * px_per_col;
            int y0 = row * px_per_row;
            uint8_t r, g, b;
            avg_color(rgba, src_w, src_h,
                      x0, y0, x0 + px_per_col, y0 + px_per_row,
                      r, g, b);
            uint8_t lum = luma(r, g, b);

            // Compute local gradient to decide whether to shade.
            uint8_t lum_nw = luma(sample(rgba,src_w,src_h,x0,     y0    )[0],
                                  sample(rgba,src_w,src_h,x0,     y0    )[1],
                                  sample(rgba,src_w,src_h,x0,     y0    )[2]);
            uint8_t lum_se = luma(sample(rgba,src_w,src_h,x0+px_per_col-1, y0+px_per_row-1)[0],
                                  sample(rgba,src_w,src_h,x0+px_per_col-1, y0+px_per_row-1)[1],
                                  sample(rgba,src_w,src_h,x0+px_per_col-1, y0+px_per_row-1)[2]);
            int diff = std::abs(static_cast<int>(lum_nw) - static_cast<int>(lum_se));

            // Flat area: enhance with shading character for depth.
            if (diff < 20) {
                char shade_ch = shade_for_luma(lum);
                // Apply only when it contributes texture rather than extremes.
                if (shade_ch != '@' && shade_ch != ' ') {
                    cell.glyph[0] = shade_ch;
                    cell.glyph[1] = 0;
                }
            }
        }
    }
}

// --------------------------------------------------------------------------
// Sobel edge pass  --  detects luminance edges, overlays box/block glyphs
// --------------------------------------------------------------------------
void TuiRenderer::apply_edge_pass(const uint8_t* rgba, int src_w, int src_h,
                                  TuiFramebuffer& fb) const {
    int px_per_col = std::max(src_w / std::max(fb.cols, 1), 1);
    int px_per_row = std::max(src_h / std::max(fb.rows, 1), 1);

    for (int row = 0; row < fb.rows; ++row) {
        for (int col = 0; col < fb.cols; ++col) {
            // Sample 3x3 neighbourhood in cell-centre coordinates.
            auto cell_luma = [&](int dc, int dr) -> int {
                int cx = (col + dc) * px_per_col + px_per_col / 2;
                int cy = (row + dr) * px_per_row + px_per_row / 2;
                const uint8_t* p = sample(rgba, src_w, src_h, cx, cy);
                return luma(p[0], p[1], p[2]);
            };

            int tl=cell_luma(-1,-1), tm=cell_luma(0,-1), tr=cell_luma(1,-1);
            int ml=cell_luma(-1, 0),                      mr=cell_luma(1, 0);
            int bl=cell_luma(-1, 1), bm=cell_luma(0, 1), br=cell_luma(1, 1);

            // Sobel operators.
            int gx = -tl + tr - 2*ml + 2*mr - bl + br;
            int gy = -tl - 2*tm - tr + bl + 2*bm + br;
            int mag = static_cast<int>(std::sqrt(gx*gx + gy*gy));

            if (mag < edge_thresh) continue;

            TuiCell& cell = fb.at(col, row);
            cell.is_edge = true;

            // Pick an edge glyph based on dominant gradient direction.
            float angle = std::atan2(static_cast<float>(gy),
                                     static_cast<float>(gx));
            float deg = angle * (180.f / 3.14159265f);
            if (deg < 0) deg += 180.f;

            uint32_t glyph_cp;
            if (deg < 22.5f || deg >= 157.5f) {
                glyph_cp = 0x2502; // vertical
            } else if (deg < 67.5f) {
                glyph_cp = 0x2571; // diagonal /
            } else if (deg < 112.5f) {
                glyph_cp = 0x2500; // horizontal
            } else {
                glyph_cp = 0x2572; // diagonal backslash
            }

            utf8_encode(glyph_cp, cell.glyph);

            // Edge color: bright accent (cyan/teal futuristic tone).
            if (mode == TuiMode::MONOCHROME) {
                cell.fg_r = 220; cell.fg_g = 220; cell.fg_b = 220;
            } else {
                cell.fg_r = 0;   cell.fg_g = 220; cell.fg_b = 200;  // teal
            }
        }
    }
}

// --------------------------------------------------------------------------
// Main render dispatch
// --------------------------------------------------------------------------
void TuiRenderer::render(const uint8_t* rgba, int src_w, int src_h,
                         TuiFramebuffer& fb) const {
    switch (mode) {
        case TuiMode::HALF_BLOCK:  build_half_block(rgba, src_w, src_h, fb); break;
        case TuiMode::BRAILLE:     build_braille   (rgba, src_w, src_h, fb); break;
        case TuiMode::MONOCHROME:  build_monochrome(rgba, src_w, src_h, fb); break;
    }
    if (shading_pass && mode == TuiMode::MONOCHROME)
        apply_shading_pass(rgba, src_w, src_h, fb);
    if (edge_pass && mode != TuiMode::BRAILLE)
        apply_edge_pass(rgba, src_w, src_h, fb);
}

// ==========================================================================
// Color serialization helpers
// ==========================================================================
std::string TuiRenderer::tc(uint8_t r, uint8_t g, uint8_t b, bool bg) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "\x1b[%s;2;%d;%d;%dm",
                  bg ? "48" : "38", r, g, b);
    return buf;
}

std::string TuiRenderer::c256(uint8_t r, uint8_t g, uint8_t b, bool bg) {
    // Convert to xterm-256 6x6x6 cube (indices 16-231).
    int ri = (r * 5 + 127) / 255;
    int gi = (g * 5 + 127) / 255;
    int bi = (b * 5 + 127) / 255;
    int idx = 16 + 36 * ri + 6 * gi + bi;
    char buf[24];
    std::snprintf(buf, sizeof(buf), "\x1b[%s;5;%dm", bg ? "48" : "38", idx);
    return buf;
}

std::string TuiRenderer::color_seq(uint8_t r, uint8_t g, uint8_t b, bool bg) const {
    switch (color_mode) {
        case TuiColorMode::TRUECOLOR:  return tc(r, g, b, bg);
        case TuiColorMode::COLOR256:   return c256(r, g, b, bg);
        case TuiColorMode::MONO:       return "";
    }
    return "";
}

// --------------------------------------------------------------------------
// Full-frame serializer
// --------------------------------------------------------------------------
void TuiRenderer::serialize(const TuiFramebuffer& fb,
                            std::string& dst,
                            bool full_redraw) const {
    dst.clear();
    dst.reserve(static_cast<std::size_t>(fb.cols * fb.rows * 16));

    if (full_redraw) {
        dst += "\x1b[?25l";   // hide cursor
        dst += "\x1b[H";      // cursor home
    }

    uint8_t prev_fg_r = 255, prev_fg_g = 255, prev_fg_b = 255;
    uint8_t prev_bg_r =   0, prev_bg_g =   0, prev_bg_b =   0;
    bool    first = true;

    for (int row = 0; row < fb.rows; ++row) {
        for (int col = 0; col < fb.cols; ++col) {
            const TuiCell& c = fb.at(col, row);

            // Only emit color escape when color changes.
            bool fg_changed = first ||
                c.fg_r != prev_fg_r || c.fg_g != prev_fg_g || c.fg_b != prev_fg_b;
            bool bg_changed = first ||
                c.bg_r != prev_bg_r || c.bg_g != prev_bg_g || c.bg_b != prev_bg_b;

            if (fg_changed) {
                dst += color_seq(c.fg_r, c.fg_g, c.fg_b, false);
                prev_fg_r = c.fg_r; prev_fg_g = c.fg_g; prev_fg_b = c.fg_b;
            }
            if (bg_changed) {
                dst += color_seq(c.bg_r, c.bg_g, c.bg_b, true);
                prev_bg_r = c.bg_r; prev_bg_g = c.bg_g; prev_bg_b = c.bg_b;
            }
            first = false;

            dst.append(c.glyph, std::strlen(c.glyph));
        }
        dst += "\x1b[0m\r\n";
        first = true;  // reset color run tracking at line break
    }

    dst += "\x1b[0m";
}

// --------------------------------------------------------------------------
// Diff serializer  --  only re-draw changed cells using cursor positioning
// --------------------------------------------------------------------------
void TuiRenderer::serialize_diff(const TuiFramebuffer& prev,
                                 const TuiFramebuffer& curr,
                                 std::string& dst) const {
    dst.clear();
    if (prev.cols != curr.cols || prev.rows != curr.rows) {
        // Size changed: full redraw.
        serialize(curr, dst, true);
        return;
    }

    dst.reserve(static_cast<std::size_t>(curr.cols * curr.rows * 4));
    dst += "\x1b[?25l";   // hide cursor

    char pos_buf[24];
    for (int row = 0; row < curr.rows; ++row) {
        for (int col = 0; col < curr.cols; ++col) {
            const TuiCell& nc = curr.at(col, row);
            const TuiCell& pc = prev.at(col, row);

            bool same = (std::memcmp(nc.glyph, pc.glyph, 5) == 0 &&
                         nc.fg_r == pc.fg_r && nc.fg_g == pc.fg_g &&
                         nc.fg_b == pc.fg_b && nc.bg_r == pc.bg_r &&
                         nc.bg_g == pc.bg_g && nc.bg_b == pc.bg_b);
            if (same) continue;

            // Position cursor (ANSI 1-based row,col).
            std::snprintf(pos_buf, sizeof(pos_buf), "\x1b[%d;%dH", row + 1, col + 1);
            dst.append(pos_buf);

            dst += color_seq(nc.fg_r, nc.fg_g, nc.fg_b, false);
            dst += color_seq(nc.bg_r, nc.bg_g, nc.bg_b, true);
            dst.append(nc.glyph, std::strlen(nc.glyph));
        }
    }
    dst += "\x1b[0m";
}

// ==========================================================================
// InputEngine
// ==========================================================================

InputEngine::InputEngine()  = default;
InputEngine::~InputEngine() { close(); }

void InputEngine::write_escape(const char* s) {
    if (fd_in_ == STDIN_FILENO) {
        ::write(STDOUT_FILENO, s, std::strlen(s));
    }
}

bool InputEngine::open(int fd_in) {
    if (open_) return true;
    fd_in_ = fd_in;
    set_raw();
    enable_mouse();
    open_ = true;
    return true;
}

void InputEngine::close() {
    if (!open_) return;
    disable_mouse();
    restore();
    open_ = false;
}

#ifndef _WIN32
void InputEngine::set_raw() {
    if (tcgetattr(fd_in_, &orig_termios_) != 0) return;
    struct termios raw = orig_termios_;
    raw.c_iflag &= ~static_cast<tcflag_t>(IGNBRK | BRKINT | PARMRK | ISTRIP |
                                           INLCR  | IGNCR  | ICRNL  | IXON);
    raw.c_oflag &= ~static_cast<tcflag_t>(OPOST);
    raw.c_cflag |=  static_cast<tcflag_t>(CS8);
    raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(fd_in_, TCSAFLUSH, &raw);
}

void InputEngine::restore() {
    tcsetattr(fd_in_, TCSAFLUSH, &orig_termios_);
}
#else
void InputEngine::set_raw()  {}
void InputEngine::restore()  {}
#endif

void InputEngine::enable_mouse() {
    // Request SGR 1006 extended mouse mode (supports >223 columns).
    write_escape("\x1b[?1000h"); // normal mouse tracking
    write_escape("\x1b[?1002h"); // button-event tracking
    write_escape("\x1b[?1006h"); // SGR extended mode
    sgr_mouse_ = true;
}

void InputEngine::disable_mouse() {
    write_escape("\x1b[?1006l");
    write_escape("\x1b[?1002l");
    write_escape("\x1b[?1000l");
}

int InputEngine::read_byte(int timeout_ms) {
#ifndef _WIN32
    struct pollfd pfd = { fd_in_, POLLIN, 0 };
    int r = ::poll(&pfd, 1, timeout_ms);
    if (r <= 0) return -1;
    unsigned char c;
    if (::read(fd_in_, &c, 1) == 1) return c;
#endif
    return -1;
}

TuiEvent InputEngine::parse_mouse_sgr(const char* seq, int len) {
    // Format: ESC [ < Pb ; Px ; Py M/m
    // where M=press, m=release.
    TuiEvent ev;
    bool release = (seq[len-1] == 'm');
    int btn = 0, col = 0, row = 0;
    std::sscanf(seq, "\x1b[<%d;%d;%d", &btn, &col, &row);
    int scroll = btn & 0x40;
    btn &= 0x03;
    ev.mouse_col = col;
    ev.mouse_row = row;
    ev.mouse_btn = btn;
    if (scroll) {
        ev.kind = TuiEventKind::SCROLL;
        ev.scroll_delta = (btn == 1) ? -1 : 1;
    } else if (!release && (seq[len-1] == 'M') && (btn == 0 || btn == 1 || btn == 2)) {
        ev.kind = TuiEventKind::MOUSE_DOWN;
    } else if (release) {
        ev.kind = TuiEventKind::MOUSE_UP;
    } else {
        ev.kind = TuiEventKind::MOUSE_MOVE;
    }
    return ev;
}

TuiEvent InputEngine::parse_escape() {
    // Read the rest of the escape sequence with short timeout.
    char buf[64];
    int  len = 0;
    buf[len++] = '\x1b';
    int b;
    while (len < 62 && (b = read_byte(20)) >= 0) {
        buf[len++] = static_cast<char>(b);
        char last = buf[len-1];
        // Terminator characters for CSI sequences.
        if (last >= 0x40 && last <= 0x7e) break;
    }
    buf[len] = 0;

    // Mouse SGR.
    if (len > 3 && buf[1] == '[' && buf[2] == '<') {
        return parse_mouse_sgr(buf, len);
    }

    // Cursor keys.
    if (len == 3 && buf[1] == '[') {
        TuiEvent ev; ev.kind = TuiEventKind::KEY;
        switch (buf[2]) {
            case 'A': ev.codepoint = Key::UP;    return ev;
            case 'B': ev.codepoint = Key::DOWN;  return ev;
            case 'C': ev.codepoint = Key::RIGHT; return ev;
            case 'D': ev.codepoint = Key::LEFT;  return ev;
            case 'H': ev.codepoint = Key::HOME;  return ev;
            case 'F': ev.codepoint = Key::END;   return ev;
        }
    }
    // Page up/down.
    if (len == 4 && buf[1] == '[' && buf[3] == '~') {
        TuiEvent ev; ev.kind = TuiEventKind::KEY;
        switch (buf[2]) {
            case '5': ev.codepoint = Key::PAGE_UP;   return ev;
            case '6': ev.codepoint = Key::PAGE_DOWN; return ev;
            case '2': ev.codepoint = Key::INSERT;    return ev;
            case '3': ev.codepoint = Key::DELETE_K;  return ev;
        }
    }
    // F1-F4 via SS3.
    if (len == 3 && buf[1] == 'O') {
        TuiEvent ev; ev.kind = TuiEventKind::KEY;
        switch (buf[2]) {
            case 'P': ev.codepoint = Key::F1; return ev;
            case 'Q': ev.codepoint = Key::F2; return ev;
            case 'R': ev.codepoint = Key::F3; return ev;
            case 'S': ev.codepoint = Key::F4; return ev;
        }
    }
    // Standalone ESC.
    if (len == 1) {
        TuiEvent ev; ev.kind = TuiEventKind::KEY;
        ev.codepoint = Key::ESC;
        return ev;
    }

    return TuiEvent{};
}

TuiEvent InputEngine::poll() {
    int b = read_byte(0);
    if (b < 0) return TuiEvent{};

    if (b == 0x1b) return parse_escape();

    TuiEvent ev;
    ev.kind = TuiEventKind::KEY;
    ev.ctrl  = (b < 0x20 && b != 0x09 && b != 0x0d && b != 0x7f);
    if (ev.ctrl) {
        ev.codepoint = static_cast<uint32_t>('@' + b);
    } else {
        ev.codepoint = static_cast<uint32_t>(b);
    }
    return ev;
}

TuiEvent InputEngine::wait(int timeout_ms) {
    int b = read_byte(timeout_ms);
    if (b < 0) return TuiEvent{};
    if (b == 0x1b) return parse_escape();

    TuiEvent ev;
    ev.kind      = TuiEventKind::KEY;
    ev.ctrl      = (b < 0x20 && b != 0x09 && b != 0x0d && b != 0x7f);
    ev.codepoint = static_cast<uint32_t>(ev.ctrl ? '@' + b : b);
    return ev;
}

// ==========================================================================
// TuiApp
// ==========================================================================

TuiApp* TuiApp::instance_ = nullptr;

void TuiApp::on_sigwinch(int) {
    if (instance_) {
        instance_->detect_size();
        instance_->dirty_ = true;
    }
}

TuiApp::TuiApp(const TuiAppConfig& cfg) : cfg_(cfg) {
    renderer_.mode        = cfg.mode;
    renderer_.color_mode  = cfg.color_mode;
    renderer_.edge_pass   = cfg.edge_pass;
    renderer_.shading_pass= cfg.shading_pass;
    renderer_.edge_thresh = cfg.edge_thresh;

    instance_ = this;
#ifndef _WIN32
    struct sigaction sa = {};
    sa.sa_handler = on_sigwinch;
    sigaction(SIGWINCH, &sa, nullptr);
#endif
    detect_size();
    fb_cur_.resize(cols_, rows_);
    fb_prev_.resize(cols_, rows_);
}

TuiApp::~TuiApp() {
    if (instance_ == this) instance_ = nullptr;
    // Restore cursor visibility on exit.
    ::write(STDOUT_FILENO, "\x1b[?25h", 6);
    ::write(STDOUT_FILENO, "\x1b[0m",   4);
}

void TuiApp::detect_size() {
    if (cfg_.force_cols > 0 && cfg_.force_rows > 0) {
        cols_ = cfg_.force_cols;
        rows_ = cfg_.force_rows;
        return;
    }
#ifndef _WIN32
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        cols_ = std::max(1, static_cast<int>(ws.ws_col));
        rows_ = std::max(1, static_cast<int>(ws.ws_row));
        if (cfg_.show_statusbar && rows_ > 1) rows_ -= 1;
    }
#endif
}

void TuiApp::set_frame(const uint8_t* rgba, int src_w, int src_h) {
    fb_cur_.resize(cols_, rows_);
    renderer_.render(rgba, src_w, src_h, fb_cur_);
    dirty_ = true;
}

// Draw a one-line futuristic HUD status bar.
void TuiApp::draw_statusbar() {
    if (!cfg_.show_statusbar) return;

    std::string bar;
    bar.reserve(cols_ + 64);

    // Position cursor at the real last row.
    char pos[24];
    std::snprintf(pos, sizeof(pos), "\x1b[%d;1H", rows_ + 1);
    bar += pos;

    // Dark bg + bright accent styling.
    bar += "\x1b[48;2;12;12;24m"; // near-black bg
    bar += "\x1b[38;2;0;220;180m"; // teal fg

    // Left side: decorative bracket + status text.
    std::string text = " [ xcm ] " + status_text_;
    if (static_cast<int>(text.size()) < cols_) {
        text.append(static_cast<std::size_t>(cols_ - static_cast<int>(text.size())), ' ');
    } else {
        text = text.substr(0, static_cast<std::size_t>(cols_));
    }
    bar += text;
    bar += "\x1b[0m";

    ::write(STDOUT_FILENO, bar.c_str(), bar.size());
}

void TuiApp::flush_frame(bool full) {
    if (!dirty_) return;

    if (full || fb_prev_.cols != fb_cur_.cols || fb_prev_.rows != fb_cur_.rows) {
        renderer_.serialize(fb_cur_, frame_buf_, true);
    } else {
        renderer_.serialize_diff(fb_prev_, fb_cur_, frame_buf_);
    }

    ::write(STDOUT_FILENO, frame_buf_.c_str(), frame_buf_.size());
    draw_statusbar();

    fb_prev_ = fb_cur_;
    dirty_ = false;
}

void TuiApp::run(EventCb cb) {
    if (cfg_.mouse) input_.open();

    // Full clear on first frame.
    flush_frame(true);

    const int frame_ms = 1000 / std::max(1, cfg_.target_fps);

    while (true) {
        TuiEvent ev = input_.wait(frame_ms);

        bool should_continue = true;

        if (ev.kind == TuiEventKind::RESIZE) {
            detect_size();
            fb_cur_.resize(cols_, rows_);
            fb_prev_.resize(cols_, rows_);
            dirty_ = true;
        }

        if (ev.kind != TuiEventKind::NONE) {
            should_continue = cb(ev);
            if (!should_continue) break;
        }

        // Periodic frame flush whether or not there was input.
        flush_frame(false);
    }

    input_.close();
}

} // namespace xcm
