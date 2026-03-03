#ifndef RECENT_MANAGER_H
#define RECENT_MANAGER_H

#include "app_state.h"

char *get_recent_config_path(void);
void populate_recent_combo(AppState *state);
void load_recent_dbs(AppState *state);
void save_recent_dbs(AppState *state);
void add_to_recent(AppState *state, const char *path);

#endif
