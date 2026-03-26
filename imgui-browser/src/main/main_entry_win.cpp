// main_entry_win.cpp -- Application entry point for Windows.
//
// Mirrors src/main.mm but replaces all Obj-C / Cocoa calls with:
//   - platform_* calls (platform_win.cpp)
//   - Win32 / standard C++ equivalents
//   - Direct GLFW / Dear ImGui / OpenGL calls
//
// WebView2 initialisation is async (see webview_win.cpp).
// The initial tab navigation is deferred until the controller is ready.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>   // CoInitializeEx / CoUninitialize
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

#include <GL/gl.h>
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
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

// Windows requires WinMain or main; GLFW defines a wrapper that calls main()
// when GLFW_INCLUDE_NONE is defined and the project is set to a Windows subsystem.
// Using plain main() here works for both console and Windows subsystem builds.
int main(int argc, char** argv)
{
    platform_app_preinit();   // SetProcessDpiAwarenessContext

    // ── Crash / debug dir setup ───────────────────────────────────────────
    {
        char exe[MAX_PATH] = {};
        platform_exe_path(exe, sizeof(exe));
        std::string ep(exe);
        auto sl = ep.rfind('\\');
        std::string bin_dir = (sl != std::string::npos) ? ep.substr(0, sl) : ".";
        auto sl2 = bin_dir.rfind('\\');
        std::string proj_dir = (sl2 != std::string::npos) ? bin_dir.substr(0, sl2) : bin_dir;
        snprintf(g_debug_dir, sizeof(g_debug_dir), "%s\\debug", proj_dir.c_str());
        snprintf(g_crash_log, sizeof(g_crash_log), "%s\\crash.log", g_debug_dir);
        CreateDirectoryA(g_debug_dir, nullptr);
        fprintf(stderr, "[main] debug dir: %s\n", g_debug_dir);
    }

    xcm_install_signal_handlers();

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
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

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

    // Font: try Consolas (ships with Windows) then fall back to ImGui default.
    float font_size = 15.0f * g_state.dpi_scale;
    const char* font_candidates[] = {
        "C:\\Windows\\Fonts\\consola.ttf",
        "C:\\Windows\\Fonts\\cour.ttf",
        nullptr
    };
    for (int i = 0; font_candidates[i]; ++i) {
        if (GetFileAttributesA(font_candidates[i]) != INVALID_FILE_ATTRIBUTES) {
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

    // ── Native chrome (ImGui-based on Windows) ────────────────────────────
    platform_chrome_create(g_win, &g_state, args.php_port);

    // ── WebView init ──────────────────────────────────────────────────────
    // COM must be initialised on the main thread before WebView2 can create
    // its environment.  S_FALSE means it was already init'd; that's fine.
    HRESULT hr_com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr_com) && hr_com != RPC_E_CHANGED_MODE)
        fprintf(stderr, "[main] CoInitializeEx failed: 0x%08lx\n", hr_com);
    WebViewCallbacks wv_cbs = build_webview_callbacks(args);
    webview_init(g_win, &g_state, wv_cbs);

    if (!args.apps_dir.empty()) {
        std::string rules_path = args.apps_dir + "\\..\\src\\adblock-rules.json";
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
    CoUninitialize();
    return 0;
}
