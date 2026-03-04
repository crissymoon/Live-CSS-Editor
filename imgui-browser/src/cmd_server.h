#pragma once
// cmd_server.h -- lightweight HTTP command API
// Starts a background thread serving on 127.0.0.1:<port>.
// Accepts the same JSON commands as the Python command_server.py
// so existing tooling (push.sh, etc.) continues to work.

#include "app_state.h"
#include <string>

// Start the command server on a background thread.
// Pushes navigation commands into state->cmd_queue.
void cmd_server_start(AppState* state, int port = 9878);

// Stop the background thread (call on app exit).
void cmd_server_stop();
