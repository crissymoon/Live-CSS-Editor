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
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace {

bool g_debug_enabled = true;
std::string g_debug_log_path = "/tmp/xcm_sdl_preview_debug.log";
FILE* g_debug_file = nullptr;
std::string g_workspace_root;
std::vector<std::string> g_asset_failures;

void debug_close() {
    if (g_debug_file) {
        std::fclose(g_debug_file);
        g_debug_file = nullptr;
    }
}

bool debug_open(const std::string& path) {
    debug_close();
    g_debug_log_path = path;
    g_debug_file = std::fopen(g_debug_log_path.c_str(), "wb");
    return g_debug_file != nullptr;
}

void debug_logf(const char* fmt, ...) {
    if (!g_debug_enabled || !g_debug_file) return;
    std::va_list ap;
    va_start(ap, fmt);
    std::vfprintf(g_debug_file, fmt, ap);
    va_end(ap);
    std::fputc('\n', g_debug_file);
    std::fflush(g_debug_file);
}

void record_asset_failure(const std::string& kind,
                          const std::string& ref,
                          const std::string& resolved,
                          const std::string& reason) {
    std::string line = kind + ": ";
    line += ref.empty() ? "<empty>" : ref;
    if (!resolved.empty() && resolved != ref) {
        line += " -> ";
        line += resolved;
    }
    if (!reason.empty()) {
        line += " (";
        line += reason;
        line += ")";
    }
    g_asset_failures.push_back(line);
    if (g_asset_failures.size() > 12) {
        g_asset_failures.erase(g_asset_failures.begin(), g_asset_failures.begin() + 4);
    }
    debug_logf("asset_failure %s", line.c_str());
}

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

bool is_https_url(const std::string& s) {
    return s.rfind("https://", 0) == 0;
}

bool is_web_url(const std::string& s) {
    return is_http_url(s) || is_https_url(s);
}

std::string load_text_source(const std::string& src) {
    if (src.empty()) return "";
    if (is_https_url(src)) {
        debug_logf("load_text_source https unsupported: %s", src.c_str());
        record_asset_failure("source", src, src, "https unsupported by low-level fetcher");
        return "";
    }
    if (is_http_url(src)) {
        debug_logf("load_text_source http start: %s", src.c_str());
        xcm::HttpResponse r = xcm::http_fetch_get(src, 5000);
        if (!r.ok) {
            debug_logf("load_text_source http fail: %s status=%d err=%s", src.c_str(), r.status_code, r.error.c_str());
            record_asset_failure("source", src, src, r.error.empty() ? "http fetch failed" : r.error);
            return "";
        }
        debug_logf("load_text_source http ok: %s bytes=%zu status=%d", src.c_str(), r.body.size(), r.status_code);
        return r.body;
    }
    std::string out = read_file(src);
    if (out.empty()) {
        debug_logf("load_text_source file fail/empty: %s", src.c_str());
        record_asset_failure("source", src, src, "file missing or empty");
    }
    else debug_logf("load_text_source file ok: %s bytes=%zu", src.c_str(), out.size());
    return out;
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
    debug_logf("extract_linked_stylesheet_hrefs count=%zu", out.size());
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
    debug_logf("extract_script_srcs count=%zu", out.size());
    return out;
}

std::size_t count_tag_occurrences(const std::string& html, const std::string& tag_name) {
    if (tag_name.empty()) return 0;
    std::size_t count = 0;
    std::string lower = lower_copy(html);
    std::string needle = "<" + lower_copy(tag_name);
    std::size_t pos = 0;
    while (true) {
        std::size_t p = lower.find(needle, pos);
        if (p == std::string::npos) break;
        ++count;
        pos = p + needle.size();
    }
    return count;
}

std::size_t count_visible_text_chars(const std::string& html) {
    bool in_tag = false;
    std::size_t n = 0;
    for (char ch : html) {
        if (ch == '<') {
            in_tag = true;
            continue;
        }
        if (ch == '>') {
            in_tag = false;
            continue;
        }
        if (!in_tag && !std::isspace(static_cast<unsigned char>(ch))) ++n;
    }
    return n;
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

std::string url_origin(const std::string& url) {
    if (url.rfind("http://", 0) != 0) return "";
    std::size_t start = 7;
    std::size_t slash = url.find('/', start);
    if (slash == std::string::npos) return url;
    return url.substr(0, slash);
}

std::string url_dirname(const std::string& url) {
    if (url.rfind("http://", 0) != 0) return dirname_copy(url);
    std::size_t slash = url.find_last_of('/');
    if (slash == std::string::npos || slash < 7) return url + "/";
    return url.substr(0, slash + 1);
}

std::string resolve_asset_path(const std::string& url, const std::string& html_path) {
    if (url.empty()) return "";
    if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0 || url.rfind("data:", 0) == 0) {
        return url;
    }

    if (is_http_url(html_path)) {
        if (!url.empty() && url[0] == '/') {
            std::string origin = url_origin(html_path);
            if (origin.empty()) return "";
            return origin + url;
        }
        return url_dirname(html_path) + url;
    }

    const std::string workspace_root = g_workspace_root.empty() ? std::string(".") : g_workspace_root;
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

std::string path_to_file_url(std::string path);

std::string rewrite_bridge_urls_for_file_preview(std::string s) {
    if (g_workspace_root.empty()) return s;
    const std::string bridge_root = path_to_file_url(g_workspace_root + "/my_project/vscode-bridge/");
    const std::string project_root = path_to_file_url(g_workspace_root + "/my_project/");
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
SDL_Surface* load_image_surface(const std::string& resolved) {
    if (resolved.empty()) return nullptr;
    if (is_https_url(resolved)) {
        record_asset_failure("image", resolved, resolved, "https unsupported by low-level fetcher");
        return nullptr;
    }
    if (is_http_url(resolved)) {
        xcm::HttpResponse r = xcm::http_fetch_get(resolved, 5000);
        if (!r.ok || r.body.empty()) {
            record_asset_failure("image", resolved, resolved, r.error.empty() ? "http fetch failed" : r.error);
            return nullptr;
        }
        SDL_RWops* rw = SDL_RWFromConstMem(r.body.data(), static_cast<int>(r.body.size()));
        if (!rw) {
            record_asset_failure("image", resolved, resolved, "SDL_RWFromConstMem failed");
            return nullptr;
        }
        SDL_Surface* surf = IMG_Load_RW(rw, 1);
        if (!surf) {
            record_asset_failure("image", resolved, resolved, IMG_GetError());
        }
        return surf;
    }
    SDL_Surface* surf = IMG_Load(resolved.c_str());
    if (!surf) {
        record_asset_failure("image", resolved, resolved, IMG_GetError());
    }
    return surf;
}

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

std::string detect_workspace_root() {
    const char* env_root = std::getenv("XCM_WORKSPACE_ROOT");
    if (env_root && *env_root) {
        return std::string(env_root);
    }

    std::error_code ec;
    std::filesystem::path cur = std::filesystem::current_path(ec);
    if (ec) return "";

    auto looks_like_root = [](const std::filesystem::path& p) -> bool {
        std::error_code iec;
        return std::filesystem::exists(p / "my_project" / "vscode-bridge" / "projects", iec) ||
               std::filesystem::exists(p / "dev-tools" / "dev-browser", iec) ||
               std::filesystem::exists(p / "launcher.json", iec);
    };

    std::filesystem::path p = cur;
    for (int i = 0; i < 12; ++i) {
        if (looks_like_root(p)) return p.string();
        if (!p.has_parent_path()) break;
        std::filesystem::path next = p.parent_path();
        if (next == p) break;
        p = next;
    }
    return cur.string();
}

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

    // Diagnostic: walk the layout tree and log stats.
    {
        struct TreeStats {
            int total_boxes = 0;
            int text_boxes = 0;
            int bg_color_boxes = 0;
            int bg_image_boxes = 0;
            float first_text_x = 0, first_text_y = 0;
            float root_w = 0, root_h = 0;
        };
        TreeStats ts;
        if (root) {
            ts.root_w = root->width;
            ts.root_h = root->height;
        }
        std::function<void(xcm::LayoutBox*, int)> walk = [&](xcm::LayoutBox* b, int depth) {
            if (!b) return;
            ++ts.total_boxes;
            const xcm::ComputedStyle* bcs = b->node && b->node->computed_style
                ? static_cast<const xcm::ComputedStyle*>(b->node->computed_style)
                : nullptr;
            if (b->node && b->node->kind == xcm::NodeKind::TEXT && b->node->text) {
                ++ts.text_boxes;
                if (ts.text_boxes == 1) {
                    ts.first_text_x = b->x; ts.first_text_y = b->y;
                }
            }
            if (bcs && bcs->background_color.a > 0) ++ts.bg_color_boxes;
            if (bcs && bcs->background_image && bcs->background_image[0]) ++ts.bg_image_boxes;
            for (auto* ch : b->children) walk(ch, depth + 1);
        };
        walk(root, 0);
        debug_logf("layout_tree boxes=%d text=%d bg_color=%d bg_image=%d root=%.0fx%.0f first_text=%.0f,%.0f",
                   ts.total_boxes, ts.text_boxes, ts.bg_color_boxes, ts.bg_image_boxes,
                   ts.root_w, ts.root_h, ts.first_text_x, ts.first_text_y);
    }

    xcm::PaintEngine paint(w, h);
    paint.set_scroll(scroll_y);
    paint.paint(root);

    doc_height = root ? static_cast<int>(root->height) : h;

    rgba.assign(paint.pixels(), paint.pixels() + static_cast<std::size_t>(w * h * 4));
    return true;
}

bool frame_is_mostly_white(const std::vector<uint8_t>& rgba) {
    if (rgba.size() < 4) return true;
    std::size_t px = rgba.size() / 4;
    if (px == 0) return true;

    const std::size_t step = std::max<std::size_t>(1, px / 12000);
    std::size_t sampled = 0;
    std::size_t white_like = 0;
    for (std::size_t i = 0; i < px; i += step) {
        std::size_t o = i * 4;
        if (o + 3 >= rgba.size()) break;
        const uint8_t r = rgba[o + 0];
        const uint8_t g = rgba[o + 1];
        const uint8_t b = rgba[o + 2];
        if (r >= 248 && g >= 248 && b >= 248) ++white_like;
        ++sampled;
    }
    if (sampled == 0) return true;
    return (static_cast<double>(white_like) / static_cast<double>(sampled)) > 0.995;
}

struct FrameContentStats {
    bool has_non_white = false;
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;
    std::size_t non_white_count = 0;
};

FrameContentStats analyze_frame_content(const std::vector<uint8_t>& rgba, int w, int h) {
    FrameContentStats stats;
    if (w <= 0 || h <= 0 || rgba.size() < static_cast<std::size_t>(w * h * 4)) return stats;

    stats.min_x = w;
    stats.min_y = h;
    stats.max_x = -1;
    stats.max_y = -1;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            std::size_t o = static_cast<std::size_t>((y * w + x) * 4);
            const uint8_t r = rgba[o + 0];
            const uint8_t g = rgba[o + 1];
            const uint8_t b = rgba[o + 2];
            if (r >= 248 && g >= 248 && b >= 248) continue;
            stats.has_non_white = true;
            ++stats.non_white_count;
            stats.min_x = std::min(stats.min_x, x);
            stats.min_y = std::min(stats.min_y, y);
            stats.max_x = std::max(stats.max_x, x);
            stats.max_y = std::max(stats.max_y, y);
        }
    }

    if (!stats.has_non_white) {
        stats.min_x = stats.min_y = stats.max_x = stats.max_y = 0;
    }
    return stats;
}

enum class RendererMode {
    Auto,
    Hardware,
    Software,
};

RendererMode parse_renderer_mode(const std::string& s) {
    std::string v = lower_copy(trim_copy(s));
    if (v == "hardware" || v == "gpu") return RendererMode::Hardware;
    if (v == "software" || v == "cpu") return RendererMode::Software;
    return RendererMode::Auto;
}

const char* renderer_mode_name(RendererMode m) {
    switch (m) {
        case RendererMode::Hardware: return "hardware";
        case RendererMode::Software: return "software";
        default: return "auto";
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string html_file;
    std::string css_file;
    std::string js_file;
    bool watch = false;
    bool allow_external_browser_open = false;
    std::string debug_log_path = "/tmp/xcm_sdl_preview_debug.log";
    bool using_default_bridge_files = false;
    RendererMode renderer_mode = RendererMode::Auto;
    bool use_surface_upload = true;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--watch") watch = true;
        else if (a == "--external-browser") allow_external_browser_open = true;
        else if (a == "--hardware-render") renderer_mode = RendererMode::Hardware;
        else if (a == "--software-render") renderer_mode = RendererMode::Software;
        else if (a.rfind("--renderer=", 0) == 0) renderer_mode = parse_renderer_mode(a.substr(11));
        else if (a == "--surface-upload") use_surface_upload = true;
        else if (a == "--streaming-upload") use_surface_upload = false;
        else if (a == "--no-debug-log") g_debug_enabled = false;
        else if (a.rfind("--debug-log=", 0) == 0) debug_log_path = a.substr(12);
        else if (html_file.empty()) html_file = a;
        else if (css_file.empty()) css_file = a;
        else if (js_file.empty()) js_file = a;
    }

    if (g_debug_enabled) {
        if (!debug_open(debug_log_path)) {
            g_debug_enabled = false;
        } else {
            debug_logf("xcm_sdl_preview start argc=%d", argc);
        }
    }

    g_workspace_root = detect_workspace_root();
    debug_logf("workspace_root=%s", g_workspace_root.c_str());
    debug_logf("renderer_mode=%s", renderer_mode_name(renderer_mode));
    debug_logf("texture_upload=%s", use_surface_upload ? "surface" : "streaming");

    // If no file args are provided, point SDL preview at the VSCode bridge editor files.
    // This makes xcm_sdl_preview immediately show the same content as html-editor/css-editor.
    if (html_file.empty()) {
        const char* env_html = std::getenv("XCM_DEFAULT_HTML");
        const char* env_css = std::getenv("XCM_DEFAULT_CSS");
        const char* env_js = std::getenv("XCM_DEFAULT_JS");
        const std::string fallback_html = g_workspace_root + "/my_project/vscode-bridge/projects/html-editor.html";
        const std::string fallback_css  = g_workspace_root + "/my_project/vscode-bridge/projects/css-editor.css";
        const std::string fallback_js   = g_workspace_root + "/my_project/vscode-bridge/projects/js-editor.js";

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
    auto create_renderer_for_window = [&](SDL_Window* target, const char* tag) -> SDL_Renderer* {
        auto try_create = [&](Uint32 flags) -> SDL_Renderer* {
            SDL_Renderer* r = SDL_CreateRenderer(target, -1, flags);
            if (r) {
                SDL_RendererInfo ri{};
                if (SDL_GetRendererInfo(r, &ri) == 0) {
                    debug_logf("renderer %s ok flags=%u backend=%s", tag, flags, ri.name ? ri.name : "?");
                } else {
                    debug_logf("renderer %s ok flags=%u backend=?", tag, flags);
                }
            } else {
                debug_logf("renderer %s fail flags=%u err=%s", tag, flags, SDL_GetError());
            }
            return r;
        };

        if (renderer_mode == RendererMode::Software) {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
            return try_create(SDL_RENDERER_SOFTWARE);
        }
        if (renderer_mode == RendererMode::Hardware) {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
            SDL_Renderer* r = try_create(SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!r) {
                SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
                r = try_create(SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            }
            return r;
        }

        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "");
        SDL_Renderer* r = try_create(SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!r) {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
            r = try_create(SDL_RENDERER_SOFTWARE);
        }
        return r;
    };

    SDL_Renderer* ren = create_renderer_for_window(win, "main");
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

    auto create_streaming_texture = [&](SDL_Renderer* target_ren, int w, int h, const char* tag) -> SDL_Texture* {
        SDL_Texture* t = SDL_CreateTexture(
            target_ren,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            std::max(64, w),
            std::max(64, h)
        );
        if (!t) {
            debug_logf("create_streaming_texture %s failed: %s", tag, SDL_GetError());
            return nullptr;
        }
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_NONE);
        return t;
    };

    auto replace_texture_from_rgba = [&](SDL_Renderer* target_ren,
                                         SDL_Texture*& target_tex,
                                         int w,
                                         int h,
                                         const std::vector<uint8_t>& rgba,
                                         const char* tag) -> bool {
        if (w <= 0 || h <= 0 || rgba.size() < static_cast<std::size_t>(w * h * 4)) return false;
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(
            const_cast<uint8_t*>(rgba.data()),
            w,
            h,
            32,
            w * 4,
            SDL_PIXELFORMAT_RGBA32
        );
        if (!surf) {
            debug_logf("replace_texture_from_rgba %s surface failed: %s", tag, SDL_GetError());
            return false;
        }
        SDL_Texture* next = SDL_CreateTextureFromSurface(target_ren, surf);
        SDL_FreeSurface(surf);
        if (!next) {
            debug_logf("replace_texture_from_rgba %s texture failed: %s", tag, SDL_GetError());
            return false;
        }
        SDL_SetTextureBlendMode(next, SDL_BLENDMODE_NONE);
        if (target_tex) SDL_DestroyTexture(target_tex);
        target_tex = next;
        return true;
    };

    SDL_Texture* tex = use_surface_upload ? nullptr : create_streaming_texture(ren, tex_w, tex_h, "main");
    if (!tex) {
        if (!use_surface_upload) {
            std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            SDL_Quit();
            return 1;
        }
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

        preview_ren = create_renderer_for_window(preview_win, "preview");
        if (!preview_ren) {
            destroy_preview_window();
            debug_logf("ensure_preview_window renderer create failed: %s", SDL_GetError());
            return false;
        }

        preview_tex_w = tex_w;
        preview_tex_h = tex_h;
        preview_tex = use_surface_upload ? nullptr : create_streaming_texture(preview_ren, preview_tex_w, preview_tex_h, "preview");
        if (!preview_tex && !use_surface_upload) {
            destroy_preview_window();
            debug_logf("ensure_preview_window texture create failed: %s", SDL_GetError());
            return false;
        }
        debug_logf("ensure_preview_window created win=%dx%d tex=%dx%d", preview_win_w, preview_win_h, preview_tex_w, preview_tex_h);
        return true;
    };

    if (!ensure_preview_window()) {
        debug_logf("startup preview window creation failed");
    }

    std::string html = html_file.empty() ? std::string(SAMPLE_HTML) : load_text_source(html_file);
    std::string css = css_file.empty() ? std::string() : load_text_source(css_file);
    std::string js = js_file.empty() ? std::string() : load_text_source(js_file);
    std::string linked_css;
    std::string linked_js;
    bool logged_js_runtime_warning = false;
    debug_logf("startup sources html=%zu css=%zu js=%zu", html.size(), css.size(), js.size());

    auto refresh_linked_assets = [&]() {
        std::size_t css_ok = 0;
        std::size_t css_fail = 0;
        std::size_t js_ok = 0;
        std::size_t js_fail = 0;
        linked_css.clear();
        for (const auto& href : extract_linked_stylesheet_hrefs(html)) {
            std::string src = href;
            if (!is_http_url(src) && !html_file.empty()) src = resolve_asset_path(href, html_file);
            std::string t = load_text_source(src);
            if (!t.empty()) {
                linked_css += t;
                linked_css.push_back('\n');
                ++css_ok;
            } else {
                ++css_fail;
                record_asset_failure("linked-css", href, src, "load failed");
                debug_logf("linked css fail: href=%s resolved=%s", href.c_str(), src.c_str());
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
                ++js_ok;
            } else {
                ++js_fail;
                record_asset_failure("linked-js", s, src, "load failed");
                debug_logf("linked js fail: src=%s resolved=%s", s.c_str(), src.c_str());
            }
        }
        debug_logf("refresh_linked_assets css_ok=%zu css_fail=%zu js_ok=%zu js_fail=%zu linked_css=%zu linked_js=%zu",
                   css_ok, css_fail, js_ok, js_fail, linked_css.size(), linked_js.size());
    };
    refresh_linked_assets();

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
    bool show_diagnostics = true;
    int last_overlays = 0;
    int last_doc_h = 0;
    std::size_t last_text_chars = 0;
    std::size_t last_img_tags = 0;
    std::size_t last_script_tags = 0;
    std::size_t last_asset_failures = 0;
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
        const int footer_h = 126;
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

    auto preview_window_id = [&]() -> Uint32 {
        return preview_win ? SDL_GetWindowID(preview_win) : 0;
    };

    auto is_preview_window_event = [&](Uint32 window_id) -> bool {
        return preview_win && window_id == preview_window_id();
    };

    auto input_hit_region = [&](int x, int y) -> uint8_t {
        SDL_Rect track{win_w - 10, 4, 6, win_h - 8};
        if (point_in(x, y, track)) return 3;
        if (x >= content_w && x < win_w) return 2;
        if (x >= 0 && x < content_w && y >= 0 && y < win_h) return 1;
        return 0;
    };

    auto map_window_point = [&](Uint32 window_id, int raw_x, int raw_y, int& mapped_x, int& mapped_y, uint8_t& hit_region) {
        if (window_id == main_window_id) {
            mapped_x = raw_x;
            mapped_y = raw_y;
            hit_region = input_hit_region(raw_x, raw_y);
            return true;
        }
        if (is_preview_window_event(window_id)) {
            int pw = 0;
            int ph = 0;
            SDL_GetWindowSize(preview_win, &pw, &ph);
            const float sx = (pw > 0) ? static_cast<float>(tex_w) / static_cast<float>(pw) : 1.f;
            const float sy = (ph > 0) ? static_cast<float>(tex_h) / static_cast<float>(ph) : 1.f;
            mapped_x = std::clamp(static_cast<int>(std::floor(static_cast<float>(raw_x) * sx)), 0, std::max(0, tex_w - 1));
            mapped_y = std::clamp(static_cast<int>(std::floor(static_cast<float>(raw_y) * sy)), 0, std::max(0, tex_h - 1));
            hit_region = 1;
            return true;
        }
        return false;
    };

    auto capture_mouse = [&](Uint32 window_id,
                             xcm::InputKind kind,
                             int raw_x,
                             int raw_y,
                             int dx,
                             int dy,
                             int button) {
        int mapped_x = raw_x;
        int mapped_y = raw_y;
        uint8_t hit_region = 0;
        if (!map_window_point(window_id, raw_x, raw_y, mapped_x, mapped_y, hit_region)) return;
        xcm::InputEvent e;
        e.kind = kind;
        e.timestamp_ms = SDL_GetTicks64();
        e.x = mapped_x;
        e.y = mapped_y;
        e.dx = dx;
        e.dy = dy;
        e.button = button;
        e.hit_region = hit_region;
        e.modifiers = static_cast<uint16_t>(SDL_GetModState());
        input_capture.push(e);
    };

    auto capture_wheel = [&](Uint32 window_id, int wheel_x, int wheel_y) {
        int raw_x = 0;
        int raw_y = 0;
        SDL_GetMouseState(&raw_x, &raw_y);
        int mapped_x = raw_x;
        int mapped_y = raw_y;
        uint8_t hit_region = 0;
        if (!map_window_point(window_id, raw_x, raw_y, mapped_x, mapped_y, hit_region)) return;
        xcm::InputEvent e;
        e.kind = xcm::InputKind::MouseWheel;
        e.timestamp_ms = SDL_GetTicks64();
        e.x = mapped_x;
        e.y = mapped_y;
        e.wheel_x = wheel_x;
        e.wheel_y = wheel_y;
        e.hit_region = hit_region;
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

    auto clamp_scroll = [&]() {
        const float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
        scroll_y = std::clamp(scroll_y, 0.f, max_scroll);
    };

    SDL_StartTextInput();

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
        refresh_linked_assets();
        debug_logf("do_reload html=%zu css=%zu js=%zu linked_css=%zu linked_js=%zu", html.size(), css.size(), js.size(), linked_css.size(), linked_js.size());
        write_browser_preview();
        dirty = true;
    };

    auto load_html_file = [&](const std::string& p) -> bool {
        std::string re = load_text_source(p);
        if (re.empty()) return false;
        html = std::move(re);
        html_file = p;
        html_stamp = get_stamp(html_file);
        refresh_linked_assets();
        debug_logf("load_html_file ok: %s html=%zu linked_css=%zu linked_js=%zu", p.c_str(), html.size(), linked_css.size(), linked_js.size());
        dirty = true;
        return true;
    };

    auto load_css_file = [&](const std::string& p) -> bool {
        std::string re = load_text_source(p);
        if (re.empty()) return false;
        css = std::move(re);
        css_file = p;
        css_stamp = get_stamp(css_file);
        debug_logf("load_css_file ok: %s css=%zu", p.c_str(), css.size());
        dirty = true;
        return true;
    };

    auto load_js_file = [&](const std::string& p) -> bool {
        std::string re = load_text_source(p);
        if (re.empty()) return false;
        js = std::move(re);
        js_file = p;
        js_stamp = get_stamp(js_file);
        debug_logf("load_js_file ok: %s js=%zu", p.c_str(), js.size());
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
            status = "Commands: open/html/css/js/browser/reload/watch/cssmode/zoom/diag/diagshow/diaghide/assetclear/inputlog/inputbin/inputclear/quit";
            return;
        }
        if (cmd == "assetclear") {
            g_asset_failures.clear();
            last_asset_failures = 0;
            status = "Asset failures cleared";
            return;
        }
        if (cmd == "diagshow") {
            show_diagnostics = true;
            status = "Diagnostics visible";
            return;
        }
        if (cmd == "diaghide") {
            show_diagnostics = false;
            status = "Diagnostics hidden";
            return;
        }
        if (cmd == "diag") {
            const std::size_t text_chars = count_visible_text_chars(html);
            const std::size_t img_tags = count_tag_occurrences(html, "img");
            const std::size_t script_tags = count_tag_occurrences(html, "script");
            status = "diag html=" + std::to_string(html.size()) +
                     " css=" + std::to_string(css.size()) +
                     " js=" + std::to_string(js.size()) +
                     " lcss=" + std::to_string(linked_css.size()) +
                     " ljs=" + std::to_string(linked_js.size()) +
                     " txt=" + std::to_string(text_chars) +
                     " imgs=" + std::to_string(img_tags) +
                     " scripts=" + std::to_string(script_tags) +
                     " overlays=" + std::to_string(image_overlays.size());
            debug_logf("diag status: %s", status.c_str());
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
                if (ev.wheel.windowID != main_window_id && !is_preview_window_event(ev.wheel.windowID)) continue;
                capture_wheel(ev.wheel.windowID, ev.wheel.x, ev.wheel.y);
                scroll_y -= static_cast<float>(ev.wheel.y * 48);
                clamp_scroll();
                dirty = true;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                if (ev.button.windowID != main_window_id && !is_preview_window_event(ev.button.windowID)) continue;
                int mx = ev.button.x;
                int my = ev.button.y;
                capture_mouse(ev.button.windowID, xcm::InputKind::MouseDown, mx, my, 0, 0, ev.button.button);

                if (is_preview_window_event(ev.button.windowID)) {
                    continue;
                }

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
                        clamp_scroll();
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
                if (ev.button.windowID != main_window_id && !is_preview_window_event(ev.button.windowID)) continue;
                capture_mouse(ev.button.windowID, xcm::InputKind::MouseUp, ev.button.x, ev.button.y, 0, 0, ev.button.button);
                dragging_scroll = false;
            } else if (ev.type == SDL_MOUSEMOTION) {
                if (ev.motion.windowID != main_window_id && !is_preview_window_event(ev.motion.windowID)) continue;
                capture_mouse(ev.motion.windowID, xcm::InputKind::MouseMove, ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel, 0);
                if (is_preview_window_event(ev.motion.windowID)) {
                    continue;
                }
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
                        clamp_scroll();
                        dirty = true;
                    }
                }
            } else if (ev.type == SDL_TEXTINPUT) {
                if (ev.text.windowID != main_window_id && !is_preview_window_event(ev.text.windowID)) continue;
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
                if (ev.key.windowID != main_window_id && !is_preview_window_event(ev.key.windowID)) continue;
                SDL_Keycode k = ev.key.keysym.sym;
                capture_key(xcm::InputKind::KeyDown, k);

                if (palette_mode) {
                    if (k == SDLK_ESCAPE) {
                        palette_mode = false;
                        palette_prompt.clear();
                        palette_input.clear();
                        SDL_StartTextInput();
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
                        SDL_StartTextInput();
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
                } else if (k == SDLK_F1) {
                    show_diagnostics = !show_diagnostics;
                    status = std::string("Diagnostics ") + (show_diagnostics ? "visible" : "hidden");
                    set_title();
                } else if (k == SDLK_DOWN) {
                    scroll_y += 48.f;
                    clamp_scroll();
                    dirty = true;
                } else if (k == SDLK_UP) {
                    scroll_y -= 48.f;
                    clamp_scroll();
                    dirty = true;
                }
            } else if (ev.type == SDL_KEYUP) {
                if (ev.key.windowID != main_window_id && !is_preview_window_event(ev.key.windowID)) continue;
                capture_key(xcm::InputKind::KeyUp, ev.key.keysym.sym);
            }
        }

        if (watch) {
            bool changed_any = false;
            if (!html_file.empty() && changed(html_file, html_stamp)) {
                std::string re = load_text_source(html_file);
                if (!re.empty()) {
                    html = std::move(re);
                    refresh_linked_assets();
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
                if (!use_surface_upload) {
                    SDL_DestroyTexture(tex);
                    tex = create_streaming_texture(ren, tex_w, tex_h, "main");
                    if (!tex) {
                        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
                        running = false;
                        break;
                    }
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
                const std::size_t text_chars = count_visible_text_chars(html_for_render);
                const std::size_t img_tags = count_tag_occurrences(html_for_render, "img");
                const std::size_t script_tags = count_tag_occurrences(html_for_render, "script");
                last_overlays = static_cast<int>(image_overlays.size());
                last_doc_h = doc_h;
                last_text_chars = text_chars;
                last_img_tags = img_tags;
                last_script_tags = script_tags;
                last_asset_failures = g_asset_failures.size();
                const FrameContentStats frame_stats = analyze_frame_content(frame, tex_w, tex_h);
                if (frame_is_mostly_white(frame) && (text_chars > 0 || img_tags > 0)) {
                    record_asset_failure("render", "frame", "", "frame mostly white with non-empty content");
                }
                debug_logf("render_page ok tex=%dx%d doc_h=%d overlays=%zu html=%zu css=%zu text_chars=%zu img_tags=%zu script_tags=%zu",
                           tex_w, tex_h, doc_h, image_overlays.size(), html_for_render.size(), css_for_render.size(), text_chars, img_tags, script_tags);
                debug_logf("frame_content non_white=%zu has=%d bounds=%d,%d..%d,%d",
                           frame_stats.non_white_count,
                           frame_stats.has_non_white ? 1 : 0,
                           frame_stats.min_x,
                           frame_stats.min_y,
                           frame_stats.max_x,
                           frame_stats.max_y);
                if (!logged_js_runtime_warning && (js.size() + linked_js.size()) > 0) {
                    debug_logf("JS_RUNTIME_NOTE software renderer does not execute JS; dynamic DOM text/images may not appear in software output");
                    logged_js_runtime_warning = true;
                }
                float max_scroll = std::max(0.f, static_cast<float>(doc_h - tex_h));
                if (scroll_y > max_scroll) {
                    scroll_y = max_scroll;
                    render_page(tex_w, tex_h, html_for_render, css_for_render, scroll_y, frame, doc_h, &image_overlays);
                }

#ifdef XCM_HAS_SDL_IMAGE
                if (sdl_image_ready) {
                    for (const auto& ov : image_overlays) {
                        if (ov.is_background || ov.w < 2.f || ov.h < 2.f || ov.url.empty()) continue;
                        std::string resolved = resolve_asset_path(ov.url, html_file);
                        if (resolved.empty()) continue;
                        SDL_Surface* surf = load_image_surface(resolved);
                        if (!surf) {
                            debug_logf("image load fail: %s", resolved.c_str());
                            continue;
                        }
                        SDL_Surface* rgba_surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
                        SDL_FreeSurface(surf);
                        if (!rgba_surf) continue;
                        blend_surface_into_rgba(frame, tex_w, tex_h, rgba_surf, ov, scroll_y);
                        SDL_FreeSurface(rgba_surf);
                    }
                }
#endif

                if (use_surface_upload) {
                    if (!replace_texture_from_rgba(ren, tex, tex_w, tex_h, frame, "main")) {
                        running = false;
                        break;
                    }
                } else {
                    SDL_UpdateTexture(tex, nullptr, frame.data(), tex_w * 4);
                }

                if (preview_win && preview_ren) {
                    if (!preview_tex || preview_tex_w != tex_w || preview_tex_h != tex_h) {
                        preview_tex_w = tex_w;
                        preview_tex_h = tex_h;
                        if (!use_surface_upload) {
                            if (preview_tex) SDL_DestroyTexture(preview_tex);
                            preview_tex = create_streaming_texture(preview_ren, preview_tex_w, preview_tex_h, "preview");
                        }
                    }
                    if (use_surface_upload) {
                        if (!replace_texture_from_rgba(preview_ren, preview_tex, tex_w, tex_h, frame, "preview")) {
                            running = false;
                            break;
                        }
                    } else if (preview_tex) {
                        SDL_UpdateTexture(preview_tex, nullptr, frame.data(), tex_w * 4);
                    }
                }
            } else {
                status = "Render failed (parse/layout)";
                debug_logf("render_page failed html=%zu css=%zu", html_for_render.size(), css_for_render.size());
                set_title();
            }
            dirty = false;
        }

        // Draw page content into the left portion of the window only
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_Rect content_dst{0, 0, content_w, win_h};
        SDL_RenderCopy(ren, tex, nullptr, &content_dst);

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
            const int footer_h = show_diagnostics ? 198 : 91;
            const int fy = win_h - footer_h;
            pc_fill({content_w + 1, fy, PANEL_W - 2, footer_h - 1}, {10, 16, 28, 255});
            const std::string i1 = "Zoom: " + std::to_string(static_cast<int>(std::round(zoom * 100.f))) + "%";
            const std::string i2 =
                "Watch:" + std::string(watch ? "on" : "off") +
                " CSS:" + (css_enabled ? "on" : "off") +
                " Cmp:" + (bridge_compat_mode ? "on" : "off") +
                " Img:" + (bridge_sanitize_html ? "on" : "off");
            pc_text(content_w + 8, fy +  4, i1,     {160, 200, 235, 255});
            pc_text(content_w + 8, fy + 24, i2,     {160, 200, 235, 255});
            pc_text(content_w + 8, fy + 46, status, {255, 210, 130, 255});
            if (show_diagnostics) {
                const std::string d1 = "doc=" + std::to_string(last_doc_h) +
                                       " ov=" + std::to_string(last_overlays) +
                                       " txt=" + std::to_string(last_text_chars);
                const std::string d2 = "img=" + std::to_string(last_img_tags) +
                                       " script=" + std::to_string(last_script_tags) +
                                       " fail=" + std::to_string(last_asset_failures);
                pc_text(content_w + 8, fy + 64, d1, {140, 188, 220, 255});
                pc_text(content_w + 8, fy + 80, d2, {140, 188, 220, 255});

                const int fail_lines = std::min<int>(4, static_cast<int>(g_asset_failures.size()));
                for (int i = 0; i < fail_lines; ++i) {
                    const std::string& line = g_asset_failures[g_asset_failures.size() - fail_lines + i];
                    std::string clipped = line;
                    if (clipped.size() > 45) clipped = clipped.substr(0, 45) + "...";
                    pc_text(content_w + 8, fy + 98 + i * 16, clipped, {245, 150, 150, 255});
                }
            }
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
    IMG_Quit();
#endif
    destroy_preview_window();
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    debug_logf("xcm_sdl_preview shutdown");
    debug_close();
    SDL_Quit();
    return 0;
}
