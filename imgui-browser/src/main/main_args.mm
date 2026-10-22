// main_args.mm -- CLI argument parser.
// Uses _NSGetExecutablePath to derive apps_dir relative to the binary.

#include "main_args.h"
#include "platform/xcm_paths.h"
#include <cstdlib>

Args parse_args(int argc, char** argv) {
    Args a;
    // apps_dir: the PHP dev-tool apps served on php_port.  Resolved from the
    // project root, so flat dev builds, .app bundles and installed copies all
    // agree.  An explicit --apps-dir on argv still wins (see loop below).
    a.apps_dir = xcm::first_existing_dir({
        "../dev-tools/dev-browser/apps",
        "src/apps",
        "../apps"});
    if (a.apps_dir.empty()) a.apps_dir = xcm::asset_path("src/apps");
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
