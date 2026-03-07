// main.mm -- imgui-browser entry point (slim dispatcher).
//
// Implementation split across src/main/:
//   main_crash.mm     -- crash/signal handling
//   main_args.mm      -- arg parsing (Args struct)
//   main_globals.mm   -- file-scope global definitions
//   main_layout.mm    -- reposition_webviews()
//   main_dispatch.mm  -- dispatch_nav()
//   main_callbacks.mm -- GLFW window callbacks
//   main_menu.mm      -- NSApp menubar + XCMMenuActions
//   main_wvcbs.mm     -- build_webview_callbacks()
//   main_servers.mm   -- start_all_servers()  <- PYTHON LINK
//   main_render.mm    -- main_render_loop()

#include "main/main_priv.h"
#include "main/main_crash.h"
#include "main/main_args.h"
#include "main/main_globals.h"
#include "main/main_funcs.h"

int main(int argc, char** argv) {
    // Debug / crash log setup
    {
        char exe[1024] = {};
        uint32_t sz = sizeof(exe);
        _NSGetExecutablePath(exe, &sz);
        std::string ep(exe);
        auto sl = ep.rfind('/');
        std::string bin_dir = (sl != std::string::npos) ? ep.substr(0, sl) : ".";
        std::string proj_dir = bin_dir;
        auto sl2 = bin_dir.rfind('/');
        if (sl2 != std::string::npos) proj_dir = bin_dir.substr(0, sl2);
        if (proj_dir.find("Contents/MacOS") != std::string::npos ||
            proj_dir.find(".app") != std::string::npos) {
            for (int up = 0; up < 3; up++) {
                auto s = proj_dir.rfind('/');
                if (s != std::string::npos) proj_dir = proj_dir.substr(0, s);
            }
        }
        snprintf(g_debug_dir, sizeof(g_debug_dir), "%s/debug", proj_dir.c_str());
        snprintf(g_crash_log, sizeof(g_crash_log), "%s/crash.log", g_debug_dir);
        mkdir(g_debug_dir, 0755);
        fprintf(stderr, "[main] debug dir: %s\n", g_debug_dir);
    }

    xcm_install_signal_handlers();
    xcm_install_objc_exception_handler();

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    Args args = parse_args(argc, argv);
    g_php_port    = args.php_port;
    g_state.win_w = args.win_w;
    g_state.win_h = args.win_h;

    // GLFW
    glfwSetErrorCallback(xcm_cb_error);
    if (!glfwInit()) { fprintf(stderr, "[main] glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

    g_win = glfwCreateWindow(args.win_w, args.win_h, "Crissy's Style Tool", nullptr, nullptr);
    if (!g_win) {
        fprintf(stderr, "[main] glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(g_win);
    glfwSwapInterval(1);

    // Menu bar (must come after glfwCreateWindow)
    build_app_menubar();

    // GLFW callbacks
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
    g_state.dpi_scale = (float)g_fb_w / (float)_lw;

    // Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename  = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigMacOSXBehaviors = true;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Font (Retina-scaled)
    float font_size = 15.0f * g_state.dpi_scale;
    NSString* fontPath = [[NSBundle bundleWithPath:@"/System/Library/Fonts"]
                          pathForResource:@"Menlo" ofType:@"ttc"];
    if (!fontPath) fontPath = @"/System/Library/Fonts/Menlo.ttc";
    if ([[NSFileManager defaultManager] fileExistsAtPath:fontPath]) {
        io.Fonts->AddFontFromFileTTF(fontPath.UTF8String, font_size);
        fprintf(stderr, "[ui] loaded font: %s @ %.0fpx\n", fontPath.UTF8String, font_size);
    } else {
        fontPath = @"/System/Library/Fonts/SFMono-Regular.otf";
        if ([[NSFileManager defaultManager] fileExistsAtPath:fontPath]) {
            io.Fonts->AddFontFromFileTTF(fontPath.UTF8String, font_size);
            fprintf(stderr, "[ui] loaded font: %s @ %.0fpx\n", fontPath.UTF8String, font_size);
        } else {
            fprintf(stderr, "[ui] WARNING: no system font found, using ImGui default\n");
        }
    }
    io.FontGlobalScale = (g_state.dpi_scale > 0.0f) ? 1.0f / g_state.dpi_scale : 1.0f;

    chrome_init(&g_state);
    g_state.history   = persist_load_history();
    g_state.bookmarks = persist_load_bookmarks();
    fprintf(stderr, "[main] loaded %zu history entries, %zu bookmarks\n",
            g_state.history.size(), g_state.bookmarks.size());

    // Logo texture
    {
        NSString* logo_path = @"/Users/mac/Documents/live-css/xcalibur-the-cat-logo.png";
        NSImage*  ns_img    = [[NSImage alloc] initWithContentsOfFile:logo_path];
        if (ns_img) {
            [NSApp setApplicationIconImage:ns_img];
            NSSize nsz = ns_img.size;
            int w = (int)nsz.width, h = (int)nsz.height;
            if (w > 0 && h > 0) {
                std::vector<uint8_t> px((size_t)(w * h * 4), 0);
                CGColorSpaceRef cs  = CGColorSpaceCreateDeviceRGB();
                CGContextRef    ctx = CGBitmapContextCreate(
                    px.data(), w, h, 8, w * 4, cs,
                    kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
                CGColorSpaceRelease(cs);
                if (ctx) {
                    CGImageRef cg = [ns_img CGImageForProposedRect:nil context:nil hints:nil];
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
            fprintf(stderr, "[main] WARNING: could not load logo: %s\n", logo_path.UTF8String);
        }
    }

    ImGui_ImplGlfw_InitForOpenGL(g_win, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // NSWindow appearance
    NSWindow* ns_win = glfwGetCocoaWindow(g_win);
    ns_win.titlebarAppearsTransparent = YES;
    ns_win.titleVisibility            = NSWindowTitleHidden;
    ns_win.styleMask |= NSWindowStyleMaskFullSizeContentView;
    [ns_win setTitle:@"Crissy's Style Tool"];

    // Window drag monitor
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                         handler:^NSEvent*(NSEvent* ev) {
        if (ev.window != ns_win) return ev;
        NSPoint p    = [ns_win.contentView convertPoint:ev.locationInWindow fromView:nil];
        float view_h = (float)ns_win.contentView.bounds.size.height;
        float zone_h = (float)TAB_BAR_HEIGHT_PX;
        bool  in_tb  = (p.y >= view_h - zone_h);
        bool  in_tl  = (p.x < (float)TRAFFIC_LIGHT_W);
        if (in_tb && !in_tl && !g_chrome_has_hover) {
            [ns_win performWindowDragWithEvent:ev];
            return nil;
        }
        return ev;
    }];

    // WebView callbacks + init
    WebViewCallbacks wv_cbs = build_webview_callbacks(args);
    webview_init((__bridge void*)ns_win, &g_state, wv_cbs);

    // Adblock
    if (!args.apps_dir.empty()) {
        std::string rules_path = args.apps_dir + "/../src/adblock-rules.json";
        FILE* f = fopen(rules_path.c_str(), "r");
        if (f) {
            fseek(f, 0, SEEK_END); long fsz = ftell(f); rewind(f);
            std::string rules(fsz, '\0');
            fread(&rules[0], 1, fsz, f); fclose(f);
            webview_load_adblock(rules);
            fprintf(stderr, "[main] adblock rules loaded (%ld bytes)\n", fsz);
        } else {
            fprintf(stderr, "[main] WARNING: adblock-rules.json not at %s\n",
                    rules_path.c_str());
        }
    }

    // Native AppKit chrome
    native_chrome_create((__bridge void*)ns_win, &g_state, args.php_port);

    if (args.clear_data) {
        webview_clear_data();
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.3]];
    }

    // Servers
    start_all_servers(args);

    // Command API
    cmd_server_start(&g_state, args.cmd_port);

    // Initial tab
    {
        int idx = g_state.new_tab(args.url);
        g_state.tabs[idx].wv_handle = webview_create(g_state.tabs[idx].id, args.url);
    }

    // Render loop (blocks until window close, persists data on return)
    main_render_loop(args);

    // Cleanup
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
