#pragma once
/*
 * xcm_tui.h  --  TUI / CLI terminal renderer for xcm render core
 *
 * Architecture:
 *   - RenderCore (HTML/CSS -> RGBA framebuffer) feeds TuiRenderer
 *   - TuiRenderer maps each pixel region to a terminal cell using one of:
 *       HALF_BLOCK  -- upper-half block U+2580 with fg=top-pixel, bg=bot-pixel
 *                      gives 2 rows of true color per terminal row (best-looking)
 *       BRAILLE     -- 2x4 pixel sampling per cell; ideal for fine edge drawing
 *       MONOCHROME  -- pure ASCII shading chars with no color (max compat)
 *   - EdgePass      -- Sobel-filter on the luminance map adds character-level
 *                      edge glyphs (box-drawing / block-element overlays) for a
 *                      stylized "vector over raster" look.
 *   - ShadingPass   -- Maps luminance bands to a dense Unicode shading palette
 *                      so gradients read clearly even in 256-color or mono modes.
 *   - InputEngine   -- raw terminal mode, mouse tracking (SGR 1006 / fallback
 *                      X10), resize SIGWINCH, keyboard decode table.
 *   - TuiApp        -- main event loop: handles resize, mouse, keyboard,
 *                      scroll and delegates to a user-supplied callback.
 *
 * Design goals:
 *   - No external dependencies beyond libc / POSIX.
 *   - Works on macOS, Linux, BSD; no Windows CRT assumptions.
 *   - Cross-platform scripting integration: call from Python via ctypes/cffi,
 *     from Node via ffi-napi, from shell via xcm_tui binary.
 *   - "Futuristic TUI" aesthetic: slim box-drawing borders, half-block
 *     pixel cells, sharp edge outlines, smooth luminance shading.
 */

#include "paint.h"
#include "layout.h"

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// POSIX terminal includes
#ifndef _WIN32
#  include <termios.h>
#  include <sys/ioctl.h>
#  include <unistd.h>
#  include <signal.h>
#endif

namespace xcm {

// ==========================================================================
// Render mode
// ==========================================================================
enum class TuiMode : uint8_t {
    HALF_BLOCK  = 0,   // U+2580 upper-half: 2px rows per terminal row, full color
    BRAILLE     = 1,   // 2x4 braille dots per cell, fg only  (best edge resolution)
    MONOCHROME  = 2,   // ASCII shading palette, no color -- maximum compatibility
};

// ==========================================================================
// Color capability
// ==========================================================================
enum class TuiColorMode : uint8_t {
    TRUECOLOR  = 0,   // 24-bit \x1b[38;2;R;G;Bm
    COLOR256   = 1,   // xterm-256 palette
    MONO       = 2,   // no color -- structure only
};

// ==========================================================================
// Input event types
// ==========================================================================
enum class TuiEventKind : uint8_t {
    NONE       = 0,
    KEY        = 1,
    MOUSE_MOVE = 2,
    MOUSE_DOWN = 3,
    MOUSE_UP   = 4,
    SCROLL     = 5,
    RESIZE     = 6,
    QUIT       = 7,
};

struct TuiEvent {
    TuiEventKind kind = TuiEventKind::NONE;

    // KEY
    uint32_t codepoint = 0;   // Unicode codepoint or special key constant
    bool     ctrl      = false;
    bool     shift     = false;
    bool     alt       = false;

    // MOUSE (cell coordinates, 1-based)
    int mouse_col  = 0;
    int mouse_row  = 0;
    int mouse_btn  = 0;   // 0=left 1=mid 2=right

    // SCROLL
    int scroll_delta = 0;  // +1 up,  -1 down

    // RESIZE
    int new_cols = 0;
    int new_rows = 0;
};

// Special key constants (stored in codepoint field)
namespace Key {
    constexpr uint32_t UP        = 0x100001;
    constexpr uint32_t DOWN      = 0x100002;
    constexpr uint32_t LEFT      = 0x100003;
    constexpr uint32_t RIGHT     = 0x100004;
    constexpr uint32_t PAGE_UP   = 0x100005;
    constexpr uint32_t PAGE_DOWN = 0x100006;
    constexpr uint32_t HOME      = 0x100007;
    constexpr uint32_t END       = 0x100008;
    constexpr uint32_t F1        = 0x100011;
    constexpr uint32_t F2        = 0x100012;
    constexpr uint32_t F3        = 0x100013;
    constexpr uint32_t F4        = 0x100014;
    constexpr uint32_t F5        = 0x100015;
    constexpr uint32_t F6        = 0x100016;
    constexpr uint32_t F7        = 0x100017;
    constexpr uint32_t F8        = 0x100018;
    constexpr uint32_t F9        = 0x100019;
    constexpr uint32_t F10       = 0x10001a;
    constexpr uint32_t F11       = 0x10001b;
    constexpr uint32_t F12       = 0x10001c;
    constexpr uint32_t ESC       = 0x1b;
    constexpr uint32_t ENTER     = 0x0d;
    constexpr uint32_t BACKSPACE = 0x7f;
    constexpr uint32_t TAB       = 0x09;
    constexpr uint32_t DELETE_K  = 0x100020;
    constexpr uint32_t INSERT    = 0x100021;
}

// ==========================================================================
// TuiCell  --  one terminal cell worth of data  (pre-rendered)
// ==========================================================================
struct TuiCell {
    // UTF-8 glyph (up to 4 bytes + NUL).  Typically a single block/braille char.
    char glyph[5]  = {' ', 0, 0, 0, 0};

    // Colors (R,G,B each 0-255).
    uint8_t fg_r = 255, fg_g = 255, fg_b = 255;
    uint8_t bg_r = 0,   bg_g = 0,   bg_b = 0;

    // Edge overlay flag -- set by EdgePass to preserve sharp outline character.
    bool is_edge = false;
};

// ==========================================================================
// TuiFramebuffer  --  cell grid
// ==========================================================================
struct TuiFramebuffer {
    int cols = 0;
    int rows = 0;
    std::vector<TuiCell> cells; // row-major

    void resize(int c, int r) {
        cols = c; rows = r;
        cells.assign(static_cast<std::size_t>(c * r), TuiCell{});
    }

    TuiCell& at(int col, int row) {
        return cells[static_cast<std::size_t>(row * cols + col)];
    }

    const TuiCell& at(int col, int row) const {
        return cells[static_cast<std::size_t>(row * cols + col)];
    }
};

// ==========================================================================
// TuiRenderer  --  converts RGBA pixel buffer -> TuiFramebuffer
// ==========================================================================
class TuiRenderer {
public:
    TuiMode      mode       = TuiMode::HALF_BLOCK;
    TuiColorMode color_mode = TuiColorMode::TRUECOLOR;

    // Enable Sobel edge detection overlay.
    bool edge_pass    = true;
    // Strength threshold for edge detection (0-255 luminance gradient).
    int  edge_thresh  = 28;
    // Enable luminance-shading palette mapping inside flat areas.
    bool shading_pass = true;

    // Render pixel buffer into cell framebuffer.
    void render(const uint8_t* rgba, int src_w, int src_h,
                TuiFramebuffer& fb) const;

    // Serialize framebuffer to ANSI escape sequences in dst string.
    // Clears dst first.  Adds cursor-home/clear prefix when full_redraw=true.
    void serialize(const TuiFramebuffer& fb,
                   std::string& dst,
                   bool full_redraw = true) const;

    // Produce a diff-update string: only re-emit cells that differ from prev.
    void serialize_diff(const TuiFramebuffer& prev,
                        const TuiFramebuffer& curr,
                        std::string& dst) const;

private:
    // Per-mode cell builders.
    void build_half_block (const uint8_t* rgba, int src_w, int src_h,
                           TuiFramebuffer& fb) const;
    void build_braille    (const uint8_t* rgba, int src_w, int src_h,
                           TuiFramebuffer& fb) const;
    void build_monochrome (const uint8_t* rgba, int src_w, int src_h,
                           TuiFramebuffer& fb) const;

    void apply_edge_pass   (const uint8_t* rgba, int src_w, int src_h,
                            TuiFramebuffer& fb) const;
    void apply_shading_pass(const uint8_t* rgba, int src_w, int src_h,
                            TuiFramebuffer& fb) const;

    // Color quantization helpers.
    static std::string tc(uint8_t r, uint8_t g, uint8_t b, bool bg);
    static std::string c256(uint8_t r, uint8_t g, uint8_t b, bool bg);
    std::string color_seq(uint8_t r, uint8_t g, uint8_t b, bool bg) const;
};

// ==========================================================================
// InputEngine  --  raw terminal input with mouse support
// ==========================================================================
class InputEngine {
public:
    InputEngine();
    ~InputEngine();

    // Enter raw mode, enable mouse tracking.
    bool open(int fd_in = STDIN_FILENO);
    // Restore terminal state, disable mouse tracking.
    void close();

    // Non-blocking read of next pending event.  Returns NONE if no input.
    TuiEvent poll();

    // Blocking read -- waits up to timeout_ms (-1 = forever).
    TuiEvent wait(int timeout_ms = -1);

    bool is_open() const { return open_; }

private:
    int   fd_in_   = STDIN_FILENO;
    bool  open_    = false;
    bool  sgr_mouse_ = false;  // SGR 1006 vs legacy X10

#ifndef _WIN32
    struct termios orig_termios_ = {};
#endif

    int  read_byte(int timeout_ms);
    TuiEvent parse_escape();
    TuiEvent parse_mouse_sgr(const char* seq, int len);
    void set_raw();
    void restore();
    void enable_mouse();
    void disable_mouse();
    void write_escape(const char* s);
};

// ==========================================================================
// TuiApp  --  top-level event loop binding
// ==========================================================================
struct TuiAppConfig {
    TuiMode      mode        = TuiMode::HALF_BLOCK;
    TuiColorMode color_mode  = TuiColorMode::TRUECOLOR;
    bool         edge_pass   = true;
    bool         shading_pass= true;
    int          edge_thresh = 28;
    int          target_fps  = 30;
    bool         mouse       = true;
    bool         show_statusbar = true;

    // Override terminal size (0 = autodetect).
    int force_cols = 0;
    int force_rows = 0;
};

class TuiApp {
public:
    using EventCb = std::function<bool(const TuiEvent&)>;  // return false to quit

    explicit TuiApp(const TuiAppConfig& cfg = TuiAppConfig{});
    ~TuiApp();

    // Push a new rendered RGBA frame.  Call whenever content changes.
    void set_frame(const uint8_t* rgba, int src_w, int src_h);

    // Attach event callback then enter blocking loop until callback returns false.
    void run(EventCb cb);

    // Get current terminal dimensions.
    int cols() const { return cols_; }
    int rows() const { return rows_; }

    // Show a one-line status bar at the bottom (futuristic HUD style).
    void set_status(const std::string& text) { status_text_ = text; }

private:
    TuiAppConfig    cfg_;
    TuiRenderer     renderer_;
    InputEngine     input_;
    TuiFramebuffer  fb_cur_, fb_prev_;

    int  cols_ = 80;
    int  rows_ = 24;
    bool dirty_ = true;

    std::string status_text_;
    std::string frame_buf_;  // assembled ANSI output

    void detect_size();
    void draw_statusbar();
    void flush_frame(bool full);
    void draw_cursor_hud(int col, int row);

    // SIGWINCH handler support.
    static TuiApp* instance_;
    static void    on_sigwinch(int);
};

} // namespace xcm
