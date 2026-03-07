// main_callbacks.mm -- GLFW window callbacks (registered in main()).
// The names are prefixed xcm_cb_ to keep them distinct from the bare
// static cb_* names that were in the original monolithic main.mm.  The
// xcm_cb_* symbols are declared in main_funcs.h.

#include "main_funcs.h"

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

// Window refresh -- fires during the macOS live-resize modal loop that blocks
// glfwPollEvents.  Without this callback the window goes black while the user
// drags the edge.
void xcm_cb_window_refresh(GLFWwindow*) {
    if (g_resize_dirty) {
        native_chrome_resize(g_state.win_w, g_state.win_h);
        reposition_webviews(g_prev_top, g_prev_bot, g_state.win_w, g_state.win_h);
        g_resize_dirty = false;
    }
    glViewport(0, 0, g_fb_w, g_fb_h);
    glClearColor(0.047f, 0.047f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(g_win);
}

void xcm_cb_error(int, const char* desc) {
    fprintf(stderr, "[glfw] Error: %s\n", desc);
}
