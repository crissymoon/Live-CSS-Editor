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

// Start the PHP-WASM dev server (Node server.js inside the php-wasm-project).
// wasm_dir is the project root (directory that contains server.js).
// port is the port to listen on (default 8082).
// Returns immediately; the server is included in server_shutdown() cleanup.
void server_start_wasm(const std::string& wasm_dir, int port = 8082);

// Attempt to auto-locate the php-wasm-project server.js beside the binary.
// Returns the directory path, or empty string if not found.
std::string server_find_wasm_dir(const std::string& apps_dir);

// Poll liveness of both servers.  Returns current status.
ServerStatus server_poll_status(int php_port = 9879, int node_port = 7779);

// Kill all managed child processes (call on app exit).
void server_shutdown();
