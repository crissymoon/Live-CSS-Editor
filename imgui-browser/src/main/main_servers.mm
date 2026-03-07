// main_servers.mm -- Subprocess lifecycle: PHP, Node, and PHP-WASM.

#include "main_priv.h"
#include "main_args.h"
#include "main_globals.h"
#include "../cmds-and-server/server_manager.h"

void start_all_servers(const Args& args) {
    // PHP built-in server -- serves the live-css web app.
    if (!args.apps_dir.empty()) {
        server_start_php(args.apps_dir, args.php_port);
    }

    // Node image-cache server (script lives in dev-tools/dev-browser/src/).
    {
        std::string node_script;
        auto sl = args.apps_dir.rfind('/');
        if (sl != std::string::npos)
            node_script = args.apps_dir.substr(0, sl) + "/src/image-cache-server.js";
        server_start_node(node_script);
    }

    // PHP-WASM dev server -- only started when wasm_enabled_override == 1.
    // The caller (main.mm) resolves the value from settings + argv before
    // calling this function, so -1 will never reach here.
    if (args.wasm_enabled_override == 1) {
        int  port    = (args.wasm_port > 0) ? args.wasm_port : 8082;
        server_start_wasm(args.wasm_dir, port);
    }

}
