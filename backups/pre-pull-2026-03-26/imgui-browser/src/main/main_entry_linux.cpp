// main_entry_linux.cpp -- Application entry point for Linux.
//
// Mirrors src/main.mm but replaces all Obj-C / Cocoa calls with:
//   - platform_* calls (platform_linux.cpp)
//   - POSIX / standard C++ equivalents
//   - Direct GLFW / Dear ImGui / OpenGL calls
//
// macOS-only subsystems that have no Linux equivalent (NSWindow appearance,
// window drag monitor, logo NSImage loading) are omitted or replaced with
// GLFW equivalents.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include <GL/gl.h>
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../app_state.h"
#include "../webview.h"
#include "../persistence.h"
#include "../platform/platform.h"
#include "main_args.h"
#include "main_globals.h"
#include "main_funcs.h"
#include "main_crash.h"
#include "../cmds-and-server/server_manager.h"
#include "../cmds-and-server/cmd_server.h"
#include "../top-of-gui/chrome.h"

int main(int argc, char** argv)
{
    // ── Crash / debug dir setup ───────────────────────────────────────────
    {
        char exe[PATH_MAX] = {};
        platform_exe_path(exe, sizeof(exe));
        std::string ep(exe);
        auto sl = ep.rfind('/');
        std::string bin_dir = (sl != std::string::npos) ? ep.substr(0, sl) : ".";
        // Assume project root is one level above the build directory.
        auto sl2 = bin_dir.rfind('/');
        std::string proj_dir = (sl2 != std::string::npos) ? bin_dir.substr(0, sl2) : bin_dir;
        snprintf(g_debug_dir, sizeof(g_debug_dir), "%s/debug", proj_dir.c_str());
        snprintf(g_crash_log, sizeof(g_crash_log), "%s/crash.log", g_debug_dir);
        mkdir(g_debug_dir, 0755);
        fprintf(stderr, "[main] debug dir: %s\n", g_debug_dir);
    }

    xcm_install_signal_handlers();

    platform_app_preinit();

    Args args = parse_args(argc, argv);
    g_php_port    = args.php_port;
    g_state.win_w = args.win_w;
    g_state.win_h = args.win_h;

    // ── GLFW ──────────────────────────────────────────────────────────────
    glfwSetErrorCallback(xcm_cb_error);
    if (!glfwInit()) { fprintf(stderr, "[main] glfwInit failed\n"); return 1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

    g_win = glfwCreateWindow(args.win_w, args.win_h, "Crissy's Style Tool", nullptr, nullptr);
    if (!g_win) {
        fprintf(stderr, "[main] glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(g_win);
    glfwSwapInterval(1);

    platform_app_postinit(g_win);
    platform_menu_init();

    // ── GLFW callbacks ────────────────────────────────────────────────────
    glfwSetWindowSizeCallback(g_win, xcm_cb_window_size);
    glfwSetFramebufferSizeCallback(g_win, xcm_cb_framebuffer_size);
    glfwSetWindowRefreshCallback(g_win, xcm_cb_window_refresh);
    glfwSetWindowContentScaleCallback(g_win, [](GLFWwindow*, float xscale, float) {
        g_state.dpi_scale = xscale;
        g_resize_dirty    = true;
    });
    glfwSetWindowIconifyCallback(g_win, [](GLFWwindow*, int iconified) {
        g_win_iconified = (iconified != 0);
    });

    int _lw, _lh;
    glfwGetWindowSize(g_win, &_lw, &_lh);
    g_state.win_w = _lw;
    g_state.win_h = _lh;
    glfwGetFramebufferSize(g_win, &g_fb_w, &g_fb_h);
    g_state.dpi_scale = (_lw > 0) ? (float)g_fb_w / (float)_lw : 1.0f;

    // ── Dear ImGui ────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename  = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Font: try common Linux monospace fonts in order.
    float font_size = 15.0f * g_state.dpi_scale;
    const char* font_candidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
        nullptr
    };
    for (int i = 0; font_candidates[i]; ++i) {
        if (access(font_candidates[i], R_OK) == 0) {
            io.Fonts->AddFontFromFileTTF(font_candidates[i], font_size);
            fprintf(stderr, "[ui] loaded font: %s @ %.0fpx\n", font_candidates[i], font_size);
            break;
        }
    }
    io.FontGlobalScale = (g_state.dpi_scale > 0.0f) ? 1.0f / g_state.dpi_scale : 1.0f;

    chrome_init(&g_state);
    g_state.history   = persist_load_history();
    g_state.bookmarks = persist_load_bookmarks();
    fprintf(stderr, "[main] loaded %zu history entries, %zu bookmarks\n",
            g_state.history.size(), g_state.bookmarks.size());

    ImGui_ImplGlfw_InitForOpenGL(g_win, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // ── Native chrome (ImGui-based on Linux) ──────────────────────────────
    platform_chrome_create(g_win, &g_state, args.php_port);

    // ── WebView init ──────────────────────────────────────────────────────
    WebViewCallbacks wv_cbs = build_webview_callbacks(args);
    webview_init(g_win, &g_state, wv_cbs);

    if (!args.apps_dir.empty()) {
        std::string rules_path = args.apps_dir + "/../src/adblock-rules.json";
        FILE* f = fopen(rules_path.c_str(), "r");
        if (f) {
            fseek(f, 0, SEEK_END); long fsz = ftell(f); rewind(f);
            std::string rules(fsz, '\0');
            fread(&rules[0], 1, fsz, f); fclose(f);
            webview_load_adblock(rules);
            fprintf(stderr, "[main] adblock rules loaded (%ld bytes)\n", fsz);
        }
    }

    if (args.clear_data) webview_clear_data();

    // ── Servers ───────────────────────────────────────────────────────────
    start_all_servers(args);
    cmd_server_start(&g_state, args.cmd_port);

    // ── Initial tab ───────────────────────────────────────────────────────
    {
        int idx = g_state.new_tab(args.url);
        g_state.tabs[idx].wv_handle = webview_create(g_state.tabs[idx].id, args.url);
    }

    // ── Render loop ───────────────────────────────────────────────────────
    main_render_loop(args);

    // ── Cleanup ───────────────────────────────────────────────────────────
    cmd_server_stop();
    server_shutdown();
    for (auto& tab : g_state.tabs)
        if (tab.wv_handle) webview_destroy(tab.wv_handle);
    webview_shutdown();
    platform_chrome_destroy();
    platform_app_cleanup();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(g_win);
    glfwTerminate();
    return 0;
}
