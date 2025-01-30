#include "logger.h"
#include "stdarg.h"

Log_Error
Logger_init(Logger* logger, FILE* out, Log_Level level)
{
    if (logger == NULL) return LOGGER_ERR_NULL_PARAM;
    logger->out = out;
    logger->level = level;
    return LOGGER_OK;
}

Log_Error
Logger_init_from_file(Logger* logger, const char* filepath, Log_Level level)
{
    if (filepath == NULL) return LOGGER_ERR_NULL_PARAM;
    FILE* file = fopen(filepath, "a");
    if (file == NULL) return LOGGER_ERR_NULL_FILE;

    return Logger_init(logger, file, level);
}

const char*
Logger_strlevel(Log_Level level)
{
    switch (level) {
        case LOG_INFO:
            return "INFO";
        case LOG_WARN:
            return "WARN";
        case LOG_ERROR:
            return "ERROR";
        case LOG_FATAL:
            return "FATAL";
        case LOG_DEBUG:
            return "DEBUG";
        default:
            return NULL;
    }
}

Log_Error
Logger_log(Logger* logger, const char* message)
{
    if (logger->out == NULL) return LOGGER_ERR_NULL_FILE;
    if (message == NULL) return LOGGER_ERR_NULL_MSG;
    fprintf(logger->out, "[%s] %s\n", Logger_strlevel(logger->level), message);
}

Log_Error
Logger_log_fmt(Logger* logger, const char* fmt, ...)
{
    if (logger->out == NULL) return LOGGER_ERR_NULL_FILE;
    if (fmt == NULL) return LOGGER_ERR_NULL_MSG;
    va_list args;
    va_start(args, fmt);
    
    vfprintf(logger->out, fmt, args);

    va_end(args);
}