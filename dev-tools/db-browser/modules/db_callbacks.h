#ifndef DB_CALLBACKS_H
#define DB_CALLBACKS_H

#include <gtk/gtk.h>
#include "app_state.h"

void on_open_database(GtkWidget *widget, gpointer data);
void on_new_database(GtkWidget *widget, gpointer data);
void on_close_database(GtkWidget *widget, gpointer data);
void on_quit(GtkWidget *widget, gpointer data);
void on_refresh_tables(GtkWidget *widget, gpointer data);
void refresh_table_list(AppState *state);
void on_table_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
void show_table_data(AppState *state, const char *table_name);
void on_data_cell_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
void on_open_recent_db(GtkWidget *widget, gpointer data);

#endif
