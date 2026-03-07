// main_servers.mm -- Subprocess lifecycle: PHP and Node.

#include "main_priv.h"
#include "main_args.h"
#include "../cmds-and-server/server_manager.h"

void start_all_servers(const Args& args) {
    // PHP built-in server -- serves the live-css web app.
    if (!args.apps_dir.empty()) {
        server_start_php(args.apps_dir, args.php_port);
    }

    // Node image-cache server (script lives in dev-browser/src/).
    {
        std::string node_script;
        auto sl = args.apps_dir.rfind('/');
        if (sl != std::string::npos)
            node_script = args.apps_dir.substr(0, sl) + "/src/image-cache-server.js";
        server_start_node(node_script);
    }

}
