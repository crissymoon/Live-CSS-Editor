#include "xcm_http_fetch.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: xcm_http_fetch <http://host/path> [timeout_ms]\n";
        return 2;
    }

    int timeout_ms = 5000;
    if (argc > 2) {
        timeout_ms = std::atoi(argv[2]);
        if (timeout_ms <= 0) timeout_ms = 5000;
    }

    xcm::HttpResponse r = xcm::http_fetch_get(argv[1], timeout_ms);

    if (!r.headers.empty()) {
        std::cout << "Status: " << r.status_code << "\n";
        for (const auto& h : r.headers) {
            std::cout << h << "\n";
        }
        std::cout << "\n";
    }

    if (!r.ok) {
        std::cerr << "Fetch failed: " << r.error << "\n";
        if (!r.body.empty()) std::cerr << r.body << "\n";
        return 1;
    }

    std::cout << r.body;
    return 0;
}
