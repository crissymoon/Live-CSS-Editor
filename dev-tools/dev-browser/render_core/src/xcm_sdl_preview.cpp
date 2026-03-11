/*
 * xcm_sdl_preview.cpp  --  SDL2 desktop preview app for render_core
 *
 * Purpose:
 *   - Native window preview with precise frame control.
 *   - Direct HTML/CSS file loading.
 *   - Mouse wheel scroll and hot-reload-friendly loop.
 *
 * Usage:
 *   xcm_sdl_preview [html_file] [css_file] [--watch]
 *
 * Controls:
 *   Esc / q   quit
 *   r         force reload html/css from disk
 *   wheel     scroll
 */

#include "arena.h"
#include "css_parser.h"
#include "html_tokenizer.h"
#include "layout.h"
#include "paint.h"
#include "style_resolver.h"

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#ifdef XCM_HAS_SDL_TTF
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace {

struct FileStamp {
    bool valid = false;
    long long mtime = 0;
};

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string lower_copy(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string trim_copy(const std::string& s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::vector<std::string> split_words(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    bool in_quote = false;
    for (char c : s) {
        if (c == '"') {
            in_quote = !in_quote;
            continue;
        }
        if (!in_quote && std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
            continue;
        }
        cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string extract_embedded_css(const std::string& html) {
    std::string lower = lower_copy(html);
    std::string css;

    std::size_t pos = 0;
    while (true) {
        std::size_t open = lower.find("<style", pos);
        if (open == std::string::npos) break;

        std::size_t gt = lower.find('>', open);
        if (gt == std::string::npos) break;

        std::size_t close = lower.find("</style>", gt + 1);
        if (close == std::string::npos) break;

        css.append(html.substr(gt + 1, close - (gt + 1)));
        css.push_back('\n');
        pos = close + 8;
    }
    return css;
}

FileStamp get_stamp(const std::string& path) {
    FileStamp fs{};
    if (path.empty()) return fs;
#ifndef _WIN32
    struct stat st{};
    if (stat(path.c_str(), &st) == 0) {
        fs.valid = true;
        fs.mtime = static_cast<long long>(st.st_mtime);
    }
#endif
    return fs;
}

bool changed(const std::string& path, FileStamp& prev) {
    FileStamp now = get_stamp(path);
    if (!now.valid) return false;
    if (!prev.valid) {
        prev = now;
        return false;
    }
    if (now.mtime != prev.mtime) {
        prev = now;
        return true;
    }
    return false;
}

const char* SAMPLE_HTML =
    "<!DOCTYPE html><html><head><style>"
    "body{margin:0;padding:18px;background:#0b1220;color:#dff6ff;font-family:monospace;}"
    ".hero{background:#111a2b;border:2px solid #22344f;padding:14px;}"
    "h1{color:#33e6d1;margin:0;font-size:42px;}"
    "p{color:#8bb8d7;font-size:18px;}"
    ".row{display:flex;gap:12px;margin-top:14px;}"
    ".card{flex:1;background:#121d31;border:2px solid #243750;padding:10px;}"
    ".k{color:#91d5ff}.v{color:#ffbe65}"
    "</style></head><body>"
    "<div class='hero'><h1>XCM SDL PREVIEW</h1><p>Desktop viewport for HTML/CSS iteration</p></div>"
    "<div class='row'>"
    "<div class='card'><div class='k'>SOURCE</div><div class='v'>HTML + CSS</div></div>"
    "<div class='card'><div class='k'>INPUT</div><div class='v'>WHEEL + RELOAD</div></div>"
    "<div class='card'><div class='k'>TARGET</div><div class='v'>FUTURE APP SHELL</div></div>"
    "</div></body></html>";

bool render_page(int w, int h,
                 const std::string& html,
                 const std::string& css,
                 float scroll_y,
                 std::vector<uint8_t>& rgba,
                 int& doc_height) {
    xcm::Arena dom_arena;
    xcm::Arena layout_arena;

    xcm::Document* doc = xcm::parse_html(html.c_str(), html.size(), dom_arena);
    if (!doc) return false;

    std::string merged_css = css;
    std::string embedded = extract_embedded_css(html);
    if (!embedded.empty()) {
        if (!merged_css.empty()) merged_css.push_back('\n');
        merged_css += embedded;
    }

    std::vector<xcm::StyleSheet> sheets;
    if (!merged_css.empty()) {
        sheets.push_back(xcm::parse_css(merged_css.c_str(), merged_css.size(), 2));
    }

    xcm::resolve_styles(doc, sheets, static_cast<float>(w), static_cast<float>(h));

    xcm::LayoutEngine eng(static_cast<float>(w), static_cast<float>(h), layout_arena);
    xcm::LayoutBox* root = eng.layout(doc);

    xcm::PaintEngine paint(w, h);
    paint.set_scroll(scroll_y);
    paint.paint(root);

    doc_height = root ? static_cast<int>(root->height) : h;

    rgba.assign(paint.pixels(), paint.pixels() + static_cast<std::size_t>(w * h * 4));
    return true;
}

} // namespace

int main(int argc, char** argv) {
    std::string html_file;
    std::string css_file;
    bool watch = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--watch") watch = true;
        else if (html_file.empty()) html_file = a;
        else if (css_file.empty()) css_file = a;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

#ifdef XCM_HAS_SDL_TTF
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "TTF_Init warning: %s\n", TTF_GetError());
    }
#endif

    int win_w = 1280;
    int win_h = 800;

    SDL_Window* win = SDL_CreateWindow(
        "xcm_sdl_preview",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_RESIZABLE
    );
    if (!win) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // bilinear -- avoids jagged edges when zoom != 1
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    static constexpr int PANEL_W = 220; // right-side control panel width in screen pixels

    float scroll_y = 0.f;
    int doc_h = win_h;
    bool dirty = true;
    bool running = true;
    float zoom = 0.85f; // slightly zoomed out while keeping text readable
    int content_w = std::max(64, win_w - PANEL_W); // page render area does not include the panel
    int tex_w = content_w;
    int tex_h = win_h;

    SDL_Texture* tex = SDL_CreateTexture(
        ren,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        tex_w,
        tex_h
    );
    if (!tex) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    std::string html = html_file.empty() ? std::string(SAMPLE_HTML) : read_file(html_file);
    std::string css = css_file.empty() ? std::string() : read_file(css_file);

    if (html.empty()) {
        std::fprintf(stderr, "Failed to load HTML input.\n");
        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    FileStamp html_stamp = get_stamp(html_file);
    FileStamp css_stamp = get_stamp(css_file);

    std::vector<uint8_t> frame;

    bool css_enabled = true;
    bool palette_mode = false;
    std::string palette_prompt;   // "html", "css", or "" for generic command
    std::string palette_input;
    std::string status = "Ready";
    bool dragging_scroll = false;
    int drag_scroll_offset = 0;

#ifdef XCM_HAS_SDL_TTF
    TTF_Font* ui_font = nullptr;
    const char* font_candidates[] = {
        "/System/Library/Fonts/Supplemental/Menlo Regular.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
    };
    for (const char* fp : font_candidates) {
        ui_font = TTF_OpenFont(fp, 13);
        if (ui_font) break;
    }
#endif

    auto set_title = [&]() {
        std::string title = "xcm_sdl_preview";
        title += " | ";
        title += status;
        title += " | zoom=";
        title += std::to_string(static_cast<int>(std::round(zoom * 100.f)));
        title += "%";
        if (palette_mode) {
            title += " | :";
            title += palette_input;
        }
        if (palette_mode) {
            if (palette_prompt == "html") title += " | Open HTML: ";
            else if (palette_prompt == "css") title += " | Open CSS: ";
            else title += " | :";
            title += palette_input;
            title += "_";
        }
        SDL_SetWindowTitle(win, title.c_str());
    };

    struct UiButton {
        SDL_Rect rect{};
        const char* label = "";
    };

    auto build_buttons = [&]() {
        std::vector<UiButton> buttons;
        int px = content_w;
        int bx = px + 10;
        int by = 44;
        int bw = PANEL_W - 20;
        int bh = 28;
        auto add = [&](const char* label) {
            buttons.push_back({SDL_Rect{bx, by, bw, bh}, label});
            by += bh + 6;
        };
        // half-width pair -- two buttons side by side
        auto add_pair = [&](const char* a, const char* b) {
            int hw = (bw - 4) / 2;
            buttons.push_back({SDL_Rect{bx, by, hw, bh}, a});
            buttons.push_back({SDL_Rect{bx + hw + 4, by, hw, bh}, b});
            by += bh + 6;
        };
        add("Palette (:)");    // 0
        add("Open HTML...");   // 1
        add("Open CSS...");    // 2
        add("Reload (R)");     // 3
        add("Watch (W)");      // 4
        add("CSS Toggle (C)"); // 5
        add_pair("50%", "75%");     // 6, 7
        add_pair("100%", "125%");   // 8, 9
        add("Sample");         // 10
        return buttons;
    };

    auto point_in = [](int x, int y, const SDL_Rect& r) {
        return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
    };

#ifdef XCM_HAS_SDL_TTF
    auto draw_text = [&](int x, int y, const std::string& text, SDL_Color color) {
        if (!ui_font || text.empty()) return;
        SDL_Surface* surf = TTF_RenderUTF8_Blended(ui_font, text.c_str(), color);
        if (!surf) return;
        SDL_Texture* t = SDL_CreateTextureFromSurface(ren, surf);
        if (t) {
            SDL_Rect dst{x, y, surf->w, surf->h};
            SDL_RenderCopy(ren, t, nullptr, &dst);
            SDL_DestroyTexture(t);
        }
        SDL_FreeSurface(surf);
    };
#endif

    auto do_reload = [&]() {
        if (!html_file.empty()) {
            std::string re = read_file(html_file);
            if (!re.empty()) html = std::move(re);
        }
        if (!css_file.empty()) {
            css = read_file(css_file);
        }
        dirty = true;
    };

    auto load_html_file = [&](const std::string& p) -> bool {
        std::string re = read_file(p);
        if (re.empty()) return false;
        html = std::move(re);
        html_file = p;
        html_stamp = get_stamp(html_file);
        dirty = true;
        return true;
    };

    auto load_css_file = [&](const std::string& p) -> bool {
        std::string re = read_file(p);
        if (re.empty()) return false;
        css = std::move(re);
        css_file = p;
        css_stamp = get_stamp(css_file);
        dirty = true;
        return true;
    };

    auto run_command = [&](const std::string& raw) {
        std::string cmdline = trim_copy(raw);
        auto parts = split_words(cmdline);
        if (parts.empty()) {
            status = "Palette canceled";
            return;
        }
        std::string cmd = lower_copy(parts[0]);

        if (cmd == "help") {
            status = "Commands: open/html/css/reload/watch/cssmode/zoom/quit";
            return;
        }
        if (cmd == "reload") {
            do_reload();
            status = "Reloaded";
            return;
        }
        if (cmd == "watch") {
            std::string arg = (parts.size() > 1) ? lower_copy(parts[1]) : "toggle";
            if (arg == "on") watch = true;
            else if (arg == "off") watch = false;
            else watch = !watch;
            status = std::string("Watch ") + (watch ? "on" : "off");
            return;
        }
        if (cmd == "cssmode") {
            std::string arg = (parts.size() > 1) ? lower_copy(parts[1]) : "toggle";
            if (arg == "on") css_enabled = true;
            else if (arg == "off") css_enabled = false;
            else css_enabled = !css_enabled;
            dirty = true;
            status = std::string("CSS ") + (css_enabled ? "on" : "off");
            return;
        }
        if (cmd == "zoom") {
            if (parts.size() < 2) {
                status = "Usage: zoom <percent|in|out|reset>";
                return;
            }
            std::string arg = lower_copy(parts[1]);
            if (arg == "in") zoom = std::min(3.0f, zoom * 1.15f);
            else if (arg == "out") zoom = std::max(0.25f, zoom / 1.15f);
            else if (arg == "reset") zoom = 1.0f;
            else {
                int pct = std::atoi(arg.c_str());
                if (pct < 25 || pct > 300) {
                    status = "Zoom range: 25..300";
                    return;
                }
                zoom = static_cast<float>(pct) / 100.f;
            }
            dirty = true;
            status = "Zoom changed";
            return;
        }
        if (cmd == "open" || cmd == "html") {
            if (parts.size() < 2) {
                status = "Usage: open <html> [css]";
                return;
            }
            if (!load_html_file(parts[1])) {
                status = "Failed to load html";
                return;
            }
            if (parts.size() > 2) {
                if (!load_css_file(parts[2])) status = "HTML loaded, CSS failed";
                else status = "HTML/CSS loaded";
            } else {
                status = "HTML loaded";
            }
            return;
        }
        if (cmd == "css") {
            if (parts.size() < 2) {
                status = "Usage: css <file>";
                return;
            }
            if (!load_css_file(parts[1])) {
                status = "Failed to load css";
                return;
            }
            status = "CSS loaded";
            return;
        }
        if (cmd == "quit" || cmd == "exit") {
            running = false;
            return;
        }

        status = "Unknown command";
    };

    set_title();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                win_w = std::max(64 + PANEL_W, ev.window.data1);
                win_h = std::max(64, ev.window.data2);
                content_w = win_w - PANEL_W;
                dirty = true;
                status = "Resized";
                set_title();
            } else if (ev.type == SDL_MOUSEWHEEL) {
                scroll_y -= static_cast<float>(ev.wheel.y * 48);
                scroll_y = std::max(0.f, scroll_y);
                dirty = true;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x;
                int my = ev.button.y;

                // Scrollbar track is at the far right edge of the window
                float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
                SDL_Rect track{win_w - 12, 8, 6, std::max(24, win_h - 16)};
                int thumb_h = (doc_h <= 0) ? track.h : std::max(24, static_cast<int>(track.h * (static_cast<float>(tex_h) / static_cast<float>(doc_h))));
                if (thumb_h > track.h) thumb_h = track.h;
                int thumb_y = track.y;
                if (max_scroll > 0.0f) {
                    thumb_y = track.y + static_cast<int>((scroll_y / max_scroll) * static_cast<float>(track.h - thumb_h));
                }
                SDL_Rect thumb{track.x, thumb_y, track.w, thumb_h};

                if (point_in(mx, my, thumb)) {
                    dragging_scroll = true;
                    drag_scroll_offset = my - thumb.y;
                    continue;
                }
                if (point_in(mx, my, track)) {
                    if (max_scroll > 0.0f) {
                        float t = static_cast<float>(my - track.y - thumb_h / 2) / static_cast<float>(std::max(1, track.h - thumb_h));
                        t = std::clamp(t, 0.0f, 1.0f);
                        scroll_y = t * max_scroll;
                        dirty = true;
                    }
                    continue;
                }

                if (mx >= content_w && mx < win_w) {
                    auto buttons = build_buttons();
                    for (std::size_t i = 0; i < buttons.size(); ++i) {
                        if (!point_in(mx, my, buttons[i].rect)) continue;

                        switch (i) {
                            case 0: // palette
                                palette_mode = true;
                                palette_prompt.clear();
                                palette_input.clear();
                                SDL_StartTextInput();
                                status = "Palette (type command)";
                                set_title();
                                break;
                            case 1: // open html
                                palette_mode = true;
                                palette_prompt = "html";
                                palette_input.clear();
                                SDL_StartTextInput();
                                status = "Type HTML path + Enter";
                                set_title();
                                break;
                            case 2: // open css
                                palette_mode = true;
                                palette_prompt = "css";
                                palette_input.clear();
                                SDL_StartTextInput();
                                status = "Type CSS path + Enter";
                                set_title();
                                break;
                            case 3: // reload
                                do_reload();
                                status = "Reloaded";
                                set_title();
                                break;
                            case 4: // watch
                                watch = !watch;
                                status = std::string("Watch ") + (watch ? "on" : "off");
                                set_title();
                                break;
                            case 5: // css toggle
                                css_enabled = !css_enabled;
                                dirty = true;
                                status = std::string("CSS ") + (css_enabled ? "on" : "off");
                                set_title();
                                break;
                            case 6: zoom = 0.5f;  dirty = true; status = "Zoom 50%";  set_title(); break;
                            case 7: zoom = 0.75f; dirty = true; status = "Zoom 75%";  set_title(); break;
                            case 8: zoom = 1.0f;  dirty = true; status = "Zoom 100%"; set_title(); break;
                            case 9: zoom = 1.25f; dirty = true; status = "Zoom 125%"; set_title(); break;
                            case 10: // sample
                                html = SAMPLE_HTML;
                                css.clear();
                                html_file.clear();
                                css_file.clear();
                                scroll_y = 0.f;
                                dirty = true;
                                status = "Sample loaded";
                                set_title();
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                dragging_scroll = false;
            } else if (ev.type == SDL_MOUSEMOTION) {
                if (dragging_scroll) {
                    float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
                    if (max_scroll > 0.0f) {
                        SDL_Rect track{win_w - 12, 8, 6, std::max(24, win_h - 16)};
                        int thumb_h = (doc_h <= 0) ? track.h : std::max(24, static_cast<int>(track.h * (static_cast<float>(tex_h) / static_cast<float>(doc_h))));
                        if (thumb_h > track.h) thumb_h = track.h;
                        int thumb_top = ev.motion.y - drag_scroll_offset;
                        float t = static_cast<float>(thumb_top - track.y) / static_cast<float>(std::max(1, track.h - thumb_h));
                        t = std::clamp(t, 0.0f, 1.0f);
                        scroll_y = t * max_scroll;
                        dirty = true;
                    }
                }
            } else if (ev.type == SDL_TEXTINPUT && palette_mode) {
                palette_input += ev.text.text;
                set_title();
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;

                if (palette_mode) {
                    if (k == SDLK_ESCAPE) {
                        palette_mode = false;
                        palette_prompt.clear();
                        palette_input.clear();
                        SDL_StopTextInput();
                        status = "Canceled";
                        set_title();
                    } else if (k == SDLK_BACKSPACE) {
                        if (!palette_input.empty()) palette_input.pop_back();
                        set_title();
                    } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
                        std::string cmd = palette_input;
                        std::string prompt = palette_prompt;
                        palette_mode = false;
                        palette_prompt.clear();
                        palette_input.clear();
                        SDL_StopTextInput();
                        if (prompt == "html") {
                            if (cmd.empty()) { status = "Canceled"; }
                            else if (!load_html_file(cmd)) { status = "Failed: " + cmd; }
                            else { scroll_y = 0.f; status = "HTML loaded: " + cmd; }
                        } else if (prompt == "css") {
                            if (cmd.empty()) { status = "Canceled"; }
                            else if (!load_css_file(cmd)) { status = "Failed: " + cmd; }
                            else { status = "CSS loaded: " + cmd; }
                        } else {
                            run_command(cmd);
                        }
                        set_title();
                    }
                    continue;
                }

                if (k == SDLK_COLON || k == SDLK_SLASH) {
                    palette_mode = true;
                    palette_prompt.clear();
                    palette_input.clear();
                    SDL_StartTextInput();
                    status = "Palette (type command)";
                    set_title();
                    continue;
                }

                if (k == SDLK_ESCAPE || k == SDLK_q) running = false;
                else if (k == SDLK_r) {
                    do_reload();
                    status = "Reloaded";
                    set_title();
                } else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
                    zoom = std::max(0.25f, zoom / 1.15f);
                    dirty = true;
                    status = "Zoom out";
                    set_title();
                } else if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) {
                    zoom = std::min(3.0f, zoom * 1.15f);
                    dirty = true;
                    status = "Zoom in";
                    set_title();
                } else if (k == SDLK_0) {
                    zoom = 1.0f;
                    dirty = true;
                    status = "Zoom reset";
                    set_title();
                } else if (k == SDLK_c) {
                    css_enabled = !css_enabled;
                    dirty = true;
                    status = std::string("CSS ") + (css_enabled ? "on" : "off");
                    set_title();
                } else if (k == SDLK_w) {
                    watch = !watch;
                    status = std::string("Watch ") + (watch ? "on" : "off");
                    set_title();
                } else if (k == SDLK_DOWN) {
                    scroll_y += 48.f;
                    dirty = true;
                } else if (k == SDLK_UP) {
                    scroll_y = std::max(0.f, scroll_y - 48.f);
                    dirty = true;
                }
            }
        }

        if (watch) {
            bool changed_any = false;
            if (!html_file.empty() && changed(html_file, html_stamp)) {
                std::string re = read_file(html_file);
                if (!re.empty()) { html = std::move(re); changed_any = true; }
            }
            if (!css_file.empty() && changed(css_file, css_stamp)) {
                std::string re = read_file(css_file);
                css = std::move(re);
                changed_any = true;
            }
            if (changed_any) dirty = true;
        }

        if (dirty) {
            // Render page into the content area only (window minus panel)
            int render_w = std::max(64, static_cast<int>(std::round(static_cast<float>(content_w) / zoom)));
            int render_h = std::max(64, static_cast<int>(std::round(static_cast<float>(win_h) / zoom)));
            if (render_w != tex_w || render_h != tex_h) {
                tex_w = render_w;
                tex_h = render_h;
                SDL_DestroyTexture(tex);
                tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, tex_w, tex_h);
                if (!tex) {
                    std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
                    running = false;
                    break;
                }
            }

            const std::string css_for_render = css_enabled ? css : std::string();
            if (render_page(tex_w, tex_h, html, css_for_render, scroll_y, frame, doc_h)) {
                float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
                if (scroll_y > max_scroll) {
                    scroll_y = max_scroll;
                    render_page(tex_w, tex_h, html, css_for_render, scroll_y, frame, doc_h);
                }

                SDL_UpdateTexture(tex, nullptr, frame.data(), tex_w * 4);
            }
            dirty = false;
        }

        // Draw page content into the left portion of the window only
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_Rect content_dst{0, 0, content_w, win_h};
        SDL_RenderCopy(ren, tex, nullptr, &content_dst);

        // Right command panel + visible scrollbar so mouse operations are discoverable.
        int panel_w = PANEL_W;
        int panel_x = content_w;

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_Rect panel{panel_x, 0, panel_w, win_h};
        SDL_SetRenderDrawColor(ren, 10, 16, 26, 210);
        SDL_RenderFillRect(ren, &panel);

        SDL_SetRenderDrawColor(ren, 34, 56, 84, 255);
        SDL_RenderDrawRect(ren, &panel);

#ifdef XCM_HAS_SDL_TTF
        draw_text(panel_x + 10, 10, "XCM Controls", SDL_Color{222, 238, 255, 255});
#else
        SDL_Rect title_bar{panel_x + 10, 10, panel_w - 20, 20};
        SDL_SetRenderDrawColor(ren, 30, 44, 66, 255);
        SDL_RenderFillRect(ren, &title_bar);
#endif

        auto buttons = build_buttons();
        for (const auto& b : buttons) {
            SDL_SetRenderDrawColor(ren, 22, 34, 52, 230);
            SDL_RenderFillRect(ren, &b.rect);
            SDL_SetRenderDrawColor(ren, 60, 96, 136, 255);
            SDL_RenderDrawRect(ren, &b.rect);
#ifdef XCM_HAS_SDL_TTF
            draw_text(b.rect.x + 8, b.rect.y + 7, b.label, SDL_Color{212, 232, 255, 255});
#endif
        }

        // Input bar (always visible when palette_mode, text when font available)
        if (palette_mode) {
            SDL_Rect input_bg{panel_x + 6, win_h - 92, panel_w - 12, 24};
            SDL_SetRenderDrawColor(ren, 14, 22, 36, 245);
            SDL_RenderFillRect(ren, &input_bg);
            SDL_SetRenderDrawColor(ren, 100, 160, 230, 255);
            SDL_RenderDrawRect(ren, &input_bg);
#ifdef XCM_HAS_SDL_TTF
            std::string pinp = (palette_prompt == "html") ? "HTML: " :
                               (palette_prompt == "css")  ? "CSS:  " : ": ";
            draw_text(input_bg.x + 4, input_bg.y + 4,
                      pinp + palette_input + "_",
                      SDL_Color{255, 240, 140, 255});
#endif
        }

#ifdef XCM_HAS_SDL_TTF
        std::string info1 = std::string("Zoom: ") + std::to_string(static_cast<int>(std::round(zoom * 100.f))) + "%";
        std::string info2 = std::string("Watch: ") + (watch ? "on" : "off") + "  CSS: " + (css_enabled ? "on" : "off");
        draw_text(panel_x + 10, win_h - 64, info1, SDL_Color{181, 210, 238, 255});
        draw_text(panel_x + 10, win_h - 44, info2, SDL_Color{181, 210, 238, 255});
        draw_text(panel_x + 10, win_h - 24, status, SDL_Color{255, 215, 152, 255});
#endif

        SDL_Rect track{win_w - 12, 8, 6, std::max(24, win_h - 16)};
        SDL_SetRenderDrawColor(ren, 210, 220, 236, 185);
        SDL_RenderFillRect(ren, &track);

        float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
        int thumb_h = (doc_h <= 0) ? track.h : std::max(24, static_cast<int>(track.h * (static_cast<float>(tex_h) / static_cast<float>(doc_h))));
        if (thumb_h > track.h) thumb_h = track.h;
        int thumb_y = track.y;
        if (max_scroll > 0.0f) {
            thumb_y = track.y + static_cast<int>((scroll_y / max_scroll) * static_cast<float>(track.h - thumb_h));
        }
        SDL_Rect thumb{track.x, thumb_y, track.w, thumb_h};
        SDL_SetRenderDrawColor(ren, dragging_scroll ? 38 : 62, dragging_scroll ? 132 : 104, 186, 255);
        SDL_RenderFillRect(ren, &thumb);

        SDL_RenderPresent(ren);
    }

#ifdef XCM_HAS_SDL_TTF
    if (ui_font) TTF_CloseFont(ui_font);
    if (TTF_WasInit()) TTF_Quit();
#endif
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
