// main_args.h -- CLI argument struct and parser.
#pragma once
#include <string>

struct Args {
    std::string url        = "https://localhost:8443/pb_admin/dashboard.php";
    std::string apps_dir   = "";
    int         php_port   = 9879;
    int         cmd_port   = 9878;
    int         win_w      = 1400;
    int         win_h      = 900;
    bool        clear_data = false;
};

Args parse_args(int argc, char** argv);
