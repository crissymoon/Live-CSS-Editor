#ifndef ROW_OPERATIONS_H
#define ROW_OPERATIONS_H

#include <gtk/gtk.h>
#include "app_state.h"

// Show dialog to add a new row to the current table
void on_add_row_clicked(GtkWidget *widget, gpointer data);

// Show dialog to delete selected row(s) with password authentication
void on_delete_row_clicked(GtkWidget *widget, gpointer data);

// Initialize row operations system
void row_operations_init(void);

// Cleanup row operations system
void row_operations_cleanup(void);

#endif // ROW_OPERATIONS_H
