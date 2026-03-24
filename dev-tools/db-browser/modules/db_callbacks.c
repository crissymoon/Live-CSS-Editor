#include "db_callbacks.h"
#include "ui_utils.h"
#include "ui_drawing.h"
#include "recent_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void on_open_database(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open Database",
                                                     GTK_WINDOW(state->window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Open", GTK_RESPONSE_ACCEPT,
                                                     NULL);

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
            gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), 0);
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

void on_new_database(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Create New Database",
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Create", GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "new_database.db");

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "SQLite Database Files");
    gtk_file_filter_add_pattern(filter, "*.db");
    gtk_file_filter_add_pattern(filter, "*.sqlite");
    gtk_file_filter_add_pattern(filter, "*.sqlite3");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        // Close any existing database
        if (state->db_manager) {
            db_manager_destroy(state->db_manager);
            state->db_manager = NULL;
        }

        // Create and open new database
        state->db_manager = db_manager_create(filename, false);
        if (state->db_manager) {
            int result = db_manager_open(state->db_manager);
            if (result == 0) {
                char status[512];
                snprintf(status, sizeof(status), "Created database: %s", filename);
                update_status(status);
                show_info_dialog("Success", "Database created successfully.");
                add_to_recent(state, filename);
                refresh_table_list(state);
            } else {
                show_error_dialog("Error", "Failed to create database.");
                db_manager_destroy(state->db_manager);
                state->db_manager = NULL;
            }
        } else {
            show_error_dialog("Error", "Failed to initialize database manager.");
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void on_close_database(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    if (state->db_manager) {
        db_manager_destroy(state->db_manager);
        state->db_manager = NULL;
        update_status("Database closed");
    }
}

void on_quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

void on_refresh_tables(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;
    refresh_table_list(state);
}

void refresh_table_list(AppState *state) {
    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        update_status("No database open");
        return;
    }

    TableInfo *tables = db_manager_get_tables(state->db_manager);
    printf("[db-browser] refresh_table_list: db_manager_get_tables returned %s\n",
           tables ? "tables" : "NULL");
    if (!tables) {
        update_status("No tables found or error reading database");
        return;
    }

    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    int count = 0;
    for (TableInfo *t = tables; t != NULL; t = t->next) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);

        char info[128];
        snprintf(info, sizeof(info), "%d rows", t->row_count);

        gtk_list_store_set(store, &iter,
                          0, t->name,
                          1, "table",
                          2, info,
                          -1);
        count++;
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(state->table_view), GTK_TREE_MODEL(store));

    if (gtk_tree_view_get_n_columns(GTK_TREE_VIEW(state->table_view)) == 0) {
        GtkCellRenderer *r0 = gtk_cell_renderer_text_new();
        g_object_set(r0, "xpad", 10, "ypad", 4, NULL);
        GtkTreeViewColumn *col_name = gtk_tree_view_column_new_with_attributes(
            "Table Name", r0, "text", 0, NULL);
        gtk_tree_view_column_set_cell_data_func(col_name, r0, zebra_cell_data_func, NULL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(state->table_view), col_name);

        GtkCellRenderer *r1 = gtk_cell_renderer_text_new();
        g_object_set(r1, "xpad", 10, "ypad", 4, NULL);
        GtkTreeViewColumn *col_type = gtk_tree_view_column_new_with_attributes(
            "Type", r1, "text", 1, NULL);
        gtk_tree_view_column_set_cell_data_func(col_type, r1, zebra_cell_data_func, NULL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(state->table_view), col_type);

        GtkCellRenderer *r2 = gtk_cell_renderer_text_new();
        g_object_set(r2, "xpad", 10, "ypad", 4, NULL);
        GtkTreeViewColumn *col_info = gtk_tree_view_column_new_with_attributes(
            "Info", r2, "text", 2, NULL);
        gtk_tree_view_column_set_cell_data_func(col_info, r2, zebra_cell_data_func, NULL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(state->table_view), col_info);
    }

    g_object_unref(store);

    TableInfo *t = tables;
    while (t) {
        TableInfo *next = t->next;
        free(t->name);
        free(t);
        t = next;
    }

    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), "Loaded %d table(s)", count);
    update_status(status_msg);
}

void on_table_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer data) {
    (void)column;
    AppState *state = (AppState*)data;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *table_name;
        gtk_tree_model_get(model, &iter, 0, &table_name, -1);

        gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), 2);
        show_table_data(state, table_name);

        g_free(table_name);
    }
}

void show_table_data(AppState *state, const char *table_name) {
    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        gtk_label_set_markup(GTK_LABEL(state->data_info_label),
            "<b>Error:</b> No database open");
        return;
    }

    g_free(state->current_table);
    state->current_table = g_strdup(table_name);

    char info[512];
    snprintf(info, sizeof(info),
        "<b>Table:</b> %s   |   <b>Tip:</b> Showing up to 100 rows",
        table_name);
    gtk_label_set_markup(GTK_LABEL(state->data_info_label), info);

    char query[512];
    snprintf(query, sizeof(query), "SELECT * FROM %s LIMIT 100", table_name);

    QueryResult *result = db_manager_execute_query(state->db_manager, query);

    if (result && result->data && result->row_count > 0) {
        GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(state->data_view));
        for (GList *col = columns; col != NULL; col = col->next) {
            gtk_tree_view_remove_column(GTK_TREE_VIEW(state->data_view), GTK_TREE_VIEW_COLUMN(col->data));
        }
        g_list_free(columns);

        GType *types = g_malloc(sizeof(GType) * result->column_count);
        for (int i = 0; i < result->column_count; i++) {
            types[i] = G_TYPE_STRING;
        }
        GtkListStore *store = gtk_list_store_newv(result->column_count, types);
        g_free(types);

        for (int row = 0; row < result->row_count; row++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            for (int col = 0; col < result->column_count; col++) {
                const char *value = result->data[row][col] ? result->data[row][col] : "";
                gtk_list_store_set(store, &iter, col, value, -1);
            }
        }

        gtk_tree_view_set_model(GTK_TREE_VIEW(state->data_view), GTK_TREE_MODEL(store));
        g_object_unref(store);

        for (int i = 0; i < result->column_count; i++) {
            GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
            g_object_set(renderer, "xpad", 10, "ypad", 4, NULL);
            GtkTreeViewColumn *column = gtk_tree_view_column_new();
            gtk_tree_view_column_pack_start(column, renderer, TRUE);
            gtk_tree_view_column_add_attribute(column, renderer, "text", i);
            /* Use gtk_label_new so '_' is never treated as a mnemonic accelerator */
            GtkWidget *header = gtk_label_new(result->column_names[i]);
            gtk_widget_show(header);
            gtk_tree_view_column_set_widget(column, header);
            gtk_tree_view_column_set_cell_data_func(column, renderer, zebra_cell_data_func, NULL, NULL);
            gtk_tree_view_column_set_resizable(column, TRUE);
            gtk_tree_view_column_set_sort_column_id(column, i);
            gtk_tree_view_column_set_min_width(column, 120);
            gtk_tree_view_append_column(GTK_TREE_VIEW(state->data_view), column);
        }

        char status[256];
        snprintf(status, sizeof(status), "Loaded %d rows from '%s'", result->row_count, table_name);
        update_status(status);

        snprintf(info, sizeof(info),
            "<b>Table:</b> %s   |   <b>Rows:</b> %d (limited to 100)",
            table_name, result->row_count);
        gtk_label_set_markup(GTK_LABEL(state->data_info_label), info);
    } else {
        gtk_label_set_markup(GTK_LABEL(state->data_info_label),
            "<b>Error:</b> No data found or table is empty");
        update_status("No data to display");
    }

    if (result) db_manager_free_query_result(result);
}

void on_data_cell_activated(GtkTreeView *tree_view, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer data)
{
    AppState *state = (AppState *)data;
    if (!state->db_manager || !db_manager_is_open(state->db_manager))
        return;
    if (!state->current_table)
        return;

    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;

    GList *all_cols = gtk_tree_view_get_columns(tree_view);
    int col_idx = g_list_index(all_cols, column);
    g_list_free(all_cols);
    if (col_idx < 0) return;

    const char *col_name = gtk_tree_view_column_get_title(column);
    if (!col_name) return;

    gchar *old_value = NULL;
    gtk_tree_model_get(model, &iter, col_idx, &old_value, -1);

    int row_idx = gtk_tree_path_get_indices(path)[0];

    char rowid_query[512];
    snprintf(rowid_query, sizeof(rowid_query),
             "SELECT rowid FROM \"%s\" LIMIT 100", state->current_table);
    QueryResult *rid_result = db_manager_execute_query(state->db_manager, rowid_query);
    if (!rid_result || row_idx >= rid_result->row_count) {
        g_free(old_value);
        if (rid_result) db_manager_free_query_result(rid_result);
        show_error_dialog("Error", "Could not determine row ID for this cell.");
        return;
    }
    const char *rowid_str = rid_result->data[row_idx][0];
    long rowid = atol(rowid_str);

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Edit Cell",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel",   GTK_RESPONSE_CANCEL,
        "Set NULL", GTK_RESPONSE_YES,
        "Save",     GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 480, 280);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);

    char header[256];
    snprintf(header, sizeof(header),
             "<b>Table:</b> %s  |  <b>Column:</b> %s  |  <b>Row ID:</b> %ld",
             state->current_table, col_name, rowid);
    GtkWidget *header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label), header);
    gtk_widget_set_halign(header_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), header_label, FALSE, FALSE, 4);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_view), 8);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_view), 8);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    if (old_value)
        gtk_text_buffer_set_text(buffer, old_value, -1);

    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 4);

    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT || response == GTK_RESPONSE_YES) {
        char update_sql[4096];

        if (response == GTK_RESPONSE_YES) {
            snprintf(update_sql, sizeof(update_sql),
                     "UPDATE \"%s\" SET \"%s\" = NULL WHERE rowid = %ld",
                     state->current_table, col_name, rowid);
        } else {
            GtkTextIter start, end;
            gtk_text_buffer_get_start_iter(buffer, &start);
            gtk_text_buffer_get_end_iter(buffer, &end);
            gchar *new_value = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

            GString *escaped = g_string_new("");
            for (const char *p = new_value; *p; p++) {
                if (*p == '\'')
                    g_string_append(escaped, "''");
                else
                    g_string_append_c(escaped, *p);
            }

            snprintf(update_sql, sizeof(update_sql),
                     "UPDATE \"%s\" SET \"%s\" = '%s' WHERE rowid = %ld",
                     state->current_table, col_name, escaped->str, rowid);
            g_string_free(escaped, TRUE);
            g_free(new_value);
        }

        QueryResult *upd = db_manager_execute_query(state->db_manager, update_sql);
        if (upd && upd->error) {
            show_error_dialog("Update Error", upd->error);
        } else {
            show_table_data(state, state->current_table);
            update_status("Cell updated successfully");
        }
        if (upd) db_manager_free_query_result(upd);
    }

    gtk_widget_destroy(dialog);
    db_manager_free_query_result(rid_result);
    g_free(old_value);
}

void on_open_recent_db(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState *)data;

    const gchar *path = (const gchar *)g_object_get_data(G_OBJECT(widget), "db-path");
    if (!path || strlen(path) == 0) {
        fprintf(stderr, "[db-browser] on_open_recent_db: no db-path on widget\n");
        return;
    }

    gchar *path_owned = g_strdup(path);

    printf("[db-browser] on_open_recent_db: opening '%s'\n", path_owned);

    if (state->db_manager) {
        db_manager_destroy(state->db_manager);
        state->db_manager = NULL;
    }

    state->db_manager = db_manager_create(path_owned, false);
    if (!state->db_manager) {
        fprintf(stderr, "[db-browser] on_open_recent_db: db_manager_create failed for '%s'\n", path_owned);
        show_error_dialog("Error", "Failed to create database manager");
        g_free(path_owned);
        return;
    }

    if (db_manager_open(state->db_manager) == SQLITE_OK) {
        char status[512];
        snprintf(status, sizeof(status), "Opened: %s", path_owned);
        update_status(status);

        gchar *title = g_strdup_printf("Crissy's DB Browser - %s", path_owned);
        gtk_window_set_title(GTK_WINDOW(state->window), title);
        g_free(title);

        refresh_table_list(state);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), 0);
        add_to_recent(state, path_owned);
        printf("[db-browser] on_open_recent_db: success for '%s'\n", path_owned);
    } else {
        const char *db_err = db_manager_get_last_error(state->db_manager);
        fprintf(stderr, "[db-browser] on_open_recent_db: failed to open '%s': %s\n",
                path_owned, db_err ? db_err : "unknown error");
        show_error_dialog("Error", "Failed to open database from recent list");
        db_manager_destroy(state->db_manager);
        state->db_manager = NULL;
    }
    g_free(path_owned);
}
