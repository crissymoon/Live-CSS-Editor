// main.mm -- imgui-browser entry point (Objective-C++)
// Window: GLFW + OpenGL3 for ImGui chrome
// Content: WKWebView embedded as native NSView subview
//
// Eliminates all Python/PyObjC/PyQt6 overhead.  The only layers are:
//   macOS WebKit (WKWebView)  <-->  ImGui chrome  <-->  OpenGL

#import <Cocoa/Cocoa.h>
#include <mach-o/dyld.h>    // _NSGetExecutablePath
#include <OpenGL/gl3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "app_state.h"
#include "chrome.h"
#include "webview.h"
#include "server_manager.h"
#include "cmd_server.h"

#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Forward declaration
void fps_host_tick(AppState& st, double now_sec);

// ── Arg parsing ───────────────────────────────────────────────────────

struct Args {
    std::string url       = "http://127.0.0.1:8080/pb_admin/login.php";
    std::string apps_dir  = "";
    int         php_port  = 9879;
    int         cmd_port  = 9878;
    int         win_w     = 1400;
    int         win_h     = 900;
};

static Args parse_args(int argc, char** argv) {
    Args a;
    // Derive apps_dir relative to the executable when not specified
    // (assumes binary lives in imgui-browser/build/)
    char exe_path[1024] = {};
    uint32_t sz = sizeof(exe_path);
    _NSGetExecutablePath(exe_path, &sz);
    std::string exe_str(exe_path);
    auto pos = exe_str.rfind('/');
    if (pos != std::string::npos) {
        // build/ -> parent -> apps
        std::string parent = exe_str.substr(0, pos);
        auto p2 = parent.rfind('/');
        if (p2 != std::string::npos) {
            // Go up one more level to find dev-browser/apps relative to imgui-browser
            std::string root = parent.substr(0, p2);
            // root is imgui-browser/
            a.apps_dir = root + "/../dev-browser/apps";
        }
    }

    for (int i = 1; i < argc; i++) {
        std::string s(argv[i]);
        if ((s == "--url") && i+1 < argc)        { a.url       = argv[++i]; continue; }
        if ((s == "--apps-dir") && i+1 < argc)   { a.apps_dir  = argv[++i]; continue; }
        if ((s == "--php-port") && i+1 < argc)   { a.php_port  = atoi(argv[++i]); continue; }
        if ((s == "--cmd-port") && i+1 < argc)   { a.cmd_port  = atoi(argv[++i]); continue; }
        if ((s == "--width") && i+1 < argc)      { a.win_w     = atoi(argv[++i]); continue; }
        if ((s == "--height") && i+1 < argc)     { a.win_h     = atoi(argv[++i]); continue; }
    }
    return a;
}

// ── Globals ───────────────────────────────────────────────────────────

static AppState      g_state;
static GLFWwindow*   g_win      = nullptr;
static int           g_prev_top = 0;   // last chrome top height (logical pts)
static int           g_prev_bot = 0;   // last chrome bottom height (logical pts)
// Physical framebuffer pixels -- only used for glViewport
static int           g_fb_w     = 0;
static int           g_fb_h     = 0;

// ── Content area positioning ──────────────────────────────────────────

static void reposition_webviews(int chrome_top, int chrome_bot, int w, int h) {
    // NSView coordinates are bottom-left; webview_resize() handles the flip.
    int content_y = chrome_top;
    int content_h = h - chrome_top - chrome_bot;
    if (content_h < 1) content_h = 1;

    for (auto& tab : g_state.tabs) {
        if (!tab.wv_handle) continue;
        if (tab.wv_handle) {
            webview_resize(tab.wv_handle, 0, content_y, w, content_h);
            if (&tab == g_state.current_tab())
                webview_show(tab.wv_handle);
            else
                webview_hide(tab.wv_handle);
        }
    }
}

// ── Navigation dispatch (processes cmd_queue) ─────────────────────────

static void dispatch_nav(AppState::NavCmd& cmd) {
    // Resolve tab
    Tab* tab = nullptr;
    if (cmd.tab_id == -1) {
        tab = g_state.current_tab();
    } else if (cmd.tab_id == -2) {
        // New tab request from cmd_server /newtab
        int idx = g_state.new_tab(cmd.url);
        tab = &g_state.tabs[idx];
        // Create WKWebView for the new tab
        tab->wv_handle = webview_create(tab->id, cmd.url);
        reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
        return;
    } else if (cmd.tab_id == -3) {
        // Eval request: "__eval__:<js>"
        tab = g_state.current_tab();
        if (tab && tab->wv_handle && cmd.url.substr(0, 8) == "__eval__") {
            webview_eval_js(tab->wv_handle, cmd.url.substr(8), nullptr);
        }
        return;
    } else {
        for (auto& t : g_state.tabs)
            if (t.id == cmd.tab_id) { tab = &t; break; }
    }

    if (!tab) return;
    void* h = tab->wv_handle;
    if (!h) return;

    if      (cmd.url == "__back__")    webview_go_back(h);
    else if (cmd.url == "__forward__") webview_go_forward(h);
    else if (cmd.url == "__reload__")  webview_reload(h);
    else if (cmd.url == "__stop__")    webview_stop(h);
    else                               webview_load_url(h, cmd.url);
}

// ── GLFW callbacks ────────────────────────────────────────────────────

// Called by GLFW with LOGICAL POINT dimensions -- used for WKWebView and ImGui.
static void cb_window_size(GLFWwindow*, int w, int h) {
    g_state.win_w = w;
    g_state.win_h = h;
    reposition_webviews(g_prev_top, g_prev_bot, w, h);
}

// Called by GLFW with PHYSICAL PIXEL dimensions -- used only for glViewport.
static void cb_framebuffer_size(GLFWwindow*, int w, int h) {
    g_fb_w = w;
    g_fb_h = h;
}

static void cb_error(int, const char* desc) {
    fprintf(stderr, "[glfw] Error: %s\n", desc);
}

// ── Main ──────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    // NSApplication needs to be running on the main thread
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    // ── App menu bar ─────────────────────────────────────────────────
    // Without a proper Edit menu, Cmd+C/V/X/Z still work in ImGui via
    // io.ConfigMacOSXBehaviors, but WKWebView also benefits from having
    // first-responder Edit actions in the responder chain.
    {
        NSMenu* menubar = [[NSMenu alloc] init];

        // Application menu
        NSMenuItem* appItem = [[NSMenuItem alloc] init];
        [menubar addItem:appItem];
        NSMenu* appMenu = [[NSMenu alloc] init];
        [appMenu addItemWithTitle:@"Quit imgui-browser"
                          action:@selector(terminate:)
                   keyEquivalent:@"q"];
        [appItem setSubmenu:appMenu];

        // Edit menu -- gives WKWebView a proper responder-chain target for
        // copy/paste/cut/select-all so Cmd+C etc. work in web text inputs.
        NSMenuItem* editItem = [[NSMenuItem alloc] initWithTitle:@"Edit"
                                                          action:nil
                                                   keyEquivalent:@""];
        [menubar addItem:editItem];
        NSMenu* editMenu  = [[NSMenu alloc] initWithTitle:@"Edit"];
        [editMenu addItemWithTitle:@"Undo"  action:@selector(undo:)  keyEquivalent:@"z"];
        [editMenu addItemWithTitle:@"Redo"  action:@selector(redo:)  keyEquivalent:@"Z"];
        [editMenu addItem:[NSMenuItem separatorItem]];
        [editMenu addItemWithTitle:@"Cut"   action:@selector(cut:)   keyEquivalent:@"x"];
        [editMenu addItemWithTitle:@"Copy"  action:@selector(copy:)  keyEquivalent:@"c"];
        [editMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
        [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
        [editItem setSubmenu:editMenu];

        [NSApp setMainMenu:menubar];
    }

    Args args = parse_args(argc, argv);

    // Initial window state
    g_state.win_w = args.win_w;
    g_state.win_h = args.win_h;

    // ── GLFW init ────────────────────────────────────────────────────
    glfwSetErrorCallback(cb_error);
    if (!glfwInit()) {
        fprintf(stderr, "[main] glfwInit failed\n");
        return 1;
    }

    // OpenGL 3.2 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // Retina support
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    // Transparent chrome regions
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

    g_win = glfwCreateWindow(args.win_w, args.win_h, "Crissy's Style Tool", nullptr, nullptr);
    if (!g_win) {
        fprintf(stderr, "[main] glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(g_win);
    glfwSwapInterval(1);  // vsync ON
    // Logical point size -- drives WKWebView frame and ImGui display size.
    glfwSetWindowSizeCallback(g_win, cb_window_size);
    // Physical pixel size -- drives glViewport only.
    glfwSetFramebufferSizeCallback(g_win, cb_framebuffer_size);

    // Seed both dimension pairs before the first frame.
    int _lw, _lh;
    glfwGetWindowSize(g_win, &_lw, &_lh);
    g_state.win_w = _lw;
    g_state.win_h = _lh;
    glfwGetFramebufferSize(g_win, &g_fb_w, &g_fb_h);
    g_state.dpi_scale = (float)g_fb_w / (float)_lw;

    // ── Dear ImGui ───────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename  = nullptr;  // no imgui.ini persistence
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Map Cmd+C/V/X/Z/A to clipboard and undo on macOS (instead of Ctrl).
    io.ConfigMacOSXBehaviors = true;
    // Disable ImGui's per-frame glfwSetCursor call. Without this, ImGui resets
    // the cursor to Arrow every frame, which overrides WKWebView's NSTrackingArea
    // cursor changes (IBeam over text inputs, pointer over links, etc.).
    // We manage the cursor manually below in the render loop.
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Load font - will use default if JetBrains Mono not present
    // (add font loading here if you bundle a TTF)

    chrome_init(&g_state);

    ImGui_ImplGlfw_InitForOpenGL(g_win, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // ── Get NSWindow and init WKWebView subsystem ────────────────────
    NSWindow* ns_win = glfwGetCocoaWindow(g_win);

    WebViewCallbacks wv_cbs;
    wv_cbs.on_url_change = [](int tab_id, const std::string& url) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.url = url; break; }
    };
    wv_cbs.on_title_change = [](int tab_id, const std::string& title) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.title = title; break; }
    };
    wv_cbs.on_progress = [](int tab_id, float p) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.progress = p; break; }
    };
    wv_cbs.on_loading = [](int tab_id, bool loading) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.loading = loading; break; }
    };
    wv_cbs.on_nav_state = [](int tab_id, bool back, bool fwd) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.can_back = back; t.can_forward = fwd; break; }
    };
    wv_cbs.on_wkwv_fps = [](double fps) {
        g_state.fps_wkwv = fps;
    };

    // ── NSWindow title bar theming ────────────────────────────────────
    // Makes the GLFW NSWindow full-content so our dark ImGui chrome fills
    // the entire top area. macOS traffic lights stay visible but sit directly
    // on our dark surface with no separate title bar strip.
    ns_win.titlebarAppearsTransparent = YES;
    ns_win.titleVisibility            = NSWindowTitleHidden;
    ns_win.styleMask |= NSWindowStyleMaskFullSizeContentView;
    [ns_win setTitle:@"Crissy's Style Tool"];

    webview_init((__bridge void*)ns_win, &g_state, wv_cbs);

    // ── Initial tab ──────────────────────────────────────────────────
    {
        int idx = g_state.new_tab(args.url);
        g_state.tabs[idx].wv_handle = webview_create(g_state.tabs[idx].id, args.url);
    }

    // ── Server processes ─────────────────────────────────────────────
    if (!args.apps_dir.empty()) {
        server_start_php(args.apps_dir, args.php_port);
    }
    {
        // Node image-cache server (path relative to apps_dir's parent)
        std::string node_script;
        auto sl = args.apps_dir.rfind('/');
        if (sl != std::string::npos) {
            node_script = args.apps_dir.substr(0, sl) + "/src/image-cache-server.js";
        }
        server_start_node(node_script);
    }

    // ── Command API ───────────────────────────────────────────────────
    cmd_server_start(&g_state, args.cmd_port);

    // ── Server status poll: every 3 seconds on a cheap timer ─────────
    double last_server_poll = 0.0;

    // ── Render loop ───────────────────────────────────────────────────
    while (!glfwWindowShouldClose(g_win)) {
        // Process macOS events (needed for WKWebView / NSRunLoop)
        @autoreleasepool {
            NSEvent* ev;
            while ((ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:nil
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES])) {
                [NSApp sendEvent:ev];
            }
        }

        glfwPollEvents();

        double now = glfwGetTime();
        fps_host_tick(g_state, now);

        // Poll server status every 3 seconds
        if (now - last_server_poll > 3.0) {
            auto ss = server_poll_status(args.php_port, 7779);
            g_state.php_server_ok  = ss.php_ok;
            g_state.node_server_ok = ss.node_ok;
            last_server_poll = now;
        }

        // ── Keyboard shortcuts ────────────────────────────────────────
        // io.KeySuper = macOS Cmd key (via io.ConfigMacOSXBehaviors).
        // We fire these once per key-down (false = no repeat for tab switches).
        {
            ImGuiIO& kio = ImGui::GetIO();
            bool cmd = kio.KeySuper;

            if (cmd && !kio.WantTextInput) {
                // Cmd+L or Cmd+D -- focus URL bar
                if (ImGui::IsKeyPressed(ImGuiKey_L, false) ||
                    ImGui::IsKeyPressed(ImGuiKey_D, false)) {
                    g_state.focus_url_next_frame = true;
                }
                // Cmd+T -- new tab
                if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
                    std::string nt_url = "http://127.0.0.1:" +
                                         std::to_string(args.php_port) + "/";
                    int idx = g_state.new_tab(nt_url);
                    g_state.tabs[idx].wv_handle =
                        webview_create(g_state.tabs[idx].id, nt_url);
                }
                // Cmd+W -- close current tab (no close if only one left)
                if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
                    int ci = g_state.active_tab;
                    if ((int)g_state.tabs.size() > 1 &&
                        ci < (int)g_state.tabs.size()) {
                        webview_destroy(g_state.tabs[ci].wv_handle);
                        g_state.tabs[ci].wv_handle = nullptr;
                        g_state.close_tab(ci);
                    }
                }
                // Cmd+R -- reload active tab
                if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__reload__");
                }
                // Cmd+[ -- back
                if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__back__");
                }
                // Cmd+] -- forward
                if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__forward__");
                }
                // Cmd+1..9 -- jump to tab by position
                for (int ki = 0; ki < 9 && ki < (int)g_state.tabs.size(); ki++) {
                    ImGuiKey kn = (ImGuiKey)((int)ImGuiKey_1 + ki);
                    if (ImGui::IsKeyPressed(kn, false))
                        g_state.active_tab = ki;
                }
            }
        }

        // Process command queue
        AppState::NavCmd cmd;
        while (g_state.pop_nav(cmd)) {
            dispatch_nav(cmd);
        }

        // ── Dear ImGui frame ─────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Chrome positions are in LOGICAL POINTS (same as glfwGetWindowSize).
        // ImGui_ImplGlfw sets io.DisplaySize to logical points and
        // io.DisplayFramebufferScale to the pixel ratio; the OpenGL3 renderer
        // applies the scale when building the draw commands, so all ImGui
        // SetNextWindowPos/Size calls use logical points.
        int ww = g_state.win_w;
        int wh = g_state.win_h;

        bool new_tab_req = false;
        int  close_tab   = -1;
        int  chrome_top  = chrome_draw_top(&g_state, ww, wh,
                                           new_tab_req, close_tab);
        int  chrome_bot  = chrome_draw_bottom(&g_state, ww, wh);

        // Handle tab actions
        if (new_tab_req) {
            std::string nt_url = "http://127.0.0.1:" + std::to_string(args.php_port) + "/";
            int idx = g_state.new_tab(nt_url);
            g_state.tabs[idx].wv_handle = webview_create(g_state.tabs[idx].id, nt_url);
        }
        if (close_tab >= 0 && close_tab < (int)g_state.tabs.size()) {
            webview_destroy(g_state.tabs[close_tab].wv_handle);
            g_state.tabs[close_tab].wv_handle = nullptr;
            g_state.close_tab(close_tab);
        }

        // Reposition content views if chrome height changed.
        // All values here are logical points -- WKWebView NSView uses points.
        if (chrome_top != g_prev_top || chrome_bot != g_prev_bot) {
            g_prev_top = chrome_top;
            g_prev_bot = chrome_bot;
            reposition_webviews(chrome_top, chrome_bot, ww, wh);
        }

        // Always maintain correct visibility for active vs inactive tabs
        for (int i = 0; i < (int)g_state.tabs.size(); i++) {
            void* h = g_state.tabs[i].wv_handle;
            if (!h) continue;
            if (i == g_state.active_tab) webview_show(h);
            else                         webview_hide(h);
        }

        ImGui::Render();

        // ── Cursor management ─────────────────────────────────────────
        // ImGuiConfigFlags_NoMouseCursorChange stops ImGui from calling
        // glfwSetCursor every frame. We only touch the cursor when the mouse
        // is over the ImGui chrome (top bar or bottom bar). When the mouse is
        // in the content area we do nothing, so WKWebView's NSTrackingArea
        // can set IBeam over text inputs, pointer over links, etc. freely.
        {
            double mx, my;
            glfwGetCursorPos(g_win, &mx, &my);
            bool over_chrome = (my < (double)g_prev_top) ||
                               (my > (double)(g_state.win_h - g_prev_bot));
            if (over_chrome) {
                ImGuiMouseCursor want = ImGui::GetMouseCursor();
                if (want == ImGuiMouseCursor_TextInput)
                    [[NSCursor IBeamCursor] set];
                else if (want == ImGuiMouseCursor_Hand)
                    [[NSCursor pointingHandCursor] set];
                else if (want == ImGuiMouseCursor_ResizeEW)
                    [[NSCursor resizeLeftRightCursor] set];
                else if (want == ImGuiMouseCursor_ResizeNS)
                    [[NSCursor resizeUpDownCursor] set];
                else if (want == ImGuiMouseCursor_ResizeAll)
                    [[NSCursor crosshairCursor] set];
                else if (want == ImGuiMouseCursor_NotAllowed)
                    [[NSCursor operationNotAllowedCursor] set];
                else
                    [[NSCursor arrowCursor] set];
            }
            // No else: content area cursor is owned by WKWebView.
        }

        // glViewport uses PHYSICAL PIXELS.
        glViewport(0, 0, g_fb_w, g_fb_h);
        // Transparent clear so WKWebView beneath shows through
        glClearColor(0.047f, 0.047f, 0.063f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw ImGui only over the chrome regions (top + bottom bars)
        // The center content area is left transparent so WKWebView is visible.
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(g_win);
    }

    // ── Cleanup ───────────────────────────────────────────────────────
    cmd_server_stop();
    server_shutdown();

    for (auto& tab : g_state.tabs)
        if (tab.wv_handle) webview_destroy(tab.wv_handle);
    webview_shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(g_win);
    glfwTerminate();
    return 0;
}
