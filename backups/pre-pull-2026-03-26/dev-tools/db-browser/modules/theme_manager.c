#include "theme_manager.h"
#include <string.h>
#include <stdio.h>

char *get_theme_pref_path(void) {
    const char *cfg = g_get_user_config_dir();
    char *dir  = g_build_filename(cfg, "crissy-db-browser", NULL);
    g_mkdir_with_parents(dir, 0755);
    char *path = g_build_filename(dir, "theme", NULL);
    g_free(dir);
    return path;
}

bool load_theme_pref(void) {
    char *path = get_theme_pref_path();
    FILE *f = fopen(path, "r");
    g_free(path);
    if (!f) return true;
    char buf[32] = {0};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    return strncmp(buf, "light", 5) != 0;
}

void save_theme_pref(bool is_dark) {
    char *path = get_theme_pref_path();
    FILE *f = fopen(path, "w");
    g_free(path);
    if (!f) return;
    fprintf(f, "%s\n", is_dark ? "dark" : "light");
    fclose(f);
}

void on_toggle_theme(GtkWidget *widget, gpointer data) {
    AppState   *state   = (AppState *)data;
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen  *screen  = gdk_display_get_default_screen(display);
    (void)widget;

    if (app_css_provider) {
        gtk_style_context_remove_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(app_css_provider));
        g_object_unref(app_css_provider);
        app_css_provider = NULL;
    }

    state->theme_is_dark = !state->theme_is_dark;

    const char *css_file = state->theme_is_dark ? "theme.css" : "theme-simple.css";
    char css_path[1024];
    snprintf(css_path, sizeof(css_path), "%s/css/%s", app_exe_dir, css_file);
    FILE *f = fopen(css_path, "r");
    if (f) {
        fclose(f);
    } else {
        snprintf(css_path, sizeof(css_path), "css/%s", css_file);
    }

    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;
    if (gtk_css_provider_load_from_path(provider, css_path, &error)) {
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);
        app_css_provider = provider;
    } else {
        if (error) { g_error_free(error); }
        g_object_unref(provider);
    }

    if (state->theme_btn) {
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(state->theme_btn),
                                  state->theme_is_dark ? "Light" : "Dark");
    }

    save_theme_pref(state->theme_is_dark);
}
