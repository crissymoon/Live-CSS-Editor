#include "function_runner.h"
#include "functions/function_categories.h"
#include "ui_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
static void on_load_function(GtkWidget *button, gpointer user_data);
static void on_preview_selection_changed(GtkTreeSelection *selection, gpointer user_data);

static void on_function_double_clicked(GtkTreeView *tree_view, GtkTreePath *path,
                                        GtkTreeViewColumn *column, gpointer user_data) {
    (void)tree_view;
    (void)path;
    (void)column;
    on_load_function(NULL, user_data);
}

static void on_load_function(GtkWidget *button, gpointer user_data) {
    (void)button;
    GtkWidget **widgets = (GtkWidget**)user_data;
    GtkDialog *dialog = GTK_DIALOG(widgets[0]);
    AppState *state = (AppState*)widgets[1];
    GtkTreeView *tree_view = GTK_TREE_VIEW(widgets[2]);
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog("Error", "Please select a function to load.");
        return;
    }
    
    gchar *sql_template = NULL;
    gtk_tree_model_get(model, &iter, 3, &sql_template, -1);
    
    if (!sql_template) {
        show_error_dialog("Error", "Failed to get function template.");
        return;
    }
    
    // Load SQL into query editor
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->query_editor));
    gtk_text_buffer_set_text(buffer, sql_template, -1);
    g_free(sql_template);
    
    // Switch to query tab
    gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), 1);
    
    update_status("Function loaded. Modify table and column names, then execute.");
    
    gtk_dialog_response(dialog, GTK_RESPONSE_CLOSE);
}

static void on_preview_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    GtkTextView *preview = GTK_TEXT_VIEW(user_data);
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *name = NULL;
        gchar *desc = NULL;
        gchar *sql = NULL;
        gtk_tree_model_get(model, &iter, 
                          0, &name, 
                          2, &desc, 
                          3, &sql, 
                          -1);
        
        if (name && sql) {
            GString *preview_text = g_string_new("");
            g_string_append_printf(preview_text, "%s\n\n", name);
            if (desc) {
                g_string_append_printf(preview_text, "%s\n\n", desc);
            }
            g_string_append(preview_text, sql);
            
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(preview);
            gtk_text_buffer_set_text(buffer, preview_text->str, -1);
            
            g_string_free(preview_text, TRUE);
        }
        
        g_free(name);
        g_free(desc);
        g_free(sql);
    }
}

void on_run_function(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppState *state = (AppState*)data;
    
    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database open. Please open a database first.");
        return;
    }
    
    // Get all function categories
    const FunctionCategory *categories[] = {
        &aggregate_category,
        &statistical_category,
        &mathematical_category,
        &datetime_category,
        &string_category,
        &window_category,
        &business_category
    };
    int category_count = sizeof(categories) / sizeof(categories[0]);
    
    // Create dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "SQL Function Library",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 650);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 12);
    
    // Info label
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label),
        "<b>SQL Data Analysis Functions</b>\n"
        "Select a function to load into the SQL editor. Works with any database.");
    gtk_label_set_line_wrap(GTK_LABEL(info_label), TRUE);
    gtk_widget_set_halign(info_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content_area), info_label, FALSE, FALSE, 6);
    
    // Horizontal paned for list and preview
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(paned), 400);
    gtk_box_pack_start(GTK_BOX(content_area), paned, TRUE, TRUE, 6);
    
    // Left side: Function list
    GtkWidget *list_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(list_scroll), GTK_SHADOW_IN);
    
    // List store columns: Name | Category | Description | SQL Template
    GtkListStore *store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, 
                                              G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), TRUE);
    
    // Function name column
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes(
        "Function", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_min_width(col_name, 180);
    gtk_tree_view_column_set_resizable(col_name, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_name);
    
    // Category column
    GtkTreeViewColumn *col_category = gtk_tree_view_column_new_with_attributes(
        "Category", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_min_width(col_category, 120);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_category);
    
    // Populate with all functions from all categories
    for (int i = 0; i < category_count; i++) {
        const FunctionCategory *cat = categories[i];
        for (int j = 0; j < cat->function_count; j++) {
            const SQLFunction *func = &cat->functions[j];
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                              0, func->name,
                              1, cat->category_name,
                              2, func->description,
                              3, func->sql_template,
                              -1);
        }
    }
    
    g_object_unref(store);
    gtk_container_add(GTK_CONTAINER(list_scroll), tree_view);
    gtk_paned_add1(GTK_PANED(paned), list_scroll);
    
    // Right side: Preview area
    GtkWidget *preview_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(preview_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(preview_scroll), GTK_SHADOW_IN);
    
    GtkWidget *preview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(preview), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(preview), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(preview), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(preview), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(preview), 6);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(preview), 6);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(preview), GTK_WRAP_WORD);
    
    GtkTextBuffer *preview_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(preview));
    gtk_text_buffer_set_text(preview_buffer, 
        "Select a function to preview its SQL template...", -1);
    
    gtk_container_add(GTK_CONTAINER(preview_scroll), preview);
    gtk_paned_add2(GTK_PANED(paned), preview_scroll);
    
    // Button box
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_top(button_box, 6);
    
    GtkWidget *load_button = gtk_button_new_with_label("Load into Editor");
    gtk_widget_set_name(load_button, "btn-primary");
    gtk_widget_set_tooltip_text(load_button, "Load the selected function into the SQL query editor");
    gtk_box_pack_start(GTK_BOX(button_box), load_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(content_area), button_box, FALSE, FALSE, 0);
    
    // Connect signals - store widgets for callbacks
    GtkWidget **widgets = g_malloc(sizeof(GtkWidget*) * 3);
    widgets[0] = dialog;
    widgets[1] = (GtkWidget*)state;
    widgets[2] = tree_view;
    
    // Double-click to load immediately
    g_signal_connect(tree_view, "row-activated", 
                     G_CALLBACK(on_function_double_clicked), widgets);
    
    // Selection changed - update preview
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
    g_signal_connect(selection, "changed",
                     G_CALLBACK(on_preview_selection_changed), preview);
    
    g_signal_connect(load_button, "clicked",
                     G_CALLBACK(on_load_function), widgets);
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    g_free(widgets);
    gtk_widget_destroy(dialog);
}
