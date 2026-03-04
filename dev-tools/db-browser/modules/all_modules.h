/* all_modules.h - Aggregate include for all db-browser modules.
 * Include this in main.c instead of each module header individually.
 * Individual .c files should still include only the headers they need.
 */

#ifndef ALL_MODULES_H
#define ALL_MODULES_H

#include "app_state.h"
#include "ui_drawing.h"
#include "ui_utils.h"
#include "theme_manager.h"
#include "recent_manager.h"
#include "ui_panels.h"
#include "db_callbacks.h"
#include "query_callbacks.h"
#include "table_callbacks.h"
#include "csv_handler.h"
#include "misc_callbacks.h"
#include "auth.h"

#endif /* ALL_MODULES_H */
