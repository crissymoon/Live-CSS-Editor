#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "app_state.h"

char *get_theme_pref_path(void);
bool load_theme_pref(void);
void save_theme_pref(bool is_dark);
void on_toggle_theme(GtkWidget *widget, gpointer data);

#endif
