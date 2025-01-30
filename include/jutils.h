#ifndef JUTILS_H
#define JUTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

typedef enum {
    JU_OK,
    JU_ERR_NULL_PARAM,
    JU_ERR_MEMORY_ALLOCATION,
    JU_ERR_APPEND_FSTRING,
} Ju_Error;

#define LOG(lvl, fmt, ...) printf("[" lvl "] " fmt "\n", ##__VA_ARGS__)
#define INFO(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...) LOG("DEBUG", fmt, ##__VA_ARGS__)
#define CHECK(cond, msg) do { if (!(cond)) { LOG("ERROR", "%s", msg); exit(EXIT_FAILURE); } } while (0)

typedef struct StringBuilder StringBuilder;

struct StringBuilder {
    char* string;
    size_t count;
    size_t capacity;
};

#define Ju_str_append_null(builder, ...) Ju_str_append(builder, __VA_ARGS__, NULL)

Ju_Error 
Ju_str_append(StringBuilder* builder, ...);

#define Ju_str_append_fmt_null(builder, ...) Ju_str_append_fmt(builder, __VA_ARGS__, NULL)

Ju_Error 
Ju_str_append_fmt(StringBuilder* builder, const char* fmt, ...);

void
Ju_builder_free(StringBuilder *builder);

char*
Ju_toLower(const char* s);

char*
Ju_toUpper(const char* s);

#endif // JUTILS_H