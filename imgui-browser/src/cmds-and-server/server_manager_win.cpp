// server_manager_win.cpp -- Windows process/port manager
// Starts PHP/Node child processes and tracks them for shutdown.

#include "server_manager.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

static std::vector<PROCESS_INFORMATION> s_procs;
static bool s_winsock_ready = false;

static void ensure_winsock() {
    if (s_winsock_ready) return;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) {
        s_winsock_ready = true;
    }
}

static bool port_open(int port) {
    ensure_winsock();
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    bool ok = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
    closesocket(sock);
    return ok;
}

static std::string quote_arg(const std::string& s) {
    if (s.find(' ') == std::string::npos && s.find('\t') == std::string::npos) return s;
    return "\"" + s + "\"";
}

static bool spawn(const std::vector<std::string>& args, const std::string& cwd = "") {
    if (args.empty()) return false;

    std::string cmd;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) cmd += ' ';
        cmd += quote_arg(args[i]);
    }

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        nullptr,
        cmd.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        cwd.empty() ? nullptr : cwd.c_str(),
        &si,
        &pi
    );

    if (!ok) {
        fprintf(stderr, "[server] CreateProcess failed for: %s (err=%lu)\n", cmd.c_str(), GetLastError());
        return false;
    }

    CloseHandle(pi.hThread);
    s_procs.push_back(pi);
    return true;
}

static std::string parent_dir(const std::string& path) {
    size_t p = path.find_last_of("/\\");
    if (p == std::string::npos) return "";
    return path.substr(0, p);
}

void server_start_php(const std::string& apps_dir, int php_port) {
    if (port_open(php_port)) {
        printf("[server] PHP already running on :%d\n", php_port);
        return;
    }
    char bind[64];
    snprintf(bind, sizeof(bind), "127.0.0.1:%d", php_port);

    if (spawn({"php", "-S", bind, "-t", apps_dir}, apps_dir)) {
        printf("[server] PHP started on :%d -> %s\n", php_port, apps_dir.c_str());
    } else {
        printf("[server] php not found or failed to start\n");
    }
}

void server_start_node(const std::string& node_script) {
    if (node_script.empty()) return;
    if (port_open(7779)) {
        printf("[server] Node already running on :7779\n");
        return;
    }
    std::string cwd = parent_dir(node_script);
    if (spawn({"node", node_script}, cwd)) {
        printf("[server] Node started on :7779\n");
    } else {
        printf("[server] node not found or failed to start\n");
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

    std::string server_js = wasm_dir + "/server.js";
    DWORD attr = GetFileAttributesA(server_js.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        printf("[server] WASM server.js not found at %s\n", server_js.c_str());
        return;
    }

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    if (spawn({"node", server_js, port_str}, wasm_dir)) {
        printf("[server] PHP-WASM server started on :%d -> %s\n", port, wasm_dir.c_str());
    } else {
        printf("[server] node not found or failed to start WASM\n");
    }
}

std::string server_find_wasm_dir(const std::string& apps_dir) {
    std::vector<std::string> candidates;
    if (!apps_dir.empty()) {
        std::string a = apps_dir;
        for (char& ch : a) if (ch == '\\') ch = '/';
        auto p = a.find("/dev-browser/apps");
        if (p != std::string::npos) {
            std::string root = a.substr(0, p);
            candidates.push_back(root + "/imgui-browser/php-wasm-project");
        }
    }

    for (const auto& c : candidates) {
        std::string server_js = c + "/server.js";
        DWORD attr = GetFileAttributesA(server_js.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES) return c;
    }
    return "";
}

ServerStatus server_poll_status(int php_port, int node_port) {
    ServerStatus st;
    st.php_ok = port_open(php_port);
    st.node_ok = port_open(node_port);
    return st;
}

void server_shutdown() {
    for (auto& pi : s_procs) {
        if (pi.hProcess) {
            TerminateProcess(pi.hProcess, 0);
            WaitForSingleObject(pi.hProcess, 1500);
            CloseHandle(pi.hProcess);
            pi.hProcess = nullptr;
        }
    }
    s_procs.clear();

    if (s_winsock_ready) {
        WSACleanup();
        s_winsock_ready = false;
    }
}
