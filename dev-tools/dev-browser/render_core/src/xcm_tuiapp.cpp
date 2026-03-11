/*
 * xcm_tuiapp.cpp  --  Standalone TUI CLI binary entry point
 *
 * Usage:
 *   xcm_tui [options] [--html <file.html>] [--css <file.css>]
 *   xcm_tui --width 1280 --height 800 --mode half  < page.html
 *
 * Options:
 *   --width  N      Pixel render width  (default 1280)
 *   --height N      Pixel render height (default 800)
 *   --mode   MODE   Render mode: half | braille | mono  (default: half)
 *   --color  MODE   Color mode: true | 256 | mono       (default: true)
 *   --no-edge        Disable Sobel edge overlay
 *   --no-shade       Disable luminance shading pass
 *   --edge-thresh N  Edge detection sensitivity (default 28, lower=more edges)
 *   --fps    N      Target frame rate for live mode (default 30)
 *   --html   FILE   HTML input file (default: read stdin until EOF for static render)
 *   --css    FILE   CSS input file
 *   --no-mouse       Disable mouse tracking
 *   --no-status      Hide HUD status bar
 *   --static         Render one frame to stdout and exit (non-interactive)
 *   --cols   N      Override terminal column count
 *   --rows   N      Override terminal row count
 *
 * Interactive controls:
 *   q / ESC         Quit
 *   UP / DOWN       Scroll
 *   PAGE UP/DOWN    Fast scroll
 *   r               Hard reload / re-render
 *   m               Cycle render modes (half -> braille -> mono)
 *   e               Toggle edge overlay
 *   s               Toggle shading pass
 *   +/-             Adjust edge threshold
 *   ?               Show help overlay
 *
 * Mouse:
 *   Click           Highlight hovered cell position in status bar
 *   Scroll wheel    Page scroll
 */

#include "xcm_tui.h"
#include "html_tokenizer.h"
#include "css_parser.h"
#include "style_resolver.h"
#include "layout.h"
#include "paint.h"
#include "arena.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ==========================================================================
// File / stdin helpers
// ==========================================================================
static std::string read_file(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "xcm_tui: cannot open %s\n", path);
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string read_stdin() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
}

// ==========================================================================
// Render state (shared between event loop callbacks)
// ==========================================================================
struct AppState {
    // Config
    int px_w = 1280;
    int px_h = 800;
    float scroll_y = 0.f;

    // Content
    std::string html;
    std::string css;

    // Render core objects (heap-allocated for simplicity in main).
    xcm::Arena      dom_arena;
    xcm::Arena      layout_arena;
    xcm::PaintEngine* paint = nullptr;
    xcm::TuiApp*      app   = nullptr;

    // TUI config (mutable via key bindings).
    xcm::TuiMode      mode       = xcm::TuiMode::HALF_BLOCK;
    xcm::TuiColorMode color_mode = xcm::TuiColorMode::TRUECOLOR;
    bool edge_pass    = true;
    bool shading_pass = true;
    int  edge_thresh  = 28;

    // Mouse position feedback.
    int  mouse_col = 0;
    int  mouse_row = 0;
    bool help_visible = false;

    bool render_frame() {
        dom_arena.reset();
        layout_arena.reset();

        xcm::Document* doc = xcm::parse_html(html.c_str(), html.size(), dom_arena);
        if (!doc) return false;

        std::vector<xcm::StyleSheet> sheets;
        if (!css.empty()) {
            sheets.push_back(xcm::parse_css(css.c_str(), css.size(), 2));
        }
        xcm::resolve_styles(doc, sheets,
                            static_cast<float>(px_w),
                            static_cast<float>(px_h));

        xcm::LayoutEngine eng(static_cast<float>(px_w),
                              static_cast<float>(px_h),
                              layout_arena);
        xcm::LayoutBox* root = eng.layout(doc);

        paint->set_scroll(scroll_y);
        paint->paint(root);

        app->set_frame(paint->pixels(), px_w, px_h);
        return true;
    }

    void push_status() {
        char buf[128];
        const char* mode_str =
            (mode == xcm::TuiMode::HALF_BLOCK) ? "half" :
            (mode == xcm::TuiMode::BRAILLE)    ? "braille" : "mono";
        std::snprintf(buf, sizeof(buf),
                      "mode:%s  edge:%s  shade:%s  thresh:%d  scroll:%.0f  "
                      "mouse:[%d,%d]%s",
                      mode_str,
                      edge_pass    ? "on" : "off",
                      shading_pass ? "on" : "off",
                      edge_thresh,
                      scroll_y,
                      mouse_col, mouse_row,
                      help_visible ? "  [?=help]" : "  [?=help q=quit]");
        app->set_status(buf);
    }
};

// ==========================================================================
// Help overlay  --  printed to a small box in the terminal frame
// ==========================================================================
static const char* HELP_TEXT =
    "\x1b[1m  xcm_tui keyboard shortcuts \x1b[0m\r\n"
    "  q / ESC     quit\r\n"
    "  UP / DN     scroll\r\n"
    "  PgUp/PgDn   fast scroll\r\n"
    "  m           cycle render mode (half/braille/mono)\r\n"
    "  e           toggle edge overlay\r\n"
    "  s           toggle shading\r\n"
    "  +  -        edge sensitivity\r\n"
    "  r           re-render\r\n"
    "  ?           toggle this help\r\n";

// ==========================================================================
// Argument parsing
// ==========================================================================
struct CliArgs {
    int   px_w         = 1280;
    int   px_h         = 800;
    const char* html_file  = nullptr;
    const char* css_file   = nullptr;
    xcm::TuiMode      mode       = xcm::TuiMode::HALF_BLOCK;
    xcm::TuiColorMode color_mode = xcm::TuiColorMode::TRUECOLOR;
    bool  edge_pass    = true;
    bool  shading_pass = true;
    int   edge_thresh  = 28;
    int   fps          = 30;
    bool  mouse        = true;
    bool  statusbar    = true;
    bool  static_mode  = false;
    int   force_cols   = 0;
    int   force_rows   = 0;
};

static void print_usage() {
    std::fputs(
        "Usage: xcm_tui [options]\n"
        "  --width N       pixel render width  (default 1280)\n"
        "  --height N      pixel render height (default 800)\n"
        "  --html FILE     HTML input file (or pipe to stdin)\n"
        "  --css  FILE     CSS input file\n"
        "  --mode MODE     half | braille | mono  (default half)\n"
        "  --color MODE    true | 256 | mono      (default true)\n"
        "  --no-edge       disable edge overlay\n"
        "  --no-shade      disable shading pass\n"
        "  --edge-thresh N sensitivity (default 28)\n"
        "  --fps N         target FPS  (default 30)\n"
        "  --no-mouse      disable mouse tracking\n"
        "  --no-status     hide HUD status bar\n"
        "  --static        one-shot render then exit\n"
        "  --cols N        override terminal columns\n"
        "  --rows N        override terminal rows\n"
        , stdout);
}

static CliArgs parse_args(int argc, char** argv) {
    CliArgs a;
    for (int i = 1; i < argc; ++i) {
        auto arg = [&](const char* flag) { return std::strcmp(argv[i], flag) == 0; };
        auto next_int = [&]() -> int {
            return (i + 1 < argc) ? std::atoi(argv[++i]) : 0;
        };
        auto next_str = [&]() -> const char* {
            return (i + 1 < argc) ? argv[++i] : nullptr;
        };

        if      (arg("--width"))       a.px_w        = next_int();
        else if (arg("--height"))      a.px_h        = next_int();
        else if (arg("--html"))        a.html_file   = next_str();
        else if (arg("--css"))         a.css_file    = next_str();
        else if (arg("--fps"))         a.fps         = next_int();
        else if (arg("--edge-thresh")) a.edge_thresh = next_int();
        else if (arg("--cols"))        a.force_cols  = next_int();
        else if (arg("--rows"))        a.force_rows  = next_int();
        else if (arg("--no-edge"))     a.edge_pass   = false;
        else if (arg("--no-shade"))    a.shading_pass= false;
        else if (arg("--no-mouse"))    a.mouse       = false;
        else if (arg("--no-status"))   a.statusbar   = false;
        else if (arg("--static"))      a.static_mode = true;
        else if (arg("--mode")) {
            const char* m = next_str();
            if (m) {
                if  (std::strcmp(m, "braille") == 0) a.mode = xcm::TuiMode::BRAILLE;
                else if (std::strcmp(m, "mono") == 0) a.mode = xcm::TuiMode::MONOCHROME;
            }
        }
        else if (arg("--color")) {
            const char* c = next_str();
            if (c) {
                if  (std::strcmp(c, "256")  == 0) a.color_mode = xcm::TuiColorMode::COLOR256;
                else if (std::strcmp(c, "mono") == 0) a.color_mode = xcm::TuiColorMode::MONO;
            }
        }
        else if (arg("--help") || arg("-h")) { print_usage(); std::exit(0); }
    }
    return a;
}

// ==========================================================================
// main
// ==========================================================================
int main(int argc, char** argv) {
    CliArgs args = parse_args(argc, argv);

    // Disable stdout buffering -- TUI writes raw escape bytes directly.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    // Load content.
    std::string html_src = args.html_file ? read_file(args.html_file) : read_stdin();
    std::string css_src  = args.css_file  ? read_file(args.css_file)  : "";

    if (html_src.empty()) {
        std::fputs("xcm_tui: no HTML input\n", stderr);
        return 1;
    }

    // --static mode: render one ANSI frame to stdout and exit.
    if (args.static_mode) {
        xcm::Arena dom_arena, layout_arena;
        xcm::PaintEngine paint(args.px_w, args.px_h);

        xcm::Document* doc = xcm::parse_html(html_src.c_str(), html_src.size(), dom_arena);
        if (!doc) { std::fputs("xcm_tui: HTML parse failed\n", stderr); return 1; }

        std::vector<xcm::StyleSheet> sheets;
        if (!css_src.empty())
            sheets.push_back(xcm::parse_css(css_src.c_str(), css_src.size(), 2));

        xcm::resolve_styles(doc, sheets,
                            static_cast<float>(args.px_w),
                            static_cast<float>(args.px_h));

        xcm::LayoutEngine eng(static_cast<float>(args.px_w),
                              static_cast<float>(args.px_h),
                              layout_arena);
        xcm::LayoutBox* root = eng.layout(doc);
        paint.paint(root);

        // Determine terminal size for output.
        int cols = args.force_cols, rows = args.force_rows;
        if (cols <= 0 || rows <= 0) {
#ifndef _WIN32
            struct winsize ws;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
                if (cols <= 0) cols = ws.ws_col;
                if (rows <= 0) rows = ws.ws_row;
            }
#endif
            if (cols <= 0) cols = 80;
            if (rows <= 0) rows = 24;
        }

        xcm::TuiRenderer renderer;
        renderer.mode        = args.mode;
        renderer.color_mode  = args.color_mode;
        renderer.edge_pass   = args.edge_pass;
        renderer.shading_pass= args.shading_pass;
        renderer.edge_thresh = args.edge_thresh;

        xcm::TuiFramebuffer fb;
        fb.resize(cols, rows);
        renderer.render(paint.pixels(), args.px_w, args.px_h, fb);

        std::string out;
        renderer.serialize(fb, out, false);
        out += "\x1b[0m\n";
        ::write(STDOUT_FILENO, out.c_str(), out.size());
        return 0;
    }

    // Interactive TUI mode.
    xcm::TuiAppConfig cfg;
    cfg.mode         = args.mode;
    cfg.color_mode   = args.color_mode;
    cfg.edge_pass    = args.edge_pass;
    cfg.shading_pass = args.shading_pass;
    cfg.edge_thresh  = args.edge_thresh;
    cfg.target_fps   = args.fps;
    cfg.mouse        = args.mouse;
    cfg.show_statusbar = args.statusbar;
    cfg.force_cols   = args.force_cols;
    cfg.force_rows   = args.force_rows;

    AppState state;
    state.px_w         = args.px_w;
    state.px_h         = args.px_h;
    state.html         = html_src;
    state.css          = css_src;
    state.mode         = args.mode;
    state.color_mode   = args.color_mode;
    state.edge_pass    = args.edge_pass;
    state.shading_pass = args.shading_pass;
    state.edge_thresh  = args.edge_thresh;
    state.paint        = new xcm::PaintEngine(args.px_w, args.px_h);

    xcm::TuiApp app(cfg);
    state.app = &app;

    // Initial render.
    state.render_frame();
    state.push_status();

    app.run([&](const xcm::TuiEvent& ev) -> bool {
        bool need_render = false;

        if (ev.kind == xcm::TuiEventKind::KEY) {
            switch (ev.codepoint) {
                case 'q':
                case xcm::Key::ESC:
                    return false; // quit

                case xcm::Key::UP:
                    state.scroll_y = std::max(0.f, state.scroll_y - 40.f);
                    need_render = true;
                    break;

                case xcm::Key::DOWN:
                    state.scroll_y += 40.f;
                    need_render = true;
                    break;

                case xcm::Key::PAGE_UP:
                    state.scroll_y = std::max(0.f, state.scroll_y - 300.f);
                    need_render = true;
                    break;

                case xcm::Key::PAGE_DOWN:
                    state.scroll_y += 300.f;
                    need_render = true;
                    break;

                case 'r': case 'R':
                    need_render = true;
                    break;

                case 'm': case 'M': {
                    int next = (static_cast<int>(state.mode) + 1) % 3;
                    state.mode = static_cast<xcm::TuiMode>(next);
                    app.set_status("mode changed");
                    need_render = true;
                    break;
                }

                case 'e': case 'E':
                    state.edge_pass = !state.edge_pass;
                    need_render = true;
                    break;

                case 's': case 'S':
                    state.shading_pass = !state.shading_pass;
                    need_render = true;
                    break;

                case '+': case '=':
                    state.edge_thresh = std::max(1, state.edge_thresh - 4);
                    need_render = true;
                    break;

                case '-':
                    state.edge_thresh = std::min(255, state.edge_thresh + 4);
                    need_render = true;
                    break;

                case '?':
                    state.help_visible = !state.help_visible;
                    if (state.help_visible)
                        ::write(STDOUT_FILENO, HELP_TEXT, std::strlen(HELP_TEXT));
                    break;

                default: break;
            }
        }

        if (ev.kind == xcm::TuiEventKind::SCROLL) {
            state.scroll_y = std::max(0.f,
                state.scroll_y - static_cast<float>(ev.scroll_delta) * 40.f);
            need_render = true;
        }

        if (ev.kind == xcm::TuiEventKind::MOUSE_MOVE ||
            ev.kind == xcm::TuiEventKind::MOUSE_DOWN) {
            state.mouse_col = ev.mouse_col;
            state.mouse_row = ev.mouse_row;
        }

        if (ev.kind == xcm::TuiEventKind::RESIZE) {
            need_render = true;
        }

        if (need_render) {
            // Push updated renderer settings.
            xcm::TuiAppConfig updated = cfg;
            updated.mode         = state.mode;
            updated.edge_pass    = state.edge_pass;
            updated.shading_pass = state.shading_pass;
            updated.edge_thresh  = state.edge_thresh;

            // Re-configure app renderer directly.
            // (xcm_tui exposes renderer as public for live tuning)
            state.render_frame();
        }

        state.push_status();
        return true;  // keep running
    });

    delete state.paint;
    return 0;
}
