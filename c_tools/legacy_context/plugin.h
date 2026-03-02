#ifndef PLUGIN_H
#define PLUGIN_H

#include "linter.h"
#include <stdbool.h>

/* Plugin version */
#define PLUGIN_API_VERSION 1

/* Editor integration structure */
typedef struct EditorPlugin {
    int api_version;
    char *editor_name;
    char *plugin_version;
    LinterContext *linter;
    void *editor_context;
} EditorPlugin;

/* Editor callbacks */
typedef struct EditorCallbacks {
    void (*on_file_open)(const char *file_path, void *context);
    void (*on_file_save)(const char *file_path, void *context);
    void (*on_file_close)(const char *file_path, void *context);
    void (*on_text_change)(const char *file_path, const char *content, void *context);
    void (*show_message)(const char *message, int severity, void *context);
    void (*highlight_issue)(const char *file_path, int line, int column, const char *message, void *context);
} EditorCallbacks;

/* Plugin API */
EditorPlugin* plugin_initialize(const char *db_path, const char *editor_name);
void plugin_shutdown(EditorPlugin *plugin);

int plugin_lint_file(EditorPlugin *plugin, const char *file_path);
int plugin_lint_buffer(EditorPlugin *plugin, const char *content, const char *language);

void plugin_register_callbacks(EditorPlugin *plugin, EditorCallbacks *callbacks, void *context);
void plugin_clear_issues(EditorPlugin *plugin, const char *file_path);

/* Configuration */
int plugin_set_config(EditorPlugin *plugin, const char *key, const char *value);
const char* plugin_get_config(EditorPlugin *plugin, const char *key);

/* Pattern management for editors */
int plugin_add_pattern(EditorPlugin *plugin, const char *name, const char *description,
                       const char *pattern, const char *language, int severity);
int plugin_enable_pattern(EditorPlugin *plugin, const char *pattern_name, bool enabled);
char** plugin_list_patterns(EditorPlugin *plugin, const char *language, int *count);

/* Merge support for editors */
int plugin_propose_merge(EditorPlugin *plugin, const char *source_file,
                        const char *target_file, const char *changes);
int plugin_apply_merge(EditorPlugin *plugin, const char *file_path);
char* plugin_preview_merge(EditorPlugin *plugin, const char *file_path);

/* Status and diagnostics */
typedef struct PluginStatus {
    bool is_active;
    int issues_count;
    int patterns_count;
    char *last_error;
} PluginStatus;

PluginStatus* plugin_get_status(EditorPlugin *plugin);
void plugin_free_status(PluginStatus *status);

/* JSON API for editor integration */
char* plugin_get_issues_json(EditorPlugin *plugin);
char* plugin_get_patterns_json(EditorPlugin *plugin, const char *language);

#endif /* PLUGIN_H */
