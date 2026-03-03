#ifndef UI_DRAWING_H
#define UI_DRAWING_H

#include <gtk/gtk.h>
#include <cairo.h>

gboolean draw_treeview_border(GtkWidget *widget, cairo_t *cr, gpointer user_data);
gboolean draw_column_lines(GtkWidget *widget, cairo_t *cr, gpointer user_data);
void zebra_cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer, 
                          GtkTreeModel *model, GtkTreeIter *iter, gpointer data);

#endif
