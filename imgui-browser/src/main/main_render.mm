// main_render.mm -- Main render / event loop.
// main_render_loop() runs until glfwWindowShouldClose() returns true, then
// persists session data and returns to main() which handles cleanup.

#include "main_funcs.h"
#include "main_crash.h"

void main_render_loop(const Args& args) {
    double last_server_poll = 0.0;

    while (!glfwWindowShouldClose(g_win)) {
        // Process macOS events (needed for WKWebView / NSRunLoop).
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
        {
            ImGuiIO& kio = ImGui::GetIO();
            bool cmd = kio.KeySuper;

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
        @try {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
        }

        glViewport(0, 0, g_fb_w, g_fb_h);
        glClearColor(0.047f, 0.047f, 0.063f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(g_win);

        } @catch (NSException* e) {
            NSString* msg = [NSString stringWithFormat:@"name=%@ reason=%@\n%@",
                             e.name, e.reason,
                             [e.callStackSymbols componentsJoinedByString:@"\n"]];
            xcm_write_crash("ObjC exception in render loop (frame skipped)", msg.UTF8String);
        }
    } // end render loop

    // Persist session data on clean exit
    persist_save_history(g_state.history);
    persist_save_bookmarks(g_state.bookmarks);
}
