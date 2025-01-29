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

Ju_Error 
Ju_str_append(StringBuilder* builder, ...);

Ju_Error 
Ju_str_append_fmt(StringBuilder* builder, const char* fmt, ...);

void
Ju_str_free(StringBuilder *builder);

#endif // JUTILS_H

#ifndef JUTILS_DEFINITIONS
#ifdef JUTILS_IMPLEMENTATION
#define JUTILS_DEFINITIONS

#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define Ju_str_append_null(builder, ...) Ju_str_append(builder, __VA_ARGS__, NULL)
#define Ju_str_append_fmt_null(builder, ...) Ju_str_append_fmt(builder, __VA_ARGS__, NULL)

Ju_Error 
Ju_str_append(StringBuilder* builder, ...) 
{
    if (builder == NULL) {
        return JU_ERR_NULL_PARAM;
    }
    va_list args;
    va_start(args, builder);

    const char* arg = va_arg(args, const char*);
    while (arg != NULL) 
    {
        size_t size = strlen(arg);
        if (size > 0) {
            if (builder->count + size + 1 > builder->capacity)
            {
                size_t new_capacity = builder->count + size + 1;
                builder->string = realloc(builder->string, new_capacity);
                if (builder->string == NULL) return JU_ERR_MEMORY_ALLOCATION;
                builder->capacity = new_capacity;
            }
            strncpy(builder->string + builder->count, arg, size);
            builder->count += size;
            builder->string[builder->count] = '\0';
        }
        arg = va_arg(args, const char*);
    }

    va_end(args);
    return JU_OK;
}

Ju_Error 
Ju_str_append_fmt(StringBuilder* builder, const char* fmt, ...)
{
    if (builder == NULL) {
        return JU_ERR_NULL_PARAM;
    }
    int n;
    size_t size = 0;
    va_list args;
    va_start(args, fmt);
    n = vsnprintf(NULL, size, fmt, args);
    va_end(args);
    
    if (n < 0)
        return JU_ERR_APPEND_FSTRING;

    size = (size_t) n + 1;
    if (builder->capacity < builder->count + size) {
        size_t new_capacity = builder->count + size;
        char* tmp = realloc(builder->string, new_capacity);
        if (tmp == NULL) {
            return JU_ERR_MEMORY_ALLOCATION;
        }
        builder->string = tmp;
        builder->capacity = new_capacity;
    }

    va_start(args, fmt);
    vsnprintf(builder->string + builder->count, size, fmt, args);
    va_end(args);

    builder->count += size - 1;

    if (n < 0) {
        return JU_ERR_MEMORY_ALLOCATION;
    }

    return JU_OK;
}

void
Ju_str_free(StringBuilder *builder)
{
    if (builder->string != NULL) 
    {
        free(builder->string);
        builder->string = NULL;
    }
}

char*
Ju_toLower(const char* s)
{
    char* dupe = strdup(s);
    if (dupe == NULL) return NULL;
    for (char* p = dupe; *p; ++p) {
        *p = tolower(*p);
    }
    return dupe;
}

char*
Ju_toUpper(const char* s)
{
    char* dupe = strdup(s);
    if (dupe == NULL) return NULL;
    for (char* p = dupe; *p; ++p) {
        *p = toupper(*p);
    }
    return dupe;
}

#endif // JUTILS_IMPLEMENTATION
#endif // JUTILS_DEFINITIONS
