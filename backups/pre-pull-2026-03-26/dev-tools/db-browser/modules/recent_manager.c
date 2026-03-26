#include "recent_manager.h"
#include <stdio.h>
#include <string.h>

/* Forward declaration - on_open_recent_db is in db_callbacks */
void on_open_recent_db(GtkWidget *widget, gpointer data);

char *get_recent_config_path(void) {
    const char *home = g_get_home_dir();
    if (!home) {
        fprintf(stderr, "[db-browser] get_recent_config_path: g_get_home_dir returned NULL\n");
        return g_strdup("/tmp/crissy-db-browser-recent.txt");
    }
    char *dir = g_build_filename(home, ".config", "crissy-db-browser", NULL);
    if (g_mkdir_with_parents(dir, 0755) != 0) {
        fprintf(stderr, "[db-browser] get_recent_config_path: cannot create config dir: %s\n", dir);
    }
    char *path = g_build_filename(dir, "recent.txt", NULL);
    g_free(dir);
    return path;
}

void populate_recent_combo(AppState *state) {
    if (!state || !state->recent_btn) {
        fprintf(stderr, "[db-browser] populate_recent_combo: state or recent_btn is NULL\n");
        return;
    }

    GtkWidget *popover = gtk_menu_button_get_popover(
                             GTK_MENU_BUTTON(state->recent_btn));
    if (!popover) return;

    GtkWidget *box = gtk_bin_get_child(GTK_BIN(popover));
    if (!box) return;

    GList *children = gtk_container_get_children(GTK_CONTAINER(box));
    for (GList *l = children; l; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    int n = 0;
    if (!state->recent_paths) {
        GtkWidget *lbl = gtk_label_new("(no recent databases)");
        gtk_widget_set_sensitive(lbl, FALSE);
        gtk_widget_set_margin_start(lbl, 16);
        gtk_widget_set_margin_end(lbl, 16);
        gtk_widget_set_margin_top(lbl, 6);
        gtk_widget_set_margin_bottom(lbl, 6);
        gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
    } else {
        for (GList *l = state->recent_paths; l != NULL; l = l->next) {
            GtkWidget *btn = gtk_model_button_new();
            g_object_set(btn, "text", (const char *)l->data, NULL);
            g_object_set_data_full(G_OBJECT(btn), "db-path",
                                  g_strdup((const char *)l->data), g_free);
            g_signal_connect(btn, "clicked", G_CALLBACK(on_open_recent_db), state);
            gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);
            n++;
        }
    }

    gtk_widget_show_all(box);
    printf("[db-browser] populate_recent_combo: %d entries\n", n);
}

void load_recent_dbs(AppState *state) {
    if (!state) {
        fprintf(stderr, "[db-browser] load_recent_dbs: state is NULL\n");
        return;
    }

    char *config_path = get_recent_config_path();
    FILE *f = fopen(config_path, "r");
    if (!f) {
        printf("[db-browser] load_recent_dbs: no existing recent file at %s\n", config_path);
        g_free(config_path);
        return;
    }

    char line[2048];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < 10) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;
        state->recent_paths = g_list_append(state->recent_paths, g_strdup(line));
        count++;
    }

    fclose(f);
    g_free(config_path);

    populate_recent_combo(state);
    printf("[db-browser] load_recent_dbs: loaded %d path(s)\n", count);
}

void save_recent_dbs(AppState *state) {
    if (!state || !state->recent_paths) {
        printf("[db-browser] save_recent_dbs: nothing to save\n");
        return;
    }

    char *config_path = get_recent_config_path();
    FILE *f = fopen(config_path, "w");
    if (!f) {
        fprintf(stderr, "[db-browser] save_recent_dbs: cannot open '%s' for writing\n", config_path);
        g_free(config_path);
        return;
    }

    int count = 0;
    for (GList *l = state->recent_paths; l != NULL; l = l->next) {
        fprintf(f, "%s\n", (char *)l->data);
        count++;
    }

    fclose(f);
    g_free(config_path);
    printf("[db-browser] save_recent_dbs: saved %d path(s)\n", count);
}

void add_to_recent(AppState *state, const char *path) {
    if (!state || !path || strlen(path) == 0) {
        fprintf(stderr, "[db-browser] add_to_recent: invalid arguments\n");
        return;
    }

    GList *existing = g_list_find_custom(state->recent_paths, path,
                                         (GCompareFunc)strcmp);
    if (existing) {
        g_free(existing->data);
        state->recent_paths = g_list_delete_link(state->recent_paths, existing);
    }

    state->recent_paths = g_list_prepend(state->recent_paths, g_strdup(path));

    while (g_list_length(state->recent_paths) > 10) {
        GList *last = g_list_last(state->recent_paths);
        g_free(last->data);
        state->recent_paths = g_list_delete_link(state->recent_paths, last);
    }

    populate_recent_combo(state);
    save_recent_dbs(state);
    printf("[db-browser] add_to_recent: added '%s'\n", path);
}
