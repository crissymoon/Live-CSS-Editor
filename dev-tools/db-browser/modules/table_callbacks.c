#include "table_callbacks.h"
#include "ui_utils.h"

void on_new_table(GtkWidget *widget, gpointer data) {
    show_info_dialog("Not Implemented", "New table creation dialog coming soon!");
}

void on_drop_table(GtkWidget *widget, gpointer data) {
    if (confirm_action("Are you sure you want to delete this table?\nThis action cannot be undone!")) {
        show_info_dialog("Not Implemented", "Drop table functionality coming soon!");
    }
}
