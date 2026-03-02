#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

/* Log levels */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_CRITICAL = 4
} LogLevel;

/* Logger structure */
typedef struct Logger {
    FILE *file;
    char *log_path;
    LogLevel min_level;
    bool console_output;
    bool timestamp_enabled;
    unsigned long entry_count;
} Logger;

/* Logger API */
Logger* logger_create(const char *log_path);
void logger_destroy(Logger *logger);

void logger_set_level(Logger *logger, LogLevel level);
void logger_enable_console(Logger *logger, bool enabled);
void logger_enable_timestamp(Logger *logger, bool enabled);

void logger_log(Logger *logger, LogLevel level, const char *module, const char *format, ...);

/* Convenience macros */
#define LOG_DEBUG(logger, module, ...) logger_log(logger, LOG_LEVEL_DEBUG, module, __VA_ARGS__)
#define LOG_INFO(logger, module, ...) logger_log(logger, LOG_LEVEL_INFO, module, __VA_ARGS__)
#define LOG_WARNING(logger, module, ...) logger_log(logger, LOG_LEVEL_WARNING, module, __VA_ARGS__)
#define LOG_ERROR(logger, module, ...) logger_log(logger, LOG_LEVEL_ERROR, module, __VA_ARGS__)
#define LOG_CRITICAL(logger, module, ...) logger_log(logger, LOG_LEVEL_CRITICAL, module, __VA_ARGS__)

/* Session management */
void logger_start_session(Logger *logger, const char *session_name);
void logger_end_session(Logger *logger);

/* Performance tracking */
typedef struct {
    clock_t start_time;
    char *operation_name;
} PerformanceTimer;

PerformanceTimer* logger_start_timer(const char *operation);
void logger_end_timer(Logger *logger, PerformanceTimer *timer);

/* Statistics */
void logger_print_stats(Logger *logger);
void logger_flush(Logger *logger);
void logger_rotate(Logger *logger);

/* Helper functions */
const char* logger_level_to_string(LogLevel level);
char* logger_get_timestamp(void);

#endif /* LOGGER_H */
