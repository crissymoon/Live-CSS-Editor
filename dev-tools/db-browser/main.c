/* db-browser - SQLite Database Browser
 * Crissy's DB Browser
 * Built with GTK+3
 */

#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
static char *_compat_realpath(const char *path, char *resolved) {
    return GetFullPathNameA(path, 1024, resolved, NULL) ? resolved : NULL;
}
static char *_compat_dirname(char *path) {
    char *sep = strrchr(path, '\\');
    if (!sep) sep = strrchr(path, '/');
    if (sep) *sep = '\0';
    return path;
}
#define realpath(p, r) _compat_realpath(p, r)
#define dirname(p)     _compat_dirname(p)
#else
#include <unistd.h>
#include <libgen.h>
#endif

#include "core/db_manager.h"
#include "core/db_transfer.h"
#include "ui/tooltips.h"

#include "modules/all_modules.h"

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Determine executable directory for resource loading
    // On macOS, use argv[0] since /proc/self/exe doesn't exist
    char exe_path[1024];
    if (realpath(argv[0], exe_path)) {
        char *dir = dirname(exe_path);
        strncpy(app_exe_dir, dir, sizeof(app_exe_dir) - 1);
        app_exe_dir[sizeof(app_exe_dir) - 1] = '\0';
    } else {
        getcwd(app_exe_dir, sizeof(app_exe_dir));
    }

    // Initialize database storage directory
    char db_storage_path[2048];
    snprintf(db_storage_path, sizeof(db_storage_path), "%s/databases", app_exe_dir);
    if (db_transfer_init(db_storage_path)) {
        printf("[db-browser] Database storage initialized: %s\n", db_storage_path);
    } else {
        fprintf(stderr, "[db-browser] Warning: Failed to initialize database storage\n");
    }

    // Allocate and initialize application state
    app = g_new0(AppState, 1);
    app->db_manager = NULL;
    app->tooltip_manager = tooltip_manager_create();
    app->current_table = NULL;
    app->modified = false;
    app->recent_paths = NULL;
    app->theme_is_dark = false;
    app->query_dirty = false;
    app->last_saved_query = NULL;
    
    // Initialize authentication
    auth_init();
    app->auth_session.authenticated = false;
    app->auth_session.username = NULL;
    app->auth_session.role = ROLE_VIEWER;
    app->auth_session.login_time = 0;
    
    // Show login dialog
    if (!auth_show_login(NULL, &app->auth_session)) {
        fprintf(stderr, "[db-browser] Authentication failed or cancelled\n");
        g_free(app);
        return 1;
    }
    
    printf("[db-browser] User '%s' logged in as %s\n", 
           app->auth_session.username, 
           auth_get_role_name(app->auth_session.role));

    // Load theme preference
    app->theme_is_dark = load_theme_pref();

    // Create and load CSS provider
    app_css_provider = gtk_css_provider_new();
    const char *css_file = app->theme_is_dark ? "theme.css" : "theme-simple.css";
    char css_path[2048];
    snprintf(css_path, sizeof(css_path), "%s/css/%s", app_exe_dir, css_file);

    GError *error = NULL;
    if (gtk_css_provider_load_from_path(app_css_provider, css_path, &error)) {
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(app_css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
        printf("[db-browser] Loaded CSS theme: %s\n", css_path);
    } else {
        fprintf(stderr, "[db-browser] Failed to load CSS from %s: %s\n", 
                css_path, error ? error->message : "unknown error");
        if (error) g_error_free(error);
    }

    // Create main application window
    create_main_window(app);

    // Load recent databases after UI is created
    load_recent_dbs(app);

    // Show the window
    gtk_widget_show_all(app->window);

    // Run GTK main loop
    gtk_main();

    // Cleanup
    
    // Cleanup authentication
    auth_logout(&app->auth_session);
    
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
    if (app->last_saved_query) {
        g_free(app->last_saved_query);
    }
    if (app_css_provider) {
        g_object_unref(app_css_provider);
    }
    g_free(app);

    return 0;
}

