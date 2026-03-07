// server_manager.cpp -- PHP/Node process management

#include "server_manager.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

static std::vector<pid_t> s_procs;

static bool port_open(int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;
    struct timeval tv{0, 200000};  // 200ms
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bool ok = (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    ::close(fd);
    return ok;
}

static pid_t spawn(const std::vector<std::string>& args,
                   const std::string& cwd = "") {
    std::vector<const char*> argv;
    for (auto& a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        // Child
        if (!cwd.empty()) chdir(cwd.c_str());
        // Redirect stdout/stderr to /dev/null
        FILE* dev_null = fopen("/dev/null", "w");
        if (dev_null) {
            dup2(fileno(dev_null), STDOUT_FILENO);
            dup2(fileno(dev_null), STDERR_FILENO);
            fclose(dev_null);
        }
        execvp(argv[0], (char* const*)argv.data());
        _exit(127);
    }
    return pid;
}

void server_start_php(const std::string& apps_dir, int php_port) {
    if (port_open(php_port)) {
        printf("[server] PHP already running on :%d\n", php_port);
        return;
    }
    // Locate php binary
    const char* php = nullptr;
    for (auto* p : {"/opt/homebrew/bin/php", "/usr/local/bin/php", "/usr/bin/php"}) {
        if (access(p, X_OK) == 0) { php = p; break; }
    }
    if (!php) {
        printf("[server] php not found -- apps server will not start\n");
        return;
    }
    char bind[64];
    snprintf(bind, sizeof(bind), "127.0.0.1:%d", php_port);
    pid_t pid = spawn({php, "-S", bind, "-t", apps_dir}, apps_dir);
    if (pid > 0) {
        s_procs.push_back(pid);
        printf("[server] PHP started (PID %d) on :%d -> %s\n",
               pid, php_port, apps_dir.c_str());
    }
}

void server_start_node(const std::string& node_script) {
    if (node_script.empty()) return;
    if (port_open(7779)) {
        printf("[server] Node already running on :7779\n");
        return;
    }
    // Locate node binary
    const char* node = nullptr;
    for (auto* p : {"/opt/homebrew/bin/node", "/usr/local/bin/node", "/usr/bin/node"}) {
        if (access(p, X_OK) == 0) { node = p; break; }
    }
    if (!node) {
        printf("[server] node not found -- image cache server will not start\n");
        return;
    }
    // cwd = parent directory of the script
    std::string cwd;
    auto slash = node_script.rfind('/');
    if (slash != std::string::npos) cwd = node_script.substr(0, slash);

    pid_t pid = spawn({node, node_script}, cwd);
    if (pid > 0) {
        s_procs.push_back(pid);
        printf("[server] Node started (PID %d) :7779\n", pid);
    }
}

void server_start_wasm(const std::string& wasm_dir, int port) {
    if (wasm_dir.empty()) {
        printf("[server] wasm_dir is empty -- WASM dev server will not start\n");
        return;
    }
    if (port_open(port)) {
        printf("[server] WASM server already running on :%d\n", port);
        return;
    }
    // Locate node binary
    const char* node = nullptr;
    for (auto* p : {"/opt/homebrew/bin/node", "/usr/local/bin/node", "/usr/bin/node"}) {
        if (access(p, X_OK) == 0) { node = p; break; }
    }
    if (!node) {
        printf("[server] node not found -- WASM dev server will not start\n");
        return;
    }
    std::string server_js = wasm_dir + "/server.js";
    if (access(server_js.c_str(), R_OK) != 0) {
        printf("[server] WASM server.js not found at %s\n", server_js.c_str());
        return;
    }
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    pid_t pid = spawn({node, server_js, port_str}, wasm_dir);
    if (pid > 0) {
        s_procs.push_back(pid);
        printf("[server] PHP-WASM server started (PID %d) on :%d -> %s\n",
               pid, port, wasm_dir.c_str());
    }
}

std::string server_find_wasm_dir(const std::string& apps_dir) {
    // Try paths relative to apps_dir which is typically <root>/dev-browser/apps.
    // The php-wasm-project lives at <root>/imgui-browser/php-wasm-project.
    std::vector<std::string> candidates;
    if (!apps_dir.empty()) {
        // apps_dir = .../dev-browser/apps  -- go up two levels to repo root
        auto sl = apps_dir.rfind('/');
        if (sl != std::string::npos) {
            std::string dev_browser = apps_dir.substr(0, sl);
            auto sl2 = dev_browser.rfind('/');
            if (sl2 != std::string::npos) {
                std::string root = dev_browser.substr(0, sl2);
                candidates.push_back(root + "/imgui-browser/php-wasm-project");
            }
        }
    }
    // Also try the bundled sibling path next to the binary (for .app bundles)
    candidates.push_back("/Users/mac/Documents/live-css/imgui-browser/php-wasm-project");
    for (auto& c : candidates) {
        std::string srv = c + "/server.js";
        if (access(srv.c_str(), R_OK) == 0) return c;
    }
    return "";
}

ServerStatus server_poll_status(int php_port, int node_port) {
    ServerStatus st;
    st.php_ok  = port_open(php_port);
    st.node_ok = port_open(node_port);
    // Reap any dead children without blocking
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {}
    return st;
}

void server_shutdown() {
    // Signal all child processes to exit at once.
    for (pid_t pid : s_procs) {
        if (pid > 0) kill(pid, SIGTERM);
    }
    // Poll ALL processes inside one shared 1.5s window, then SIGKILL survivors.
    const int timeout_ms = 1500;
    const int step_ms    = 30;
    int waited = 0;
    while (waited < timeout_ms) {
        bool any_alive = false;
        for (pid_t pid : s_procs) {
            if (pid <= 0) continue;
            int st;
            if (waitpid(pid, &st, WNOHANG) == 0)
                any_alive = true;  // still running
        }
        if (!any_alive) break;
        usleep(step_ms * 1000);
        waited += step_ms;
    }
    // Force-kill anything still alive.
    for (pid_t pid : s_procs) {
        if (pid <= 0) continue;
        if (kill(pid, 0) == 0) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
    }
    s_procs.clear();
}


