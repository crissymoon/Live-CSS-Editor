/*
 * xcm_tui_shell.cpp  --  REPL shell for testing TUI app rendering in terminal
 *
 * This tool is intentionally script-friendly and cross-platform (POSIX terminals):
 *   - Interactive REPL to load HTML/CSS and tune render options
 *   - One-command static render previews in current terminal
 *   - Useful for prototyping TUI app visuals before full app integration
 */

#include "xcm_tui.h"
#include "html_tokenizer.h"
#include "css_parser.h"
#include "style_resolver.h"
#include "layout.h"
#include "paint.h"
#include "arena.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef _WIN32
#  include <sys/ioctl.h>
#  include <unistd.h>
#endif

namespace {

struct ShellState {
    int px_w = 1280;
    int px_h = 800;
    int term_cols = 0; // 0 = auto
    int term_rows = 0; // 0 = auto

    xcm::TuiMode mode = xcm::TuiMode::MONOCHROME;
    xcm::TuiColorMode color = xcm::TuiColorMode::MONO;
    bool edge = true;
    bool shade = true;
    int edge_thresh = 28;

    // Output controls
    // -1 auto, 0 plain (non-ANSI readable), 1 tty (force ANSI)
    int render_target = -1;
    // -1 auto, 0 light, 1 dark
    int theme_override = -1;

    std::string html_path;
    std::string css_path;
    std::string html;
    std::string css;
};

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string to_lower_copy(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string extract_embedded_css(const std::string& html) {
    std::string lower = to_lower_copy(html);
    std::string css;

    std::size_t pos = 0;
    while (true) {
        std::size_t open = lower.find("<style", pos);
        if (open == std::string::npos) break;

        std::size_t tag_end = lower.find('>', open);
        if (tag_end == std::string::npos) break;

        std::size_t close = lower.find("</style>", tag_end + 1);
        if (close == std::string::npos) break;

        css.append(html.substr(tag_end + 1, close - (tag_end + 1)));
        css.push_back('\n');
        pos = close + 8;
    }
    return css;
}

void detect_term_size(int& cols, int& rows) {
#ifndef _WIN32
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        cols = std::max(1, static_cast<int>(ws.ws_col));
        rows = std::max(1, static_cast<int>(ws.ws_row));
        return;
    }
#endif
    cols = 80;
    rows = 24;
}

bool output_is_tty() {
#ifndef _WIN32
    return isatty(STDOUT_FILENO) != 0;
#else
    return true;
#endif
}

const char* mode_str(xcm::TuiMode m) {
    switch (m) {
        case xcm::TuiMode::HALF_BLOCK: return "half";
        case xcm::TuiMode::BRAILLE: return "braille";
        case xcm::TuiMode::MONOCHROME: return "mono";
    }
    return "half";
}

const char* color_str(xcm::TuiColorMode c) {
    switch (c) {
        case xcm::TuiColorMode::TRUECOLOR: return "true";
        case xcm::TuiColorMode::COLOR256: return "256";
        case xcm::TuiColorMode::MONO: return "mono";
    }
    return "true";
}

bool env_has(const char* key, const char* needle) {
    const char* v = std::getenv(key);
    if (!v || !needle) return false;
    std::string s = to_lower_copy(v);
    std::string n = to_lower_copy(needle);
    return s.find(n) != std::string::npos;
}

bool supports_truecolor() {
    return env_has("COLORTERM", "truecolor") || env_has("COLORTERM", "24bit");
}

bool supports_256color() {
    return env_has("TERM", "256color");
}

bool supports_utf8() {
    return env_has("LANG", "utf-8") || env_has("LC_ALL", "utf-8") || env_has("LC_CTYPE", "utf-8");
}

void apply_auto_profile(ShellState& s) {
    // Prefer visual fidelity when terminal advertises UTF-8 + truecolor.
    if (supports_utf8() && supports_truecolor()) {
        s.mode = xcm::TuiMode::HALF_BLOCK;
        s.color = xcm::TuiColorMode::TRUECOLOR;
        s.edge = false;
        s.shade = true;
        s.edge_thresh = 72;
        return;
    }

    // Mid-tier fallback for terminals with xterm-256.
    if (supports_utf8() && supports_256color()) {
        s.mode = xcm::TuiMode::HALF_BLOCK;
        s.color = xcm::TuiColorMode::COLOR256;
        s.edge = false;
        s.shade = true;
        s.edge_thresh = 80;
        return;
    }

    // Maximum compatibility fallback for weak glyph/color support.
    s.mode = xcm::TuiMode::MONOCHROME;
    s.color = xcm::TuiColorMode::MONO;
    s.edge = false;
    s.shade = true;
    s.edge_thresh = 96;
}

void print_help() {
    std::cout
        << "Commands:\n"
        << "  help                         show commands\n"
        << "  auto                         choose best mode for this terminal\n"
        << "  sample                       load built-in sample HTML and render\n"
        << "  render tty                   force ANSI terminal render mode\n"
        << "  render plain                 force plain readable text render mode\n"
        << "  load <file.html>             load HTML file\n"
        << "  css <file.css>               load CSS file\n"
        << "  clearcss                     clear CSS\n"
        << "  theme <auto|light|dark>      sample theme preference\n"
        << "  mode <half|braille|mono>     set render mode\n"
        << "  color <true|256|mono>        set color mode\n"
        << "  edge <on|off>                toggle edge pass\n"
        << "  shade <on|off>               toggle shading pass\n"
        << "  thresh <1-255>               set edge threshold\n"
        << "  px <w> <h>                   set source pixel size\n"
        << "  term <cols> <rows>           set terminal output size (0 0 = auto)\n"
        << "  show                         show current config\n"
        << "  render                       render frame to terminal\n"
        << "  clear                        clear terminal\n"
        << "  quit                         exit shell\n";
}

void print_state(const ShellState& s) {
    std::cout
        << "mode=" << mode_str(s.mode)
        << " color=" << color_str(s.color)
        << " edge=" << (s.edge ? "on" : "off")
        << " shade=" << (s.shade ? "on" : "off")
        << " thresh=" << s.edge_thresh
        << " render=" << (s.render_target == 1 ? "tty" : (s.render_target == 0 ? "plain" : "auto"))
        << " theme=" << (s.theme_override == 1 ? "dark" : (s.theme_override == 0 ? "light" : "auto"))
        << " px=" << s.px_w << "x" << s.px_h
        << " term=" << s.term_cols << "x" << s.term_rows
        << " html=" << (s.html_path.empty() ? "<inline>" : s.html_path)
        << " css=" << (s.css_path.empty() ? "<none>" : s.css_path)
        << "\n";
}

bool render_frame(const ShellState& s) {
    if (s.html.empty()) {
        std::cerr << "No HTML loaded. Use 'sample' or 'load <file.html>'.\n";
        return false;
    }

    int cols = s.term_cols;
    int rows = s.term_rows;
    if (cols <= 0 || rows <= 0) detect_term_size(cols, rows);

    bool is_tty = true;
#ifndef _WIN32
    is_tty = isatty(STDOUT_FILENO) != 0;
#endif
    if (s.render_target == 1) is_tty = true;
    if (s.render_target == 0) is_tty = false;

    // Effective profile for this output target.
    xcm::TuiMode effective_mode = s.mode;
    xcm::TuiColorMode effective_color = s.color;
    bool effective_edge = s.edge;
    bool effective_shade = s.shade;
    int effective_thresh = s.edge_thresh;

#ifndef _WIN32
    if (!is_tty) {
        // Piped output should stay ASCII-readable.
        effective_mode = xcm::TuiMode::MONOCHROME;
        effective_color = xcm::TuiColorMode::MONO;
        effective_edge = false;
        effective_shade = true;
        effective_thresh = 96;
    }
#endif

    // Auto-fit render surface to terminal cell geometry so details survive
    // downsampling. Large source resolutions can collapse text into flat bands.
    int fit_w = cols;
    int fit_h = rows;
    if (effective_mode == xcm::TuiMode::HALF_BLOCK) {
        fit_w = cols;
        fit_h = rows * 2;
    } else if (effective_mode == xcm::TuiMode::BRAILLE) {
        fit_w = cols * 2;
        fit_h = rows * 4;
    }

    int render_w = s.px_w;
    int render_h = s.px_h;
    // If configured resolution is far larger than terminal capacity, clamp.
    if (s.px_w > fit_w * 4 || s.px_h > fit_h * 4) {
        render_w = std::max(64, fit_w);
        render_h = std::max(32, fit_h);
    }

    xcm::Arena dom_arena;
    xcm::Arena layout_arena;

    xcm::Document* doc = xcm::parse_html(s.html.c_str(), s.html.size(), dom_arena);
    if (!doc) {
        std::cerr << "HTML parse failed\n";
        return false;
    }

    std::string merged_css = s.css;
    std::string embedded_css = extract_embedded_css(s.html);
    if (!embedded_css.empty()) {
        if (!merged_css.empty()) merged_css.push_back('\n');
        merged_css += embedded_css;
    }

    std::vector<xcm::StyleSheet> sheets;
    if (!merged_css.empty()) {
        sheets.push_back(xcm::parse_css(merged_css.c_str(), merged_css.size(), 2));
    }

    xcm::resolve_styles(doc, sheets, static_cast<float>(render_w), static_cast<float>(render_h));

    xcm::LayoutEngine eng(static_cast<float>(render_w), static_cast<float>(render_h), layout_arena);
    xcm::LayoutBox* root = eng.layout(doc);

    xcm::PaintEngine paint(render_w, render_h);
    paint.paint(root);

    xcm::TuiRenderer renderer;
    renderer.mode = effective_mode;
    renderer.color_mode = effective_color;
    renderer.edge_pass = effective_edge;
    renderer.shading_pass = effective_shade;
    renderer.edge_thresh = effective_thresh;

    xcm::TuiFramebuffer fb;
    fb.resize(cols, rows);
    renderer.render(paint.pixels(), render_w, render_h, fb);

    std::string out;
    renderer.serialize(fb, out, is_tty);
#ifndef _WIN32
    if (is_tty) {
        out.insert(0, "\x1b[2J\x1b[H");
        out += "\x1b[0m\x1b[?25h\n";
    } else {
        // Remove reset escapes and carriage returns for clean piped text.
        std::string cleaned;
        cleaned.reserve(out.size());
        for (std::size_t i = 0; i < out.size(); ++i) {
            if (out[i] == '\x1b' && i + 3 < out.size() && out[i+1] == '[' && out[i+2] == '0' && out[i+3] == 'm') {
                i += 3;
                continue;
            }
            if (out[i] == '\r') continue;
            cleaned.push_back(out[i]);
        }
        out.swap(cleaned);
        out += "\n";
    }
#else
    out += "\n";
#endif
    std::cout << out;
    return true;
}

void load_sample(ShellState& s, bool force_mono_theme = false) {
    s.html =
        "<!DOCTYPE html><html><head></head><body>"
        "<div class='hero'>"
        "<div class='title'>XCM TUI SHELL</div>"
        "<div class='subtitle'>modern terminal preview profile</div>"
        "</div>"
        "<div class='band b1'></div>"
        "<div class='band b2'></div>"
        "<div class='band b3'></div>"
        "<div class='grid'>"
        "<div class='card'><b class='k'>PROFILE</b><p class='v'>AUTO ADAPTIVE</p></div>"
        "<div class='card'><b class='k'>RENDER</b><p class='v'>SMOOTH SHADING</p></div>"
        "<div class='card'><b class='k'>FLOW</b><p class='v'>LOAD CSS RENDER</p></div>"
        "</div>"
        "<div class='foot'>TIP: mode half for color, mode mono for ascii</div>"
        "</div></body></html>";
    bool mono_like;
    if (s.theme_override == 0) mono_like = true;
    else if (s.theme_override == 1) mono_like = false;
    else mono_like = force_mono_theme ||
                     (s.mode == xcm::TuiMode::MONOCHROME || s.color == xcm::TuiColorMode::MONO);
    if (mono_like) {
        // Light mono theme avoids a full-screen dark fill of dense ASCII glyphs.
        s.css =
            "body{margin:0;padding:16px;background:#f0f4f8;color:#142033;font-family:monospace;}"
            ".hero{background:#dfe8f0;border:1px solid #8fa2b4;padding:14px;border-radius:6px;}"
            ".title{font-size:38px;color:#101820;margin:0;}"
            ".subtitle{color:#2c3d50;margin-top:6px;}"
            ".band{height:28px;border-radius:4px;margin-top:8px;}"
            ".b1{background:#1f3b57}.b2{background:#4f6d8a}.b3{background:#9db4ca}"
            ".grid{display:flex;gap:10px;margin-top:14px;}"
            ".card{background:#e7edf3;border:1px solid #8fa2b4;border-radius:6px;padding:10px;flex:1;}"
            ".k{color:#0f1720}.v{color:#36485c}"
            ".foot{margin-top:12px;color:#24364a;}";
    } else {
        s.css =
            "body{margin:0;padding:16px;background:#0b1220;color:#dff6ff;font-family:monospace;}"
            ".hero{background:#111a2b;border:1px solid #22344f;padding:14px;border-radius:6px;}"
            ".title{font-size:38px;color:#33e6d1;margin:0;}"
            ".subtitle{color:#8bb8d7;margin-top:6px;}"
            ".band{height:28px;border-radius:4px;margin-top:8px;}"
            ".b1{background:#15485f}.b2{background:#1f6d8a}.b3{background:#37a4c2}"
            ".grid{display:flex;gap:10px;margin-top:14px;}"
            ".card{background:#121d31;border:1px solid #243750;border-radius:6px;padding:10px;flex:1;}"
            ".k{color:#91d5ff}.v{color:#ffbe65}"
            ".foot{margin-top:12px;color:#78a5c4;}";
    }
    s.html_path.clear();
    s.css_path.clear();
}

} // namespace

int main() {
    ShellState s;
    apply_auto_profile(s);
    load_sample(s, !output_is_tty());

    std::cout << "xcm_tui_shell ready. type 'help' for commands.\n";

    std::string line;
    while (true) {
        std::cout << "xcm_tui_shell> ";
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty()) continue;

        if (cmd == "help") {
            print_help();
        } else if (cmd == "auto") {
            apply_auto_profile(s);
            std::cout << "auto profile selected: mode=" << mode_str(s.mode)
                      << " color=" << color_str(s.color)
                      << " edge=" << (s.edge ? "on" : "off")
                      << " shade=" << (s.shade ? "on" : "off")
                      << " thresh=" << s.edge_thresh << "\n";
        } else if (cmd == "sample") {
            bool plain_target = (!output_is_tty()) || (s.render_target == 0);
            load_sample(s, plain_target);
            std::cout << "sample loaded, rendering preview\n";
            if (!render_frame(s)) {
                std::cout << "sample render failed\n";
            }
        } else if (cmd == "theme") {
            std::string t;
            iss >> t;
            if (t == "auto") s.theme_override = -1;
            else if (t == "light") s.theme_override = 0;
            else if (t == "dark") s.theme_override = 1;
            else {
                std::cout << "theme must be auto|light|dark\n";
                continue;
            }
            std::cout << "theme set to " << (s.theme_override == -1 ? "auto" : (s.theme_override == 0 ? "light" : "dark")) << "\n";
        } else if (cmd == "load") {
            std::string path;
            iss >> path;
            if (path.empty()) {
                std::cout << "usage: load <file.html>\n";
                continue;
            }
            std::string text = read_file(path);
            if (text.empty()) {
                std::cout << "failed to read: " << path << "\n";
                continue;
            }
            s.html = std::move(text);
            s.html_path = path;
            std::cout << "loaded html: " << path << "\n";
        } else if (cmd == "css") {
            std::string path;
            iss >> path;
            if (path.empty()) {
                std::cout << "usage: css <file.css>\n";
                continue;
            }
            std::string text = read_file(path);
            if (text.empty()) {
                std::cout << "failed to read: " << path << "\n";
                continue;
            }
            s.css = std::move(text);
            s.css_path = path;
            std::cout << "loaded css: " << path << "\n";
        } else if (cmd == "clearcss") {
            s.css.clear();
            s.css_path.clear();
            std::cout << "css cleared\n";
        } else if (cmd == "mode") {
            std::string m;
            iss >> m;
            if (m == "half") s.mode = xcm::TuiMode::HALF_BLOCK;
            else if (m == "braille") s.mode = xcm::TuiMode::BRAILLE;
            else if (m == "mono") s.mode = xcm::TuiMode::MONOCHROME;
            else std::cout << "mode must be half|braille|mono\n";
        } else if (cmd == "color") {
            std::string c;
            iss >> c;
            if (c == "true") s.color = xcm::TuiColorMode::TRUECOLOR;
            else if (c == "256") s.color = xcm::TuiColorMode::COLOR256;
            else if (c == "mono") s.color = xcm::TuiColorMode::MONO;
            else std::cout << "color must be true|256|mono\n";
        } else if (cmd == "edge") {
            std::string onoff;
            iss >> onoff;
            if (onoff == "on") s.edge = true;
            else if (onoff == "off") s.edge = false;
            else std::cout << "edge must be on|off\n";
        } else if (cmd == "shade") {
            std::string onoff;
            iss >> onoff;
            if (onoff == "on") s.shade = true;
            else if (onoff == "off") s.shade = false;
            else std::cout << "shade must be on|off\n";
        } else if (cmd == "thresh") {
            int t = 0;
            iss >> t;
            if (t < 1 || t > 255) std::cout << "thresh must be 1..255\n";
            else s.edge_thresh = t;
        } else if (cmd == "px") {
            int w = 0, h = 0;
            iss >> w >> h;
            if (w < 32 || h < 32) std::cout << "px expects values >= 32\n";
            else { s.px_w = w; s.px_h = h; }
        } else if (cmd == "term") {
            int c = 0, r = 0;
            iss >> c >> r;
            s.term_cols = c;
            s.term_rows = r;
        } else if (cmd == "show") {
            print_state(s);
        } else if (cmd == "clear") {
            std::cout << "\x1b[2J\x1b[H";
        } else if (cmd == "render") {
            std::string mode;
            iss >> mode;
            if (mode == "tty") {
                s.render_target = 1;
                std::cout << "render target set to tty\n";
            } else if (mode == "plain") {
                s.render_target = 0;
                std::cout << "render target set to plain\n";
            } else if (mode.empty()) {
                render_frame(s);
            } else {
                std::cout << "render options: tty | plain\n";
            }
        } else if (cmd == "quit" || cmd == "exit") {
            break;
        } else {
            std::cout << "unknown command: " << cmd << "\n";
        }
    }

    std::cout << "bye\n";
    return 0;
}
