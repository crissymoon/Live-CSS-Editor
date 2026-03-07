// main_args.mm -- CLI argument parser.
// Uses _NSGetExecutablePath to derive apps_dir relative to the binary.

#include "main_args.h"
#include <mach-o/dyld.h>
#include <cstdlib>

Args parse_args(int argc, char** argv) {
    Args a;
    // Derive apps_dir relative to the executable (binary lives in build/).
    char exe_path[1024] = {};
    uint32_t sz = sizeof(exe_path);
    _NSGetExecutablePath(exe_path, &sz);
    std::string exe_str(exe_path);
    auto pos = exe_str.rfind('/');
    if (pos != std::string::npos) {
        std::string parent = exe_str.substr(0, pos);
        auto p2 = parent.rfind('/');
        if (p2 != std::string::npos) {
            std::string root = parent.substr(0, p2);
            a.apps_dir = root + "/../dev-browser/apps";
        }
    }
    for (int i = 1; i < argc; i++) {
        std::string s(argv[i]);
        if ((s == "--url")      && i+1 < argc) { a.url      = argv[++i]; continue; }
        if ((s == "--apps-dir") && i+1 < argc) { a.apps_dir = argv[++i]; continue; }
        if ((s == "--php-port") && i+1 < argc) { a.php_port = atoi(argv[++i]); continue; }
        if ((s == "--cmd-port") && i+1 < argc) { a.cmd_port = atoi(argv[++i]); continue; }
        if ((s == "--width")    && i+1 < argc) { a.win_w    = atoi(argv[++i]); continue; }
        if ((s == "--height")   && i+1 < argc) { a.win_h    = atoi(argv[++i]); continue; }
        if (s == "--clear-data")               { a.clear_data = true; continue; }
    }
    return a;
}
