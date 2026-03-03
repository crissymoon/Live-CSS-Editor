#include "query_callbacks.h"
#include "ui_utils.h"
#include "ui_drawing.h"
#include <stdio.h>
#include <string.h>

void on_template_clicked(GtkWidget *button, gpointer data) {
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);
    const char *template = g_object_get_data(G_OBJECT(button), "template");

    if (template && buffer) {
        gtk_text_buffer_set_text(buffer, template, -1);
    }
}

void on_sql_query_builder(GtkWidget *widget, gpointer data) {
    show_info_dialog("Query Builder", "Query builder UI coming soon!");
}

void on_execute_query(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database open. Please open a database first.");
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->query_editor));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *query = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    if (!query || strlen(query) == 0) {
        show_error_dialog("Error", "Please enter a SQL query to execute.");
        g_free(query);
        return;
    }

    QueryResult *result = db_manager_execute_query(state->db_manager, query);
    g_free(query);

    if (!result) {
        show_error_dialog("Query Error", "Failed to execute query.");
        return;
    }

    if (result->error) {
        show_error_dialog("SQL Error", result->error);
        return;
    }

    GList *columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(state->result_view));
    for (GList *col = columns; col != NULL; col = col->next) {
        gtk_tree_view_remove_column(GTK_TREE_VIEW(state->result_view), GTK_TREE_VIEW_COLUMN(col->data));
    }
    g_list_free(columns);

    if (result->row_count > 0 && result->data) {
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
                const char *value = result->data[row][col] ? result->data[row][col] : "NULL";
                gtk_list_store_set(store, &iter, col, value, -1);
            }
        }

        gtk_tree_view_set_model(GTK_TREE_VIEW(state->result_view), GTK_TREE_MODEL(store));
        g_object_unref(store);

        for (int i = 0; i < result->column_count; i++) {
            GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
            g_object_set(renderer, "xpad", 10, "ypad", 4, NULL);
            GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
                result->column_names[i], renderer, "text", i, NULL);
            gtk_tree_view_column_set_cell_data_func(column, renderer, zebra_cell_data_func, NULL, NULL);
            gtk_tree_view_column_set_resizable(column, TRUE);
            gtk_tree_view_column_set_sort_column_id(column, i);
            gtk_tree_view_column_set_min_width(column, 100);
            gtk_tree_view_append_column(GTK_TREE_VIEW(state->result_view), column);
        }

        char status[256];
        snprintf(status, sizeof(status), "Query executed successfully: %d rows returned", result->row_count);
        update_status(status);
        show_info_dialog("Success", status);
    } else {
        update_status("Query executed successfully: 0 rows returned");
        show_info_dialog("Success", "Query executed successfully. No rows to display.");
    }
}

void on_open_sql_file(GtkWidget *widget, gpointer data) {
    show_info_dialog("Load SQL File", "SQL file loading coming soon!");
}
