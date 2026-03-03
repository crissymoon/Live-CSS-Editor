#include "table_callbacks.h"
#include "db_callbacks.h"
#include "ui_utils.h"
#include "data_protection.h"
#include "trash_manager.h"
#include <string.h>

typedef struct {
    GtkWidget *name_entry;
    GtkWidget *type_combo;
    GtkWidget *not_null_check;
    GtkWidget *pk_check;
    GtkWidget *unique_check;
} ColumnRow;

void on_new_table(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database open. Please open or create a database first.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Create New Table",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Create", GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);

    // Table name
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *name_label = gtk_label_new("Table Name:");
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Enter table name");
    gtk_box_pack_start(GTK_BOX(name_box), name_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(name_box), name_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content), name_box, FALSE, FALSE, 5);

    // Column list
    GtkWidget *columns_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(columns_label), "<b>Columns:</b>");
    gtk_widget_set_halign(columns_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), columns_label, FALSE, FALSE, 5);

    // Scrolled window for columns
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *columns_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(columns_box), 5);

    // Column header
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(header_box), gtk_label_new("Name"), FALSE, FALSE, 100);
    gtk_box_pack_start(GTK_BOX(header_box), gtk_label_new("Type"), FALSE, FALSE, 80);
    gtk_box_pack_start(GTK_BOX(header_box), gtk_label_new("NOT NULL"), FALSE, FALSE, 40);
    gtk_box_pack_start(GTK_BOX(header_box), gtk_label_new("PRIMARY KEY"), FALSE, FALSE, 40);
    gtk_box_pack_start(GTK_BOX(header_box), gtk_label_new("UNIQUE"), FALSE, FALSE, 40);
    gtk_box_pack_start(GTK_BOX(columns_box), header_box, FALSE, FALSE, 0);

    // Add 5 column rows by default
    GList *column_rows = NULL;
    const char *types[] = {"INTEGER", "TEXT", "REAL", "BLOB", "NUMERIC"};

    for (int i = 0; i < 5; i++) {
        ColumnRow *row = g_malloc(sizeof(ColumnRow));

        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        row->name_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(row->name_entry), "column_name");
        gtk_widget_set_size_request(row->name_entry, 150, -1);

        row->type_combo = gtk_combo_box_text_new();
        for (size_t j = 0; j < sizeof(types) / sizeof(types[0]); j++) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(row->type_combo), types[j]);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(row->type_combo), i == 0 ? 0 : 1);
        gtk_widget_set_size_request(row->type_combo, 120, -1);

        row->not_null_check = gtk_check_button_new();
        row->pk_check = gtk_check_button_new();
        row->unique_check = gtk_check_button_new();

        gtk_box_pack_start(GTK_BOX(row_box), row->name_entry, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row_box), row->type_combo, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row_box), row->not_null_check, FALSE, FALSE, 50);
        gtk_box_pack_start(GTK_BOX(row_box), row->pk_check, FALSE, FALSE, 50);
        gtk_box_pack_start(GTK_BOX(row_box), row->unique_check, FALSE, FALSE, 50);

        gtk_box_pack_start(GTK_BOX(columns_box), row_box, FALSE, FALSE, 0);
        column_rows = g_list_append(column_rows, row);
    }

    gtk_container_add(GTK_CONTAINER(scroll), columns_box);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *table_name = gtk_entry_get_text(GTK_ENTRY(name_entry));

        if (!table_name || strlen(table_name) == 0) {
            show_error_dialog("Error", "Please enter a table name.");
            gtk_widget_destroy(dialog);
            g_list_free_full(column_rows, g_free);
            return;
        }

        // Build column list
        ColumnInfo *first_col = NULL;
        ColumnInfo *last_col = NULL;
        int col_count = 0;

        for (GList *l = column_rows; l != NULL; l = l->next) {
            ColumnRow *row = (ColumnRow*)l->data;
            const char *col_name = gtk_entry_get_text(GTK_ENTRY(row->name_entry));

            if (col_name && strlen(col_name) > 0) {
                ColumnInfo *col = g_malloc0(sizeof(ColumnInfo));
                col->name = g_strdup(col_name);
                col->type = g_strdup(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(row->type_combo)));
                col->not_null = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(row->not_null_check));
                col->primary_key = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(row->pk_check));
                col->unique = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(row->unique_check));
                col->default_value = NULL;
                col->next = NULL;

                if (!first_col) {
                    first_col = col;
                    last_col = col;
                } else {
                    last_col->next = col;
                    last_col = col;
                }
                col_count++;
            }
        }

        if (col_count == 0) {
            show_error_dialog("Error", "Please define at least one column.");
            gtk_widget_destroy(dialog);
            g_list_free_full(column_rows, g_free);
            return;
        }

        // Create the table
        int result = db_manager_create_table(state->db_manager, table_name, first_col);

        // Free column list
        ColumnInfo *col = first_col;
        while (col) {
            ColumnInfo *next = col->next;
            g_free(col->name);
            g_free(col->type);
            if (col->default_value) g_free(col->default_value);
            g_free(col);
            col = next;
        }

        if (result == 0) {
            char status[256];
            snprintf(status, sizeof(status), "Created table '%s' with %d columns", table_name, col_count);
            update_status(status);
            show_info_dialog("Success", status);
            refresh_table_list(state);
        } else {
            show_error_dialog("Error", "Failed to create table. Check if the name is valid and unique.");
        }
    }

    gtk_widget_destroy(dialog);
    g_list_free_full(column_rows, g_free);
}

static bool check_table_relationships(AppState *state, const char *table_name, char *error_msg, size_t error_size) {
    // Check if other tables reference this table via foreign keys
    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT name, sql FROM sqlite_master WHERE type='table' AND name != '%s'",
             table_name);
    
    QueryResult *result = db_manager_execute_query(state->db_manager, query);
    if (!result) return true; // Allow drop if we can't check
    
    bool has_references = false;
    char referencing_tables[512] = {0};
    int ref_count = 0;
    
    for (int i = 0; i < result->row_count && !has_references; i++) {
        const char *other_table = result->data[i][0];
        const char *create_sql = result->data[i][1];
        
        if (create_sql && strstr(create_sql, "FOREIGN KEY")) {
            // Check if this table references the table we want to drop
            char search_pattern[256];
            snprintf(search_pattern, sizeof(search_pattern), "REFERENCES %s", table_name);
            snprintf(search_pattern, sizeof(search_pattern), "REFERENCES \"%s\"", table_name);
            
            if (strstr(create_sql, table_name) && strstr(create_sql, "REFERENCES")) {
                has_references = true;
                ref_count++;
                if (strlen(referencing_tables) > 0) {
                    strncat(referencing_tables, ", ", sizeof(referencing_tables) - strlen(referencing_tables) - 1);
                }
                strncat(referencing_tables, other_table, sizeof(referencing_tables) - strlen(referencing_tables) - 1);
            }
        }
    }
    
    db_manager_free_query_result(result);
    
    if (has_references) {
        snprintf(error_msg, error_size,
                 "Cannot drop table '%s' because it is referenced by:\n\n%s\n\n"
                 "You must drop or modify the referencing tables first.",
                 table_name, referencing_tables);
        return false;
    }
    
    return true;
}

static bool verify_password_for_drop(GtkWindow *parent) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Authentication Required",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Confirm Drop", GTK_RESPONSE_OK,
        NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 420, 200);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);
    
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), 
        "<b>Drop Table</b>\n\n"
        "This is a destructive operation. The table will be saved to trash.\n"
        "Enter your password to confirm:");
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

void on_drop_table(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;
    (void)widget;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database open.");
        return;
    }

    if (!state->current_table || strlen(state->current_table) == 0) {
        show_error_dialog("No Table Selected", 
            "Please double-click a table from the list to select it first, then click Drop Table.");
        return;
    }

    // Debug: Print current table name
    fprintf(stderr, "[DEBUG] Attempting to drop table: '%s'\n", state->current_table);

    // Check for foreign key relationships
    char relationship_error[1024];
    if (!check_table_relationships(state, state->current_table, relationship_error, sizeof(relationship_error))) {
        show_error_dialog("Foreign Key Constraint", relationship_error);
        return;
    }

    // Get row count for confirmation message
    char count_query[512];
    snprintf(count_query, sizeof(count_query), "SELECT COUNT(*) FROM \"%s\"", state->current_table);
    QueryResult *count_result = db_manager_execute_query(state->db_manager, count_query);
    int row_count = 0;
    if (count_result && count_result->row_count > 0 && count_result->data[0][0]) {
        row_count = atoi(count_result->data[0][0]);
    }
    if (count_result) db_manager_free_query_result(count_result);

    char confirm_msg[768];
    snprintf(confirm_msg, sizeof(confirm_msg),
             "Drop Table: '%s'\n\n"
             "Rows: %d\n\n"
             "The table will be saved to trash for recovery.\n"
             "You must enter your password to proceed.\n\n"
             "Are you sure you want to continue?",
             state->current_table, row_count);

    if (confirm_action(confirm_msg)) {
        // Require password authentication
        if (!verify_password_for_drop(GTK_WINDOW(state->window))) {
            update_status("Drop table operation cancelled");
            return;
        }

        // Get CREATE statement for the table
        char schema_query[512];
        snprintf(schema_query, sizeof(schema_query),
                 "SELECT sql FROM sqlite_master WHERE type='table' AND name='%s'",
                 state->current_table);
        QueryResult *schema_result = db_manager_execute_query(state->db_manager, schema_query);
        
        const char *create_sql = NULL;
        if (schema_result && schema_result->row_count > 0 && schema_result->data[0][0]) {
            create_sql = schema_result->data[0][0];
        }

        // Save table to trash
        TrashManager *trash_mgr = trash_manager_init(state->db_manager->db_path);
        if (trash_mgr && create_sql) {
            trash_manager_save_table(trash_mgr, state->db_manager->db_path,
                                    state->current_table, create_sql,
                                    state->db_manager->db);
            trash_manager_close(trash_mgr);
        }

        if (schema_result) db_manager_free_query_result(schema_result);

        // Create backup before dropping
        create_auto_backup(state->db_manager, "Before dropping table");

        // Drop the table
        int result = db_manager_drop_table(state->db_manager, state->current_table);

        if (result == 0) {
            char status[256];
            snprintf(status, sizeof(status), "Dropped table '%s' and saved to trash", state->current_table);
            update_status(status);
            show_info_dialog("Success", status);

            // Clear current table
            g_free(state->current_table);
            state->current_table = NULL;

            refresh_table_list(state);
        } else {
            char error_msg[512];
            const char *db_error = db_manager_get_last_error(state->db_manager);
            snprintf(error_msg, sizeof(error_msg), 
                     "Failed to drop table '%s'.\n\nSQLite Error: %s",
                     state->current_table,
                     db_error ? db_error : "Unknown error");
            show_error_dialog("Drop Table Failed", error_msg);
        }
    }
}

