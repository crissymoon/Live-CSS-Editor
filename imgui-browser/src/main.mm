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
#include "native_chrome.h"
#include "webview.h"
#include "server_manager.h"
#include "cmd_server.h"
#include "virt_overlay.h"

#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/stat.h>

// ── Crash / signal handling ──────────────────────────────────────────

static char g_debug_dir[1024] = {};
static char g_crash_log[1024] = {};

static void xcm_write_crash(const char* header, const char* detail) {
    // Write to both stderr and the crash log file.
    fprintf(stderr, "\n[CRASH] %s\n%s\n", header, detail);
    fflush(stderr);
    if (g_crash_log[0]) {
        FILE* f = fopen(g_crash_log, "a");
        if (f) {
            time_t t = time(nullptr);
            char tbuf[64];
            strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&t));
            fprintf(f, "========================================\n");
            fprintf(f, "[%s] CRASH: %s\n", tbuf, header);
            fprintf(f, "%s\n", detail);

            // backtrace
            void* frames[64];
            int   nframes = backtrace(frames, 64);
            char** syms   = backtrace_symbols(frames, nframes);
            fprintf(f, "--- backtrace (%d frames) ---\n", nframes);
            for (int i = 0; i < nframes; i++) {
                fprintf(f, "  %s\n", syms ? syms[i] : "(unavailable)");
            }
            if (syms) free(syms);
            fprintf(f, "========================================\n");
            fclose(f);
        }
    }
}

static void xcm_signal_handler(int sig) {
    const char* name = "UNKNOWN";
    switch (sig) {
        case SIGABRT: name = "SIGABRT (abort/assertion)"; break;
        case SIGSEGV: name = "SIGSEGV (segmentation fault)"; break;
        case SIGBUS:  name = "SIGBUS (bus error)"; break;
        case SIGILL:  name = "SIGILL (illegal instruction)"; break;
        case SIGFPE:  name = "SIGFPE (floating point exception)"; break;
        case SIGPIPE: name = "SIGPIPE (broken pipe -- ignored)"; return;  // not fatal
    }
    xcm_write_crash(name, "see backtrace above");
    // Re-raise with the default handler so macOS can generate a .crash report.
    signal(sig, SIG_DFL);
    raise(sig);
}

static void xcm_objc_exception_handler(NSException* e) {
    NSString* msg = [NSString stringWithFormat:@"name=%@ reason=%@\nstack=\n%@",
                     e.name, e.reason,
                     e.callStackSymbols ? [e.callStackSymbols componentsJoinedByString:@"\n"] : @"(none)"];
    xcm_write_crash("Uncaught ObjC exception", msg.UTF8String);
}

// Forward declaration
void fps_host_tick(AppState& st, double now_sec);

// ── Arg parsing ───────────────────────────────────────────────────────

struct Args {
    std::string url        = "https://localhost:8443/pb_admin/dashboard.php";
    std::string apps_dir   = "";
    int         php_port   = 9879;
    int         cmd_port   = 9878;
    int         win_w      = 1400;
    int         win_h      = 900;
    bool        clear_data = false;  // --clear-data: flush cookies/cache on startup
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
        if (s == "--clear-data")                  { a.clear_data = true; continue; }
    }
    return a;
}

// ── Globals ───────────────────────────────────────────────────────────

static AppState      g_state;
static GLFWwindow*   g_win      = nullptr;
static int           g_prev_top = 0;   // last chrome top height (logical pts)
static int           g_prev_bot = 0;   // last chrome bottom height (logical pts)
static int           g_php_port = 9879; // set from args after parse_args
// Physical framebuffer pixels -- only used for glViewport
static int           g_fb_w     = 0;
static int           g_fb_h     = 0;
// Set true by window/framebuffer size callbacks; consumed once per frame so
// WKWebView repositioning happens exactly once per resize step, not twice
// (once from cb_window_size and once from cb_framebuffer_size).
static bool          g_resize_dirty = false;
// Set true each frame when any ImGui item is hovered in the chrome zone.
// Used by the drag-bar NSEvent monitor to decide whether to let the OS
// drag the window vs. letting ImGui handle the click (tab, button, URL bar).
static bool          g_chrome_has_hover = false;

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
    // tab_id -5: close tab -- url is the tab .id as a decimal string
    } else if (cmd.tab_id == -5) {
        int tid = 0;
        try { tid = std::stoi(cmd.url); } catch (...) {}
        for (int i = 0; i < (int)g_state.tabs.size(); i++) {
            if (g_state.tabs[i].id == tid) {
                webview_destroy(g_state.tabs[i].wv_handle);
                g_state.tabs[i].wv_handle = nullptr;
                g_state.close_tab(i);
                reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
                return;
            }
        }
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
    else if (cmd.url == "__js_on__")  { tab->js_enabled = true;  webview_set_js_enabled(h, true); }
    else if (cmd.url == "__js_off__") { tab->js_enabled = false; webview_set_js_enabled(h, false); }
    else if (cmd.url == "__bookmark_toggle__") {
        // Toggle bookmark for the current URL
        bool found = false;
        for (int bi = 0; bi < (int)g_state.bookmarks.size(); bi++) {
            if (g_state.bookmarks[bi].url == tab->url) {
                g_state.bookmarks.erase(g_state.bookmarks.begin() + bi);
                found = true;
                break;
            }
        }
        if (!found)
            g_state.bookmarks.push_back({tab->url, tab->title});
        persist_save_bookmarks(g_state.bookmarks);
    }
    else if (cmd.url == "__devtools__") {
        g_state.dev_tools_open = !g_state.dev_tools_open;
        webview_open_inspector(h);
    }
    else                               webview_load_url(h, cmd.url);
}

// ── GLFW callbacks ────────────────────────────────────────────────────

// Called by GLFW with LOGICAL POINT dimensions -- used for WKWebView and ImGui.
static void cb_window_size(GLFWwindow*, int w, int h) {
    g_state.win_w  = w;
    g_state.win_h  = h;
    g_resize_dirty = true;
}

// Called by GLFW with PHYSICAL PIXEL dimensions -- used only for glViewport.
// Also fires when the window moves to a display with a different DPI.
static void cb_framebuffer_size(GLFWwindow*, int w, int h) {
    g_fb_w = w;
    g_fb_h = h;
    if (g_state.win_w > 0)
        g_state.dpi_scale = (float)w / (float)g_state.win_w;
    g_resize_dirty = true;
}

// Called by GLFW when the window content needs to be redrawn, which on macOS
// fires during the live-resize modal tracking loop that blocks glfwPollEvents.
// Without this callback the render loop is frozen during resize dragging and
// the window goes black or shows a stretched stale frame.
static void cb_window_refresh(GLFWwindow*) {
    // Flush any pending resize state immediately.
    if (g_resize_dirty) {
        native_chrome_resize(g_state.win_w, g_state.win_h);
        reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
        g_resize_dirty = false;
    }
    // Push a clear frame so the window does not go black while dragging.
    glViewport(0, 0, g_fb_w, g_fb_h);
    glClearColor(0.047f, 0.047f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(g_win);
}

static void cb_error(int, const char* desc) {
    fprintf(stderr, "[glfw] Error: %s\n", desc);
}
// ── Tools menu action handler ──────────────────────────────────────────
// Lightweight ObjC target wired to Tools menu items.
// Lives at file scope so it can reach g_state, g_php_port, etc.

@interface XCMMenuActions : NSObject
- (void)openPDFSign:(id)sender;
@end

@implementation XCMMenuActions
- (void)openPDFSign:(id)sender {
    dispatch_async(dispatch_get_main_queue(), ^{
        std::string url = "http://127.0.0.1:"
                        + std::to_string(g_php_port)
                        + "/pdf-sign/";
        int idx = g_state.new_tab(url);
        g_state.tabs[(size_t)idx].wv_handle =
            webview_create(g_state.tabs[(size_t)idx].id, url);
        reposition_webviews(g_prev_top, g_prev_bot,
                            g_state.win_w, g_state.win_h);
    });
}
@end
// ── Main ──────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    // ── Debug / crash log setup ───────────────────────────────────────
    // Resolve the debug directory relative to the executable so it works
    // both from run.sh and when launched from the .app bundle.
    {
        char exe[1024] = {};
        uint32_t sz = sizeof(exe);
        _NSGetExecutablePath(exe, &sz);
        std::string ep(exe);
        auto sl = ep.rfind('/');
        std::string bin_dir = (sl != std::string::npos) ? ep.substr(0, sl) : ".";
        // build/ -> imgui-browser/
        std::string proj_dir = bin_dir;
        auto sl2 = bin_dir.rfind('/');
        if (sl2 != std::string::npos) proj_dir = bin_dir.substr(0, sl2);
        // If inside an .app bundle (MacOS/), go up three more levels
        if (proj_dir.find("Contents/MacOS") != std::string::npos ||
            proj_dir.find(".app") != std::string::npos) {
            for (int up = 0; up < 3; up++) {
                auto s = proj_dir.rfind('/');
                if (s != std::string::npos) proj_dir = proj_dir.substr(0, s);
            }
        }
        snprintf(g_debug_dir, sizeof(g_debug_dir), "%s/debug", proj_dir.c_str());
        snprintf(g_crash_log, sizeof(g_crash_log), "%s/crash.log", g_debug_dir);
        // Create the directory if it does not exist (mkdir -p equivalent)
        mkdir(g_debug_dir, 0755);
        fprintf(stderr, "[main] debug dir: %s\n", g_debug_dir);
    }

    // Install signal handlers for common fatal signals.
    // SIGPIPE is non-fatal (broken pipe from a dead server connection).
    signal(SIGABRT, xcm_signal_handler);
    signal(SIGSEGV, xcm_signal_handler);
    signal(SIGBUS,  xcm_signal_handler);
    signal(SIGILL,  xcm_signal_handler);
    signal(SIGFPE,  xcm_signal_handler);
    signal(SIGPIPE, xcm_signal_handler);  // returns without raising

    // Install ObjC uncaught exception handler.
    NSSetUncaughtExceptionHandler(xcm_objc_exception_handler);

    // NSApplication needs to be running on the main thread
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    Args args = parse_args(argc, argv);
    g_php_port = args.php_port;

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

    // ── App menu bar ─────────────────────────────────────────────────
    // Installed AFTER glfwInit/glfwCreateWindow because GLFW replaces the
    // NSApp main menu during its Cocoa initialisation. Setting the menu
    // here ensures our version is the last one written.
    {
        NSMenu* menubar = [[NSMenu alloc] init];

        // Application menu
        NSMenuItem* appItem = [[NSMenuItem alloc] init];
        [menubar addItem:appItem];
        NSMenu* appMenu = [[NSMenu alloc] init];
        [appMenu addItemWithTitle:@"Quit"
                          action:@selector(terminate:)
                   keyEquivalent:@"q"];
        [appItem setSubmenu:appMenu];

        // Edit menu -- gives WKWebView a proper responder-chain target for
        // copy/paste/cut/select-all so Cmd+C etc. work in web text inputs.
        NSMenuItem* editItem = [[NSMenuItem alloc] initWithTitle:@"Edit"
                                                          action:nil
                                                   keyEquivalent:@""];
        [menubar addItem:editItem];
        NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
        [editMenu addItemWithTitle:@"Undo"  action:@selector(undo:)  keyEquivalent:@"z"];
        [editMenu addItemWithTitle:@"Redo"  action:@selector(redo:)  keyEquivalent:@"Z"];
        [editMenu addItem:[NSMenuItem separatorItem]];
        [editMenu addItemWithTitle:@"Cut"        action:@selector(cut:)       keyEquivalent:@"x"];
        [editMenu addItemWithTitle:@"Copy"       action:@selector(copy:)      keyEquivalent:@"c"];
        [editMenu addItemWithTitle:@"Paste"      action:@selector(paste:)     keyEquivalent:@"v"];
        [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
        [editItem setSubmenu:editMenu];

        // Tools menu
        static XCMMenuActions* xcmMenuActions = [[XCMMenuActions alloc] init];
        NSMenuItem* toolsItem = [[NSMenuItem alloc] initWithTitle:@"Tools"
                                                           action:nil
                                                    keyEquivalent:@""];
        [menubar addItem:toolsItem];
        NSMenu* toolsMenu = [[NSMenu alloc] initWithTitle:@"Tools"];
        NSMenuItem* pdfSignItem = [toolsMenu
            addItemWithTitle:@"PDF Fill & Sign"
                     action:@selector(openPDFSign:)
              keyEquivalent:@"p"];
        pdfSignItem.target = xcmMenuActions;
        pdfSignItem.keyEquivalentModifierMask =
            NSEventModifierFlagCommand | NSEventModifierFlagShift;
        [toolsItem setSubmenu:toolsMenu];

        [NSApp setMainMenu:menubar];
    }
    // Logical point size -- drives WKWebView frame and ImGui display size.
    glfwSetWindowSizeCallback(g_win, cb_window_size);
    // Physical pixel size -- drives glViewport only (also fires on DPI change).
    glfwSetFramebufferSizeCallback(g_win, cb_framebuffer_size);
    // Refresh fires during macOS live-resize modal loop (blocks glfwPollEvents).
    // Without this callback the window goes black while the user drags the edge.
    glfwSetWindowRefreshCallback(g_win, cb_window_refresh);
    // Content scale change -- fires when the window moves to a different display.
    // The framebuffer callback covers the DPI recalc, but this one guarantees
    // we also flush WKWebView positioning on scale-only events (e.g. mirror mode).
    glfwSetWindowContentScaleCallback(g_win, [](GLFWwindow*, float xscale, float) {
        g_state.dpi_scale = xscale;
        g_resize_dirty    = true;
    });

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

    // ── Load font scaled for Retina ──────────────────────────────────
    // Strategy: load at (base * dpi_scale) so the rasterised glyphs are
    // physically sharp on HiDPI displays, then set FontGlobalScale to
    // 1/dpi_scale so ImGui renders at the correct LOGICAL size (15pt).
    // ImGui_ImplGlfw already sets io.DisplayFramebufferScale to the pixel
    // ratio, so the OpenGL3 backend handles the physical-to-logical mapping;
    // we must NOT call ScaleAllSizes here or all dimensions are doubled.
    float font_size = 15.0f * g_state.dpi_scale;
    NSString* fontPath = [[NSBundle bundleWithPath:@"/System/Library/Fonts"]
                          pathForResource:@"Menlo" ofType:@"ttc"];
    if (!fontPath) fontPath = @"/System/Library/Fonts/Menlo.ttc";
    if ([[NSFileManager defaultManager] fileExistsAtPath:fontPath]) {
        io.Fonts->AddFontFromFileTTF(fontPath.UTF8String, font_size);
        fprintf(stderr, "[ui] loaded font: %s @ %.0fpx\n", fontPath.UTF8String, font_size);
    } else {
        // Fallback: try SF Mono
        fontPath = @"/System/Library/Fonts/SFMono-Regular.otf";
        if ([[NSFileManager defaultManager] fileExistsAtPath:fontPath]) {
            io.Fonts->AddFontFromFileTTF(fontPath.UTF8String, font_size);
            fprintf(stderr, "[ui] loaded font: %s @ %.0fpx\n", fontPath.UTF8String, font_size);
        } else {
            fprintf(stderr, "[ui] WARNING: no system font found, using ImGui default\n");
        }
    }
    // Scale the font back down to logical points so it renders at 15pt.
    io.FontGlobalScale = (g_state.dpi_scale > 0.0f) ? 1.0f / g_state.dpi_scale : 1.0f;

    chrome_init(&g_state);

    // Load persistent data from ~/.xcm-browser/ (outside the repo)
    g_state.history   = persist_load_history();
    g_state.bookmarks = persist_load_bookmarks();
    fprintf(stderr, "[main] loaded %zu history entries, %zu bookmarks\n",
            g_state.history.size(), g_state.bookmarks.size());

    // ── Logo texture (Xcalibur The Cat) ──────────────────────────────
    // Load the transparent RGBA PNG via CoreGraphics into an OpenGL texture
    // after the GLFW/OpenGL context is current.
    {
        NSString* logo_path = @"/Users/mac/Documents/live-css/xcalibur-the-cat-logo.png";
        NSImage*  ns_img    = [[NSImage alloc] initWithContentsOfFile:logo_path];
        if (ns_img) {
            // Set the macOS Dock icon to the cat logo
            [NSApp setApplicationIconImage:ns_img];
            NSSize sz = ns_img.size;
            int w = (int)sz.width;
            int h = (int)sz.height;
            if (w > 0 && h > 0) {
                std::vector<uint8_t> px((size_t)(w * h * 4), 0);
                // Flip Y: CoreGraphics origin is bottom-left, ImGui/OpenGL UVs
                // start top-left. We draw the CGImage flipped vertically so the
                // texture reads top-down correctly in ImGui::Image().
                CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
                CGContextRef ctx = CGBitmapContextCreate(
                    px.data(), w, h, 8, w * 4, cs,
                    kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
                CGColorSpaceRelease(cs);
                if (ctx) {
                    CGImageRef cg = [ns_img CGImageForProposedRect:nil
                                                           context:nil
                                                             hints:nil];
                    // No Y-flip: CGBitmapContext stores row 0 at top; OpenGL
                    // glTexImage2D maps row 0 to t=0 (bottom of texture);
                    // ImGui::Image uv0={0,0} samples t=0 at screen top-left,
                    // so the image renders right-side-up automatically.
                    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg);
                    CGContextRelease(ctx);

                    GLuint tex = 0;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, px.data());
                    glBindTexture(GL_TEXTURE_2D, 0);

                    chrome_set_logo(tex, w, h);
                    fprintf(stderr, "[main] logo loaded: %dx%d tex=%u\n", w, h, tex);
                }
            }
        } else {
            fprintf(stderr, "[main] WARNING: could not load logo: %s\n",
                    logo_path.UTF8String);
        }
    }

    ImGui_ImplGlfw_InitForOpenGL(g_win, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // ── Get NSWindow and init WKWebView subsystem ────────────────────
    NSWindow* ns_win = glfwGetCocoaWindow(g_win);

    WebViewCallbacks wv_cbs;
    wv_cbs.on_url_change = [](int tab_id, const std::string& url) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) {
                t.url       = url;
                t.is_secure = url.size() >= 8 && url.substr(0, 8) == "https://";
                break;
            }
        // Push to browsing history (skip consecutive duplicates)
        auto& hist = g_state.history;
        if (!url.empty() && (hist.empty() || hist.back().url != url)) {
            HistoryEntry he;
            he.url = url;
            he.ts  = (int64_t)time(nullptr);
            if (hist.size() >= 1000) hist.erase(hist.begin());
            hist.push_back(he);
            // Throttled disk save (every 5 navigations)
            static int s_hist_n = 0;
            if (++s_hist_n % 5 == 0) persist_save_history(g_state.history);
        }
        // Virt overlay: show Qt Chromium on matching URLs, hide on others.
        // Only react to the active tab.
        Tab* active = g_state.current_tab();
        if (active && active->id == tab_id) {
            if (virt_overlay_check_url(url)) {
                int wx = 0, wy = 0;
                glfwGetWindowPos(g_win, &wx, &wy);
                // Capture position + url by value; callback fires on main thread
                int vx = wx, vy = wy + g_prev_top,
                    vw = g_state.win_w,
                    vh = g_state.win_h - g_prev_top - g_prev_bot;
                std::string vurl = url;
                webview_dump_cookies_json([vurl, vx, vy, vw, vh](const std::string& cj) {
                    virt_overlay_show(vurl, vx, vy, vw, vh, cj);
                });
            } else if (virt_overlay_is_active()) {
                virt_overlay_hide();
            }
        }
    };
    wv_cbs.on_favicon_change = [](int tab_id, const std::string& fav_url) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) { t.favicon = fav_url; break; }
    };

    wv_cbs.on_title_change = [](int tab_id, const std::string& title) {
        for (auto& t : g_state.tabs)
            if (t.id == tab_id) {
                t.title = title;
                // Back-fill title on the most recent matching history entry
                for (int hi = (int)g_state.history.size() - 1; hi >= 0; hi--) {
                    if (g_state.history[hi].url == t.url) {
                        g_state.history[hi].title = title;
                        break;
                    }
                }
                break;
            }
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

    // ── Window drag monitor ───────────────────────────────────────────
    // NSWindowStyleMaskFullSizeContentView makes the OS title bar area part of
    // the GL content view, so the normal OS title-bar drag is gone. We restore
    // dragging by watching for left-mouse-down in the title bar zone and calling
    // performWindowDragWithEvent: -- but only when ImGui has nothing hovered
    // (g_chrome_has_hover == false), which means the click is on dead space
    // rather than a tab, close button, or URL bar. If a button IS hovered we
    // return the event unmodified so ImGui handles the click normally.
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                         handler:^NSEvent*(NSEvent* ev) {
        if (ev.window != ns_win) return ev;
        // Convert click location to content-view coords (origin = bottom-left).
        NSPoint p = [ns_win.contentView convertPoint:ev.locationInWindow
                                             fromView:nil];
        float view_h = (float)ns_win.contentView.bounds.size.height;
        float zone_h = (float)TAB_BAR_HEIGHT_PX;
        bool  in_titlebar = (p.y >= view_h - zone_h);
        // Never intercept clicks in the traffic light zone (x < TRAFFIC_LIGHT_W).
        // Those NSButton subviews need to receive the event directly.
        bool  in_tl_zone  = (p.x < (float)TRAFFIC_LIGHT_W);
        if (in_titlebar && !in_tl_zone && !g_chrome_has_hover) {
            [ns_win performWindowDragWithEvent:ev];
            return nil;   // event is consumed by the drag; do not forward to ImGui
        }
        return ev;
    }];

    // Load xcm performance scripts from dev-browser/src/ and register them
    // for injection into every WebView tab and popup at document-start.
    // apps_dir = .../dev-browser/apps  →  src_dir = .../dev-browser/src
    if (!args.apps_dir.empty()) {
        std::string src_dir = args.apps_dir + "/../src";
        auto load_file = [](const std::string& p) -> std::string {
            FILE* f = fopen(p.c_str(), "r");
            if (!f) { fprintf(stderr, "[main] WARNING: cannot load script: %s\n", p.c_str()); return ""; }
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            rewind(f);
            std::string s(sz, '\0');
            fread(&s[0], 1, sz, f);
            fclose(f);
            return s;
        };
        // Injection order matters: ticker-lite first (sets __xcmIdleThreshold
        // and __xcmTick that the lazy/virtualizer/compress scripts read),
        // then lazy (patches document.createElement before any page scripts),
        // then virtualizer and compress (content-visibility + image proxy),
        // then scroll-restore, and finally the existing operation scripts.
        for (const char* name : {
            "xcm-ticker-lite.js",
            "xcm-clip-watcher.js",
            "xcm-smooth-scroll.js",
            "xcm-media-preload.js",
            "lazy-inject.js",
            "virtualizer-inject.js",
            "compress-inject.js",
            "xcm-app-helper.js",
            "xcm-scroll-restore.js",
            "input-watcher.js",
            "chrome-gl-compositor.js",
        }) {
            std::string src = load_file(src_dir + "/" + name);
            if (!src.empty()) {
                wv_cbs.extra_scripts.push_back(std::move(src));
                fprintf(stderr, "[main] loaded extra script: %s\n", name);
            }
        }

        // All-frames scripts: injected into every sub-frame including
        // third-party iframes (forMainFrameOnly:NO). Must have host-gate guards.
        for (const char* name : {
            "xcm-stripe-shim.js"   // needs to run inside js.stripe.com iframe
        }) {
            std::string src = load_file(src_dir + "/" + name);
            if (!src.empty()) {
                wv_cbs.extra_scripts_all_frames.push_back(std::move(src));
                fprintf(stderr, "[main] loaded all-frames script: %s\n", name);
            }
        }
    }

    webview_init((__bridge void*)ns_win, &g_state, wv_cbs);

    // Start ad blocking rule compilation.  Asynchronous -- tabs created
    // before compilation finishes receive the rule list via the retroactive
    // apply in webview_load_adblock's completion handler.
    if (!args.apps_dir.empty()) {
        std::string src_dir = args.apps_dir + "/../src";

        // Load virt-page patterns (URLs that should open in Qt Chromium overlay).
        // Config lives at dev-browser/src/virt-pages.json (same dir as xcm JS files).
        virt_overlay_init(src_dir + "/virt-pages.json");

        std::string rules_path = src_dir + "/adblock-rules.json";
        FILE* f = fopen(rules_path.c_str(), "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f); rewind(f);
            std::string rules(sz, '\0');
            fread(&rules[0], 1, sz, f);
            fclose(f);
            webview_load_adblock(rules);
            fprintf(stderr, "[main] adblock rules file loaded (%ld bytes)\n", sz);
        } else {
            fprintf(stderr, "[main] WARNING: adblock-rules.json not found at %s\n",
                    rules_path.c_str());
        }
    }

    // ── Native AppKit chrome (tab bar + toolbar + status bar) ────────
    native_chrome_create((__bridge void*)ns_win, &g_state, args.php_port);

    // If --clear-data was passed, flush all cookies/cache/SW before any tab
    // loads. This clears stuck auth tokens that may cause login loops.
    if (args.clear_data) {
        webview_clear_data();
        // Give the async clear a moment to flush before loading the first tab.
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.3]];
    }

    // ── Server processes ─────────────────────────────────────────────
    // Start servers first so the initial URL is more likely to be
    // reachable by the time WKWebView sends its first request.
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
    {
        // cf_bridge Chromium cookie harvester for Cloudflare Turnstile
        std::string dev_browser_dir;
        auto sl = args.apps_dir.rfind('/');
        if (sl != std::string::npos)
            dev_browser_dir = args.apps_dir.substr(0, sl); // strip /apps
        server_start_cf_bridge(dev_browser_dir);
    }

    // ── Command API ───────────────────────────────────────────────────
    cmd_server_start(&g_state, args.cmd_port);

    // ── Initial tab ──────────────────────────────────────────────────
    {
        int idx = g_state.new_tab(args.url);
        g_state.tabs[idx].wv_handle = webview_create(g_state.tabs[idx].id, args.url);
    }

    // ── Server status poll: every 3 seconds on a cheap timer ─────────
    double last_server_poll = 0.0;

    // ── Render loop ───────────────────────────────────────────────────
    while (!glfwWindowShouldClose(g_win)) {
        // Process macOS events (needed for WKWebView / NSRunLoop).
        @autoreleasepool {
            // Drain AppKit keyboard/mouse/window events.
            NSEvent* ev;
            while ((ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:nil
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES])) {
                [NSApp sendEvent:ev];
            }
            // Tick the run loop once with distantPast (returns immediately if
            // nothing is pending). This drains timer and source callbacks that
            // WKWebView schedules outside the default mode, such as navigation
            // progress updates and JS completion handlers.
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                    beforeDate:[NSDate distantPast]];
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
                // Cmd+L -- focus URL bar
                if (ImGui::IsKeyPressed(ImGuiKey_L, false)) {
                    g_state.focus_url_next_frame = true;
                }
                // Cmd+D -- toggle bookmark for current page
                if (ImGui::IsKeyPressed(ImGuiKey_D, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__bookmark_toggle__");
                }
                // Cmd+H -- toggle history panel
                if (ImGui::IsKeyPressed(ImGuiKey_H, false)) {
                    g_state.show_history_panel   = !g_state.show_history_panel;
                    g_state.show_bookmarks_panel = false;
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
                // Cmd+Option+I -- open Web Inspector for current tab
                if (kio.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_I, false)) {
                    Tab* t = g_state.current_tab();
                    if (t && t->wv_handle) webview_open_inspector(t->wv_handle);
                }
                // Cmd+Shift+Delete (Backspace) -- clear all cookies/cache/SW
                // Use this to flush a stuck auth state without restarting.
                if (kio.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
                    webview_clear_data();
                }
            }
        }

        // Process command queue
        AppState::NavCmd cmd;
        while (g_state.pop_nav(cmd)) {
            dispatch_nav(cmd);
        }

        // ── Dear ImGui frame ─────────────────────────────────────────
        // Wrapped in @try/@catch so that an ObjC exception thrown by a
        // WKWebView delegate or AppKit callback during a single frame does
        // not kill the process. The exception is logged to debug/crash.log
        // and the frame is skipped; the render loop continues.
        @try {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Chrome positions are in LOGICAL POINTS (same as glfwGetWindowSize).
        // ImGui_ImplGlfw sets io.DisplaySize to logical points and
        // io.DisplayFramebufferScale to the pixel ratio; the OpenGL3 renderer
        // applies the scale when building the draw commands, so all ImGui
        // SetNextWindowPos/Size calls use logical points.
        // Consume any pending resize (fires when the refresh callback did not
        // get a chance to run, e.g. programmatic window resize).
        if (g_resize_dirty) {
            native_chrome_resize(g_state.win_w, g_state.win_h);
            reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
            g_resize_dirty = false;
        }

        int ww = g_state.win_w;
        int wh = g_state.win_h;

        // ── Native chrome sync (updates NSViews, returns pixel heights) ──
        int  chrome_top  = native_chrome_update(&g_state);
        int  chrome_bot  = native_chrome_status_h();
        g_chrome_has_hover = native_chrome_has_hover();

        // ── Floating ImGui panels (bookmarks / history) ───────────────
        chrome_draw_panels(&g_state,
                           native_chrome_bm_btn_x(),
                           native_chrome_hist_btn_x(),
                           chrome_top);

        // Reposition content views if chrome height changed.
        // All values here are logical points -- WKWebView NSView uses points.
        if (chrome_top != g_prev_top || chrome_bot != g_prev_bot) {
            g_prev_top = chrome_top;
            g_prev_bot = chrome_bot;
            reposition_webviews(chrome_top, chrome_bot, ww, wh);
        }

        // Always maintain correct visibility for active vs inactive tabs
        static int s_last_active_tab = -1;
        for (int i = 0; i < (int)g_state.tabs.size(); i++) {
            void* h = g_state.tabs[i].wv_handle;
            if (!h) continue;
            if (i == g_state.active_tab) webview_show(h);
            else                         webview_hide(h);
        }

        // Virt overlay: detect tab switches
        if (g_state.active_tab != s_last_active_tab) {
            s_last_active_tab = g_state.active_tab;
            Tab* t = g_state.current_tab();
            if (t && virt_overlay_check_url(t->url)) {
                int wx = 0, wy = 0;
                glfwGetWindowPos(g_win, &wx, &wy);
                int vx = wx, vy = wy + g_prev_top;
                int vw = ww, vh = wh - g_prev_top - g_prev_bot;
                std::string vurl = t->url;
                webview_dump_cookies_json([vurl, vx, vy, vw, vh](const std::string& cj) {
                    virt_overlay_show(vurl, vx, vy, vw, vh, cj);
                });
            } else if (virt_overlay_is_active()) {
                virt_overlay_hide();
            }
        }

        // Sync overlay position while active (fires async only when window moved)
        if (virt_overlay_is_active()) {
            int wx = 0, wy = 0;
            glfwGetWindowPos(g_win, &wx, &wy);
            virt_overlay_tick(wx, wy + g_prev_top, ww, wh - g_prev_top - g_prev_bot);
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
            // The top chrome (my < g_prev_top) is entirely covered by the
            // WKWebView chrome panel. WebKit manages cursor there via its own
            // NSTrackingAreas (IBeam over the URL input, pointer over buttons,
            // etc.). If we call NSCursor here we overwrite what WebKit just set,
            // so we must leave that zone alone.
            // Only manage cursor for the bottom ImGui chrome (if any).
            bool over_bottom_chrome = g_prev_bot > 0 &&
                                      (my > (double)(g_state.win_h - g_prev_bot));
            if (over_bottom_chrome) {
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
            // Top chrome and content area: cursor is owned by WKWebView.
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
        } @catch (NSException* e) {
            // Log and skip the frame -- do not let a single-frame ObjC exception
            // terminate the entire process.
            NSString* msg = [NSString stringWithFormat:@"name=%@ reason=%@\n%@",
                             e.name, e.reason,
                             [e.callStackSymbols componentsJoinedByString:@"\n"]];
            xcm_write_crash("ObjC exception in render loop (frame skipped)", msg.UTF8String);
        }
    } // end render loop

    // ── Cleanup ───────────────────────────────────────────────────────
    // Persist bookmarks and history before tearing down
    persist_save_history(g_state.history);
    persist_save_bookmarks(g_state.bookmarks);

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
