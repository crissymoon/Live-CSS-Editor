#include "table_callbacks.h"
#include "db_callbacks.h"
#include "ui_utils.h"
#include "data_protection.h"
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

void on_drop_table(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database open.");
        return;
    }

    if (!state->current_table || strlen(state->current_table) == 0) {
        show_error_dialog("Error", "No table selected. Please select a table from the list.");
        return;
    }

    char confirm_msg[512];
    snprintf(confirm_msg, sizeof(confirm_msg),
             "Are you sure you want to DROP TABLE '%s'?\n\n"
             "This will permanently delete the table and all its data.\n"
             "This action CANNOT be undone!",
             state->current_table);

    if (confirm_action(confirm_msg)) {
        // Create backup before dropping
        create_auto_backup(state->db_manager, "Before dropping table");

        int result = db_manager_drop_table(state->db_manager, state->current_table);

        if (result == 0) {
            char status[256];
            snprintf(status, sizeof(status), "Dropped table '%s'", state->current_table);
            update_status(status);
            show_info_dialog("Success", status);

            // Clear current table
            g_free(state->current_table);
            state->current_table = NULL;

            refresh_table_list(state);
        } else {
            show_error_dialog("Error", "Failed to drop table. Check if the table exists.");
        }
    }
}
