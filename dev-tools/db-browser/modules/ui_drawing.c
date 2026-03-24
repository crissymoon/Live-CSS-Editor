#include "ui_drawing.h"
#include "app_state.h"
#include <math.h>

gboolean draw_treeview_border(GtkWidget *widget, cairo_t *cr,
                              gpointer user_data G_GNUC_UNUSED)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    if (app && app->theme_is_dark) {
        cairo_set_source_rgb(cr, 0.227, 0.157, 0.439);   /* #3a2870 */
    } else {
        cairo_set_source_rgb(cr, 0.741, 0.765, 0.780);   /* #bdc3c7 */
    }
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, alloc.width - 1.0, alloc.height - 1.0);
    cairo_stroke(cr);
    return FALSE;
}

gboolean draw_column_lines(GtkWidget *widget, cairo_t *cr,
                           gpointer user_data G_GNUC_UNUSED)
{
    GtkTreeView *tv = GTK_TREE_VIEW(widget);
    int n_cols = gtk_tree_view_get_n_columns(tv);
    if (n_cols < 2) return FALSE;

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    if (app && app->theme_is_dark) {
        cairo_set_source_rgb(cr, 0.227, 0.157, 0.439);   /* #3a2870 */
    } else {
        cairo_set_source_rgb(cr, 0.741, 0.765, 0.780);   /* #bdc3c7 */
    }
    cairo_set_line_width(cr, 1.0);

    gdouble h_scroll_offset = 0.0;
    GtkAdjustment *h_adj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widget));
    if (h_adj) {
        h_scroll_offset = gtk_adjustment_get_value(h_adj);
    }

    double x = 0;
    for (int i = 0; i < n_cols - 1; i++) {
        GtkTreeViewColumn *col = gtk_tree_view_get_column(tv, i);
        x += gtk_tree_view_column_get_width(col);
        double lx = floor(x - h_scroll_offset) + 0.5;
        cairo_move_to(cr, lx, 0);
        cairo_line_to(cr, lx, alloc.height);
        cairo_stroke(cr);
    }
    return FALSE;
}

void zebra_cell_data_func(GtkTreeViewColumn *col G_GNUC_UNUSED,
                          GtkCellRenderer   *renderer,
                          GtkTreeModel      *model G_GNUC_UNUSED,
                          GtkTreeIter       *iter,
                          gpointer           data G_GNUC_UNUSED)
{
    GtkTreePath *path = gtk_tree_model_get_path(model, iter);
    int row = gtk_tree_path_get_indices(path)[0];
    gtk_tree_path_free(path);

    if (app && app->theme_is_dark) {
        const char *bg = (row % 2 == 0) ? "#0c071c" : "#160e38";
        g_object_set(renderer,
                     "cell-background",     bg,
                     "cell-background-set", TRUE,
                     "foreground",          "#c8c0e8",
                     "foreground-set",      TRUE,
                     NULL);
    } else {
        const char *bg = (row % 2 == 0) ? "#ffffff" : "#e8f0fa";
        g_object_set(renderer,
                     "cell-background",     bg,
                     "cell-background-set", TRUE,
                     "foreground",          "#1a1a2e",
                     "foreground-set",      TRUE,
                     NULL);
    }
}
