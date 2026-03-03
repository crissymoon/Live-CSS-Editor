#include "csv_handler.h"
#include "ui_utils.h"
#include <string.h>

void csv_write_field(FILE *f, const char *val) {
    if (!val) return;
    bool need_quote = (strchr(val, ',') || strchr(val, '"') ||
                       strchr(val, '\n') || strchr(val, '\r'));
    if (need_quote) {
        fputc('"', f);
        for (const char *p = val; *p; p++) {
            if (*p == '"') fputc('"', f);
            fputc(*p, f);
        }
        fputc('"', f);
    } else {
        fputs(val, f);
    }
}

char **csv_parse_line(const char *line, int *col_count) {
    GPtrArray *fields = g_ptr_array_new();
    const char *p = line;
    while (1) {
        GString *field = g_string_new("");
        if (*p == '"') {
            p++;
            while (*p) {
                if (*p == '"') {
                    p++;
                    if (*p == '"') { g_string_append_c(field, '"'); p++; }
                    else break;
                } else {
                    g_string_append_c(field, *p++);
                }
            }
        } else {
            while (*p && *p != ',' && *p != '\n' && *p != '\r')
                g_string_append_c(field, *p++);
        }
        g_ptr_array_add(fields, g_string_free(field, FALSE));
        if (*p == ',') { p++; continue; }
        break;
    }
    *col_count = (int)fields->len;
    g_ptr_array_add(fields, NULL);
    return (char **)g_ptr_array_free(fields, FALSE);
}

void on_export_csv(GtkWidget *widget, gpointer data) {
    show_info_dialog("Export CSV", "CSV export functionality coming soon!");
}

void on_import_csv(GtkWidget *widget, gpointer data) {
    show_info_dialog("Import CSV", "CSV import functionality coming soon!");
}
