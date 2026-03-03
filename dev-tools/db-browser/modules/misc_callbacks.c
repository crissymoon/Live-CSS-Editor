#include "misc_callbacks.h"
#include "ui_utils.h"

void on_backup_database(GtkWidget *widget, gpointer data) {
    show_info_dialog("Not Implemented", "Database backup coming soon!");
}

void on_help_tutorial(GtkWidget *widget, gpointer data) {
    AppState *state = (AppState*)data;
    tooltip_manager_enable_tutorial(state->tooltip_manager);
    show_info_dialog("Tutorial Mode",
                    "Tutorial mode enabled! Hover over any button or field "
                    "to see detailed explanations and examples.");
}

void on_help_about(GtkWidget *widget, gpointer data) {
    show_info_dialog("About",
                    "Crissy's DB Browser\n\n"
                    "A lightweight SQLite database browser\n"
                    "Version 1.0.0\n\n"
                    "Built with GTK+3 and SQLite3.");
}
