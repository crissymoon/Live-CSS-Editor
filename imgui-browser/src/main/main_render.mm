// main_render.mm -- Main render / event loop.
// main_render_loop() runs until glfwWindowShouldClose() returns true, then
// persists session data and returns to main() which handles cleanup.

#include "main_funcs.h"
#include "main_crash.h"

void main_render_loop(const Args& args) {
    double last_server_poll = 0.0;
    bool   drag_active      = false;
    bool   left_prev        = false;
    int    drag_win_x       = 0;
    int    drag_win_y       = 0;
    double drag_mouse_x     = 0.0;
    double drag_mouse_y     = 0.0;

    while (!glfwWindowShouldClose(g_win)) {
        // Process macOS events (needed for WKWebView / NSRunLoop).
#if defined(__APPLE__)
        @autoreleasepool {
            NSEvent* ev;
            while ((ev = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:nil
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES])) {
                [NSApp sendEvent:ev];
            }
            [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                    beforeDate:[NSDate distantPast]];
        }
#endif

        glfwPollEvents();

        // Window dragging for undecorated windows.
        // Drag only from the non-interactive top strip so tabs/buttons keep normal behavior.
        {
            double mx = 0.0, my = 0.0;
            glfwGetCursorPos(g_win, &mx, &my);
            bool left_now = glfwGetMouseButton(g_win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool pressed  = left_now && !left_prev;
            bool released = !left_now && left_prev;

            // Only start drag from the 30-px TOP_PAD strip at the very top.
            // Exclude the three window-control button areas so their clicks
            // pass through to ImGui normally.
            // On Windows, WM_NCHITTEST already handles drag via HTCAPTION; this
            // path is kept as a cross-platform fallback.
            const double TOP_PAD = 30.0;
            const double BTN_CY  = 15.0, BTN_R = 6.0;
            struct WBR { double x0, x1; } win_btns[] = {{8,20},{28,40},{48,60}};
            bool in_top_pad = (my >= 0.0 && my < TOP_PAD);
            bool over_win_btn = false;
            if (in_top_pad && my >= BTN_CY - BTN_R && my <= BTN_CY + BTN_R) {
                for (auto& b : win_btns)
                    if (mx >= b.x0 && mx <= b.x1) { over_win_btn = true; break; }
            }

            if (pressed && in_top_pad && !over_win_btn) {
                drag_active = true;
                glfwGetWindowPos(g_win, &drag_win_x, &drag_win_y);
                drag_mouse_x = mx;
                drag_mouse_y = my;
            }

            if (drag_active && left_now) {
                // Use current window pos each frame: as the window moves, the
                // cursor's client-relative coords shift, so we can't use the
                // initial drag_win_x/y.  Read current pos + apply client delta.
                int cwx, cwy;
                glfwGetWindowPos(g_win, &cwx, &cwy);
                int nx = cwx + (int)(mx - drag_mouse_x);
                int ny = cwy + (int)(my - drag_mouse_y);
                glfwSetWindowPos(g_win, nx, ny);
            }

            if (released) {
                drag_active = false;
            }

            left_prev = left_now;
        }

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
        {
            ImGuiIO& kio = ImGui::GetIO();
#if defined(__APPLE__)
            bool cmd = kio.KeySuper;   // Cmd key on macOS
#else
            bool cmd = kio.KeyCtrl;    // Ctrl key on Windows / Linux
#endif

            if (cmd && !kio.WantTextInput) {

                if (ImGui::IsKeyPressed(ImGuiKey_L, false))
                    g_state.focus_url_next_frame = true;

                if (ImGui::IsKeyPressed(ImGuiKey_D, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__bookmark_toggle__");
                }
                if (ImGui::IsKeyPressed(ImGuiKey_H, false)) {
                    g_state.show_history_panel   = !g_state.show_history_panel;
                    g_state.show_bookmarks_panel = false;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
                    std::string nt_url = "http://127.0.0.1:" +
                                         std::to_string(args.php_port) + "/";
                    int idx = g_state.new_tab(nt_url);
                    g_state.tabs[idx].wv_handle =
                        webview_create(g_state.tabs[idx].id, nt_url);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
                    int ci = g_state.active_tab;
                    if ((int)g_state.tabs.size() > 1 &&
                        ci < (int)g_state.tabs.size()) {
                        webview_destroy(g_state.tabs[ci].wv_handle);
                        g_state.tabs[ci].wv_handle = nullptr;
                        g_state.close_tab(ci);
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__reload__");
                }
                if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__back__");
                }
                if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false)) {
                    Tab* t = g_state.current_tab();
                    if (t) g_state.push_nav(t->id, "__forward__");
                }
                for (int ki = 0; ki < 9 && ki < (int)g_state.tabs.size(); ki++) {
                    ImGuiKey kn = (ImGuiKey)((int)ImGuiKey_1 + ki);
                    if (ImGui::IsKeyPressed(kn, false))
                        g_state.active_tab = ki;
                }
                if (kio.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_I, false)) {
                    Tab* t = g_state.current_tab();
                    if (t && t->wv_handle) webview_open_inspector(t->wv_handle);
                }
                if (kio.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
                    webview_clear_data();
                }
            }
        }

        // Drain navigation command queue
        AppState::NavCmd cmd;
        while (g_state.pop_nav(cmd)) {
            dispatch_nav(cmd);
        }

        // ── Dear ImGui frame ─────────────────────────────────────────
    #if defined(__APPLE__)
        @try {
    #endif
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // DEBUG: on-screen mouse position overlay -- remove after diagnosis.
        {
            double gx = 0, gy = 0;
            glfwGetCursorPos(g_win, &gx, &gy);
            ImVec2 mp = ImGui::GetMousePos();
            char dbg[128];
            if (mp.x < -100000.0f)
                snprintf(dbg, sizeof(dbg), "GLFW=(%.0f,%.0f) ImGui=NO_DATA", gx, gy);
            else
                snprintf(dbg, sizeof(dbg), "GLFW=(%.0f,%.0f) ImGui=(%.0f,%.0f)",
                         gx, gy, mp.x, mp.y);
            ImGui::GetForegroundDrawList()->AddText(
                {10.0f, 40.0f}, IM_COL32(255, 50, 50, 255), dbg);
        }

        if (g_resize_dirty) {
            native_chrome_resize(g_state.win_w, g_state.win_h);
            reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
            g_resize_dirty = false;
        }

        int ww = g_state.win_w;
        (void)ww;
        int wh = g_state.win_h;
        (void)wh;

        int  chrome_top = native_chrome_update(&g_state);
        int  chrome_bot = native_chrome_status_h();
        g_chrome_has_hover = native_chrome_has_hover();

        chrome_draw_panels(&g_state,
                           native_chrome_bm_btn_x(),
                           native_chrome_hist_btn_x(),
                           chrome_top);

        if (chrome_top != g_prev_top || chrome_bot != g_prev_bot) {
            g_prev_top = chrome_top;
            g_prev_bot = chrome_bot;
            reposition_webviews(chrome_top, chrome_bot, g_state.win_w, g_state.win_h);
        }

        for (int i = 0; i < (int)g_state.tabs.size(); i++) {
            void* h = g_state.tabs[i].wv_handle;
            if (!h) continue;
            if (i == g_state.active_tab) webview_show(h);
            else                         webview_hide(h);
        }

        ImGui::Render();

        // ── Cursor management ─────────────────────────────────────────
        {
            double mx, my;
            glfwGetCursorPos(g_win, &mx, &my);
            bool over_bottom_chrome = g_prev_bot > 0 &&
                                      (my > (double)(g_state.win_h - g_prev_bot));
#if defined(__APPLE__)
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
#else
            (void)over_bottom_chrome;
#endif
        }

        glViewport(0, 0, g_fb_w, g_fb_h);
        glClearColor(0.047f, 0.047f, 0.063f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(g_win);

#if defined(__APPLE__)
        } @catch (NSException* e) {
            NSString* msg = [NSString stringWithFormat:@"name=%@ reason=%@\n%@",
                             e.name, e.reason,
                             [e.callStackSymbols componentsJoinedByString:@"\n"]];
            xcm_write_crash("ObjC exception in render loop (frame skipped)", msg.UTF8String);
        }
#endif
    } // end render loop

    // Persist session data on clean exit
    persist_save_history(g_state.history);
    persist_save_bookmarks(g_state.bookmarks);
}
