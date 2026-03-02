#include "plugin.h"
#include "merger.h"
#include <string.h>
#include <stdio.h>

/* Plugin initialization */
EditorPlugin* plugin_initialize(const char *db_path, const char *editor_name) {
    if (!db_path || !editor_name) return NULL;

    EditorPlugin *plugin = (EditorPlugin*)malloc(sizeof(EditorPlugin));
    if (!plugin) return NULL;

    plugin->api_version = PLUGIN_API_VERSION;
    plugin->editor_name = strdup(editor_name);
    plugin->plugin_version = strdup(LINTER_VERSION);
    plugin->linter = linter_create(db_path);
    plugin->editor_context = NULL;

    if (!plugin->linter) {
        free(plugin->editor_name);
        free(plugin->plugin_version);
        free(plugin);
        return NULL;
    }

    return plugin;
}

void plugin_shutdown(EditorPlugin *plugin) {
    if (!plugin) return;

    if (plugin->editor_name) free(plugin->editor_name);
    if (plugin->plugin_version) free(plugin->plugin_version);
    if (plugin->linter) linter_destroy(plugin->linter);

    free(plugin);
}

/* Linting operations */
int plugin_lint_file(EditorPlugin *plugin, const char *file_path) {
    if (!plugin || !plugin->linter || !file_path) return -1;

    return linter_lint_file(plugin->linter, file_path);
}

int plugin_lint_buffer(EditorPlugin *plugin, const char *content, const char *language) {
    if (!plugin || !plugin->linter || !content) return -1;

    return linter_lint_string(plugin->linter, content, language);
}

void plugin_clear_issues(EditorPlugin *plugin, const char *file_path) {
    if (!plugin || !plugin->linter) return;

    if (plugin->linter->issues) {
        linter_free_issues(plugin->linter->issues);
        plugin->linter->issues = NULL;
        plugin->linter->issue_count = 0;
    }

    /* Clear tracking in database */
    if (plugin->linter->db_handle && file_path) {
        LinterDB *db = (LinterDB*)plugin->linter->db_handle;
        db_clear_tracking(db, file_path);
    }
}

/* Editor callbacks */
static EditorCallbacks *g_callbacks = NULL;
static void *g_editor_context = NULL;

void plugin_register_callbacks(EditorPlugin *plugin, EditorCallbacks *callbacks, void *context) {
    if (!plugin || !callbacks) return;

    g_callbacks = callbacks;
    g_editor_context = context;
    plugin->editor_context = context;
}

/* Pattern management */
int plugin_add_pattern(EditorPlugin *plugin, const char *name, const char *description,
                       const char *pattern, const char *language, int severity) {
    if (!plugin || !plugin->linter || !plugin->linter->db_handle) return -1;

    LinterDB *db = (LinterDB*)plugin->linter->db_handle;
    return db_add_pattern(db, name, description, pattern, language, severity);
}

int plugin_enable_pattern(EditorPlugin *plugin, const char *pattern_name, bool enabled) {
    if (!plugin || !plugin->linter || !plugin->linter->db_handle || !pattern_name) return -1;

    /* This is simplified - in production, look up pattern ID by name */
    return 0;
}

char** plugin_list_patterns(EditorPlugin *plugin, const char *language, int *count) {
    if (!plugin || !plugin->linter || !plugin->linter->db_handle || !count) return NULL;

    LinterDB *db = (LinterDB*)plugin->linter->db_handle;

    int pattern_count = 0;
    Pattern *patterns = db_get_patterns(db, language, &pattern_count);

    if (!patterns || pattern_count == 0) {
        *count = 0;
        db_free_patterns(patterns);
        return NULL;
    }

    char **names = (char**)malloc(pattern_count * sizeof(char*));
    if (!names) {
        db_free_patterns(patterns);
        return NULL;
    }

    Pattern *current = patterns;
    int i = 0;

    while (current && i < pattern_count) {
        names[i++] = strdup(current->name);
        current = current->next;
    }

    db_free_patterns(patterns);
    *count = pattern_count;

    return names;
}

/* Merge operations */
int plugin_propose_merge(EditorPlugin *plugin, const char *source_file,
                        const char *target_file, const char *changes) {
    if (!plugin || !plugin->linter || !plugin->linter->db_handle) return -1;

    LinterDB *db = (LinterDB*)plugin->linter->db_handle;
    MergerContext *merger = merger_create(db);

    if (!merger) return -1;

    /* Parse changes and propose - simplified version */
    int result = merger_propose_change(merger, source_file, target_file, 1, "", changes);

    merger_destroy(merger);
    return result;
}

int plugin_apply_merge(EditorPlugin *plugin, const char *file_path) {
    if (!plugin || !plugin->linter || !plugin->linter->db_handle || !file_path) return -1;

    LinterDB *db = (LinterDB*)plugin->linter->db_handle;
    MergerContext *merger = merger_create(db);

    if (!merger) return -1;

    MergeResult *result = merger_apply_changes(merger, file_path);
    int applied = result ? result->applied_changes : 0;

    if (result) merger_free_result(result);
    merger_destroy(merger);

    return applied;
}

char* plugin_preview_merge(EditorPlugin *plugin, const char *file_path) {
    if (!plugin || !plugin->linter || !plugin->linter->db_handle || !file_path) return NULL;

    LinterDB *db = (LinterDB*)plugin->linter->db_handle;
    MergerContext *merger = merger_create(db);

    if (!merger) return NULL;

    char *preview = merger_preview_changes(merger, file_path);

    merger_destroy(merger);
    return preview;
}

/* Configuration */
static char config_store[256][256];
static int config_count = 0;

int plugin_set_config(EditorPlugin *plugin, const char *key, const char *value) {
    if (!plugin || !key || !value) return -1;

    /* Simple key-value store */
    for (int i = 0; i < config_count; i++) {
        if (strncmp(config_store[i], key, strlen(key)) == 0) {
            snprintf(config_store[i], 256, "%s=%s", key, value);
            return 0;
        }
    }

    if (config_count < 256) {
        snprintf(config_store[config_count++], 256, "%s=%s", key, value);
        return 0;
    }

    return -1;
}

const char* plugin_get_config(EditorPlugin *plugin, const char *key) {
    if (!plugin || !key) return NULL;

    size_t key_len = strlen(key);

    for (int i = 0; i < config_count; i++) {
        if (strncmp(config_store[i], key, key_len) == 0 && config_store[i][key_len] == '=') {
            return config_store[i] + key_len + 1;
        }
    }

    return NULL;
}

/* Status */
PluginStatus* plugin_get_status(EditorPlugin *plugin) {
    if (!plugin) return NULL;

    PluginStatus *status = (PluginStatus*)malloc(sizeof(PluginStatus));
    if (!status) return NULL;

    status->is_active = (plugin->linter != NULL);
    status->issues_count = plugin->linter ? plugin->linter->issue_count : 0;
    status->patterns_count = 0;
    status->last_error = NULL;

    if (plugin->linter && plugin->linter->db_handle) {
        LinterDB *db = (LinterDB*)plugin->linter->db_handle;
        Pattern *patterns = db_get_patterns(db, NULL, &status->patterns_count);
        db_free_patterns(patterns);
    }

    return status;
}

void plugin_free_status(PluginStatus *status) {
    if (!status) return;

    if (status->last_error) free(status->last_error);
    free(status);
}

/* JSON API */
char* plugin_get_issues_json(EditorPlugin *plugin) {
    if (!plugin || !plugin->linter) return NULL;

    int count = 0;
    LinterIssue *issues = linter_get_issues(plugin->linter, &count);

    if (!issues || count == 0) {
        return strdup("{\"issues\":[]}");
    }

    /* Allocate buffer for JSON */
    char *json = (char*)malloc(4096 * count);
    if (!json) return NULL;

    strcpy(json, "{\"issues\":[");

    LinterIssue *current = issues;
    bool first = true;

    while (current) {
        if (!first) strcat(json, ",");
        first = false;

        char issue_json[4096];
        snprintf(issue_json, sizeof(issue_json),
                "{\"file\":\"%s\",\"line\":%d,\"column\":%d,\"rule\":\"%s\",\"message\":\"%s\",\"severity\":%d}",
                current->file_path ? current->file_path : "",
                current->line_number,
                current->column_number,
                current->rule_name ? current->rule_name : "",
                current->message ? current->message : "",
                current->severity);

        strcat(json, issue_json);
        current = current->next;
    }

    strcat(json, "]}");
    return json;
}

char* plugin_get_patterns_json(EditorPlugin *plugin, const char *language) {
    if (!plugin || !plugin->linter || !plugin->linter->db_handle) return NULL;

    LinterDB *db = (LinterDB*)plugin->linter->db_handle;

    int count = 0;
    Pattern *patterns = db_get_patterns(db, language, &count);

    if (!patterns || count == 0) {
        db_free_patterns(patterns);
        return strdup("{\"patterns\":[]}");
    }

    /* Allocate buffer for JSON */
    char *json = (char*)malloc(4096 * count);
    if (!json) {
        db_free_patterns(patterns);
        return NULL;
    }

    strcpy(json, "{\"patterns\":[");

    Pattern *current = patterns;
    bool first = true;

    while (current) {
        if (!first) strcat(json, ",");
        first = false;

        char pattern_json[4096];
        snprintf(pattern_json, sizeof(pattern_json),
                "{\"id\":%d,\"name\":\"%s\",\"description\":\"%s\",\"pattern\":\"%s\",\"language\":\"%s\",\"severity\":%d,\"enabled\":%s}",
                current->id,
                current->name ? current->name : "",
                current->description ? current->description : "",
                current->regex_pattern ? current->regex_pattern : "",
                current->language ? current->language : "",
                current->severity,
                current->enabled ? "true" : "false");

        strcat(json, pattern_json);
        current = current->next;
    }

    strcat(json, "]}");
    db_free_patterns(patterns);

    return json;
}
