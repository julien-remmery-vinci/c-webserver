#ifndef HMAC_H
#define HMAC_H

#include <stdint.h>
#include <stddef.h>

char*
hmac(char* message, const char* key, 
    int (*hash)(uint8_t *output, const uint8_t *input, size_t input_len), 
    size_t blocksize, size_t output_size);

#endif // HMAC_H