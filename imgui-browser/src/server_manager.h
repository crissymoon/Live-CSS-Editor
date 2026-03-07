#pragma once
// server_manager.h -- spawn/monitor PHP and Node.js server processes

#include <string>
#include <functional>

struct ServerStatus {
    bool php_ok  = false;
    bool node_ok = false;
    int  php_pid = -1;
    int  node_pid= -1;
};

// Start the PHP built-in server (port 9879, serving dev-browser/apps/).
// Returns immediately; poll with server_poll_status().
void server_start_php(const std::string& apps_dir, int php_port = 9879);

// Start the Node.js image-cache server (port 7779).
void server_start_node(const std::string& node_script);

// Start the cf_bridge Chromium cookie harvester (port 9925).
// dev_browser_dir: absolute path to the dev-browser/ directory so the runner
// can find the .venv Python and the cf_bridge module.
void server_start_cf_bridge(const std::string& dev_browser_dir);

// Poll liveness of both servers.  Returns current status.
ServerStatus server_poll_status(int php_port = 9879, int node_port = 7779);

// Kill all managed child processes (call on app exit).
void server_shutdown();
