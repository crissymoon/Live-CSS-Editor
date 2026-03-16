// main_args.mm -- CLI argument parser.
// Uses _NSGetExecutablePath to derive apps_dir relative to the binary.

#include "main_args.h"
#include <cstdlib>

#if defined(__APPLE__)
#  include <mach-o/dyld.h>
#elif defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#elif defined(__linux__)
#  include <unistd.h>
#  include <limits.h>
#endif

Args parse_args(int argc, char** argv) {
    Args a;
    // Derive apps_dir relative to the executable (binary lives in build/).
    char exe_path[1024] = {};
    
#if defined(__APPLE__)
    uint32_t sz = sizeof(exe_path);
    _NSGetExecutablePath(exe_path, &sz);
#elif defined(_WIN32)
    GetModuleFileNameA(nullptr, exe_path, (DWORD)sizeof(exe_path));
#elif defined(__linux__)
    ssize_t n = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (n > 0) exe_path[n] = '\0';
#endif

    std::string exe_str(exe_path);

#if defined(_WIN32)
    auto pos = exe_str.rfind('\\');
#else
    auto pos = exe_str.rfind('/');
#endif

    if (pos != std::string::npos) {
        std::string parent = exe_str.substr(0, pos);
#if defined(_WIN32)
        auto p2 = parent.rfind('\\');
#else
        auto p2 = parent.rfind('/');
#endif
        if (p2 != std::string::npos) {
            std::string root = parent.substr(0, p2);
#if defined(_WIN32)
            a.apps_dir = root + "\\..\\dev-tools\\dev-browser\\apps";
#else
            a.apps_dir = root + "/../dev-tools/dev-browser/apps";
#endif
        }
    }
    for (int i = 1; i < argc; i++) {
        std::string s(argv[i]);
        if ((s == "--url")      && i+1 < argc) { a.url = argv[++i]; a.url_explicit = true; continue; }
        if ((s == "--apps-dir") && i+1 < argc) { a.apps_dir = argv[++i]; continue; }
        if ((s == "--php-port") && i+1 < argc) { a.php_port = atoi(argv[++i]); continue; }
        if ((s == "--cmd-port") && i+1 < argc) { a.cmd_port = atoi(argv[++i]); continue; }
        if ((s == "--width")    && i+1 < argc) { a.win_w    = atoi(argv[++i]); continue; }
        if ((s == "--height")   && i+1 < argc) { a.win_h    = atoi(argv[++i]); continue; }
        if ((s == "--wasm-port") && i+1 < argc) { a.wasm_port = atoi(argv[++i]); continue; }
        if ((s == "--wasm-dir")  && i+1 < argc) { a.wasm_dir  = argv[++i];      continue; }
        if (s == "--wasm")                      { a.wasm_enabled_override = 1;                  continue; }
        if (s == "--no-wasm")                   { a.wasm_enabled_override = 0;                  continue; }
        if (s == "--clear-data")                { a.clear_data = true;                          continue; }
        if ((s == "--ui-mode") && i+1 < argc)   { a.ui_mode_override = argv[++i];               continue; }
    }
    return a;
}
