#ifndef CSV_HANDLER_H
#define CSV_HANDLER_H

#include <gtk/gtk.h>
#include <stdio.h>
#include "app_state.h"

void csv_write_field(FILE *f, const char *val);
char **csv_parse_line(const char *line, int *col_count);
void on_export_csv(GtkWidget *widget, gpointer data);
void on_import_csv(GtkWidget *widget, gpointer data);

#endif
