#include "ui_panels.h"
#include "ui_utils.h"
#include "ui_drawing.h"
#include <gtk/gtk.h>

/* Forward declarations for callbacks defined in other modules */
void on_quit(GtkWidget *widget, gpointer data);
void on_open_database(GtkWidget *widget, gpointer data);
void on_new_database(GtkWidget *widget, gpointer data);
void on_new_table(GtkWidget *widget, gpointer data);
void on_execute_query(GtkWidget *widget, gpointer data);
void on_import_csv(GtkWidget *widget, gpointer data);
void on_export_csv(GtkWidget *widget, gpointer data);
void on_backup_database(GtkWidget *widget, gpointer data);
void on_open_sql_file(GtkWidget *widget, gpointer data);
void on_refresh_tables(GtkWidget *widget, gpointer data);
void on_table_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
void on_sql_query_builder(GtkWidget *widget, gpointer data);
void on_drop_table(GtkWidget *widget, gpointer data);
void on_data_cell_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
void on_toggle_theme(GtkWidget *widget, gpointer data);
void on_query_buffer_changed(GtkTextBuffer *buffer, gpointer data);
void on_run_function(GtkWidget *widget, gpointer data);
void on_add_row_clicked(GtkWidget *widget, gpointer data);
void on_delete_row_clicked(GtkWidget *widget, gpointer data);

void create_main_window(AppState *state) {
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "Crissy's DB Browser");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 1024, 768);
    gtk_window_set_position(GTK_WINDOW(state->window), GTK_WIN_POS_CENTER);

    g_signal_connect(state->window, "destroy", G_CALLBACK(on_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), vbox);

    create_toolbar(state);

    state->notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->notebook, TRUE, TRUE, 0);

    create_table_panel(state);
    create_query_panel(state);
    create_data_panel(state);

    state->status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->status_bar, FALSE, FALSE, 0);

    update_status("Ready - Open or create a database to begin");
}

void create_menu_bar(AppState *state) {
    /* TODO: Implement full menu bar with File, Edit, View, Tools, Help */
}

void create_toolbar(AppState *state) {
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

    GtkToolItem *btn_open = gtk_tool_button_new(NULL, "Open");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_open), "Open");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_open), "Open an existing SQLite database file");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_database), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_open, -1);

    GtkToolItem *btn_new = gtk_tool_button_new(NULL, "New DB");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_new), "New DB");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_new), "Create a new empty database");
    g_signal_connect(btn_new, "clicked", G_CALLBACK(on_new_database), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_new, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    GtkToolItem *btn_new_table = gtk_tool_button_new(NULL, "New Table");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_new_table), "New Table");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_new_table), "Create a new table in the open database");
    g_signal_connect(btn_new_table, "clicked", G_CALLBACK(on_new_table), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_new_table, -1);

    GtkToolItem *btn_execute = gtk_tool_button_new(NULL, "Run SQL");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_execute), "Run SQL");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_execute), "Execute the query in the SQL editor");
    g_signal_connect(btn_execute, "clicked", G_CALLBACK(on_execute_query), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_execute, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    GtkToolItem *btn_import = gtk_tool_button_new(NULL, "Import");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_import), "Import");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_import), "Import data from a CSV file");
    g_signal_connect(btn_import, "clicked", G_CALLBACK(on_import_csv), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_import, -1);

    GtkToolItem *btn_export = gtk_tool_button_new(NULL, "Export");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_export), "Export");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_export), "Export table data as CSV");
    g_signal_connect(btn_export, "clicked", G_CALLBACK(on_export_csv), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_export, -1);

    GtkToolItem *btn_backup = gtk_tool_button_new(NULL, "Backup");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_backup), "Backup");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_backup), "Backup the open database to a file");
    g_signal_connect(btn_backup, "clicked", G_CALLBACK(on_backup_database), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_backup, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    GtkToolItem *btn_sql_file = gtk_tool_button_new(NULL, "Load .sql");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(btn_sql_file), "Load .sql");
    gtk_widget_set_tooltip_text(GTK_WIDGET(btn_sql_file),
        "Load a .sql script file into the query editor");
    g_signal_connect(btn_sql_file, "clicked", G_CALLBACK(on_open_sql_file), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), btn_sql_file, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

    GtkToolItem *recent_item = gtk_tool_item_new();
    state->recent_btn = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(state->recent_btn), "Recent");
    gtk_widget_set_tooltip_text(state->recent_btn,
        "Recently opened databases - click to select one");

    GtkWidget *popover = gtk_popover_new(state->recent_btn);
    gtk_popover_set_constrain_to(GTK_POPOVER(popover), GTK_POPOVER_CONSTRAINT_NONE);
    GtkWidget *recent_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(recent_box, 4);
    gtk_widget_set_margin_bottom(recent_box, 4);
    GtkWidget *placeholder = gtk_label_new("(no recent databases)");
    gtk_widget_set_sensitive(placeholder, FALSE);
    gtk_widget_set_margin_start(placeholder, 16);
    gtk_widget_set_margin_end(placeholder, 16);
    gtk_widget_set_margin_top(placeholder, 6);
    gtk_widget_set_margin_bottom(placeholder, 6);
    gtk_box_pack_start(GTK_BOX(recent_box), placeholder, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(popover), recent_box);
    gtk_widget_show_all(recent_box);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(state->recent_btn), popover);
    gtk_container_add(GTK_CONTAINER(recent_item), state->recent_btn);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), recent_item, -1);

    GtkToolItem *spacer = gtk_separator_tool_item_new();
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(spacer), FALSE);
    gtk_tool_item_set_expand(spacer, TRUE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), spacer, -1);

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

void create_table_panel(AppState *state) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<b>Database Tables</b>");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 4);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
    g_signal_connect_after(scroll, "draw", G_CALLBACK(draw_treeview_border), NULL);

    state->table_view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(state->table_view), TRUE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(state->table_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(state->table_view), GTK_TREE_VIEW_GRID_LINES_NONE);
    g_signal_connect_after(state->table_view, "draw", G_CALLBACK(draw_column_lines), NULL);
    g_signal_connect(state->table_view, "row-activated", G_CALLBACK(on_table_row_activated), state);
    gtk_container_add(GTK_CONTAINER(scroll), state->table_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

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
    gtk_widget_set_tooltip_text(btn_drop, "Delete the selected table (double-click a table first, requires password, checks relationships, saves to trash)");
    gtk_widget_set_name(btn_drop, "btn-destructive");
    g_signal_connect(btn_drop, "clicked", G_CALLBACK(on_drop_table), state);
    gtk_box_pack_end(GTK_BOX(hbox), btn_drop, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), vbox,
                            gtk_label_new("Tables"));
}

void create_query_panel(AppState *state) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    GtkWidget *header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header), "<b>SQL Query Editor</b>");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 4);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
    g_signal_connect_after(scroll, "draw", G_CALLBACK(draw_treeview_border), NULL);
    gtk_widget_set_size_request(scroll, -1, 160);

    state->query_editor = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(state->query_editor), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(state->query_editor), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(state->query_editor), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(state->query_editor), 6);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(state->query_editor), 6);

    /* Cursor color: GTK3 on Windows ignores CSS caret-color, so force it here */
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    GdkRGBA caret_dark = {0.784, 0.753, 0.910, 1.0};   /* #c8c0e8 */
    GdkRGBA caret_sec  = {0.486, 0.361, 0.910, 1.0};   /* #7c5ce8 */
    gtk_widget_override_cursor(state->query_editor, &caret_dark, &caret_sec);
    G_GNUC_END_IGNORE_DEPRECATIONS
    
    // Connect buffer change signal for auto-save tracking
    GtkTextBuffer *query_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->query_editor));
    g_signal_connect(query_buffer, "changed", G_CALLBACK(on_query_buffer_changed), state);
    
    gtk_container_add(GTK_CONTAINER(scroll), state->query_editor);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, FALSE, FALSE, 0);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_top(btn_row, 4);

    GtkWidget *btn_execute = gtk_button_new_with_label("Execute");
    gtk_widget_set_name(btn_execute, "btn-primary");
    gtk_widget_set_tooltip_text(btn_execute, "Run the SQL query above");
    g_signal_connect(btn_execute, "clicked", G_CALLBACK(on_execute_query), state);
    gtk_box_pack_start(GTK_BOX(btn_row), btn_execute, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), btn_row, FALSE, FALSE, 0);

    state->query_error_label = gtk_label_new(NULL);
    gtk_widget_set_halign(state->query_error_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(state->query_error_label, 2);
    gtk_widget_set_no_show_all(state->query_error_label, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), state->query_error_label, FALSE, FALSE, 0);

    GtkWidget *results_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(results_label), "<b>Results</b>");
    gtk_widget_set_halign(results_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(results_label, 6);
    gtk_box_pack_start(GTK_BOX(vbox), results_label, FALSE, FALSE, 0);

    GtkWidget *result_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(result_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(result_scroll), GTK_SHADOW_NONE);
    g_signal_connect_after(result_scroll, "draw", G_CALLBACK(draw_treeview_border), NULL);

    state->result_view = gtk_tree_view_new();
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(state->result_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(state->result_view), GTK_TREE_VIEW_GRID_LINES_NONE);
    g_signal_connect_after(state->result_view, "draw", G_CALLBACK(draw_column_lines), NULL);
    gtk_container_add(GTK_CONTAINER(result_scroll), state->result_view);
    gtk_box_pack_start(GTK_BOX(vbox), result_scroll, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), vbox,
                            gtk_label_new("SQL Query"));
}

void create_data_panel(AppState *state) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    state->data_info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(state->data_info_label),
        "<b>Data Browser</b>  --  Double-click a table in the Tables tab to view its rows here.");
    gtk_label_set_line_wrap(GTK_LABEL(state->data_info_label), TRUE);
    gtk_widget_set_halign(state->data_info_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), state->data_info_label, FALSE, FALSE, 6);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
    g_signal_connect_after(scroll, "draw", G_CALLBACK(draw_treeview_border), NULL);

    state->data_view = gtk_tree_view_new();
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(state->data_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(state->data_view), GTK_TREE_VIEW_GRID_LINES_NONE);
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(state->data_view), TRUE);
    g_signal_connect_after(state->data_view, "draw", G_CALLBACK(draw_column_lines), NULL);
    g_signal_connect(state->data_view, "row-activated", G_CALLBACK(on_data_cell_activated), state);
    gtk_container_add(GTK_CONTAINER(scroll), state->data_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_top(hbox, 6);

    GtkWidget *btn_add_row = gtk_button_new_with_label("Add Row");
    gtk_widget_set_name(btn_add_row, "btn-primary");
    gtk_widget_set_tooltip_text(btn_add_row, "Add a new row to the current table");
    g_signal_connect(btn_add_row, "clicked", G_CALLBACK(on_add_row_clicked), state);
    gtk_box_pack_start(GTK_BOX(hbox), btn_add_row, FALSE, FALSE, 0);

    GtkWidget *btn_delete_row = gtk_button_new_with_label("Delete Row");
    gtk_widget_set_name(btn_delete_row, "btn-secondary");
    gtk_widget_set_tooltip_text(btn_delete_row, "Delete selected row (requires password, saves to trash for recovery)");
    g_signal_connect(btn_delete_row, "clicked", G_CALLBACK(on_delete_row_clicked), state);
    gtk_box_pack_start(GTK_BOX(hbox), btn_delete_row, FALSE, FALSE, 0);

    GtkWidget *btn_run_function = gtk_button_new_with_label("Run Function");
    gtk_widget_set_name(btn_run_function, "btn-primary");
    gtk_widget_set_tooltip_text(btn_run_function, "Browse and load SQL functions for data analysis on the current table");
    g_signal_connect(btn_run_function, "clicked", G_CALLBACK(on_run_function), state);
    gtk_box_pack_start(GTK_BOX(hbox), btn_run_function, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), vbox,
                            gtk_label_new("Data Browser"));
}
