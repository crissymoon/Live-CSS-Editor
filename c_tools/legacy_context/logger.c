#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

/* Logger creation and destruction */
Logger* logger_create(const char *log_path) {
    if (!log_path) return NULL;

    Logger *logger = (Logger*)malloc(sizeof(Logger));
    if (!logger) return NULL;

    logger->log_path = strdup(log_path);
    logger->file = fopen(log_path, "a");

    if (!logger->file) {
        free(logger->log_path);
        free(logger);
        return NULL;
    }

    logger->min_level = LOG_LEVEL_DEBUG;
    logger->console_output = true;
    logger->timestamp_enabled = true;
    logger->entry_count = 0;

    return logger;
}

void logger_destroy(Logger *logger) {
    if (!logger) return;

    if (logger->file) {
        fflush(logger->file);
        fclose(logger->file);
    }

    if (logger->log_path) free(logger->log_path);
    free(logger);
}

/* Configuration */
void logger_set_level(Logger *logger, LogLevel level) {
    if (logger) logger->min_level = level;
}

void logger_enable_console(Logger *logger, bool enabled) {
    if (logger) logger->console_output = enabled;
}

void logger_enable_timestamp(Logger *logger, bool enabled) {
    if (logger) logger->timestamp_enabled = enabled;
}

/* Utilities */
const char* logger_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

char* logger_get_timestamp(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char *timestamp = (char*)malloc(32);
    if (!timestamp) return NULL;

    strftime(timestamp, 32, "%Y-%m-%d %H:%M:%S", t);
    return timestamp;
}

/* Main logging function */
void logger_log(Logger *logger, LogLevel level, const char *module, const char *format, ...) {
    if (!logger || !format) return;
    if (level < logger->min_level) return;

    char message[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    char *timestamp = NULL;
    if (logger->timestamp_enabled) {
        timestamp = logger_get_timestamp();
    }

    const char *level_str = logger_level_to_string(level);

    /* Log to file */
    if (logger->file) {
        if (timestamp) {
            fprintf(logger->file, "[%s] [%s] [%s] %s\n",
                   timestamp, level_str, module ? module : "SYSTEM", message);
        } else {
            fprintf(logger->file, "[%s] [%s] %s\n",
                   level_str, module ? module : "SYSTEM", message);
        }
        fflush(logger->file);
    }

    /* Log to console */
    if (logger->console_output) {
        if (timestamp) {
            printf("[%s] [%s] [%s] %s\n",
                   timestamp, level_str, module ? module : "SYSTEM", message);
        } else {
            printf("[%s] [%s] %s\n",
                   level_str, module ? module : "SYSTEM", message);
        }
    }

    logger->entry_count++;

    if (timestamp) free(timestamp);
}

/* Session management */
void logger_start_session(Logger *logger, const char *session_name) {
    if (!logger) return;

    logger_log(logger, LOG_LEVEL_INFO, "SESSION",
              "========== SESSION START: %s ==========",
              session_name ? session_name : "Unnamed");
}

void logger_end_session(Logger *logger) {
    if (!logger) return;

    logger_log(logger, LOG_LEVEL_INFO, "SESSION",
              "========== SESSION END (entries: %lu) ==========",
              logger->entry_count);
}

/* Performance tracking */
PerformanceTimer* logger_start_timer(const char *operation) {
    PerformanceTimer *timer = (PerformanceTimer*)malloc(sizeof(PerformanceTimer));
    if (!timer) return NULL;

    timer->start_time = clock();
    timer->operation_name = operation ? strdup(operation) : NULL;

    return timer;
}

void logger_end_timer(Logger *logger, PerformanceTimer *timer) {
    if (!logger || !timer) return;

    clock_t end_time = clock();
    double elapsed = ((double)(end_time - timer->start_time)) / CLOCKS_PER_SEC * 1000.0;

    logger_log(logger, LOG_LEVEL_DEBUG, "PERFORMANCE",
              "%s completed in %.2f ms",
              timer->operation_name ? timer->operation_name : "Operation",
              elapsed);

    if (timer->operation_name) free(timer->operation_name);
    free(timer);
}

/* Statistics */
void logger_print_stats(Logger *logger) {
    if (!logger) return;

    logger_log(logger, LOG_LEVEL_INFO, "STATS",
              "Total log entries: %lu", logger->entry_count);

    /* Get file size */
    if (logger->log_path) {
        struct stat st;
        if (stat(logger->log_path, &st) == 0) {
            logger_log(logger, LOG_LEVEL_INFO, "STATS",
                      "Log file size: %lld bytes", (long long)st.st_size);
        }
    }
}

void logger_flush(Logger *logger) {
    if (logger && logger->file) {
        fflush(logger->file);
    }
}

void logger_rotate(Logger *logger) {
    if (!logger || !logger->log_path) return;

    /* Close current file */
    if (logger->file) {
        fclose(logger->file);
    }

    /* Rename to .old */
    char old_path[1024];
    snprintf(old_path, sizeof(old_path), "%s.old", logger->log_path);
    rename(logger->log_path, old_path);

    /* Open new file */
    logger->file = fopen(logger->log_path, "a");
    logger->entry_count = 0;

    logger_log(logger, LOG_LEVEL_INFO, "LOGGER", "Log rotated");
}
