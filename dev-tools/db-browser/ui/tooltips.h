#ifndef TOOLTIPS_H
#define TOOLTIPS_H

#include <gtk/gtk.h>
#include <stdbool.h>

/* Tooltip categories for students */
typedef enum {
    TOOLTIP_BASIC,          // Basic concepts
    TOOLTIP_INTERMEDIATE,   // Intermediate concepts
    TOOLTIP_ADVANCED,       // Advanced concepts
    TOOLTIP_WARNING,        // Warnings and cautions
    TOOLTIP_TIP,           // Helpful tips
    TOOLTIP_EXAMPLE        // Code examples
} TooltipLevel;

/* Tooltip entry */
typedef struct {
    const char *element_id;
    const char *title;
    const char *description;
    const char *example;
    TooltipLevel level;
} TooltipEntry;

/* Tooltip manager */
typedef struct TooltipManager {
    GHashTable *tooltips;
    bool tutorial_mode;
    int current_step;
} TooltipManager;

/* Tooltip Manager API */
TooltipManager* tooltip_manager_create();
void tooltip_manager_destroy(TooltipManager *mgr);

/* Register tooltips */
void tooltip_manager_register_all(TooltipManager *mgr);
void tooltip_manager_add_tooltip(TooltipManager *mgr, const char *element_id,
                                const char *title, const char *description,
                                const char *example, TooltipLevel level);

/* Apply tooltips to widgets */
void tooltip_manager_apply_to_widget(TooltipManager *mgr, GtkWidget *widget,
                                    const char *element_id);
void tooltip_manager_apply_to_all(TooltipManager *mgr, GtkBuilder *builder);

/* Tutorial mode */
void tooltip_manager_enable_tutorial(TooltipManager *mgr);
void tooltip_manager_disable_tutorial(TooltipManager *mgr);
bool tooltip_manager_is_tutorial_active(TooltipManager *mgr);

/* Show custom tooltip */
void tooltip_manager_show_custom(TooltipManager *mgr, GtkWidget *parent,
                                const char *title, const char *message,
                                TooltipLevel level);

/* Predefined tooltips for common database operations */
void tooltip_register_table_operations(TooltipManager *mgr);
void tooltip_register_query_operations(TooltipManager *mgr);
void tooltip_register_data_operations(TooltipManager *mgr);
void tooltip_register_import_export(TooltipManager *mgr);
void tooltip_register_sql_help(TooltipManager *mgr);

#endif /* TOOLTIPS_H */
