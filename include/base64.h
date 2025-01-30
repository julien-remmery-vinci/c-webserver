#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

typedef enum {
    BASE64_OK,
    BASE64_VALID,
    BASE64_INVALID,
    BASE64_ERR_MEMORY_ALLOCATION,
} Base64_Error;

void
Base64_encode_block(const unsigned char input[3], char output[4], int len);

int
Base64_char_to_value(char c);

/**
 * Returns BASE64_VALID if input is valid
 *  BASE64_INVALID otherwise
 */
Base64_Error
Base64_is_valid(const char* str);

Base64_Error
Base64_encode(const unsigned char* data, size_t input_length, char** output);

Base64_Error
Base64_decode(const char* encoded, size_t* output_length, unsigned char** output);

Base64_Error
Base64Url_encode(const unsigned char* data, size_t input_length, char** output);

Base64_Error
Base64Url_decode(const char* encoded, size_t* output_length, unsigned char** output);

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64url_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

#endif // BASE64_H