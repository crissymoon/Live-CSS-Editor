#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace xcm {

struct HttpResponse {
    bool ok = false;
    int status_code = 0;
    std::vector<std::string> headers;
    std::string body;
    std::string error;
};

// Simple HTTP/1.1 GET fetcher using Berkeley sockets.
// Supports only plain http:// URLs (no TLS/https).
HttpResponse http_fetch_get(const std::string& url, int timeout_ms = 5000);

} // namespace xcm
