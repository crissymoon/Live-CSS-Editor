#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <stdbool.h>

void update_status(const char *message);
void show_error_dialog(const char *title, const char *message);
void show_info_dialog(const char *title, const char *message);
bool confirm_action(const char *message);

#endif
