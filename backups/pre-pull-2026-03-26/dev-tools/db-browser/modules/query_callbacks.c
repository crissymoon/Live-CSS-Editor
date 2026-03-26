#include "query_callbacks.h"
#include "data_protection.h"
#include "ui_utils.h"
#include "ui_drawing.h"
#include "../core/db_manager.h"
#include <stdio.h>
#include <string.h>

// Callback for when table selection changes in query builder
static void on_table_changed(GtkComboBox *combo, gpointer data) {
    GtkWidget *dialog = GTK_WIDGET(data);
    DBManager *mgr = (DBManager*)g_object_get_data(G_OBJECT(dialog), "db_manager");
    GtkWidget *columns_box = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "columns_box");
    
    // Clear old column checks
    GList *old_checks = (GList*)g_object_get_data(G_OBJECT(dialog), "column_checks");
    g_list_free(old_checks);
    
    // Remove all children except first (select all)
    GList *children = gtk_container_get_children(GTK_CONTAINER(columns_box));
    for (GList *l = g_list_next(children); l != NULL; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
    
    // Get selected table
    gchar *table_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!table_name) return;
    
    // Get and display columns
    GList *new_checks = NULL;
    ColumnInfo *columns = db_manager_get_columns(mgr, table_name);
    ColumnInfo *current = columns;
    while (current) {
        GtkWidget *check = gtk_check_button_new_with_label(current->name);
        gtk_box_pack_start(GTK_BOX(columns_box), check, FALSE, FALSE, 0);
        gtk_widget_show(check);
        new_checks = g_list_append(new_checks, check);
        current = current->next;
    }
    db_manager_free_column_info(columns);
    g_object_set_data(G_OBJECT(dialog), "column_checks", new_checks);
    g_free(table_name);
}

void on_template_clicked(GtkWidget *button, gpointer data) {
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);
    const char *template = g_object_get_data(G_OBJECT(button), "template");

    if (template && buffer) {
        gtk_text_buffer_set_text(buffer, template, -1);
    }
}

/* ---- Query builder SQL-generation helpers (static, used only here) ---- */

static GString *qb_select_columns(GtkWidget *dialog, GtkWidget *select_all_check) {
    GString *cols = g_string_new("");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(select_all_check))) {
        g_string_append(cols, "*");
        return cols;
    }
    GList *checks = (GList*)g_object_get_data(G_OBJECT(dialog), "column_checks");
    bool first = true;
    for (GList *l = checks; l != NULL; l = l->next) {
        GtkWidget *ck = GTK_WIDGET(l->data);
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck))) {
            if (!first) g_string_append(cols, ", ");
            g_string_append_printf(cols, "\"%s\"", gtk_button_get_label(GTK_BUTTON(ck)));
            first = false;
        }
    }
    if (first) g_string_assign(cols, "*"); /* nothing selected, fall back */
    return cols;
}

static GString *qb_insert_columns(GtkWidget *dialog, int *out_count) {
    GString *cols = g_string_new("");
    GList *checks = (GList*)g_object_get_data(G_OBJECT(dialog), "column_checks");
    bool first = true;
    *out_count = 0;
    for (GList *l = checks; l != NULL; l = l->next) {
        GtkWidget *ck = GTK_WIDGET(l->data);
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck))) {
            if (!first) g_string_append(cols, ", ");
            g_string_append_printf(cols, "\"%s\"", gtk_button_get_label(GTK_BUTTON(ck)));
            first = false;
            (*out_count)++;
        }
    }
    return cols;
}

static GString *qb_update_set(GtkWidget *dialog) {
    GString *set = g_string_new("");
    GList *checks = (GList*)g_object_get_data(G_OBJECT(dialog), "column_checks");
    bool first = true;
    for (GList *l = checks; l != NULL; l = l->next) {
        GtkWidget *ck = GTK_WIDGET(l->data);
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck))) {
            if (!first) g_string_append(set, ", ");
            g_string_append_printf(set, "\"%s\" = ?", gtk_button_get_label(GTK_BUTTON(ck)));
            first = false;
        }
    }
    return set;
}

static void qb_append_tail(GtkWidget *dialog, GString *q, const char *qtype) {
    GtkWidget *where_entry = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "where_entry");
    GtkWidget *order_check = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "order_check");
    GtkWidget *order_entry = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "order_entry");
    GtkWidget *limit_check = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "limit_check");
    GtkWidget *limit_spin  = (GtkWidget*)g_object_get_data(G_OBJECT(dialog), "limit_spin");

    const char *where = gtk_entry_get_text(GTK_ENTRY(where_entry));
    if (where && strlen(where) > 0)
        g_string_append_printf(q, " WHERE %s", where);

    if (strcmp(qtype, "SELECT") == 0) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(order_check))) {
            const char *ord = gtk_entry_get_text(GTK_ENTRY(order_entry));
            if (ord && strlen(ord) > 0)
                g_string_append_printf(q, " ORDER BY %s", ord);
        }
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(limit_check))) {
            int lim = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(limit_spin));
            g_string_append_printf(q, " LIMIT %d", lim);
        }
    }
}

static GString *qb_build_query(GtkWidget *dialog, const char *qtype,
                                const char *table, GtkWidget *select_all_check) {
    GString *q = g_string_new("");

    if (strcmp(qtype, "SELECT") == 0) {
        g_string_append(q, "SELECT ");
        GString *cols = qb_select_columns(dialog, select_all_check);
        g_string_append_printf(q, "%s FROM \"%s\"", cols->str, table);
        g_string_free(cols, TRUE);

    } else if (strcmp(qtype, "DELETE") == 0) {
        g_string_append_printf(q, "DELETE FROM \"%s\"", table);

    } else if (strcmp(qtype, "INSERT") == 0) {
        int col_count = 0;
        GString *cols = qb_insert_columns(dialog, &col_count);
        g_string_append_printf(q, "INSERT INTO \"%s\" (%s) VALUES (", table, cols->str);
        g_string_free(cols, TRUE);
        for (int i = 0; i < col_count; i++) {
            if (i > 0) g_string_append(q, ", ");
            g_string_append(q, "?");
        }
        g_string_append(q, ")");

    } else if (strcmp(qtype, "UPDATE") == 0) {
        GString *set = qb_update_set(dialog);
        g_string_append_printf(q, "UPDATE \"%s\" SET %s", table, set->str);
        g_string_free(set, TRUE);
    }

    qb_append_tail(dialog, q, qtype);
    g_string_append(q, ";");
    return q;
}

/* ---- Public callback ---- */

void on_sql_query_builder(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;

    if (!state->db_manager || !db_manager_is_open(state->db_manager)) {
        show_error_dialog("Error", "No database open. Please open a database first.");
        return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "SQL Query Builder",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Generate", GTK_RESPONSE_ACCEPT,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 500);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);

    /* --- query type row --- */
    GtkWidget *type_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content), type_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(type_box), gtk_label_new("Query Type:"), FALSE, FALSE, 0);

    GtkWidget *type_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "SELECT");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "INSERT");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "UPDATE");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "DELETE");
    gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);
    gtk_box_pack_start(GTK_BOX(type_box), type_combo, FALSE, FALSE, 0);

    /* --- table row --- */
    GtkWidget *table_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content), table_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(table_box), gtk_label_new("Table:"), FALSE, FALSE, 0);

    GtkWidget *table_combo = gtk_combo_box_text_new();
    TableInfo *tables = db_manager_get_tables(state->db_manager);
    for (TableInfo *t = tables; t; t = t->next)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(table_combo), t->name);
    if (tables)
        gtk_combo_box_set_active(GTK_COMBO_BOX(table_combo), 0);
    gtk_box_pack_start(GTK_BOX(table_box), table_combo, TRUE, TRUE, 0);

    /* --- columns section --- */
    GtkWidget *columns_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(columns_label), "<b>Columns:</b>");
    gtk_widget_set_halign(columns_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), columns_label, FALSE, FALSE, 5);

    GtkWidget *columns_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(columns_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(columns_scroll, -1, 150);

    GtkWidget *columns_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(columns_scroll), columns_box);
    gtk_box_pack_start(GTK_BOX(content), columns_scroll, TRUE, TRUE, 0);

    GtkWidget *select_all_check = gtk_check_button_new_with_label("Select All (*)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(select_all_check), TRUE);
    gtk_box_pack_start(GTK_BOX(columns_box), select_all_check, FALSE, FALSE, 0);

    GList *column_checks = NULL;
    if (tables) {
        ColumnInfo *cols = db_manager_get_columns(state->db_manager, tables->name);
        for (ColumnInfo *c = cols; c; c = c->next) {
            GtkWidget *ck = gtk_check_button_new_with_label(c->name);
            gtk_box_pack_start(GTK_BOX(columns_box), ck, FALSE, FALSE, 0);
            column_checks = g_list_append(column_checks, ck);
        }
        db_manager_free_column_info(cols);
    }

    /* --- WHERE row --- */
    GtkWidget *where_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(where_label), "<b>WHERE Clause:</b>");
    gtk_widget_set_halign(where_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), where_label, FALSE, FALSE, 5);

    GtkWidget *where_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(where_entry), "e.g., id > 10 AND name LIKE '%test%'");
    gtk_box_pack_start(GTK_BOX(content), where_entry, FALSE, FALSE, 0);

    /* --- ORDER BY row --- */
    GtkWidget *order_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content), order_box, FALSE, FALSE, 5);

    GtkWidget *order_check = gtk_check_button_new_with_label("ORDER BY:");
    gtk_box_pack_start(GTK_BOX(order_box), order_check, FALSE, FALSE, 0);

    GtkWidget *order_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(order_entry), "column_name ASC");
    gtk_box_pack_start(GTK_BOX(order_box), order_entry, TRUE, TRUE, 0);

    /* --- LIMIT row --- */
    GtkWidget *limit_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content), limit_box, FALSE, FALSE, 5);

    GtkWidget *limit_check = gtk_check_button_new_with_label("LIMIT:");
    gtk_box_pack_start(GTK_BOX(limit_box), limit_check, FALSE, FALSE, 0);

    GtkWidget *limit_spin = gtk_spin_button_new_with_range(1, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(limit_spin), 100);
    gtk_box_pack_start(GTK_BOX(limit_box), limit_spin, FALSE, FALSE, 0);

    /* --- wire everything into dialog object data so helpers can reach it --- */
    g_object_set_data(G_OBJECT(dialog), "type_combo",       type_combo);
    g_object_set_data(G_OBJECT(dialog), "table_combo",      table_combo);
    g_object_set_data(G_OBJECT(dialog), "select_all_check", select_all_check);
    g_object_set_data(G_OBJECT(dialog), "column_checks",    column_checks);
    g_object_set_data(G_OBJECT(dialog), "where_entry",      where_entry);
    g_object_set_data(G_OBJECT(dialog), "order_check",      order_check);
    g_object_set_data(G_OBJECT(dialog), "order_entry",      order_entry);
    g_object_set_data(G_OBJECT(dialog), "limit_check",      limit_check);
    g_object_set_data(G_OBJECT(dialog), "limit_spin",       limit_spin);
    g_object_set_data(G_OBJECT(dialog), "columns_box",      columns_box);
    g_object_set_data(G_OBJECT(dialog), "db_manager",       state->db_manager);
    g_signal_connect(table_combo, "changed", G_CALLBACK(on_table_changed), dialog);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *qtype = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(type_combo));
        char *table_name  = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(table_combo));

        if (!table_name || strlen(table_name) == 0) {
            show_error_dialog("Error", "Please select a table.");
            g_free(table_name);
        } else {
            GString *query = qb_build_query(dialog, qtype, table_name, select_all_check);
            GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->query_editor));
            gtk_text_buffer_set_text(buf, query->str, -1);
            update_status("Query generated successfully");
            g_string_free(query, TRUE);
            g_free(table_name);
        }
    }

    GList *checks = (GList*)g_object_get_data(G_OBJECT(dialog), "column_checks");
    g_list_free(checks);
    gtk_widget_destroy(dialog);
    db_manager_free_table_info(tables);
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

    // Analyze query for risk level
    QueryAnalysis *analysis = analyze_query_risk(query);
    
    if (analysis && should_confirm_query(analysis)) {
        // Show confirmation dialog for risky queries
        char *confirm_msg = get_confirmation_message(analysis);
        bool proceed = confirm_action(confirm_msg);
        g_free(confirm_msg);
        
        if (!proceed) {
            update_status("Query execution cancelled by user");
            free_query_analysis(analysis);
            g_free(query);
            return;
        }
        
        // Create auto-backup for destructive operations
        if (analysis->risk_level == QUERY_DESTRUCTIVE) {
            char reason[256];
            snprintf(reason, sizeof(reason), "Before executing %s query", 
                     analysis->query_type ? analysis->query_type : "unknown");
            create_auto_backup(state->db_manager, reason);
        }
    }
    
    free_query_analysis(analysis);

    // Save query to history before execution
    save_query_to_history(state, query);
    
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
    
    // Mark query as saved after successful execution
    mark_query_saved(state);
}

void on_open_sql_file(GtkWidget *widget, gpointer data) {
    show_info_dialog("Load SQL File", "SQL file loading coming soon!");
}

void on_query_buffer_changed(GtkTextBuffer *buffer, gpointer data) {
    AppState *state = (AppState*)data;
    if (state) {
        mark_query_dirty(state);
    }
}
