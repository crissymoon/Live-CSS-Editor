/* db-browser - SQLite Database Browser
 * Crissy's DB Browser
 * Built with GTK+3
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/db_manager.h"
#include "ui/tooltips.h"

/* Application state */
typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *status_bar;
    GtkWidget *table_view;
    GtkWidget *query_editor;
    GtkWidget *result_view;
    GtkWidget *data_view;     // For Data Browser tab
    GtkWidget *data_info_label; // Info label in Data Browser

    DBManager *db_manager;
    TooltipManager *tooltip_manager;

    char *current_table;
    bool modified;
    GtkWidget *recent_btn;     /* "Recent" menu button in toolbar */
    GList    *recent_paths;    /* GList of gchar*, most-recent first, max 10 */

    bool       theme_is_dark;  /* true = dark (Crissy), false = light (simple) */
    GtkWidget *theme_btn;      /* toolbar button used to toggle theme label */
} AppState;

/* Global app state */
static AppState       *app              = NULL;
static GtkCssProvider *app_css_provider = NULL;  /* active CSS provider */
static char            app_exe_dir[1024] = {0};  /* directory of the binary */

/* Forward declarations */
static void create_main_window(AppState *state);
static void create_menu_bar(AppState *state);
static void create_toolbar(AppState *state);
static void create_table_panel(AppState *state);
static void create_query_panel(AppState *state);
static void create_data_panel(AppState *state);

/* Callbacks */
static void on_open_database(GtkWidget *widget, gpointer data);
static void on_new_database(GtkWidget *widget, gpointer data);
static void on_close_database(GtkWidget *widget, gpointer data);
static void on_quit(GtkWidget *widget, gpointer data);
static void on_refresh_tables(GtkWidget *widget, gpointer data);
static void refresh_table_list(AppState *state);
static void on_table_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
static void show_table_data(AppState *state, const char *table_name);
static void on_template_clicked(GtkWidget *button, gpointer data);
static void on_sql_query_builder(GtkWidget *widget, gpointer data);
static void on_new_table(GtkWidget *widget, gpointer data);
static void on_drop_table(GtkWidget *widget, gpointer data);
static void on_execute_query(GtkWidget *widget, gpointer data);
static void on_import_csv(GtkWidget *widget, gpointer data);
static void on_export_csv(GtkWidget *widget, gpointer data);
static void on_backup_database(GtkWidget *widget, gpointer data);
static void on_help_tutorial(GtkWidget *widget, gpointer data);
static void on_help_about(GtkWidget *widget, gpointer data);
static void on_open_recent_db(GtkWidget *widget, gpointer data);
static void on_open_sql_file(GtkWidget *widget, gpointer data);
static void on_toggle_theme(GtkWidget *widget, gpointer data);

/* Theme preference helpers */
static bool load_theme_pref(void);
static void save_theme_pref(bool is_dark);

/* Recent database helpers */
static char  *get_recent_config_path(void);
static void   populate_recent_combo(AppState *state);
static void   load_recent_dbs(AppState *state);
static void   save_recent_dbs(AppState *state);
static void   add_to_recent(AppState *state, const char *path);

/* Utility functions */
static void update_status(const char *message);
static void show_error_dialog(const char *title, const char *message);
static void show_info_dialog(const char *title, const char *message);
static bool confirm_action(const char *message);

/* Main entry point */
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    /* Save the executable directory so on_toggle_theme can locate css files */
    if (argv[0] && strrchr(argv[0], '/')) {
        snprintf(app_exe_dir, sizeof(app_exe_dir), "%.*s",
                 (int)(strrchr(argv[0], '/') - argv[0]), argv[0]);
    } else {
        snprintf(app_exe_dir, sizeof(app_exe_dir), ".");
    }

    /* Load saved theme preference (dark = default) */
    bool pref_dark     = load_theme_pref();
    const char *css_file = pref_dark ? "theme.css" : "theme-simple.css";

    /* Load app theme CSS */
    GtkCssProvider *css_provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen  *screen  = gdk_display_get_default_screen(display);

    /* Build the CSS path from exe dir; fall back to cwd-relative */
    char css_path[1024];
    snprintf(css_path, sizeof(css_path), "%s/css/%s", app_exe_dir, css_file);
    FILE *css_check = fopen(css_path, "r");
    if (css_check) {
        fclose(css_check);
    } else {
        snprintf(css_path, sizeof(css_path), "css/%s", css_file);
    }

    GError *error = NULL;
    if (gtk_css_provider_load_from_path(css_provider, css_path, &error)) {
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        app_css_provider = css_provider;
        printf("[INFO] Theme loaded: %s\n", css_path);
    } else {
        if (error) { g_error_free(error); }
        g_object_unref(css_provider);
    }

    /* Create application state */
    app = g_malloc0(sizeof(AppState));
    app->theme_is_dark = pref_dark;
    app->tooltip_manager = tooltip_manager_create();
    tooltip_manager_register_all(app->tooltip_manager);

    // Create main window
    create_main_window(app);

    // Load saved recent database paths and populate the dropdown
    load_recent_dbs(app);

    // Open database if provided as argument
    if (argc > 1) {
        app->db_manager = db_manager_create(argv[1], false);
        if (db_manager_open(app->db_manager) == SQLITE_OK) {
            update_status("Database opened successfully");
            // AUTO-POPULATE TABLES ON STARTUP
            refresh_table_list(app);
        } else {
            show_error_dialog("Error", "Failed to open database");
        }
    }

    gtk_widget_show_all(app->window);
    gtk_main();

    // Cleanup
    if (app->db_manager) {
        db_manager_destroy(app->db_manager);
    }
    save_recent_dbs(app);
    if (app->recent_paths) {
        g_list_free_full(app->recent_paths, g_free);
        app->recent_paths = NULL;
    }
    tooltip_manager_destroy(app->tooltip_manager);
    g_free(app->current_table);
    g_free(app);

    return 0;
}

/* Create main window */
static void create_main_window(AppState *state) {
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "Crissy's DB Browser");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 1024, 768);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);

    g_signal_connect(state->window, "destroy", G_CALLBACK(on_quit), NULL);

    // Main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), vbox);

    // Toolbar (serves as combined menu + toolbar)
    create_toolbar(state);

    // Notebook for different views
    state->notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->notebook, TRUE, TRUE, 0);

    // Add panels
    create_table_panel(state);
    create_query_panel(state);
    create_data_panel(state);

    // Status bar
    state->status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->status_bar, FALSE, FALSE, 0);

    update_status("Ready - Open or create a database to begin");
}

/* Create menu bar */
static void create_menu_bar(AppState *state) {
    // TODO: Implement full menu bar with File, Edit, View, Tools, Help
}

/* Theme preference helpers */
static char *get_theme_pref_path(void) {
    const char *cfg = g_get_user_config_dir();
    char *dir  = g_build_filename(cfg, "crissy-db-browser", NULL);
    g_mkdir_with_parents(dir, 0755);
    char *path = g_build_filename(dir, "theme", NULL);
    g_free(dir);
    return path;
}

static bool load_theme_pref(void) {
    char *path = get_theme_pref_path();
    FILE *f = fopen(path, "r");
    g_free(path);
    if (!f) return true;  /* default: dark */
    char buf[32] = {0};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    return strncmp(buf, "light", 5) != 0;
}

static void save_theme_pref(bool is_dark) {
    char *path = get_theme_pref_path();
    FILE *f = fopen(path, "w");
    g_free(path);
    if (!f) return;
    fprintf(f, "%s\n", is_dark ? "dark" : "light");
    fclose(f);
}

/* Theme toggle callback */
static void on_toggle_theme(GtkWidget *widget, gpointer data) {
    AppState   *state   = (AppState *)data;
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen  *screen  = gdk_display_get_default_screen(display);
    (void)widget;

    /* Remove the current provider */
    if (app_css_provider) {
        gtk_style_context_remove_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(app_css_provider));
        g_object_unref(app_css_provider);
        app_css_provider = NULL;
    }

    /* Flip the flag */
    state->theme_is_dark = !state->theme_is_dark;

    /* Build path for new theme */
    const char *css_file = state->theme_is_dark ? "theme.css" : "theme-simple.css";
    char css_path[1024];
    snprintf(css_path, sizeof(css_path), "%s/css/%s", app_exe_dir, css_file);
    FILE *f = fopen(css_path, "r");
    if (f) {
        fclose(f);
    } else {
        snprintf(css_path, sizeof(css_path), "css/%s", css_file);
    }

    /* Load and apply new provider */
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;
    if (gtk_css_provider_load_from_path(provider, css_path, &error)) {
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        app_css_provider = provider;
    } else {
        if (error) { g_error_free(error); }
        g_object_unref(provider);
    }

    /* Update button label to show what clicking next will switch TO */
    if (state->theme_btn) {
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(state->theme_btn),
                                  state->theme_is_dark ? "Light" : "Dark");
    }

    /* Persist the choice */
    save_theme_pref(state->theme_is_dark);
}

/* Create toolbar */
static void create_toolbar(AppState *state) {
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

    // Open database button
    GtkToolItem *btn_open = gtk_tool_button_new(NULL, "Open");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_open), "Open");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_open), "Open an existing SQLite database file");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_database), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_open, -1);

    // New database button
    GtkToolItem *btn_new = gtk_tool_button_new(NULL, "New DB");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_new), "New DB");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_new), "Create a new empty database");
    g_signal_connect(btn_new, "clicked", G_CALLBACK(on_new_database), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_new, -1);

    // Separator
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    // New table button
    GtkToolItem *btn_new_table = gtk_tool_button_new(NULL, "New Table");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_new_table), "New Table");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_new_table), "Create a new table in the open database");
    g_signal_connect(btn_new_table, "clicked", G_CALLBACK(on_new_table), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_new_table, -1);

    // Execute query button
    GtkToolItem *btn_execute = gtk_tool_button_new(NULL, "Run SQL");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_execute), "Run SQL");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_execute), "Execute the query in the SQL editor");
    g_signal_connect(btn_execute, "clicked", G_CALLBACK(on_execute_query), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_execute, -1);

    // Separator
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    // Import button
    GtkToolItem *btn_import = gtk_tool_button_new(NULL, "Import");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_import), "Import");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_import), "Import data from a CSV file");
    g_signal_connect(btn_import, "clicked", G_CALLBACK(on_import_csv), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_import, -1);

    // Export button
    GtkToolItem *btn_export = gtk_tool_button_new(NULL, "Export");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_export), "Export");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_export), "Export table data as CSV");
    g_signal_connect(btn_export, "clicked", G_CALLBACK(on_export_csv), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_export, -1);

    // Backup button
    GtkToolItem *btn_backup = gtk_tool_button_new(NULL, "Backup");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_backup), "Backup");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_backup), "Backup the open database to a file");
    g_signal_connect(btn_backup, "clicked", G_CALLBACK(on_backup_database), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_backup, -1);

    // Separator
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    // Load SQL script file button
    GtkToolItem *btn_sql_file = gtk_tool_button_new(NULL, "Load .sql");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_sql_file), "Load .sql");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_sql_file),
        "Load a .sql script file into the query editor");
    g_signal_connect(btn_sql_file, "clicked", G_CALLBACK(on_open_sql_file), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_sql_file, -1);

    // Separator before recent databases section
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    // Recent databases menu button
    GtkToolItem *recent_item = gtk_tool_item_new();
    state->recent_btn = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(state->recent_btn), "Recent");
    gtk_widget_set_tooltip_text(state->recent_btn,
        "Recently opened databases - click to select one");
    GtkWidget *recent_menu = gtk_menu_new();
    GtkWidget *placeholder = gtk_menu_item_new_with_label("(no recent databases)");
    gtk_widget_set_sensitive(placeholder, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), placeholder);
    gtk_widget_show_all(recent_menu);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(state->recent_btn), recent_menu);
    gtk_container_add(GTK_CONTAINER(recent_item), state->recent_btn);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), recent_item, -1);

    /* Spacer to push the theme toggle to the far right */
    GtkToolItem *spacer = gtk_separator_tool_item_new();
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(spacer), FALSE);
    gtk_tool_item_set_expand(spacer, TRUE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), spacer, -1);

    /* Theme toggle button - label shows what clicking will switch TO */
    GtkToolItem *btn_theme = gtk_tool_button_new(
        NULL, state->theme_is_dark ? "Light" : "Dark");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_theme),
                              state->theme_is_dark ? "Light" : "Dark");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_theme),
                                "Toggle between dark and light theme");
    g_signal_connect(btn_theme, "clicked", G_CALLBACK(on_toggle_theme), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_theme, -1);
    state->theme_btn = GTK_WIDGET(btn_theme);

    gtk_box_pack_start(GTK_BOX(gtk_bin_get_child(GTK_BIN(state->window))),
                       toolbar, FALSE, FALSE, 0);
}

/* Create table panel */
static void create_table_panel(AppState *state) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    // Header label
    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<b>Database Tables</b>");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 4);

    // Table list
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);

    state->table_view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(state->table_view), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(state->table_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(state->table_view), GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);
    g_signal_connect(state->table_view, "row-activated", G_CALLBACK(on_table_row_activated), state);
    gtk_container_add(GTK_CONTAINER(scroll), state->table_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    // Buttons
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_top(hbox, 6);

    GtkWidget *btn_refresh = gtk_button_new_with_label("Refresh");
    gtk_widget_set_tooltip_text(btn_refresh, "Reload the table list from the database");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_refresh_tables), state);
    gtk_box_pack_start(GTK_BOX(hbox), btn_refresh, FALSE, FALSE, 0);

    GtkWidget *btn_query_builder = gtk_button_new_with_label("Query Builder");
    gtk_widget_set_tooltip_text(btn_query_builder, "Open the SQL query builder with templates");
    g_signal_connect(btn_query_builder, "clicked", G_CALLBACK(on_sql_query_builder), state);
    gtk_box_pack_start(GTK_BOX(hbox), btn_query_builder, FALSE, FALSE, 0);

    GtkWidget *btn_drop = gtk_button_new_with_label("Drop Table");
    gtk_widget_set_tooltip_text(btn_drop, "Permanently delete the selected table");
    gtk_widget_set_name(btn_drop, "btn-destructive");
    g_signal_connect(btn_drop, "clicked", G_CALLBACK(on_drop_table), state);
    gtk_box_pack_end(GTK_BOX(hbox), btn_drop, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), vbox,
                            gtk_label_new("Tables"));
}

/* Create query panel */
static void create_query_panel(AppState *state) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    // Header label
    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<b>SQL Query Editor</b>");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 4);

    // Query editor with scrolled window and shadow
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);
    gtk_widget_set_size_request(scroll, -1, 160);

    state->query_editor = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(state->query_editor), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(state->query_editor), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(state->query_editor), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(state->query_editor), 6);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(state->query_editor), 6);
    gtk_container_add(GTK_CONTAINER(scroll), state->query_editor);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, FALSE, FALSE, 0);

    // Button row
    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_top(btn_row, 4);

    GtkWidget *btn_execute = gtk_button_new_with_label("Execute");
    gtk_widget_set_name(btn_execute, "btn-primary");
    gtk_widget_set_tooltip_text(btn_execute, "Run the SQL query above");
    g_signal_connect(btn_execute, "clicked", G_CALLBACK(on_execute_query), state);
    gtk_box_pack_start(GTK_BOX(btn_row), btn_execute, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), btn_row, FALSE, FALSE, 0);

    // Results label
    GtkWidget *results_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(results_label), "<b>Results</b>");
    gtk_widget_set_halign(results_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(results_label, 6);
    gtk_box_pack_start(GTK_BOX(vbox), results_label, FALSE, FALSE, 0);

    // Results view
    GtkWidget *result_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(result_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(result_scroll), GTK_SHADOW_IN);

    state->result_view = gtk_tree_view_new();
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(state->result_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(state->result_view), GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(result_scroll), state->result_view);
    gtk_box_pack_start(GTK_BOX(vbox), result_scroll, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), vbox,
                            gtk_label_new("SQL Query"));
}

/* Create data panel */
static void create_data_panel(AppState *state) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    // Info label
    state->data_info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(state->data_info_label),
        "<b>Data Browser</b>  --  Double-click a table in the Tables tab to view its rows here.");
    gtk_label_set_line_wrap(GTK_LABEL(state->data_info_label), TRUE);
    gtk_widget_set_halign(state->data_info_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), state->data_info_label, FALSE, FALSE, 6);

    // Scrolled window for data view
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN);

    state->data_view = gtk_tree_view_new();
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(state->data_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(state->data_view), GTK_TREE_VIEW_GRID_LINES_BOTH);
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(state->data_view), TRUE);
    gtk_container_add(GTK_CONTAINER(scroll), state->data_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), vbox,
                            gtk_label_new("Data Browser"));
}

/* Callback implementations */
static void on_open_database(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open Database",
                                                     GTK_WINDOW(state->window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Open", GTK_RESPONSE_ACCEPT,
                                                     NULL);

    // Add file filters so the file browser is easier to use
    GtkFileFilter *filter_db = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_db, "SQLite Databases (*.db, *.sqlite, *.sqlite3)");
    gtk_file_filter_add_pattern(filter_db, "*.db");
    gtk_file_filter_add_pattern(filter_db, "*.sqlite");
    gtk_file_filter_add_pattern(filter_db, "*.sqlite3");
    gtk_file_filter_add_pattern(filter_db, "*.DB");
    gtk_file_filter_add_pattern(filter_db, "*.SQLITE");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_db);

    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All Files (*)");
    gtk_file_filter_add_pattern(filter_all, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (state->db_manager) {
            db_manager_destroy(state->db_manager);
        }

        state->db_manager = db_manager_create(filename, false);
        if (!state->db_manager) {
            fprintf(stderr, "[db-browser] on_open_database: db_manager_create failed for '%s'\n", filename);
            show_error_dialog("Error", "Failed to create database manager");
            g_free(filename);
            gtk_widget_destroy(dialog);
            return;
        }

        if (db_manager_open(state->db_manager) == SQLITE_OK) {
            char status[512];
            snprintf(status, sizeof(status), "Opened: %s", filename);
            update_status(status);

            gchar *title = g_strdup_printf("Crissy's DB Browser - %s", filename);
            gtk_window_set_title(GTK_WINDOW(state->window), title);
            g_free(title);

            refresh_table_list(state);
            add_to_recent(state, filename);
            printf("[db-browser] database opened: %s\n", filename);
        } else {
            const char *db_err = db_manager_get_last_error(state->db_manager);
            fprintf(stderr, "[db-browser] failed to open database '%s': %s\n",
                    filename, db_err ? db_err : "unknown error");
            show_error_dialog("Error", "Failed to open database");
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void on_new_database(GtkWidget *widget, gpointer data) {
    show_info_dialog("Not Implemented", "New database creation coming soon!");
}

static void on_close_database(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    if (state->db_manager) {
        db_manager_destroy(state->db_manager);
        state->db_manager = NULL;
        update_status("Database closed");
    }
}

static void on_quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

/* Refresh table list callback */
static void on_refresh_tables(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;
    refresh_table_list(state);
}

/* Populate tree view with database tables */
static void refresh_table_list(AppState *state) {
    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        update_status("No database open");
        return;
    }

    // Get tables from database
    TableInfo *tables = db_manager_get_tables(state->db_manager);
    if (!tables) {
        update_status("No tables found or error reading database");
        return;
    }

    // Create new list store
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    // Populate with table data
    int count = 0;
    for (TableInfo *t = tables; t != NULL; t = t->next) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);

        // Format info string with row count
        char info[128];
        snprintf(info, sizeof(info), "%d rows", t->row_count);

        gtk_list_store_set(store, &iter,
                          0, t->name,
                          1, "table",
                          2, info,
                          -1);
        count++;
    }

    // Set the model on the tree view
    gtk_tree_view_set_model(GTK_TREE_VIEW(state->table_view), GTK_TREE_MODEL(store));

    // Create columns if they don't exist
    if (gtk_tree_view_get_n_columns(GTK_TREE_VIEW(state->table_view)) == 0) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

        GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes(
            "Table Name", renderer, "text", 0, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(state->table_view), col_name);

        GtkTreeViewColumn *col_type = gtk_tree_view_column_new_with_attributes(
            "Type", renderer, "text", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(state->table_view), col_type);

        GtkTreeViewColumn *col_info = gtk_tree_view_column_new_with_attributes(
            "Info", renderer, "text", 2, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(state->table_view), col_info);
    }

    // Cleanup
    g_object_unref(store);

    // Free table list
    TableInfo *t = tables;
    while (t) {
        TableInfo *next = t->next;
        free(t->name);
        free(t);
        t = next;
    }

    // Update status
    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), "Loaded %d table(s)", count);
    update_status(status_msg);
}

/* Handle table row double-click - Show in Data Browser tab */
static void on_table_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                   GtkTreeViewColumn *column, gpointer data) {
    (void)column; // Unused
    AppState *state = (AppState*)data;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *table_name;
        gtk_tree_model_get(model, &iter, 0, &table_name, -1);

        // Switch to Data Browser tab and show data there
        gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), 2); // Data Browser is page 2
        show_table_data(state, table_name);

        g_free(table_name);
    }
}

/* Show table data in Data Browser tab */
static void show_table_data(AppState *state, const char *table_name) {
    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        gtk_label_set_markup(GTK_LABEL(state->data_info_label),
            "<b>Error:</b> No database open");
        return;
    }

    // Update info label
    char info[512];
    snprintf(info, sizeof(info),
        "<b>Table:</b> %s   |   <b>Tip:</b> Showing up to 100 rows",
        table_name);
    gtk_label_set_markup(GTK_LABEL(state->data_info_label), info);

    // Execute query to get data
    char query[512];
    snprintf(query, sizeof(query), "SELECT * FROM %s LIMIT 100", table_name);

    QueryResult *result = db_manager_execute_query(state->db_manager, query);

    if (result && result->data && result->row_count > 0) {
        // Clear existing columns
        GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(state->data_view));
        for (GList *col = columns; col != NULL; col = col->next) {
            gtk_tree_view_remove_column(GTK_TREE_VIEW(state->data_view), GTK_TREE_VIEW_COLUMN(col->data));
        }
        g_list_free(columns);

        // Create new list store
        GType *types = g_malloc(sizeof(GType) * result->column_count);
        for (int i = 0; i < result->column_count; i++) {
            types[i] = G_TYPE_STRING;
        }
        GtkListStore *store = gtk_list_store_newv(result->column_count, types);
        g_free(types);

        // Add data rows
        for (int row = 0; row < result->row_count; row++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            for (int col = 0; col < result->column_count; col++) {
                const char *value = result->data[row][col] ? result->data[row][col] : "";
                gtk_list_store_set(store, &iter, col, value, -1);
            }
        }

        // Set model
        gtk_tree_view_set_model(GTK_TREE_VIEW(state->data_view), GTK_TREE_MODEL(store));
        g_object_unref(store);

        // Create columns
        for (int i = 0; i < result->column_count; i++) {
            GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
            GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
                result->column_names[i], renderer, "text", i, NULL);
            gtk_tree_view_column_set_resizable(column, TRUE);
            gtk_tree_view_column_set_sort_column_id(column, i);
            gtk_tree_view_column_set_min_width(column, 120);
            gtk_tree_view_append_column(GTK_TREE_VIEW(state->data_view), column);
        }

        // Update status
        char status[256];
        snprintf(status, sizeof(status), "Loaded %d rows from '%s'", result->row_count, table_name);
        update_status(status);

        // Update info label with row count
        snprintf(info, sizeof(info),
            "<b>Table:</b> %s   |   <b>Rows:</b> %d (limited to 100)",
            table_name, result->row_count);
        gtk_label_set_markup(GTK_LABEL(state->data_info_label), info);
    } else {
        gtk_label_set_markup(GTK_LABEL(state->data_info_label),
            "<b>Error:</b> No data found or table is empty");
        update_status("No data to display");
    }
}

/* Template button clicked callback */
static void on_template_clicked(GtkWidget *button, gpointer data) {
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);
    const char *template = g_object_get_data(G_OBJECT(button), "template");

    if (template && buffer) {
        gtk_text_buffer_set_text(buffer, template, -1);
    }
}

/* SQL Query Builder Dialog */
static void on_sql_query_builder(GtkWidget *widget, gpointer data) {
    (void)widget; // Unused
    AppState *state = (AppState*)data;

    // Create dialog
    GtkWidget *dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dialog), "SQL Query Builder");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 700);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(state->window));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(dialog), vbox);

    // Title
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label),
        "<span size='x-large' weight='bold'>SQL Query Builder</span>");
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 5);

    // Separator
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep1, FALSE, FALSE, 5);

    // Query editor (create first so we can reference it in callbacks)
    GtkWidget *editor_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(editor_label), "<b>SQL Query:</b>");
    gtk_widget_set_halign(editor_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), editor_label, FALSE, FALSE, 5);

    GtkWidget *scroll_editor = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_editor),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll_editor, -1, 150);

    GtkWidget *query_text = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(query_text), TRUE);

    // Let the CSS theme handle text and background colors
    gtk_widget_set_name(query_text, "query-builder-editor");

    gtk_container_add(GTK_CONTAINER(scroll_editor), query_text);
    gtk_box_pack_start(GTK_BOX(vbox), scroll_editor, FALSE, FALSE, 0);

    GtkTextBuffer *query_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(query_text));

    // Quick templates section
    GtkWidget *template_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(template_label), "<b>Quick Templates (click to insert):</b>");
    gtk_widget_set_halign(template_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), template_label, FALSE, FALSE, 5);

    // Template buttons with actual SQL
    GtkWidget *template_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    typedef struct {
        const char *label;
        const char *template;
    } TemplateInfo;

    TemplateInfo templates[] = {
        {"SELECT", "SELECT * FROM table_name WHERE condition;"},
        {"INSERT", "INSERT INTO table_name (column1, column2) VALUES (value1, value2);"},
        {"UPDATE", "UPDATE table_name SET column1 = value1 WHERE condition;"},
        {"DELETE", "DELETE FROM table_name WHERE condition;"},
        {"CREATE TABLE", "CREATE TABLE table_name (\n    id INTEGER PRIMARY KEY AUTOINCREMENT,\n    name TEXT NOT NULL,\n    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n);"},
        {"JOIN", "SELECT t1.*, t2.column FROM table1 t1\nINNER JOIN table2 t2 ON t1.id = t2.table1_id;"}
    };

    for (int i = 0; i < 6; i++) {
        GtkWidget *btn = gtk_button_new_with_label(templates[i].label);
        // Store the template in the button's data
        g_object_set_data_full(G_OBJECT(btn), "template",
                              g_strdup(templates[i].template), g_free);
        g_object_set_data(G_OBJECT(btn), "buffer", query_buffer);
        g_signal_connect_swapped(btn, "clicked",
            G_CALLBACK(gtk_text_buffer_set_text), query_buffer);
        // Better callback with proper data
        g_signal_connect(btn, "clicked", G_CALLBACK(on_template_clicked), query_buffer);
        gtk_box_pack_start(GTK_BOX(template_box), btn, TRUE, TRUE, 0);
    }
    gtk_box_pack_start(GTK_BOX(vbox), template_box, FALSE, FALSE, 5);

    // Separator
    GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep2, FALSE, FALSE, 5);

    // Examples section
    GtkWidget *examples_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(examples_label), "<b>Common SQL Examples:</b>");
    gtk_widget_set_halign(examples_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), examples_label, FALSE, FALSE, 5);

    GtkWidget *examples_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(examples_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *examples_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(examples_text), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(examples_text), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(examples_text), GTK_WRAP_WORD);

    // Let the CSS theme handle text and background colors
    gtk_widget_set_name(examples_text, "query-builder-examples");

    GtkTextBuffer *examples_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(examples_text));
    gtk_text_buffer_set_text(examples_buffer,
        "-- SELECT queries\n"
        "SELECT * FROM projects;\n"
        "SELECT name, description FROM projects WHERE created_at > '2025-12-01';\n"
        "SELECT COUNT(*) FROM tasks WHERE status = 'completed';\n"
        "SELECT p.name, COUNT(t.id) as task_count FROM projects p LEFT JOIN tasks t ON p.id = t.project_id GROUP BY p.id;\n\n"

        "-- INSERT queries\n"
        "INSERT INTO projects (name, description) VALUES ('New Project', 'Description here');\n"
        "INSERT INTO tasks (project_id, title, status, priority) VALUES (1, 'New Task', 'pending', 'high');\n\n"

        "-- UPDATE queries\n"
        "UPDATE projects SET description = 'Updated description' WHERE id = 1;\n"
        "UPDATE tasks SET status = 'completed' WHERE id = 5;\n"
        "UPDATE tasks SET priority = 'high' WHERE due_date < date('now') AND status != 'completed';\n\n"

        "-- DELETE queries\n"
        "DELETE FROM tasks WHERE status = 'cancelled';\n"
        "DELETE FROM projects WHERE id = 10;\n\n"

        "-- CREATE TABLE\n"
        "CREATE TABLE example (\n"
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
        "    name TEXT NOT NULL,\n"
        "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
        ");\n\n"

        "-- JOIN queries\n"
        "SELECT p.name as project, t.title as task, t.status\n"
        "FROM projects p\n"
        "INNER JOIN tasks t ON p.id = t.project_id\n"
        "WHERE t.status = 'in_progress';\n\n"

        "-- Aggregate functions\n"
        "SELECT status, COUNT(*) as count FROM tasks GROUP BY status;\n"
        "SELECT AVG(size_bytes) as avg_size FROM project_files WHERE file_type = 'python';\n"
        "SELECT project_id, MAX(created_at) as latest FROM tasks GROUP BY project_id;\n", -1);

    gtk_container_add(GTK_CONTAINER(examples_scroll), examples_text);
    gtk_box_pack_start(GTK_BOX(vbox), examples_scroll, TRUE, TRUE, 0);

    // Action buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

    GtkWidget *btn_execute = gtk_button_new_with_label("Execute Query");
    GtkWidget *btn_clear = gtk_button_new_with_label("Clear");
    GtkWidget *btn_close = gtk_button_new_with_label("Close");

    g_signal_connect_swapped(btn_clear, "clicked",
        G_CALLBACK(gtk_text_buffer_set_text),
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(query_text)));
    g_signal_connect_swapped(btn_close, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_box_pack_start(GTK_BOX(button_box), btn_execute, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_clear, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), btn_close, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 5);

    gtk_widget_show_all(dialog);
}

static void on_new_table(GtkWidget *widget, gpointer data) {
    show_info_dialog("Not Implemented", "New table creation dialog coming soon!");
}

static void on_drop_table(GtkWidget *widget, gpointer data) {
    if (confirm_action("Are you sure you want to delete this table?\nThis action cannot be undone!")) {
        show_info_dialog("Not Implemented", "Drop table functionality coming soon!");
    }
}

static void on_execute_query(GtkWidget *widget, gpointer data) {
    (void)widget; // Unused
    AppState *state = (AppState*)data;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database open. Please open a database first.");
        return;
    }

    // Get query text from editor
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->query_editor));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *query = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (!query || strlen(query) == 0) {
        show_error_dialog("Error", "Please enter a SQL query to execute.");
        g_free(query);
        return;
    }

    // Execute query
    QueryResult *result = db_manager_execute_query(state->db_manager, query);
    g_free(query);

    if (!result) {
        show_error_dialog("Query Error", "Failed to execute query. Check SQL syntax.");
        update_status("Query execution failed");
        return;
    }

    if (result->error) {
        show_error_dialog("SQL Error", result->error);
        update_status("Query failed");
        return;
    }

    // Clear existing columns in result view
    GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(state->result_view));
    for (GList *col = columns; col != NULL; col = col->next) {
        gtk_tree_view_remove_column(GTK_TREE_VIEW(state->result_view), GTK_TREE_VIEW_COLUMN(col->data));
    }
    g_list_free(columns);

    // Display results
    if (result->row_count > 0 && result->data) {
        // Create list store
        GType *types = g_malloc(sizeof(GType) * result->column_count);
        for (int i = 0; i < result->column_count; i++) {
            types[i] = G_TYPE_STRING;
        }
        GtkListStore *store = gtk_list_store_newv(result->column_count, types);
        g_free(types);

        // Add rows
        for (int row = 0; row < result->row_count; row++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            for (int col = 0; col < result->column_count; col++) {
                const char *value = result->data[row][col] ? result->data[row][col] : "NULL";
                gtk_list_store_set(store, &iter, col, value, -1);
            }
        }

        // Set model
        gtk_tree_view_set_model(GTK_TREE_VIEW(state->result_view), GTK_TREE_MODEL(store));
        g_object_unref(store);

        // Create columns
        for (int i = 0; i < result->column_count; i++) {
            GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
            GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
                result->column_names[i], renderer, "text", i, NULL);
            gtk_tree_view_column_set_resizable(column, TRUE);
            gtk_tree_view_column_set_sort_column_id(column, i);
            gtk_tree_view_column_set_min_width(column, 100);
            gtk_tree_view_append_column(GTK_TREE_VIEW(state->result_view), column);
        }

        // Update status
        char status[256];
        snprintf(status, sizeof(status), "Query executed successfully: %d rows returned", result->row_count);
        update_status(status);

        show_info_dialog("Success", status);
    } else {
        // Query succeeded but no rows returned (e.g., INSERT, UPDATE, DELETE)
        char status[256];
        snprintf(status, sizeof(status), "Query executed successfully: 0 rows returned (may be INSERT/UPDATE/DELETE)");
        update_status(status);
        show_info_dialog("Success", "Query executed successfully. No rows to display.");
    }
}

/* =========================================================================
 * CSV import / export helpers
 * ========================================================================= */

/* Write one CSV field, quoting if needed (RFC 4180). */
static void csv_write_field(FILE *f, const char *val) {
    if (!val) return;  /* NULL -> empty field */
    bool need_quote = (strchr(val, ',') || strchr(val, '"') ||
                       strchr(val, '\n') || strchr(val, '\r'));
    if (need_quote) {
        fputc('"', f);
        for (const char *p = val; *p; p++) {
            if (*p == '"') fputc('"', f);
            fputc(*p, f);
        }
        fputc('"', f);
    } else {
        fputs(val, f);
    }
}

/* Parse one CSV line from a buffer.  Returns a NULL-terminated array of
 * g_strdup'd strings the caller must free.  *col_count receives the count. */
static char **csv_parse_line(const char *line, int *col_count) {
    GPtrArray *fields = g_ptr_array_new();
    const char *p = line;
    while (1) {
        GString *field = g_string_new("");
        if (*p == '"') {
            p++; /* skip opening quote */
            while (*p) {
                if (*p == '"') {
                    p++;
                    if (*p == '"') { g_string_append_c(field, '"'); p++; }
                    else break; /* closing quote */
                } else {
                    g_string_append_c(field, *p++);
                }
            }
        } else {
            while (*p && *p != ',' && *p != '\n' && *p != '\r')
                g_string_append_c(field, *p++);
        }
        g_ptr_array_add(fields, g_string_free(field, FALSE));
        if (*p == ',') { p++; continue; }
        break;
    }
    *col_count = (int)fields->len;
    g_ptr_array_add(fields, NULL); /* NULL-terminate */
    return (char **)g_ptr_array_free(fields, FALSE);
}

static void on_export_csv(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState *)data;
    (void)widget;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("No Database", "Open a database first.");
        return;
    }
    if (!state->current_table) {
        show_error_dialog("No Table Selected", "Select a table in the Tables tab first.");
        return;
    }

    /* File chooser */
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Export CSV", GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save",   GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(
        GTK_FILE_CHOOSER(dialog), TRUE);
    char default_name[256];
    snprintf(default_name, sizeof(default_name), "%s.csv", state->current_table);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), default_name);

    GtkFileFilter *ff = gtk_file_filter_new();
    gtk_file_filter_set_name(ff, "CSV files (*.csv)");
    gtk_file_filter_add_pattern(ff, "*.csv");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ff);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);

    /* Query all rows */
    char query[512];
    snprintf(query, sizeof(query), "SELECT * FROM \"%s\"", state->current_table);
    QueryResult *result = db_manager_execute_query(state->db_manager, query);
    if (!result) {
        show_error_dialog("Export Failed", "Could not query table data.");
        g_free(filename);
        return;
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        show_error_dialog("Export Failed", "Could not open file for writing. Check permissions.");
        db_manager_free_query_result(result);
        g_free(filename);
        return;
    }

    /* Header row */
    for (int c = 0; c < result->column_count; c++) {
        if (c) fputc(',', f);
        csv_write_field(f, result->column_names[c]);
    }
    fputc('\n', f);

    /* Data rows */
    for (int r = 0; r < result->row_count; r++) {
        for (int c = 0; c < result->column_count; c++) {
            if (c) fputc(',', f);
            csv_write_field(f, result->data[r][c]);
        }
        fputc('\n', f);
    }
    fclose(f);

    char msg[512];
    snprintf(msg, sizeof(msg), "Exported %d rows to %s", result->row_count, filename);
    update_status(msg);
    show_info_dialog("Export Complete", msg);

    db_manager_free_query_result(result);
    g_free(filename);
}

static void on_import_csv(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState *)data;
    (void)widget;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("No Database", "Open a database first.");
        return;
    }

    /* File chooser */
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Import CSV", GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open",   GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileFilter *ff = gtk_file_filter_new();
    gtk_file_filter_set_name(ff, "CSV files (*.csv)");
    gtk_file_filter_add_pattern(ff, "*.csv");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), ff);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);

    FILE *f = fopen(filename, "r");
    if (!f) {
        show_error_dialog("Import Failed", "Could not open the selected file.");
        g_free(filename);
        return;
    }

    /* Derive default table name from filename (strip path and extension) */
    const char *base = strrchr(filename, '/');
    base = base ? base + 1 : filename;
    char table_name[256];
    snprintf(table_name, sizeof(table_name), "%s", base);
    char *dot = strrchr(table_name, '.');
    if (dot) *dot = '\0';

    /* Ask user to confirm / change the target table name */
    GtkWidget *name_dialog = gtk_dialog_new_with_buttons(
        "Import CSV - Table Name", GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Import", GTK_RESPONSE_ACCEPT,
        NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(name_dialog));
    GtkWidget *lbl = gtk_label_new(
        "Import CSV data into table (will be created if it does not exist):");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), lbl, FALSE, FALSE, 8);
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), table_name);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 4);
    gtk_widget_show_all(name_dialog);

    if (gtk_dialog_run(GTK_DIALOG(name_dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(name_dialog);
        fclose(f);
        g_free(filename);
        return;
    }
    snprintf(table_name, sizeof(table_name), "%s",
             gtk_entry_get_text(GTK_ENTRY(entry)));
    gtk_widget_destroy(name_dialog);

    if (!table_name[0]) {
        show_error_dialog("Import Failed", "Table name cannot be empty.");
        fclose(f);
        g_free(filename);
        return;
    }

    /* Read header row */
    char line_buf[65536];
    if (!fgets(line_buf, sizeof(line_buf), f)) {
        show_error_dialog("Import Failed", "CSV file is empty.");
        fclose(f);
        g_free(filename);
        return;
    }
    /* Strip trailing newline */
    char *nl = strpbrk(line_buf, "\r\n");
    if (nl) *nl = '\0';

    int col_count = 0;
    char **headers = csv_parse_line(line_buf, &col_count);
    if (col_count == 0) {
        show_error_dialog("Import Failed", "Could not parse CSV headers.");
        fclose(f);
        g_free(filename);
        return;
    }

    /* Create table if it does not exist */
    GString *create_sql = g_string_new(NULL);
    g_string_printf(create_sql, "CREATE TABLE IF NOT EXISTS \"%s\" (", table_name);
    for (int c = 0; c < col_count; c++) {
        if (c) g_string_append(create_sql, ", ");
        g_string_append_printf(create_sql, "\"%s\" TEXT", headers[c]);
    }
    g_string_append(create_sql, ")");

    QueryResult *cr = db_manager_execute_query(state->db_manager, create_sql->str);
    g_string_free(create_sql, TRUE);
    if (cr) db_manager_free_query_result(cr);

    /* Build the INSERT template */
    GString *insert_sql = g_string_new(NULL);
    g_string_printf(insert_sql, "INSERT INTO \"%s\" (", table_name);
    for (int c = 0; c < col_count; c++) {
        if (c) g_string_append(insert_sql, ", ");
        g_string_append_printf(insert_sql, "\"%s\"", headers[c]);
    }
    g_string_append(insert_sql, ") VALUES (");
    for (int c = 0; c < col_count; c++) {
        if (c) g_string_append(insert_sql, ", ");
        g_string_append(insert_sql, "?");
    }
    g_string_append(insert_sql, ")");

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(state->db_manager->db, insert_sql->str, -1, &stmt, NULL) != SQLITE_OK) {
        show_error_dialog("Import Failed", sqlite3_errmsg(state->db_manager->db));
        g_string_free(insert_sql, TRUE);
        g_strfreev(headers);
        fclose(f);
        g_free(filename);
        return;
    }
    g_string_free(insert_sql, TRUE);

    db_manager_begin_transaction(state->db_manager);

    int rows_imported = 0;
    int rows_failed   = 0;
    while (fgets(line_buf, sizeof(line_buf), f)) {
        nl = strpbrk(line_buf, "\r\n");
        if (nl) *nl = '\0';
        if (!line_buf[0]) continue;  /* skip blank lines */

        int fc = 0;
        char **fields = csv_parse_line(line_buf, &fc);

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        for (int c = 0; c < col_count; c++) {
            if (c < fc && fields[c]) {
                sqlite3_bind_text(stmt, c + 1, fields[c], -1, SQLITE_TRANSIENT);
            } else {
                sqlite3_bind_null(stmt, c + 1);
            }
        }
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            rows_imported++;
        } else {
            rows_failed++;
        }
        g_strfreev(fields);
    }

    sqlite3_finalize(stmt);
    db_manager_commit(state->db_manager);
    fclose(f);
    g_strfreev(headers);
    g_free(filename);

    refresh_table_list(state);

    char msg[512];
    if (rows_failed == 0) {
        snprintf(msg, sizeof(msg), "Imported %d rows into table '%s'.",
                 rows_imported, table_name);
        update_status(msg);
        show_info_dialog("Import Complete", msg);
    } else {
        snprintf(msg, sizeof(msg),
                 "Imported %d rows into '%s'. %d rows failed to insert.",
                 rows_imported, table_name, rows_failed);
        update_status(msg);
        show_error_dialog("Import Partial", msg);
    }
}

static void on_backup_database(GtkWidget *widget, gpointer data) {
    show_info_dialog("Not Implemented", "Database backup coming soon!");
}

static void on_help_tutorial(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;
    tooltip_manager_enable_tutorial(state->tooltip_manager);
    show_info_dialog("Tutorial Mode",
                    "Tutorial mode enabled! Hover over any button or field "
                    "to see detailed explanations and examples.");
}

static void on_help_about(GtkWidget *widget, gpointer data) {
    show_info_dialog("About",
                    "Crissy's DB Browser\n\n"
                    "A lightweight SQLite database browser\n"
                    "Version 1.0.0\n\n"
                    "Built with GTK+3 and SQLite3.");
}

/* Utility functions */
static void update_status(const char *message) {
    if (app && app->status_bar) {
        gtk_statusbar_push(GTK_STATUSBAR(app->status_bar), 0, message);
    }
}

static void show_error_dialog(const char *title, const char *message) {
    if (!app || !app->window) return;

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               "%s", title);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                             "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void show_info_dialog(const char *title, const char *message) {
    if (!app || !app->window) return;

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", title);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                             "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static bool confirm_action(const char *message) {
    if (!app || !app->window) return false;

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_WARNING,
                                               GTK_BUTTONS_YES_NO,
                                               "[WARNING] Confirmation Required");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                             "%s", message);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return response == GTK_RESPONSE_YES;
}

/* =========================================================================
 * Recent databases - config file, combo population, load/save/add
 * ========================================================================= */

/* Returns a freshly allocated path to the recent-databases config file.
 * Creates the config directory if it does not exist.  Caller must g_free(). */
static char *get_recent_config_path(void) {
    const char *home = g_get_home_dir();
    if (!home) {
        fprintf(stderr, "[db-browser] get_recent_config_path: g_get_home_dir returned NULL\n");
        return g_strdup("/tmp/crissy-db-browser-recent.txt");
    }
    char *dir = g_build_filename(home, ".config", "crissy-db-browser", NULL);
    if (g_mkdir_with_parents(dir, 0755) != 0) {
        fprintf(stderr, "[db-browser] get_recent_config_path: cannot create config dir: %s\n", dir);
    }
    char *path = g_build_filename(dir, "recent.txt", NULL);
    g_free(dir);
    return path;
}

/* Rebuild the recent-databases combo from state->recent_paths.
 * Blocks the "changed" signal while repopulating to avoid spurious opens. */
static void populate_recent_combo(AppState *state) {
    if (!state || !state->recent_btn) {
        fprintf(stderr, "[db-browser] populate_recent_combo: state or recent_btn is NULL\n");
        return;
    }

    GtkMenu *menu = GTK_MENU(gtk_menu_button_get_popup(
                                 GTK_MENU_BUTTON(state->recent_btn)));
    if (!menu) return;

    /* Remove all existing items */
    GList *children = gtk_container_get_children(GTK_CONTAINER(menu));
    for (GList *l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    int n = 0;
    if (!state->recent_paths) {
        GtkWidget *item = gtk_menu_item_new_with_label("(no recent databases)");
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    } else {
        for (GList *l = state->recent_paths; l != NULL; l = l->next) {
            GtkWidget *item = gtk_menu_item_new_with_label((const char *)l->data);
            g_signal_connect(item, "activate", G_CALLBACK(on_open_recent_db), state);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            n++;
        }
    }

    gtk_widget_show_all(GTK_WIDGET(menu));
    printf("[db-browser] populate_recent_combo: %d entries\n", n);
}

/* Load recent databases from the config file and populate the combo. */
static void load_recent_dbs(AppState *state) {
    if (!state) {
        fprintf(stderr, "[db-browser] load_recent_dbs: state is NULL\n");
        return;
    }

    char *config_path = get_recent_config_path();
    FILE *f = fopen(config_path, "r");
    if (!f) {
        printf("[db-browser] load_recent_dbs: no existing recent file at %s\n", config_path);
        g_free(config_path);
        return;
    }

    char line[2048];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < 10) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;
        state->recent_paths = g_list_append(state->recent_paths, g_strdup(line));
        count++;
    }

    fclose(f);
    g_free(config_path);

    populate_recent_combo(state);
    printf("[db-browser] load_recent_dbs: loaded %d path(s)\n", count);
}

/* Write the current recent_paths list to the config file. */
static void save_recent_dbs(AppState *state) {
    if (!state || !state->recent_paths) {
        printf("[db-browser] save_recent_dbs: nothing to save\n");
        return;
    }

    char *config_path = get_recent_config_path();
    FILE *f = fopen(config_path, "w");
    if (!f) {
        fprintf(stderr, "[db-browser] save_recent_dbs: cannot open '%s' for writing\n", config_path);
        g_free(config_path);
        return;
    }

    int count = 0;
    for (GList *l = state->recent_paths; l != NULL; l = l->next) {
        fprintf(f, "%s\n", (char *)l->data);
        count++;
    }

    fclose(f);
    g_free(config_path);
    printf("[db-browser] save_recent_dbs: saved %d path(s)\n", count);
}

/* Add a database path to the top of the recent list, trim to 10, then save. */
static void add_to_recent(AppState *state, const char *path) {
    if (!state || !path || strlen(path) == 0) {
        fprintf(stderr, "[db-browser] add_to_recent: invalid arguments\n");
        return;
    }

    /* Remove any existing duplicate */
    GList *existing = g_list_find_custom(state->recent_paths, path,
                                         (GCompareFunc)strcmp);
    if (existing) {
        g_free(existing->data);
        state->recent_paths = g_list_delete_link(state->recent_paths, existing);
    }

    /* Prepend (most recent first) */
    state->recent_paths = g_list_prepend(state->recent_paths, g_strdup(path));

    /* Trim to 10 entries */
    while (g_list_length(state->recent_paths) > 10) {
        GList *last = g_list_last(state->recent_paths);
        g_free(last->data);
        state->recent_paths = g_list_delete_link(state->recent_paths, last);
    }

    populate_recent_combo(state);
    save_recent_dbs(state);
    printf("[db-browser] add_to_recent: added '%s'\n", path);
}

/* =========================================================================
 * Callback: open a database selected from the recent dropdown
 * ========================================================================= */
static void on_open_recent_db(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState *)data;

    /* path is owned by the menu item -- do not g_free it */
    const gchar *path = gtk_menu_item_get_label(GTK_MENU_ITEM(widget));
    if (!path || strlen(path) == 0) return;

    printf("[db-browser] on_open_recent_db: opening '%s'\n", path);

    if (state->db_manager) {
        db_manager_destroy(state->db_manager);
        state->db_manager = NULL;
    }

    state->db_manager = db_manager_create(path, false);
    if (!state->db_manager) {
        fprintf(stderr, "[db-browser] on_open_recent_db: db_manager_create failed for '%s'\n", path);
        show_error_dialog("Error", "Failed to create database manager");
        return;
    }

    if (db_manager_open(state->db_manager) == SQLITE_OK) {
        char status[512];
        snprintf(status, sizeof(status), "Opened: %s", path);
        update_status(status);

        gchar *title = g_strdup_printf("Crissy's DB Browser - %s", path);
        gtk_window_set_title(GTK_WINDOW(state->window), title);
        g_free(title);

        refresh_table_list(state);
        add_to_recent(state, path);
        printf("[db-browser] on_open_recent_db: success for '%s'\n", path);
    } else {
        const char *db_err = db_manager_get_last_error(state->db_manager);
        fprintf(stderr, "[db-browser] on_open_recent_db: failed to open '%s': %s\n",
                path, db_err ? db_err : "unknown error");
        show_error_dialog("Error", "Failed to open database from recent list");
        db_manager_destroy(state->db_manager);
        state->db_manager = NULL;
    }
}

/* =========================================================================
 * Callback: open a .sql script file and load it into the query editor
 * ========================================================================= */
static void on_open_sql_file(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppState *state = (AppState *)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Load SQL Script",
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open",   GTK_RESPONSE_ACCEPT,
        NULL);

    /* Filter for .sql files */
    GtkFileFilter *sql_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(sql_filter, "SQL files (*.sql)");
    gtk_file_filter_add_pattern(sql_filter, "*.sql");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), sql_filter);

    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        printf("[db-browser] on_open_sql_file: loading '%s'\n", filename);

        gchar  *contents = NULL;
        gsize   length   = 0;
        GError *err      = NULL;

        if (g_file_get_contents(filename, &contents, &length, &err)) {
            if (state->query_editor) {
                GtkTextBuffer *buf =
                    gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->query_editor));
                gtk_text_buffer_set_text(buf, contents, (gint)length);

                /* Switch to the SQL Query tab (index 1) */
                gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), 1);

                char status[512];
                snprintf(status, sizeof(status),
                         "SQL file loaded: %s (%zu bytes)", filename, length);
                update_status(status);
                printf("[db-browser] on_open_sql_file: loaded %zu bytes\n", length);
            } else {
                fprintf(stderr,
                    "[db-browser] on_open_sql_file: query_editor is NULL, cannot load file\n");
            }
            g_free(contents);
        } else {
            fprintf(stderr, "[db-browser] on_open_sql_file: failed to read '%s': %s\n",
                    filename, err ? err->message : "unknown error");
            show_error_dialog("Error", "Failed to read SQL file");
            if (err) g_error_free(err);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}
