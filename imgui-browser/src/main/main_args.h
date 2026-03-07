// main_args.h -- CLI argument struct and parser.
#pragma once
#include <string>

struct Args {
    std::string url        = "https://localhost:8443/page-builder/pb_admin/dashboard.php";
    bool        url_explicit = false;   // true only when --url was passed on argv
    std::string apps_dir   = "";
    int         php_port   = 9879;
    int         cmd_port   = 9878;
    int         win_w      = 1400;
    int         win_h      = 900;
    bool        clear_data = false;
    // WASM dev server overrides (argv can override settings.json)
    int         wasm_port  = 0;    // 0 = use value from settings
    std::string wasm_dir;          // empty = use value from settings / auto-detect
    int         wasm_enabled_override = -1;  // -1 = unset, 0 = off, 1 = on
    // UI mode override from --ui-mode <full|no_tabs|grab_bar_only>
    // Empty string = no override; settings.json ui_mode used instead.
    std::string ui_mode_override;
};

Args parse_args(int argc, char** argv);
