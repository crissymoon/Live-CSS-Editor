#include "row_operations.h"
#include "trash_manager.h"
#include "db_callbacks.h"
#include "ui_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static TrashManager *trash_manager = NULL;

void row_operations_init(void) {
    // Trash manager will be initialized per database
}

void row_operations_cleanup(void) {
    if (trash_manager) {
        trash_manager_close(trash_manager);
        trash_manager = NULL;
    }
}

static bool verify_password(GtkWindow *parent) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Authentication Required",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Confirm", GTK_RESPONSE_OK,
        NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 180);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);
    
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), 
        "<b>Delete Row</b>\n\nEnter your password to confirm deletion:");
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 8);
    
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Password");
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 4);
    
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_widget_show_all(content);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entry));
    
    bool verified = false;
    if (response == GTK_RESPONSE_OK && password && strlen(password) > 0) {
        verified = true;
    }
    
    gtk_widget_destroy(dialog);
    return verified;
}

void on_add_row_clicked(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState *)data;
    
    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database is currently open.");
        return;
    }
    
    if (!state->current_table) {
        show_error_dialog("Error", "No table is currently loaded. Please select a table first.");
        return;
    }
    
    char query[512];
    snprintf(query, sizeof(query), "PRAGMA table_info(\"%s\")", state->current_table);
    QueryResult *col_info = db_manager_execute_query(state->db_manager, query);
    
    if (!col_info || col_info->row_count == 0) {
        show_error_dialog("Error", "Could not retrieve table structure.");
        return;
    }
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Add New Row",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Add Row", GTK_RESPONSE_OK,
        NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);
    
    char header[256];
    snprintf(header, sizeof(header), "<b>Add New Row to Table:</b> %s", state->current_table);
    GtkWidget *header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label), header);
    gtk_widget_set_halign(header_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), header_label, FALSE, FALSE, 8);
    
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 8);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
    gtk_container_add(GTK_CONTAINER(scroll), grid);
    
    GtkWidget **entries = malloc(sizeof(GtkWidget*) * col_info->row_count);
    
    for (int i = 0; i < col_info->row_count; i++) {
        const char *col_name = col_info->data[i][1];
        const char *col_type = col_info->data[i][2];
        
        char label_text[256];
        snprintf(label_text, sizeof(label_text), "<b>%s</b>\n<small>%s</small>", 
                 col_name, col_type);
        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), label_text);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        
        entries[i] = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entries[i]), "Enter value or leave empty for NULL");
        gtk_widget_set_hexpand(entries[i], TRUE);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }
    
    gtk_widget_show_all(content);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_OK) {
        char insert_query[8192];
        char columns[2048] = {0};
        char values[4096] = {0};
        int col_pos = 0;
        int val_pos = 0;
        
        for (int i = 0; i < col_info->row_count; i++) {
            const char *col_name = col_info->data[i][1];
            const char *value = gtk_entry_get_text(GTK_ENTRY(entries[i]));
            
            if (i > 0) {
                columns[col_pos++] = ',';
                columns[col_pos++] = ' ';
                values[val_pos++] = ',';
                values[val_pos++] = ' ';
            }
            
            col_pos += snprintf(columns + col_pos, sizeof(columns) - col_pos, "\"%s\"", col_name);
            
            if (value && strlen(value) > 0) {
                char escaped[1024];
                int esc_pos = 0;
                for (const char *p = value; *p && esc_pos < 1020; p++) {
                    if (*p == '\'') escaped[esc_pos++] = '\'';
                    escaped[esc_pos++] = *p;
                }
                escaped[esc_pos] = '\0';
                val_pos += snprintf(values + val_pos, sizeof(values) - val_pos, "'%s'", escaped);
            } else {
                val_pos += snprintf(values + val_pos, sizeof(values) - val_pos, "NULL");
            }
        }
        
        snprintf(insert_query, sizeof(insert_query),
                 "INSERT INTO \"%s\" (%s) VALUES (%s);",
                 state->current_table, columns, values);
        
        QueryResult *result = db_manager_execute_query(state->db_manager, insert_query);
        
        if (result) {
            update_status("Row added successfully");
            show_table_data(state, state->current_table);
        } else {
            show_error_dialog("Error", "Failed to add row. Check your values and try again.");
        }
    }
    
    free(entries);
    gtk_widget_destroy(dialog);
    db_manager_free_query_result(col_info);
}

void on_delete_row_clicked(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState *)data;
    
    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database is currently open.");
        return;
    }
    
    if (!state->current_table) {
        show_error_dialog("Error", "No table is currently loaded. Please select a table first.");
        return;
    }
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(state->data_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    
    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog("Error", "Please select a row to delete.");
        return;
    }
    
    if (!verify_password(GTK_WINDOW(state->window))) {
        update_status("Delete operation cancelled");
        return;
    }
    
    GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
    int row_idx = gtk_tree_path_get_indices(path)[0];
    gtk_tree_path_free(path);
    
    char rowid_query[512];
    snprintf(rowid_query, sizeof(rowid_query),
             "SELECT rowid FROM \"%s\" LIMIT 100", state->current_table);
    QueryResult *rid_result = db_manager_execute_query(state->db_manager, rowid_query);
    
    if (!rid_result || row_idx >= rid_result->row_count) {
        show_error_dialog("Error", "Could not determine row ID.");
        db_manager_free_query_result(rid_result);
        return;
    }
    
    const char *rowid_str = rid_result->data[row_idx][0];
    long rowid = atol(rowid_str);
    
    char data_query[512];
    snprintf(data_query, sizeof(data_query),
             "SELECT * FROM \"%s\" WHERE rowid = %ld", state->current_table, rowid);
    QueryResult *data_result = db_manager_execute_query(state->db_manager, data_query);
    
    if (data_result && data_result->row_count > 0) {
        if (!trash_manager) {
            const char *db_path = db_manager_get_current_db_path(state->db_manager);
            trash_manager = trash_manager_init(db_path);
        }
        
        if (trash_manager) {
            const char **col_names = (const char **)data_result->column_names;
            const char **values = (const char **)data_result->data[0];
            
            const char *db_path = db_manager_get_current_db_path(state->db_manager);
            trash_manager_save_row(trash_manager, db_path, state->current_table,
                                   col_names, values, data_result->column_count, rowid);
        }
    }
    
    char delete_query[512];
    snprintf(delete_query, sizeof(delete_query),
             "DELETE FROM \"%s\" WHERE rowid = %ld;", state->current_table, rowid);
    
    QueryResult *del_result = db_manager_execute_query(state->db_manager, delete_query);
    
    if (del_result) {
        update_status("Row deleted and saved to trash");
        show_table_data(state, state->current_table);
    } else {
        show_error_dialog("Error", "Failed to delete row.");
    }
    
    db_manager_free_query_result(rid_result);
    db_manager_free_query_result(data_result);
}
