#include "jutils.h"

#include <stdbool.h>
#include <string.h>
#include <ctype.h>

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
Ju_builder_free(StringBuilder *builder)
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