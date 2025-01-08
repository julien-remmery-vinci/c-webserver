#ifndef JUTILS_H
#define JUTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#define LOG(lvl, fmt, ...) printf("[" lvl "] " fmt "\n", ##__VA_ARGS__)
#define INFO(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...) LOG("DEBUG", fmt, ##__VA_ARGS__)
#define CHECK(cond) do { if (!(cond)) { LOG("ERROR", "%s", strerror(errno)); exit(EXIT_FAILURE); } } while (0)

typedef struct StringBuilder StringBuilder;

struct StringBuilder {
    char* items;
    size_t count;
    size_t capacity;
};

void 
Ju_str_append(StringBuilder* builder, ...);

void
Ju_str_free(StringBuilder *builder);

#endif // JUTILS_H

#ifdef JUTILS_IMPLEMENTATION

#include <stdbool.h>
#include <string.h>

void 
Ju_str_append(StringBuilder* builder, ...) 
{
    va_list args;
    va_start(args, builder);

    const char* arg = va_arg(args, const char*);
    while (arg != NULL) 
    {
        size_t size = strlen(arg);
        if (builder->count + size > builder->capacity)
        {
            builder->items = realloc(builder->items, size+1 * sizeof(char));
            CHECK(builder->items != NULL);
            builder->capacity += size+1;
        }
        strncpy(builder->items + builder->count, arg, size);
        builder->count += size;
        arg = va_arg(args, const char*);
    }

    va_end(args);
}

void
Ju_str_free(StringBuilder *builder)
{
    free(builder->items);
}

#endif // JUTILS_IMPLEMENTATION