/*
 * xcm_glfw_preview.cpp  --  GLFW/OpenGL native preview app for render_core
 *
 * Usage:
 *   xcm_glfw_preview [html_file] [css_file] [js_file] [--watch] [--gpu=opengl|vulkan|directx|software]
 *
 * Notes:
 *   - OpenGL path is implemented today.
 *   - Vulkan/DirectX selections are accepted and reported, then fall back to OpenGL.
 */

#include "arena.h"
#include "css_parser.h"
#include "html_tokenizer.h"
#include "layout.h"
#include "paint.h"
#include "style_resolver.h"
#include "xcm_gpu_config.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct AppState {
    std::string html_file;
    std::string css_file;
    std::string js_file;

    std::string html;
    std::string css;
    std::string js;

    std::uint64_t html_stamp = 0;
    std::uint64_t css_stamp = 0;
    std::uint64_t js_stamp = 0;

    bool watch = false;
    bool dirty = true;

    float zoom = 1.0f;
    float scroll_y = 0.0f;

    int doc_h = 0;
    int tex_w = 0;
    int tex_h = 0;

    GLuint tex = 0;
    std::vector<uint8_t> frame;

    xcm::GpuBackend requested_backend = xcm::GpuBackend::OpenGL;
    xcm::GpuBackend active_backend = xcm::GpuBackend::OpenGL;
};

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool file_exists(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

std::uint64_t file_stamp(const std::string& path) {
    if (!file_exists(path)) return 0;
    std::error_code ec;
    auto t = std::filesystem::last_write_time(path, ec);
    if (ec) return 0;
    return static_cast<std::uint64_t>(t.time_since_epoch().count());
}

std::string lower_copy(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
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

void set_window_title(GLFWwindow* window, const AppState& app) {
    std::ostringstream ss;
    ss << "XCM GLFW Preview | backend=" << xcm::backend_name(app.active_backend)
       << " | requested=" << xcm::backend_name(app.requested_backend)
       << " | zoom=" << static_cast<int>(app.zoom * 100.0f) << "%"
       << " | watch=" << (app.watch ? "on" : "off");
    glfwSetWindowTitle(window, ss.str().c_str());
}

bool reload_all(AppState& app) {
    if (!app.html_file.empty()) {
        std::string h = read_file(app.html_file);
        if (h.empty()) return false;
        app.html = h;
        app.html_stamp = file_stamp(app.html_file);
    }

    if (!app.css_file.empty()) {
        app.css = read_file(app.css_file);
        app.css_stamp = file_stamp(app.css_file);
    }

    if (!app.js_file.empty()) {
        app.js = read_file(app.js_file);
        app.js_stamp = file_stamp(app.js_file);
    }

    app.scroll_y = 0.0f;
    app.dirty = true;
    return true;
}

void upload_frame(AppState& app) {
    if (app.tex == 0) glGenTextures(1, &app.tex);
    glBindTexture(GL_TEXTURE_2D, app.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (app.tex_w > 0 && app.tex_h > 0) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            app.tex_w,
            app.tex_h,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            app.frame.data());
    }
}

void draw_frame(const AppState& app, int fb_w, int fb_h) {
    glViewport(0, 0, fb_w, fb_h);
    glClearColor(0.06f, 0.08f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, app.tex);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void on_scroll(GLFWwindow* w, double, double yoff) {
    auto* app = static_cast<AppState*>(glfwGetWindowUserPointer(w));
    if (!app) return;

    app->scroll_y -= static_cast<float>(yoff * 48.0);
    app->scroll_y = std::max(0.0f, app->scroll_y);
    app->dirty = true;
}

} // namespace

int main(int argc, char** argv) {
    AppState app;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--watch") {
            app.watch = true;
        } else if (a.rfind("--gpu=", 0) == 0) {
            app.requested_backend = xcm::parse_backend_name(a.substr(6));
        } else if (app.html_file.empty()) {
            app.html_file = a;
        } else if (app.css_file.empty()) {
            app.css_file = a;
        } else if (app.js_file.empty()) {
            app.js_file = a;
        }
    }

    if (app.html_file.empty()) {
        app.html =
            "<!DOCTYPE html><html><head><style>"
            "body{margin:0;padding:18px;background:#0b1220;color:#dff6ff;font-family:monospace;}"
            ".hero{background:#111a2b;border:2px solid #22344f;padding:14px;}"
            "h1{color:#33e6d1;margin:0;font-size:42px;}"
            "p{color:#8bb8d7;font-size:18px;}"
            ".row{display:flex;gap:12px;margin-top:14px;}"
            ".card{flex:1;background:#121d31;border:2px solid #243750;padding:10px;}"
            ".k{color:#91d5ff}.v{color:#ffbe65}"
            "</style></head><body>"
            "<div class='hero'><h1>XCM GLFW PREVIEW</h1><p>OpenGL path for native viewport/input</p></div>"
            "<div class='row'>"
            "<div class='card'><div class='k'>GPU</div><div class='v'>OpenGL active</div></div>"
            "<div class='card'><div class='k'>INPUT</div><div class='v'>GLFW wheel + keys</div></div>"
            "<div class='card'><div class='k'>MODE</div><div class='v'>C/C++ native</div></div>"
            "</div></body></html>";
    } else if (!reload_all(app)) {
        std::cerr << "Failed to load HTML input: " << app.html_file << "\n";
        return 1;
    }

    if (app.requested_backend != xcm::GpuBackend::OpenGL) {
        std::cerr << "Requested backend '" << xcm::backend_name(app.requested_backend)
                  << "' is not wired in this preview binary yet. Falling back to OpenGL.\n";
    }
    app.active_backend = xcm::GpuBackend::OpenGL;

    if (!glfwInit()) {
        std::cerr << "glfwInit failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 760, "XCM GLFW Preview", nullptr, nullptr);
    if (!window) {
        std::cerr << "glfwCreateWindow failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(window, &app);
    glfwSetScrollCallback(window, on_scroll);

    set_window_title(window, app);

    bool prev_r = false;
    bool prev_w = false;
    bool prev_minus = false;
    bool prev_equal = false;
    bool prev_zero = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        bool now_r = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        bool now_w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool now_minus = glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS;
        bool now_equal = glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS;
        bool now_zero = glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (now_r && !prev_r) {
            reload_all(app);
            set_window_title(window, app);
        }
        if (now_w && !prev_w) {
            app.watch = !app.watch;
            set_window_title(window, app);
        }
        if (now_minus && !prev_minus) {
            app.zoom = std::max(0.25f, app.zoom / 1.15f);
            app.dirty = true;
            set_window_title(window, app);
        }
        if (now_equal && !prev_equal) {
            app.zoom = std::min(3.0f, app.zoom * 1.15f);
            app.dirty = true;
            set_window_title(window, app);
        }
        if (now_zero && !prev_zero) {
            app.zoom = 1.0f;
            app.dirty = true;
            set_window_title(window, app);
        }

        prev_r = now_r;
        prev_w = now_w;
        prev_minus = now_minus;
        prev_equal = now_equal;
        prev_zero = now_zero;

        if (app.watch) {
            bool changed = false;
            if (!app.html_file.empty()) {
                std::uint64_t s = file_stamp(app.html_file);
                if (s != 0 && s != app.html_stamp) changed = true;
            }
            if (!app.css_file.empty()) {
                std::uint64_t s = file_stamp(app.css_file);
                if (s != 0 && s != app.css_stamp) changed = true;
            }
            if (!app.js_file.empty()) {
                std::uint64_t s = file_stamp(app.js_file);
                if (s != 0 && s != app.js_stamp) changed = true;
            }
            if (changed) reload_all(app);
        }

        int fb_w = 0;
        int fb_h = 0;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);

        int want_w = std::max(64, static_cast<int>(static_cast<float>(fb_w) / app.zoom));
        int want_h = std::max(64, static_cast<int>(static_cast<float>(fb_h) / app.zoom));

        if (want_w != app.tex_w || want_h != app.tex_h) {
            app.tex_w = want_w;
            app.tex_h = want_h;
            app.dirty = true;
        }

        if (app.dirty) {
            std::string css = app.css;
            if (!app.js.empty()) {
                css += "\n/* js loaded for browser parity flow: " + std::to_string(app.js.size()) + " bytes */\n";
            }

            if (render_page(app.tex_w, app.tex_h, app.html, css, app.scroll_y, app.frame, app.doc_h)) {
                float max_scroll = std::max(0.0f, static_cast<float>(app.doc_h - app.tex_h));
                if (app.scroll_y > max_scroll) app.scroll_y = max_scroll;
                upload_frame(app);
            }
            app.dirty = false;
        }

        draw_frame(app, fb_w, fb_h);
        glfwSwapBuffers(window);
    }

    if (app.tex != 0) {
        glDeleteTextures(1, &app.tex);
        app.tex = 0;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
