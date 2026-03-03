/* db-browser - SQLite Database Browser
 * Crissy's DB Browser
 * Built with GTK+3
 */

#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "core/db_manager.h"
#include "ui/tooltips.h"

#include "modules/app_state.h"
#include "modules/ui_drawing.h"
#include "modules/ui_utils.h"
#include "modules/theme_manager.h"
#include "modules/recent_manager.h"
#include "modules/ui_panels.h"
#include "modules/db_callbacks.h"
#include "modules/query_callbacks.h"
#include "modules/table_callbacks.h"
#include "modules/csv_handler.h"
#include "modules/misc_callbacks.h"

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Determine executable directory for resource loading
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char *dir = dirname(exe_path);
        strncpy(app_exe_dir, dir, sizeof(app_exe_dir) - 1);
    } else {
        getcwd(app_exe_dir, sizeof(app_exe_dir));
    }

    // Allocate and initialize application state
    app = g_new0(AppState, 1);
    app->db_manager = NULL;
    app->tooltip_manager = tooltip_manager_create();
    app->current_table = NULL;
    app->modified = false;
    app->recent_paths = NULL;
    app->theme_is_dark = false;

    // Load theme preference
    app->theme_is_dark = load_theme_pref();

    // Create and load CSS provider
    app_css_provider = gtk_css_provider_new();
    const char *css_file = app->theme_is_dark ? "theme.css" : "theme-simple.css";
    char css_path[2048];
    snprintf(css_path, sizeof(css_path), "%s/css/%s", app_exe_dir, css_file);
    FILE *f = fopen(css_path, "r");
    if (f) {
        fclose(f);
    } else {
        snprintf(css_path, sizeof(css_path), "css/%s", css_file);
    }

    GError *error = NULL;
    if (gtk_css_provider_load_from_path(app_css_provider, css_path, &error)) {
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(app_css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
    } else {
        g_warning("Failed to load CSS: %s", error ? error->message : "unknown error");
        if (error) g_error_free(error);
    }

    // Load recent databases
    load_recent_dbs(app);

    // Create main application window
    create_main_window(app);

    // Show the window
    gtk_widget_show_all(app->window);

    // Run GTK main loop
    gtk_main();

    // Cleanup
    if (app->db_manager) {
        db_manager_destroy(app->db_manager);
    }
    if (app->tooltip_manager) {
        tooltip_manager_destroy(app->tooltip_manager);
    }
    if (app->current_table) {
        g_free(app->current_table);
    }
    if (app->recent_paths) {
        g_list_free_full(app->recent_paths, g_free);
    }
    if (app_css_provider) {
        g_object_unref(app_css_provider);
    }
    g_free(app);

    return 0;
}

