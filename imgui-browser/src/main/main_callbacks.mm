// main_callbacks.mm -- GLFW window callbacks (registered in main()).
// The names are prefixed xcm_cb_ to keep them distinct from the bare
// static cb_* names that were in the original monolithic main.mm.  The
// xcm_cb_* symbols are declared in main_funcs.h.

#include "main_funcs.h"
#include "../top-of-gui/chrome_traffic_lights.h"
#include "../platform/platform.h"
#include "../webview.h"

// Logical-point window size -- drives WKWebView frame and ImGui display size.
void xcm_cb_window_size(GLFWwindow*, int w, int h) {
    g_state.win_w  = w;
    g_state.win_h  = h;
    g_resize_dirty = true;
}

// Physical-pixel framebuffer size -- used only for glViewport.
// Also fires when the window moves to a display with a different DPI.
void xcm_cb_framebuffer_size(GLFWwindow*, int w, int h) {
    g_fb_w = w;
    g_fb_h = h;
    if (g_state.win_w > 0)
        g_state.dpi_scale = (float)w / (float)g_state.win_w;
    g_resize_dirty = true;
}

// Window refresh -- fires during the OS modal resize loop (macOS or Windows)
// while glfwPollEvents is blocked.  A full ImGui frame keeps the chrome
// visible instead of the window going black.
void xcm_cb_window_refresh(GLFWwindow*) {
    if (g_resize_dirty) {
        native_chrome_resize(g_state.win_w, g_state.win_h);
        reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
        g_resize_dirty = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    platform_pre_imgui_newframe();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int chrome_top = native_chrome_update(&g_state);
    int chrome_bot = native_chrome_status_h();

    if (chrome_top != g_prev_top || chrome_bot != g_prev_bot) {
        g_prev_top = chrome_top;
        g_prev_bot = chrome_bot;
        reposition_webviews(chrome_top, chrome_bot, g_state.win_w, g_state.win_h);
    }

    ImGui::Render();

    glViewport(0, 0, g_fb_w, g_fb_h);
    glClearColor(0.047f, 0.047f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_win);

#ifndef __APPLE__
    chrome_tl_update();
    webview_reapply_corners();
#endif
}

void xcm_cb_error(int, const char* desc) {
    fprintf(stderr, "[glfw] Error: %s\n", desc);
}
