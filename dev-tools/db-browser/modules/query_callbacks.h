#ifndef QUERY_CALLBACKS_H
#define QUERY_CALLBACKS_H

#include <gtk/gtk.h>
#include "app_state.h"

void on_template_clicked(GtkWidget *button, gpointer data);
void on_sql_query_builder(GtkWidget *widget, gpointer data);
void on_execute_query(GtkWidget *widget, gpointer data);
void on_open_sql_file(GtkWidget *widget, gpointer data);

#endif
