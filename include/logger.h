#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/**
 * Error code enum, not to be mistaken with log level
 */
typedef enum {
    LOGGER_OK,
    LOGGER_ERR_NULL_FILE,
    LOGGER_ERR_NULL_PARAM,
    LOGGER_ERR_NULL_MSG,
} Log_Error;

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    LOG_DEBUG,
} Log_Level;

typedef struct Logger {
    FILE* out;
    Log_Level level;
} Logger;

Log_Error
Logger_init(Logger* logger, FILE* out, Log_Level level);

Log_Error
Logger_init_from_file(Logger* logger, const char* filepath, Log_Level level);

const char*
Logger_strlevel(Log_Level level);

Log_Error
Logger_log(Logger* logger, const char* message);

#define Logger_log_fmt_null(logger, fmt, ...) Logger_log_fmt(logger, fmt, __VA_ARGS__)

Log_Error
Logger_log_fmt(Logger* logger, const char* fmt, ...);

#endif // LOGGER_H