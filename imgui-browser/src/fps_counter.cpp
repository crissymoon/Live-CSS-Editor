// fps_counter.cpp
// FpsCounter is defined inline in app_state.h.
// This file keeps CMakeLists.txt happy as a compilation unit and can
// host any future platform-specific frame timing helpers.

#include "app_state.h"

// Called from the main render loop with the GLFW time (seconds).
// Updates the host-side FPS ring buffer.
void fps_host_tick(AppState& st, double now_sec) {
    st.fps_host.tick(now_sec);
}
