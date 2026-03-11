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
#include "xcm_http_fetch.h"
#include "xcm_input_capture.h"

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

#ifdef XCM_HAS_SDL_IMAGE
#if __has_include(<SDL2/SDL_image.h>)
#include <SDL2/SDL_image.h>
#else
#include <SDL_image.h>
#endif
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace {

struct FileStamp {
    bool valid = false;
    long long mtime = 0;
};

struct ImageOverlay {
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
    bool is_background = false;
    std::string url;
};

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool is_http_url(const std::string& s) {
    return s.rfind("http://", 0) == 0;
}

std::string load_text_source(const std::string& src) {
    if (src.empty()) return "";
    if (is_http_url(src)) {
        xcm::HttpResponse r = xcm::http_fetch_get(src, 5000);
        if (!r.ok) return "";
        return r.body;
    }
    return read_file(src);
}

bool file_exists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return static_cast<bool>(f);
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

void replace_all_inplace(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

bool write_text_file(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << content;
    return static_cast<bool>(f);
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

std::string extract_attr_value(const std::string& tag_src,
                               const std::string& attr_name) {
    std::string lower = lower_copy(tag_src);
    std::string needle = attr_name + "=";
    std::size_t p = lower.find(needle);
    if (p == std::string::npos) return "";
    p += needle.size();
    if (p >= tag_src.size()) return "";

    char q = tag_src[p];
    if (q == '"' || q == '\'') {
        ++p;
        std::size_t e = tag_src.find(q, p);
        if (e == std::string::npos) return "";
        return tag_src.substr(p, e - p);
    }

    std::size_t e = p;
    while (e < tag_src.size() && !std::isspace(static_cast<unsigned char>(tag_src[e])) && tag_src[e] != '>') ++e;
    return tag_src.substr(p, e - p);
}

std::vector<std::string> extract_linked_stylesheet_hrefs(const std::string& html) {
    std::vector<std::string> out;
    std::string lower = lower_copy(html);
    std::size_t pos = 0;
    while (true) {
        std::size_t lt = lower.find("<link", pos);
        if (lt == std::string::npos) break;
        std::size_t gt = lower.find('>', lt + 5);
        if (gt == std::string::npos) break;
        std::string tag = html.substr(lt, gt - lt + 1);
        std::string ltag = lower.substr(lt, gt - lt + 1);
        if (ltag.find("rel=\"stylesheet\"") != std::string::npos ||
            ltag.find("rel='stylesheet'") != std::string::npos ||
            ltag.find("rel=stylesheet") != std::string::npos) {
            std::string href = extract_attr_value(tag, "href");
            if (!href.empty()) out.push_back(href);
        }
        pos = gt + 1;
    }
    return out;
}

std::vector<std::string> extract_script_srcs(const std::string& html) {
    std::vector<std::string> out;
    std::string lower = lower_copy(html);
    std::size_t pos = 0;
    while (true) {
        std::size_t lt = lower.find("<script", pos);
        if (lt == std::string::npos) break;
        std::size_t gt = lower.find('>', lt + 7);
        if (gt == std::string::npos) break;
        std::string tag = html.substr(lt, gt - lt + 1);
        std::string src = extract_attr_value(tag, "src");
        if (!src.empty()) out.push_back(src);
        pos = gt + 1;
    }
    return out;
}

std::string strip_img_tags(const std::string& html) {
    std::string out;
    out.reserve(html.size());

    std::size_t i = 0;
    while (i < html.size()) {
        std::size_t lt = html.find('<', i);
        if (lt == std::string::npos) {
            out.append(html.substr(i));
            break;
        }
        out.append(html.substr(i, lt - i));

        std::size_t gt = html.find('>', lt + 1);
        if (gt == std::string::npos) {
            out.append(html.substr(lt));
            break;
        }

        std::string tag = lower_copy(html.substr(lt, gt - lt + 1));
        if (tag.rfind("<img", 0) == 0 || tag.rfind("<img ", 0) == 0) {
            i = gt + 1;
            continue;
        }

        out.append(html.substr(lt, gt - lt + 1));
        i = gt + 1;
    }

    return out;
}

std::string dirname_copy(const std::string& path) {
    std::size_t p = path.find_last_of('/');
    if (p == std::string::npos) return ".";
    return path.substr(0, p);
}

std::string resolve_asset_path(const std::string& url, const std::string& html_path) {
    if (url.empty()) return "";
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 || url.rfind("data:", 0) == 0) {
        return "";
    }

    const std::string workspace_root = "/Users/mac/Documents/live-css";
    if (url[0] == '/') {
        std::string candidate = workspace_root + url;
        if (file_exists(candidate)) return candidate;

        // Bridge-served absolute paths often omit the /my_project prefix.
        if (url.rfind("/vscode-bridge/", 0) == 0) {
            std::string bridged = workspace_root + "/my_project" + url;
            if (file_exists(bridged)) return bridged;
        }
        return candidate;
    }

    std::string base = dirname_copy(html_path);
    if (base.empty()) base = ".";
    return base + "/" + url;
}

std::string rewrite_bridge_urls_for_file_preview(std::string s) {
    const std::string bridge_root = "file:///Users/mac/Documents/live-css/my_project/vscode-bridge/";
    const std::string project_root = "file:///Users/mac/Documents/live-css/my_project/";
    replace_all_inplace(s, "/vscode-bridge/", bridge_root);
    replace_all_inplace(s, "/my_project/vscode-bridge/", bridge_root);
    replace_all_inplace(s, "/my_project/", project_root);
    return s;
}

std::string path_to_file_url(std::string path) {
    replace_all_inplace(path, " ", "%20");
    return "file://" + path;
}

std::string rewrite_css_urls_for_file_preview(const std::string& css, const std::string& css_path) {
    if (css.empty()) return css;
    std::string out;
    out.reserve(css.size() + 64);

    std::size_t pos = 0;
    while (pos < css.size()) {
        std::size_t up = css.find("url(", pos);
        if (up == std::string::npos) {
            out.append(css.substr(pos));
            break;
        }
        out.append(css.substr(pos, up - pos));
        std::size_t lp = up + 4;
        std::size_t rp = css.find(')', lp);
        if (rp == std::string::npos) {
            out.append(css.substr(up));
            break;
        }
        std::string raw = trim_copy(css.substr(lp, rp - lp));
        if (!raw.empty() && (raw.front() == '\'' || raw.front() == '"')) raw.erase(raw.begin());
        if (!raw.empty() && (raw.back() == '\'' || raw.back() == '"')) raw.pop_back();

        std::string resolved = resolve_asset_path(raw, css_path.empty() ? "" : css_path);
        if (!resolved.empty()) out += "url('" + path_to_file_url(resolved) + "')";
        else out += css.substr(up, rp - up + 1);
        pos = rp + 1;
    }
    return rewrite_bridge_urls_for_file_preview(out);
}

#ifdef XCM_HAS_SDL_IMAGE
void blend_surface_into_rgba(std::vector<uint8_t>& rgba, int dst_w, int dst_h,
                             const SDL_Surface* surf, const ImageOverlay& ov,
                             float scroll_y) {
    if (!surf || rgba.empty() || dst_w <= 0 || dst_h <= 0) return;

    int x0 = static_cast<int>(std::floor(ov.x));
    int y0 = static_cast<int>(std::floor(ov.y - scroll_y));
    int x1 = static_cast<int>(std::ceil(ov.x + ov.w));
    int y1 = static_cast<int>(std::ceil(ov.y - scroll_y + ov.h));

    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    x1 = std::min(dst_w, x1);
    y1 = std::min(dst_h, y1);
    if (x0 >= x1 || y0 >= y1) return;

    const uint8_t* src_pixels = static_cast<const uint8_t*>(surf->pixels);
    const int src_w = surf->w;
    const int src_h = surf->h;
    const int src_pitch = surf->pitch;

    for (int dy = y0; dy < y1; ++dy) {
        for (int dx = x0; dx < x1; ++dx) {
            float u = (ov.w > 0.f) ? (static_cast<float>(dx) - ov.x) / ov.w : 0.f;
            float v = (ov.h > 0.f) ? (static_cast<float>(dy) - (ov.y - scroll_y)) / ov.h : 0.f;
            int sx = std::clamp(static_cast<int>(u * static_cast<float>(src_w)), 0, src_w - 1);
            int sy = std::clamp(static_cast<int>(v * static_cast<float>(src_h)), 0, src_h - 1);

            const uint8_t* sp = src_pixels + sy * src_pitch + sx * 4;
            uint8_t* dp = rgba.data() + (dy * dst_w + dx) * 4;

            float a = sp[3] / 255.f;
            float oma = 1.f - a;
            dp[0] = static_cast<uint8_t>(sp[0] * a + dp[0] * oma);
            dp[1] = static_cast<uint8_t>(sp[1] * a + dp[1] * oma);
            dp[2] = static_cast<uint8_t>(sp[2] * a + dp[2] * oma);
            dp[3] = 255;
        }
    }
}
#endif

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

void collect_image_overlays(xcm::LayoutBox* box, std::vector<ImageOverlay>& out) {
    if (!box || !box->node) return;

    const xcm::ComputedStyle* cs = box->node->computed_style
        ? static_cast<const xcm::ComputedStyle*>(box->node->computed_style)
        : nullptr;

    if (box->node->kind == xcm::NodeKind::ELEMENT) {
        if (box->node->tag && std::string_view(box->node->tag) == "img") {
            const char* src = box->node->attr("src");
            if (src && *src && box->width > 1 && box->height > 1) {
                out.push_back(ImageOverlay{box->x, box->y, box->width, box->height, false, src});
            }
        }
        if (cs && cs->background_image && *cs->background_image && box->width > 1 && box->height > 1) {
            out.push_back(ImageOverlay{box->x, box->y, box->width, box->height, true, cs->background_image});
        }
    }

    for (auto* ch : box->children) collect_image_overlays(ch, out);
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
                 int& doc_height,
                 std::vector<ImageOverlay>* overlays) {
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

    if (overlays) {
        overlays->clear();
        collect_image_overlays(root, *overlays);
    }

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
    std::string js_file;
    bool watch = false;
    bool allow_external_browser_open = false;
    bool using_default_bridge_files = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--watch") watch = true;
        else if (a == "--external-browser") allow_external_browser_open = true;
        else if (html_file.empty()) html_file = a;
        else if (css_file.empty()) css_file = a;
        else if (js_file.empty()) js_file = a;
    }

    // If no file args are provided, point SDL preview at the VSCode bridge editor files.
    // This makes xcm_sdl_preview immediately show the same content as html-editor/css-editor.
    if (html_file.empty()) {
        const char* env_html = std::getenv("XCM_DEFAULT_HTML");
        const char* env_css = std::getenv("XCM_DEFAULT_CSS");
        const char* env_js = std::getenv("XCM_DEFAULT_JS");
        const std::string fallback_html = "/Users/mac/Documents/live-css/my_project/vscode-bridge/projects/html-editor.html";
        const std::string fallback_css  = "/Users/mac/Documents/live-css/my_project/vscode-bridge/projects/css-editor.css";
        const std::string fallback_js   = "/Users/mac/Documents/live-css/my_project/vscode-bridge/projects/js-editor.js";

        html_file = (env_html && *env_html) ? std::string(env_html) : fallback_html;
        css_file  = (env_css && *env_css)   ? std::string(env_css)  : fallback_css;
        js_file   = (env_js && *env_js)     ? std::string(env_js)   : fallback_js;

        if (file_exists(html_file)) {
            using_default_bridge_files = true;
            // Enable auto-reload behavior by default for the bridge editor workflow.
            watch = true;
        } else {
            html_file.clear();
            css_file.clear();
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

#ifdef XCM_HAS_SDL_IMAGE
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP;
    int img_init = IMG_Init(img_flags);
    bool sdl_image_ready = (img_init & img_flags) != 0;
    if (!sdl_image_ready) {
        std::fprintf(stderr, "SDL_image warning: %s\n", IMG_GetError());
    }
#endif

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
    const Uint32 main_window_id = SDL_GetWindowID(win);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest -- keeps text edges consistent
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

    SDL_Window* preview_win = nullptr;
    SDL_Renderer* preview_ren = nullptr;
    SDL_Texture* preview_tex = nullptr;
    int preview_win_w = 1000;
    int preview_win_h = 700;
    int preview_tex_w = tex_w;
    int preview_tex_h = tex_h;

    auto destroy_preview_window = [&]() {
        if (preview_tex) {
            SDL_DestroyTexture(preview_tex);
            preview_tex = nullptr;
        }
        if (preview_ren) {
            SDL_DestroyRenderer(preview_ren);
            preview_ren = nullptr;
        }
        if (preview_win) {
            SDL_DestroyWindow(preview_win);
            preview_win = nullptr;
        }
    };

    auto ensure_preview_window = [&]() -> bool {
        if (preview_win && preview_ren) return true;
        preview_win = SDL_CreateWindow(
            "xcm_sdl_preview_render",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            preview_win_w,
            preview_win_h,
            SDL_WINDOW_RESIZABLE
        );
        if (!preview_win) return false;

        preview_ren = SDL_CreateRenderer(preview_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!preview_ren) {
            destroy_preview_window();
            return false;
        }

        preview_tex_w = tex_w;
        preview_tex_h = tex_h;
        preview_tex = SDL_CreateTexture(
            preview_ren,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            std::max(64, preview_tex_w),
            std::max(64, preview_tex_h)
        );
        if (!preview_tex) {
            destroy_preview_window();
            return false;
        }
        return true;
    };

    std::string html = html_file.empty() ? std::string(SAMPLE_HTML) : load_text_source(html_file);
    std::string css = css_file.empty() ? std::string() : load_text_source(css_file);
    std::string js = js_file.empty() ? std::string() : load_text_source(js_file);
    std::string linked_css;
    std::string linked_js;

    if (html.empty()) {
        std::fprintf(stderr, "Failed to load HTML input.\n");
        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (using_default_bridge_files) {
        std::fprintf(stderr, "[xcm_sdl_preview] Using bridge files:\n  HTML: %s\n  CSS:  %s\n", html_file.c_str(), css_file.c_str());
    }

    FileStamp html_stamp = get_stamp(html_file);
    FileStamp css_stamp = get_stamp(css_file);
    FileStamp js_stamp = get_stamp(js_file);
    const std::string browser_preview_path = "/tmp/xcm_browser_preview.html";

    std::vector<uint8_t> frame;
    std::vector<ImageOverlay> image_overlays;

#ifdef XCM_HAS_SDL_IMAGE
    std::unordered_map<std::string, SDL_Texture*> image_cache;
    auto release_image_cache = [&]() {
        for (auto& kv : image_cache) {
            if (kv.second) SDL_DestroyTexture(kv.second);
        }
        image_cache.clear();
    };
#endif

    std::string preview_css_overlay;
    bool bridge_compat_mode = false;
    bool bridge_sanitize_html = false;

    auto refresh_bridge_overlay = [&]() {
        preview_css_overlay.clear();
        if (!bridge_compat_mode) return;
        // Optional compatibility layer for unsupported browser features.
        preview_css_overlay =
            "html{background:#15162a;color:#eef2ff;}"
            "body{margin:0;padding:16px;background:#15162a;color:#eef2ff;font-size:18px;}"
            ".page-wrapper{display:block;background:transparent;color:#eef2ff;padding:8px;}"
            ".site-header,.mid-section,.site-footer{display:block;padding:10px 0;}"
            ".site-title{display:block;color:#ffffff;font-size:34px;margin:0 0 10px 0;}"
            ".placeholder-text{display:block;color:#cfd5ff;font-size:22px;}";
    };

    if (using_default_bridge_files) {
        zoom = 1.0f;
        refresh_bridge_overlay();
    }

    bool css_enabled = true;
    bool palette_mode = false;
    std::string palette_prompt;   // "html", "css", or "" for generic command
    std::string palette_input;
    std::string status = "Ready";
    if (using_default_bridge_files) status = "Bridge mode (raw CSS)";
    bool dragging_scroll = false;
    int drag_scroll_offset = 0;
    bool panel_dirty = true;
    xcm::InputCapture input_capture(8192);
    // Draw command buffer -- enqueue fill/border/text per frame, flush in painter order
    struct PanelCmd {
        enum class K : uint8_t { Fill, Border, Text } kind;
        SDL_Rect rect{};
        SDL_Color color{};
        std::string text;
    };
    std::vector<PanelCmd> pcmds;
    pcmds.reserve(128);

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
            if (palette_prompt == "html")      title += " | Open HTML: ";
            else if (palette_prompt == "css")  title += " | Open CSS: ";
            else                               title += " | :";
            title += palette_input;
            title += "_";
        }
        SDL_SetWindowTitle(win, title.c_str());
    };

    struct UiButton {
        SDL_Rect rect{};
        const char* label = "";
    };
    std::vector<UiButton> panel_buttons; // rebuilt by rebuild_panel on resize

#ifdef XCM_HAS_SDL_TTF
    // draw_text must be defined before pc_flush which calls it
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

    // Flex panel layout: fills panel_buttons to use available vertical space.
    // Button index order MUST stay stable -- click handler switch uses indices.
    auto rebuild_panel = [&]() {
        panel_buttons.clear();
        const int bx  = content_w + 8;
        const int bw  = PANEL_W - 16;
        const int gap = 4;
        const int header_h = 38;
        const int footer_h = 96;
        const int avail = std::max(198, win_h - header_h - footer_h);
        struct Row { const char* a; const char* b; };
        static const Row rows[] = {
            {"Palette (:)",  nullptr},   // 0
            {"Open HTML...", nullptr},   // 1
            {"Open CSS...",  nullptr},   // 2
            {"Open JS...",   nullptr},   // 3
            {"Browser (B)",  nullptr},   // 4
            {"Reload (R)",   nullptr},   // 5
            {"Watch (W)",    nullptr},   // 6
            {"CSS Toggle",   nullptr},   // 7
            {"Compat (M)",   nullptr},   // 8
            {"ImgStrip (I)", nullptr},   // 9
            {"50%", "75%"},              // 10, 11
            {"100%", "125%"},            // 12, 13
            {"Sample",       nullptr},   // 14
        };
        const int n  = static_cast<int>(sizeof(rows) / sizeof(rows[0]));
        const int bh = std::max(22, (avail - (n - 1) * gap) / n);
        int y = header_h;
        for (int i = 0; i < n; ++i) {
            if (!rows[i].b) {
                panel_buttons.push_back({SDL_Rect{bx, y, bw, bh}, rows[i].a});
            } else {
                const int hw = (bw - gap) / 2;
                panel_buttons.push_back({SDL_Rect{bx,            y, hw, bh}, rows[i].a});
                panel_buttons.push_back({SDL_Rect{bx + hw + gap, y, hw, bh}, rows[i].b});
            }
            y += bh + gap;
        }
        panel_dirty = false;
    };

    // Command buffer lambdas: queue all primitives then flush in painter order.
    // All fills rendered first, then borders, then text -- no z-mixing bleed.
    auto pc_fill   = [&](SDL_Rect r, SDL_Color c) {
        pcmds.push_back({PanelCmd::K::Fill,   r, c, ""});
    };
    auto pc_border = [&](SDL_Rect r, SDL_Color c) {
        pcmds.push_back({PanelCmd::K::Border, r, c, ""});
    };
    auto pc_text   = [&](int x, int y, std::string t, SDL_Color c) {
        pcmds.push_back({PanelCmd::K::Text, SDL_Rect{x, y, 0, 0}, c, std::move(t)});
    };
    auto pc_flush  = [&]() {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        for (const auto& d : pcmds) {
            if (d.kind != PanelCmd::K::Fill) continue;
            SDL_SetRenderDrawColor(ren, d.color.r, d.color.g, d.color.b, d.color.a);
            SDL_RenderFillRect(ren, &d.rect);
        }
        for (const auto& d : pcmds) {
            if (d.kind != PanelCmd::K::Border) continue;
            SDL_SetRenderDrawColor(ren, d.color.r, d.color.g, d.color.b, d.color.a);
            SDL_RenderDrawRect(ren, &d.rect);
        }
#ifdef XCM_HAS_SDL_TTF
        for (const auto& d : pcmds) {
            if (d.kind != PanelCmd::K::Text) continue;
            draw_text(d.rect.x, d.rect.y, d.text, d.color);
        }
#endif
        pcmds.clear();
    };

    auto point_in = [](int x, int y, const SDL_Rect& r) {
        return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
    };

    auto input_hit_region = [&](int x, int y) -> uint8_t {
        SDL_Rect track{win_w - 10, 4, 6, win_h - 8};
        if (point_in(x, y, track)) return 3;
        if (x >= content_w && x < win_w) return 2;
        if (x >= 0 && x < content_w && y >= 0 && y < win_h) return 1;
        return 0;
    };

    auto capture_mouse = [&](xcm::InputKind kind, int x, int y, int dx, int dy, int button) {
        xcm::InputEvent e;
        e.kind = kind;
        e.timestamp_ms = SDL_GetTicks64();
        e.x = x;
        e.y = y;
        e.dx = dx;
        e.dy = dy;
        e.button = button;
        e.hit_region = input_hit_region(x, y);
        e.modifiers = static_cast<uint16_t>(SDL_GetModState());
        input_capture.push(e);
    };

    auto capture_wheel = [&](int x, int y) {
        int mx = 0;
        int my = 0;
        SDL_GetMouseState(&mx, &my);
        xcm::InputEvent e;
        e.kind = xcm::InputKind::MouseWheel;
        e.timestamp_ms = SDL_GetTicks64();
        e.x = mx;
        e.y = my;
        e.wheel_x = x;
        e.wheel_y = y;
        e.hit_region = input_hit_region(mx, my);
        e.modifiers = static_cast<uint16_t>(SDL_GetModState());
        input_capture.push(e);
    };

    auto capture_key = [&](xcm::InputKind kind, SDL_Keycode keycode) {
        xcm::InputEvent e;
        e.kind = kind;
        e.timestamp_ms = SDL_GetTicks64();
        e.keycode = static_cast<int>(keycode);
        e.modifiers = static_cast<uint16_t>(SDL_GetModState());
        input_capture.push(e);
    };

    auto write_browser_preview = [&]() -> bool {
        if (html.empty()) return false;

        std::string html_out = rewrite_bridge_urls_for_file_preview(html);
        std::string css_out = rewrite_css_urls_for_file_preview(css, css_file);
        if (!linked_css.empty()) {
            if (!css_out.empty()) css_out.push_back('\n');
            css_out += rewrite_css_urls_for_file_preview(linked_css, html_file);
        }
        std::string js_out = js;
        if (!linked_js.empty()) {
            if (!js_out.empty()) js_out.push_back('\n');
            js_out += linked_js;
        }
        std::string base_href;
        if (!html_file.empty()) {
            base_href = path_to_file_url(dirname_copy(html_file)) + "/";
        }

        std::string doc;
        doc.reserve(html_out.size() + css_out.size() + js_out.size() + 1024);
        doc += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
        doc += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        doc += "<meta http-equiv=\"refresh\" content=\"2\">";
        if (!base_href.empty()) {
            doc += "<base href=\"" + base_href + "\">";
        }
        doc += "<style>";
        doc += css_out;
        doc += "</style></head><body>";
        doc += html_out;
        doc += "<script>";
        doc += js_out;
        doc += "</script></body></html>";

        return write_text_file(browser_preview_path, doc);
    };

    auto open_browser_preview = [&]() {
        ensure_preview_window();
        if (allow_external_browser_open) {
            std::string cmd;
        #ifdef _WIN32
            cmd = "start \"\" \"" + browser_preview_path + "\"";
        #elif defined(__APPLE__)
            cmd = "open '" + browser_preview_path + "' >/dev/null 2>&1";
        #else
            cmd = "xdg-open '" + browser_preview_path + "' >/dev/null 2>&1";
        #endif
            std::system(cmd.c_str());
        }
    };

    auto do_reload = [&]() {
        if (!html_file.empty()) {
            std::string re = load_text_source(html_file);
            if (!re.empty()) html = std::move(re);
        }
        if (!css_file.empty()) {
            css = load_text_source(css_file);
        }
        if (!js_file.empty()) {
            js = load_text_source(js_file);
        }
        linked_css.clear();
        for (const auto& href : extract_linked_stylesheet_hrefs(html)) {
            std::string src = href;
            if (!is_http_url(src) && !html_file.empty()) src = resolve_asset_path(href, html_file);
            std::string t = load_text_source(src);
            if (!t.empty()) {
                linked_css += t;
                linked_css.push_back('\n');
            }
        }
        linked_js.clear();
        for (const auto& s : extract_script_srcs(html)) {
            std::string src = s;
            if (!is_http_url(src) && !html_file.empty()) src = resolve_asset_path(s, html_file);
            std::string t = load_text_source(src);
            if (!t.empty()) {
                linked_js += t;
                linked_js.push_back('\n');
            }
        }
        write_browser_preview();
        dirty = true;
    };

    auto load_html_file = [&](const std::string& p) -> bool {
        std::string re = load_text_source(p);
        if (re.empty()) return false;
        html = std::move(re);
        html_file = p;
        html_stamp = get_stamp(html_file);
        linked_css.clear();
        for (const auto& href : extract_linked_stylesheet_hrefs(html)) {
            std::string src = href;
            if (!is_http_url(src)) src = resolve_asset_path(href, html_file);
            std::string t = load_text_source(src);
            if (!t.empty()) {
                linked_css += t;
                linked_css.push_back('\n');
            }
        }
        linked_js.clear();
        for (const auto& s : extract_script_srcs(html)) {
            std::string src = s;
            if (!is_http_url(src)) src = resolve_asset_path(s, html_file);
            std::string t = load_text_source(src);
            if (!t.empty()) {
                linked_js += t;
                linked_js.push_back('\n');
            }
        }
        dirty = true;
        return true;
    };

    auto load_css_file = [&](const std::string& p) -> bool {
        std::string re = load_text_source(p);
        if (re.empty()) return false;
        css = std::move(re);
        css_file = p;
        css_stamp = get_stamp(css_file);
        dirty = true;
        return true;
    };

    auto load_js_file = [&](const std::string& p) -> bool {
        std::string re = load_text_source(p);
        if (re.empty()) return false;
        js = std::move(re);
        js_file = p;
        js_stamp = get_stamp(js_file);
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
            status = "Commands: open/html/css/js/browser/reload/watch/cssmode/zoom/inputlog/inputbin/inputclear/quit";
            return;
        }
        if (cmd == "inputclear") {
            input_capture.clear();
            status = "Input capture cleared";
            return;
        }
        if (cmd == "inputlog") {
            std::string out = (parts.size() > 1) ? parts[1] : "/tmp/xcm_sdl_input.log";
            if (input_capture.dump_to_file(out)) {
                status = "Input log: " + out + " (" + std::to_string(input_capture.size()) + " events)";
            } else {
                status = "Input log write failed";
            }
            return;
        }
        if (cmd == "inputbin") {
            std::string out = (parts.size() > 1) ? parts[1] : "/tmp/xcm_sdl_input.bin";
            if (input_capture.dump_binary_to_file(out)) {
                status = "Input bin: " + out + " (" + std::to_string(input_capture.size()) + " events)";
            } else {
                status = "Input bin write failed";
            }
            return;
        }
        if (cmd == "browser") {
            if (write_browser_preview()) {
                open_browser_preview();
                status = allow_external_browser_open ? "Preview window opened + external" : "Preview window opened";
            } else {
                status = "Browser preview failed";
            }
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
                status = "Usage: open <html> [css] [js]";
                return;
            }
            if (!load_html_file(parts[1])) {
                status = "Failed to load html";
                return;
            }
            if (parts.size() > 2) {
                if (!load_css_file(parts[2])) status = "HTML loaded, CSS failed";
                else status = "HTML/CSS loaded";
            }
            if (parts.size() > 3) {
                if (!load_js_file(parts[3])) status = "HTML/CSS loaded, JS failed";
                else status = "HTML/CSS/JS loaded";
            } else {
                status = "HTML loaded";
            }
            write_browser_preview();
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
            write_browser_preview();
            return;
        }
        if (cmd == "js") {
            if (parts.size() < 2) {
                status = "Usage: js <file>";
                return;
            }
            if (!load_js_file(parts[1])) {
                status = "Failed to load js";
                return;
            }
            status = "JS loaded";
            write_browser_preview();
            return;
        }
        if (cmd == "quit" || cmd == "exit") {
            running = false;
            return;
        }

        status = "Unknown command";
    };

    if (using_default_bridge_files) {
        if (write_browser_preview()) {
            open_browser_preview();
            status = allow_external_browser_open ? "Preview window opened + external" : "Preview window opened";
        } else {
            status = "Browser preview write failed";
        }
    }

    set_title();
    rebuild_panel();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                xcm::InputEvent ie;
                ie.kind = xcm::InputKind::Quit;
                ie.timestamp_ms = SDL_GetTicks64();
                input_capture.push(ie);
                running = false;
            }
            else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE) {
                if (preview_win && ev.window.windowID == SDL_GetWindowID(preview_win)) {
                    destroy_preview_window();
                    status = "Preview window closed";
                    set_title();
                    continue;
                }
            }
            else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                if (preview_win && ev.window.windowID == SDL_GetWindowID(preview_win)) {
                    preview_win_w = std::max(64, ev.window.data1);
                    preview_win_h = std::max(64, ev.window.data2);
                    continue;
                }
                if (ev.window.windowID != main_window_id) continue;
                xcm::InputEvent ie;
                ie.kind = xcm::InputKind::WindowResize;
                ie.timestamp_ms = SDL_GetTicks64();
                ie.dx = ev.window.data1;
                ie.dy = ev.window.data2;
                input_capture.push(ie);
                win_w = std::max(64 + PANEL_W, ev.window.data1);
                win_h = std::max(64, ev.window.data2);
                content_w = win_w - PANEL_W;
                dirty = true;
                panel_dirty = true;
                status = "Resized";
                set_title();
            } else if (ev.type == SDL_MOUSEWHEEL) {
                if (ev.wheel.windowID != main_window_id) continue;
                capture_wheel(ev.wheel.x, ev.wheel.y);
                scroll_y -= static_cast<float>(ev.wheel.y * 48);
                scroll_y = std::max(0.f, scroll_y);
                dirty = true;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                if (ev.button.windowID != main_window_id) continue;
                int mx = ev.button.x;
                int my = ev.button.y;
                capture_mouse(xcm::InputKind::MouseDown, mx, my, 0, 0, ev.button.button);

                // Scrollbar track is at the far right edge of the window
                float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
                SDL_Rect track{win_w - 10, 4, 6, win_h - 8};
                const int thumb_h = (doc_h <= 0) ? track.h : std::clamp(
                    static_cast<int>(track.h * static_cast<float>(tex_h) / static_cast<float>(doc_h)), 24, track.h);
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
                    for (std::size_t i = 0; i < panel_buttons.size(); ++i) {
                        if (!point_in(mx, my, panel_buttons[i].rect)) continue;

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
                            case 3: // open js
                                palette_mode = true;
                                palette_prompt = "js";
                                palette_input.clear();
                                SDL_StartTextInput();
                                status = "Type JS path + Enter";
                                set_title();
                                break;
                            case 4: // browser
                                if (write_browser_preview()) {
                                    open_browser_preview();
                                    status = allow_external_browser_open ? "Preview window opened + external" : "Preview window opened";
                                } else {
                                    status = "Browser preview failed";
                                }
                                set_title();
                                break;
                            case 5: // reload
                                do_reload();
                                status = "Reloaded";
                                set_title();
                                break;
                            case 6: // watch
                                watch = !watch;
                                status = std::string("Watch ") + (watch ? "on" : "off");
                                set_title();
                                break;
                            case 7: // css toggle
                                css_enabled = !css_enabled;
                                dirty = true;
                                status = std::string("CSS ") + (css_enabled ? "on" : "off");
                                set_title();
                                break;
                            case 8: // compat mode
                                bridge_compat_mode = !bridge_compat_mode;
                                refresh_bridge_overlay();
                                dirty = true;
                                status = std::string("Compat ") + (bridge_compat_mode ? "on" : "off");
                                set_title();
                                break;
                            case 9: // img strip
                                bridge_sanitize_html = !bridge_sanitize_html;
                                dirty = true;
                                status = std::string("ImgStrip ") + (bridge_sanitize_html ? "on" : "off");
                                set_title();
                                break;
                            case 10: zoom = 0.5f;  dirty = true; status = "Zoom 50%";  set_title(); break;
                            case 11: zoom = 0.75f; dirty = true; status = "Zoom 75%";  set_title(); break;
                            case 12: zoom = 1.0f;  dirty = true; status = "Zoom 100%"; set_title(); break;
                            case 13: zoom = 1.25f; dirty = true; status = "Zoom 125%"; set_title(); break;
                            case 14: // sample
                                html = SAMPLE_HTML;
                                css.clear();
                                js.clear();
                                html_file.clear();
                                css_file.clear();
                                js_file.clear();
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
                if (ev.button.windowID != main_window_id) continue;
                capture_mouse(xcm::InputKind::MouseUp, ev.button.x, ev.button.y, 0, 0, ev.button.button);
                dragging_scroll = false;
            } else if (ev.type == SDL_MOUSEMOTION) {
                if (ev.motion.windowID != main_window_id) continue;
                capture_mouse(xcm::InputKind::MouseMove, ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel, 0);
                if (dragging_scroll) {
                    float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
                    if (max_scroll > 0.0f) {
                        SDL_Rect track{win_w - 10, 4, 6, win_h - 8};
                        const int thumb_h = (doc_h <= 0) ? track.h : std::clamp(
                            static_cast<int>(track.h * static_cast<float>(tex_h) / static_cast<float>(doc_h)), 24, track.h);
                        int thumb_top = ev.motion.y - drag_scroll_offset;
                        float t = static_cast<float>(thumb_top - track.y) / static_cast<float>(std::max(1, track.h - thumb_h));
                        t = std::clamp(t, 0.0f, 1.0f);
                        scroll_y = t * max_scroll;
                        dirty = true;
                    }
                }
            } else if (ev.type == SDL_TEXTINPUT) {
                if (ev.text.windowID != main_window_id) continue;
                xcm::InputEvent ie;
                ie.kind = xcm::InputKind::TextInput;
                ie.timestamp_ms = SDL_GetTicks64();
                ie.text = ev.text.text;
                input_capture.push(ie);
                if (palette_mode) {
                    palette_input += ev.text.text;
                    set_title();
                }
            } else if (ev.type == SDL_KEYDOWN) {
                if (ev.key.windowID != main_window_id) continue;
                SDL_Keycode k = ev.key.keysym.sym;
                capture_key(xcm::InputKind::KeyDown, k);

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
                            else {
                                scroll_y = 0.f;
                                write_browser_preview();
                                status = "HTML loaded: " + cmd;
                            }
                        } else if (prompt == "css") {
                            if (cmd.empty()) { status = "Canceled"; }
                            else if (!load_css_file(cmd)) { status = "Failed: " + cmd; }
                            else {
                                write_browser_preview();
                                status = "CSS loaded: " + cmd;
                            }
                        } else if (prompt == "js") {
                            if (cmd.empty()) { status = "Canceled"; }
                            else if (!load_js_file(cmd)) { status = "Failed: " + cmd; }
                            else {
                                write_browser_preview();
                                status = "JS loaded: " + cmd;
                            }
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
                else if (k == SDLK_b) {
                    if (write_browser_preview()) {
                        open_browser_preview();
                        status = allow_external_browser_open ? "Preview window opened + external" : "Preview window opened";
                    } else {
                        status = "Browser preview failed";
                    }
                    set_title();
                }
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
                } else if (k == SDLK_m) {
                    bridge_compat_mode = !bridge_compat_mode;
                    refresh_bridge_overlay();
                    dirty = true;
                    status = std::string("Compat ") + (bridge_compat_mode ? "on" : "off");
                    set_title();
                } else if (k == SDLK_i) {
                    bridge_sanitize_html = !bridge_sanitize_html;
                    dirty = true;
                    status = std::string("ImgStrip ") + (bridge_sanitize_html ? "on" : "off");
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
            } else if (ev.type == SDL_KEYUP) {
                if (ev.key.windowID != main_window_id) continue;
                capture_key(xcm::InputKind::KeyUp, ev.key.keysym.sym);
            }
        }

        if (watch) {
            bool changed_any = false;
            if (!html_file.empty() && changed(html_file, html_stamp)) {
                std::string re = load_text_source(html_file);
                if (!re.empty()) {
                    html = std::move(re);
                    linked_css.clear();
                    for (const auto& href : extract_linked_stylesheet_hrefs(html)) {
                        std::string src = href;
                        if (!is_http_url(src)) src = resolve_asset_path(href, html_file);
                        std::string t = load_text_source(src);
                        if (!t.empty()) {
                            linked_css += t;
                            linked_css.push_back('\n');
                        }
                    }
                    linked_js.clear();
                    for (const auto& s : extract_script_srcs(html)) {
                        std::string src = s;
                        if (!is_http_url(src)) src = resolve_asset_path(s, html_file);
                        std::string t = load_text_source(src);
                        if (!t.empty()) {
                            linked_js += t;
                            linked_js.push_back('\n');
                        }
                    }
                    changed_any = true;
                }
            }
            if (!css_file.empty() && changed(css_file, css_stamp)) {
                std::string re = load_text_source(css_file);
                css = std::move(re);
                changed_any = true;
            }
            if (!js_file.empty() && changed(js_file, js_stamp)) {
                std::string re = load_text_source(js_file);
                js = std::move(re);
                changed_any = true;
            }
            if (changed_any && using_default_bridge_files) {
                write_browser_preview();
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

            std::string html_for_render = bridge_sanitize_html ? strip_img_tags(html) : html;
            std::string css_for_render = css_enabled ? css : std::string();
            if (css_enabled && !linked_css.empty()) {
                if (!css_for_render.empty()) css_for_render.push_back('\n');
                css_for_render += linked_css;
            }
            if (!preview_css_overlay.empty()) {
                if (!css_for_render.empty()) css_for_render.push_back('\n');
                css_for_render += preview_css_overlay;
            }
            if (render_page(tex_w, tex_h, html_for_render, css_for_render, scroll_y, frame, doc_h, &image_overlays)) {
                float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
                if (scroll_y > max_scroll) {
                    scroll_y = max_scroll;
                    render_page(tex_w, tex_h, html_for_render, css_for_render, scroll_y, frame, doc_h, &image_overlays);
                }

#ifdef XCM_HAS_SDL_IMAGE
                if (sdl_image_ready) {
                    for (const auto& ov : image_overlays) {
                        if (!ov.is_background || ov.w < 2.f || ov.h < 2.f || ov.url.empty()) continue;
                        std::string full_path = resolve_asset_path(ov.url, html_file);
                        if (full_path.empty()) continue;
                        SDL_Surface* surf = IMG_Load(full_path.c_str());
                        if (!surf) continue;
                        SDL_Surface* rgba_surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
                        SDL_FreeSurface(surf);
                        if (!rgba_surf) continue;
                        blend_surface_into_rgba(frame, tex_w, tex_h, rgba_surf, ov, scroll_y);
                        SDL_FreeSurface(rgba_surf);
                    }
                }
#endif

                SDL_UpdateTexture(tex, nullptr, frame.data(), tex_w * 4);

                if (preview_win && preview_ren) {
                    if (!preview_tex || preview_tex_w != tex_w || preview_tex_h != tex_h) {
                        if (preview_tex) SDL_DestroyTexture(preview_tex);
                        preview_tex_w = tex_w;
                        preview_tex_h = tex_h;
                        preview_tex = SDL_CreateTexture(
                            preview_ren,
                            SDL_PIXELFORMAT_RGBA32,
                            SDL_TEXTUREACCESS_STREAMING,
                            std::max(64, preview_tex_w),
                            std::max(64, preview_tex_h)
                        );
                    }
                    if (preview_tex) {
                        SDL_UpdateTexture(preview_tex, nullptr, frame.data(), tex_w * 4);
                    }
                }
            } else {
                status = "Render failed (parse/layout)";
                set_title();
            }
            dirty = false;
        }

        // Draw page content into the left portion of the window only
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_Rect content_dst{0, 0, content_w, win_h};
        SDL_RenderCopy(ren, tex, nullptr, &content_dst);

#ifdef XCM_HAS_SDL_IMAGE
        if (sdl_image_ready && !image_overlays.empty()) {
            const float sx = static_cast<float>(content_w) / static_cast<float>(std::max(1, tex_w));
            const float sy = static_cast<float>(win_h) / static_cast<float>(std::max(1, tex_h));
            const SDL_Rect content_clip{0, 0, content_w, win_h};
            SDL_RenderSetClipRect(ren, &content_clip);

            for (const auto& ov : image_overlays) {
                if (ov.is_background || ov.w < 2.f || ov.h < 2.f || ov.url.empty()) continue;

                std::string full_path = resolve_asset_path(ov.url, html_file);
                if (full_path.empty()) continue;

                SDL_Texture* itex = nullptr;
                auto it = image_cache.find(full_path);
                if (it != image_cache.end()) {
                    itex = it->second;
                } else {
                    SDL_Surface* surf = IMG_Load(full_path.c_str());
                    if (surf) {
                        itex = SDL_CreateTextureFromSurface(ren, surf);
                        SDL_FreeSurface(surf);
                    }
                    image_cache[full_path] = itex;
                }

                if (!itex) continue;

                int dx = static_cast<int>(std::round(ov.x * sx));
                int dy = static_cast<int>(std::round((ov.y - scroll_y) * sy));
                int dw = std::max(1, static_cast<int>(std::round(ov.w * sx)));
                int dh = std::max(1, static_cast<int>(std::round(ov.h * sy)));
                SDL_Rect dst{dx, dy, dw, dh};
                SDL_RenderCopy(ren, itex, nullptr, &dst);
            }

            SDL_RenderSetClipRect(ren, nullptr);
        }
#endif

        if (panel_dirty) rebuild_panel();

        // Panel draw command buffer: all fills -> borders -> text in one ordered pass.
        // Fully opaque backgrounds prevent TTF blended-alpha text from bleeding.

        // 1. Panel background (opaque)
        pc_fill({content_w,     0, PANEL_W,     win_h}, {8,  14,  24, 255});
        pc_fill({content_w + 1, 1, PANEL_W - 2, 34   }, {18, 30,  50, 255});
        pc_border({content_w,   0, PANEL_W,     win_h}, {36, 60,  90, 255});
        pc_text(content_w + 8, 10, "XCM Controls",      {200, 222, 255, 255});

        // 2. Buttons (flex-sized, cached by rebuild_panel)
        for (const auto& b : panel_buttons) {
            pc_fill(b.rect,   {16,  26,  42, 255});
            pc_border(b.rect, {44,  76, 112, 255});
            const int ty = b.rect.y + std::max(0, (b.rect.h - 13) / 2);
            pc_text(b.rect.x + 6, ty, b.label, {188, 216, 255, 255});
        }

        // 3. Footer status block
        {
            const int fy = win_h - 92;
            pc_fill({content_w + 1, fy, PANEL_W - 2, 91}, {10, 16, 28, 255});
            const std::string i1 = "Zoom: " + std::to_string(static_cast<int>(std::round(zoom * 100.f))) + "%";
            const std::string i2 =
                "Watch:" + std::string(watch ? "on" : "off") +
                " CSS:" + (css_enabled ? "on" : "off") +
                " Cmp:" + (bridge_compat_mode ? "on" : "off") +
                " Img:" + (bridge_sanitize_html ? "on" : "off");
            pc_text(content_w + 8, fy +  4, i1,     {160, 200, 235, 255});
            pc_text(content_w + 8, fy + 24, i2,     {160, 200, 235, 255});
            pc_text(content_w + 8, fy + 46, status, {255, 210, 130, 255});
        }

        // 4. Input bar when palette mode is active
        if (palette_mode) {
            const int iy = win_h - 38;
            pc_fill({content_w + 6, iy, PANEL_W - 12, 22},   {12, 20,  36, 255});
            pc_border({content_w + 6, iy, PANEL_W - 12, 22}, {80, 144, 222, 255});
            const std::string pfx = (palette_prompt == "html") ? "HTML: " :
                                    (palette_prompt == "css")  ? "CSS:  " : ": ";
            pc_text(content_w + 10, iy + 4, pfx + palette_input + "_", {255, 235, 128, 255});
        }

        // 5. Scrollbar track + thumb (geometry matches click/drag handlers)
        {
            const SDL_Rect trk{win_w - 10, 4, 6, win_h - 8};
            pc_fill(trk, {28, 40, 58, 255});
            const float ms  = std::max(0.f, static_cast<float>(doc_h - tex_h));
            const int   sbh = (doc_h > 0)
                ? std::clamp(static_cast<int>(trk.h * static_cast<float>(tex_h) / static_cast<float>(doc_h)), 24, trk.h)
                : trk.h;
            int sby = trk.y;
            if (ms > 0.f)
                sby += static_cast<int>((scroll_y / ms) * static_cast<float>(trk.h - sbh));
            pc_fill({trk.x, sby, trk.w, sbh},
                dragging_scroll ? SDL_Color{40, 148, 210, 255} : SDL_Color{65, 110, 190, 255});
        }

        pc_flush();

        SDL_RenderPresent(ren);

        if (preview_win && preview_ren && preview_tex) {
            SDL_SetRenderDrawColor(preview_ren, 0, 0, 0, 255);
            SDL_RenderClear(preview_ren);
            int pw = 0;
            int ph = 0;
            SDL_GetWindowSize(preview_win, &pw, &ph);
            SDL_Rect dst{0, 0, std::max(1, pw), std::max(1, ph)};
            SDL_RenderCopy(preview_ren, preview_tex, nullptr, &dst);
            SDL_RenderPresent(preview_ren);
        }
    }

#ifdef XCM_HAS_SDL_TTF
    if (ui_font) TTF_CloseFont(ui_font);
    if (TTF_WasInit()) TTF_Quit();
#endif
#ifdef XCM_HAS_SDL_IMAGE
    release_image_cache();
    IMG_Quit();
#endif
    destroy_preview_window();
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
