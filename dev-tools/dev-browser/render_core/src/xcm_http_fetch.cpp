#include "xcm_http_fetch.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace xcm {
namespace {

struct ParsedUrl {
    bool ok = false;
    std::string host;
    std::string port;
    std::string path;
    std::string error;
};

std::string lower_copy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

ParsedUrl parse_http_url(const std::string& url) {
    ParsedUrl out;
    const std::string prefix = "http://";
    if (url.rfind(prefix, 0) != 0) {
        out.error = "only http:// URLs are supported";
        return out;
    }

    std::string rest = url.substr(prefix.size());
    if (rest.empty()) {
        out.error = "missing host";
        return out;
    }

    std::string host_port;
    std::string path = "/";
    std::size_t slash = rest.find('/');
    if (slash == std::string::npos) {
        host_port = rest;
    } else {
        host_port = rest.substr(0, slash);
        path = rest.substr(slash);
        if (path.empty()) path = "/";
    }

    if (host_port.empty()) {
        out.error = "missing host";
        return out;
    }

    std::string host = host_port;
    std::string port = "80";

    // IPv6 literals in [::1]:8080 form
    if (!host_port.empty() && host_port.front() == '[') {
        std::size_t end = host_port.find(']');
        if (end == std::string::npos) {
            out.error = "invalid IPv6 host";
            return out;
        }
        host = host_port.substr(1, end - 1);
        if (end + 1 < host_port.size()) {
            if (host_port[end + 1] != ':') {
                out.error = "invalid host:port format";
                return out;
            }
            port = host_port.substr(end + 2);
        }
    } else {
        std::size_t colon = host_port.rfind(':');
        if (colon != std::string::npos && host_port.find(':') == colon) {
            host = host_port.substr(0, colon);
            port = host_port.substr(colon + 1);
        }
    }

    if (host.empty()) {
        out.error = "missing host";
        return out;
    }
    if (port.empty()) port = "80";

    out.ok = true;
    out.host = host;
    out.port = port;
    out.path = path;
    return out;
}

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
inline void close_socket(SocketHandle s) { closesocket(s); }
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
inline void close_socket(SocketHandle s) { close(s); }
#endif

bool set_timeouts(SocketHandle s, int timeout_ms) {
#ifdef _WIN32
    DWORD to = static_cast<DWORD>(std::max(1, timeout_ms));
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&to), sizeof(to)) != 0) return false;
    if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&to), sizeof(to)) != 0) return false;
#else
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) return false;
    if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) return false;
#endif
    return true;
}

int parse_status_code(const std::string& status_line) {
    // Expected: HTTP/1.1 200 OK
    std::istringstream ss(status_line);
    std::string http_ver;
    int code = 0;
    ss >> http_ver >> code;
    if (http_ver.rfind("HTTP/", 0) != 0) return 0;
    return code;
}

std::string trim_copy(const std::string& s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

bool header_value(const std::vector<std::string>& headers,
                  const std::string& name,
                  std::string& out_value) {
    const std::string needle = lower_copy(name);
    for (const auto& h : headers) {
        std::size_t p = h.find(':');
        if (p == std::string::npos) continue;
        std::string hn = lower_copy(trim_copy(h.substr(0, p)));
        if (hn == needle) {
            out_value = trim_copy(h.substr(p + 1));
            return true;
        }
    }
    return false;
}

std::string resolve_redirect_url(const std::string& base_url, const std::string& loc) {
    if (loc.rfind("http://", 0) == 0) return loc;
    if (loc.empty()) return "";

    ParsedUrl b = parse_http_url(base_url);
    if (!b.ok) return "";

    if (loc[0] == '/') {
        return "http://" + b.host + ":" + b.port + loc;
    }

    std::string base_path = b.path;
    std::size_t slash = base_path.find_last_of('/');
    if (slash == std::string::npos) base_path = "/";
    else base_path = base_path.substr(0, slash + 1);
    return "http://" + b.host + ":" + b.port + base_path + loc;
}

bool decode_chunked_body(const std::string& in, std::string& out) {
    out.clear();
    std::size_t pos = 0;
    while (pos < in.size()) {
        std::size_t line_end = in.find("\r\n", pos);
        if (line_end == std::string::npos) return false;
        std::string len_hex = in.substr(pos, line_end - pos);
        std::size_t sc = len_hex.find(';');
        if (sc != std::string::npos) len_hex = len_hex.substr(0, sc);
        len_hex = trim_copy(len_hex);
        if (len_hex.empty()) return false;

        std::size_t len = 0;
        std::istringstream ss(len_hex);
        ss >> std::hex >> len;
        if (!ss.good() && !ss.eof()) return false;

        pos = line_end + 2;
        if (len == 0) {
            // Ignore trailer headers for now.
            return true;
        }
        if (pos + len + 2 > in.size()) return false;
        out.append(in, pos, len);
        pos += len;
        if (in[pos] != '\r' || in[pos + 1] != '\n') return false;
        pos += 2;
    }
    return false;
}

HttpResponse http_fetch_get_once(const std::string& url, int timeout_ms) {
    HttpResponse out;
    if (timeout_ms <= 0) timeout_ms = 5000;

    ParsedUrl p = parse_http_url(url);
    if (!p.ok) {
        out.error = p.error;
        return out;
    }

#ifdef _WIN32
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        out.error = "WSAStartup failed";
        return out;
    }
#endif

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* head = nullptr;
    int gai = getaddrinfo(p.host.c_str(), p.port.c_str(), &hints, &head);
    if (gai != 0) {
#ifdef _WIN32
        out.error = "getaddrinfo failed";
        WSACleanup();
#else
        out.error = gai_strerror(gai);
#endif
        return out;
    }

    SocketHandle sock = kInvalidSocket;
    for (addrinfo* ai = head; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock == kInvalidSocket) continue;

        set_timeouts(sock, timeout_ms);

        if (connect(sock, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) == 0) {
            break;
        }
        close_socket(sock);
        sock = kInvalidSocket;
    }

    freeaddrinfo(head);

    if (sock == kInvalidSocket) {
        out.error = "connect failed";
#ifdef _WIN32
        WSACleanup();
#endif
        return out;
    }

    std::ostringstream req;
    req << "GET " << p.path << " HTTP/1.1\r\n"
        << "Host: " << p.host << "\r\n"
        << "User-Agent: xcm-http-fetch/1.0\r\n"
        << "Connection: close\r\n\r\n";
    const std::string reqs = req.str();

    std::size_t sent_total = 0;
    while (sent_total < reqs.size()) {
#ifdef _WIN32
        int n = send(sock, reqs.data() + sent_total, static_cast<int>(reqs.size() - sent_total), 0);
#else
        ssize_t n = send(sock, reqs.data() + sent_total, reqs.size() - sent_total, 0);
#endif
        if (n <= 0) {
            out.error = "send failed";
            close_socket(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            return out;
        }
        sent_total += static_cast<std::size_t>(n);
    }

    std::string raw;
    raw.reserve(4096);
    char buf[4096];
    while (true) {
#ifdef _WIN32
        int n = recv(sock, buf, static_cast<int>(sizeof(buf)), 0);
#else
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
#endif
        if (n <= 0) break;
        raw.append(buf, static_cast<std::size_t>(n));
    }

    close_socket(sock);
#ifdef _WIN32
    WSACleanup();
#endif

    if (raw.empty()) {
        out.error = "empty response";
        return out;
    }

    std::size_t hdr_end = raw.find("\r\n\r\n");
    if (hdr_end == std::string::npos) {
        out.error = "invalid HTTP response";
        return out;
    }

    std::string head_block = raw.substr(0, hdr_end);
    out.body = raw.substr(hdr_end + 4);

    std::istringstream hs(head_block);
    std::string line;
    if (!std::getline(hs, line)) {
        out.error = "missing status line";
        return out;
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();

    out.status_code = parse_status_code(line);
    while (std::getline(hs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) out.headers.push_back(line);
    }

    std::string te;
    if (header_value(out.headers, "Transfer-Encoding", te)) {
        std::string lte = lower_copy(te);
        if (lte.find("chunked") != std::string::npos) {
            std::string decoded;
            if (!decode_chunked_body(out.body, decoded)) {
                out.ok = false;
                out.error = "invalid chunked encoding";
                return out;
            }
            out.body = std::move(decoded);
        }
    }

    out.ok = out.status_code >= 200 && out.status_code < 400;
    if (!out.ok && out.error.empty()) {
        out.error = "http status " + std::to_string(out.status_code);
    }

    return out;
}

} // namespace

HttpResponse http_fetch_get(const std::string& url, int timeout_ms) {
    constexpr int kMaxRedirects = 5;
    std::string current = url;
    HttpResponse last;

    for (int i = 0; i <= kMaxRedirects; ++i) {
        last = http_fetch_get_once(current, timeout_ms);
        if (!last.ok) return last;

        if (last.status_code == 301 || last.status_code == 302 ||
            last.status_code == 303 || last.status_code == 307 ||
            last.status_code == 308) {
            std::string loc;
            if (!header_value(last.headers, "Location", loc)) return last;

            std::string next = resolve_redirect_url(current, loc);
            if (next.empty()) {
                last.ok = false;
                last.error = "redirect location invalid";
                return last;
            }
            current = next;
            continue;
        }

        return last;
    }

    last.ok = false;
    last.error = "too many redirects";
    return last;
}

} // namespace xcm
