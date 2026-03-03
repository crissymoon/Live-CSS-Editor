#ifndef APP_STATE_H
#define APP_STATE_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "../core/db_manager.h"
#include "../ui/tooltips.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *status_bar;
    GtkWidget *table_view;
    GtkWidget *query_editor;
    GtkWidget *result_view;
    GtkWidget *data_view;
    GtkWidget *data_info_label;

    DBManager *db_manager;
    TooltipManager *tooltip_manager;

    char *current_table;
    bool modified;
    GtkWidget *recent_btn;
    GList *recent_paths;

    bool theme_is_dark;
    GtkWidget *theme_btn;
    
    // Data protection
    bool query_dirty;
    char *last_saved_query;
} AppState;

extern AppState *app;
extern GtkCssProvider *app_css_provider;
extern char app_exe_dir[1024];

#endif
