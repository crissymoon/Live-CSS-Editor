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
    for (pid_t pid : s_procs) {
        if (pid > 0) {
            kill(pid, SIGTERM);
        }
    }
    s_procs.clear();
}

void server_start_chrome_virt_bridge(const std::string& dev_browser_dir) {
    if (dev_browser_dir.empty()) return;
    if (port_open(9928)) {
        printf("[server] virt-chrome-bridge already running on :9928\n");
        return;
    }
    // Locate node binary
    const char* node = nullptr;
    for (auto* p : {"/opt/homebrew/bin/node", "/usr/local/bin/node", "/usr/bin/node"}) {
        if (access(p, X_OK) == 0) { node = p; break; }
    }
    if (!node) {
        printf("[server] node not found -- virt-chrome-bridge will not start\n");
        return;
    }
    std::string script = dev_browser_dir + "/../imgui-browser/src/dev-src/virt-chrome-bridge.js";
    std::string cwd    = dev_browser_dir + "/../imgui-browser/src/dev-src";
    if (access(script.c_str(), R_OK) != 0) {
        printf("[server] virt-chrome-bridge.js not found at %s\n", script.c_str());
        return;
    }
    std::string log_path = dev_browser_dir + "/../imgui-browser/debug/virt-chrome-bridge.log";
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }
    if (pid == 0) {
        FILE* lf = fopen(log_path.c_str(), "a");
        if (lf) {
            dup2(fileno(lf), STDOUT_FILENO);
            dup2(fileno(lf), STDERR_FILENO);
            fclose(lf);
        }
        chdir(cwd.c_str());
        execlp(node, node, script.c_str(), nullptr);
        _exit(127);
    }
    s_procs.push_back(pid);
    printf("[server] virt-chrome-bridge started (PID %d)\n", pid);
}

void server_start_cf_bridge(const std::string& dev_browser_dir) {
    if (dev_browser_dir.empty()) {
        printf("[server] cf_bridge: dev_browser_dir not set -- skipping\n");
        return;
    }
    if (port_open(9925)) {
        printf("[server] cf_bridge already running on :9925\n");
        return;
    }
    // Runner script
    std::string runner = dev_browser_dir + "/cf_bridge_runner.py";
    if (access(runner.c_str(), R_OK) != 0) {
        printf("[server] cf_bridge_runner.py not found at %s\n", runner.c_str());
        return;
    }
    // Find python3 in the project .venv first, then fall back to system python3
    std::string venv_py = dev_browser_dir + "/../.venv/bin/python3";
    const char* python  = nullptr;
    if (access(venv_py.c_str(), X_OK) == 0) {
        python = venv_py.c_str();
    } else {
        for (auto* p : {"/opt/homebrew/bin/python3",
                        "/usr/local/bin/python3",
                        "/usr/bin/python3"}) {
            if (access(p, X_OK) == 0) { python = p; break; }
        }
    }
    if (!python) {
        printf("[server] python3 not found -- cf_bridge will not start\n");
        return;
    }
    std::string log_path = dev_browser_dir + "/../imgui-browser/debug/cf_bridge.log";

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }
    if (pid == 0) {
        // Child -- redirect stdout+stderr to log file
        FILE* lf = fopen(log_path.c_str(), "a");
        if (lf) {
            dup2(fileno(lf), STDOUT_FILENO);
            dup2(fileno(lf), STDERR_FILENO);
            fclose(lf);
        }
        chdir(dev_browser_dir.c_str());
        execlp(python, python, runner.c_str(), nullptr);
        _exit(127);
    }
    s_procs.push_back(pid);
    printf("[server] cf_bridge started (PID %d) python=%s\n", pid, python);
}
