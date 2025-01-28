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

#ifdef BASE64_IMPLEMENTATION

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

void
Base64_encode_block(const unsigned char input[3], char output[4], int len)
{
    output[0] = base64_chars[input[0] >> 2];
    output[1] = base64_chars[((input[0] & 0x03) << 4) | (input[1] >> 4)];
    output[2] = (len > 1) ? base64_chars[((input[1] & 0x0F) << 2) | (input[2] >> 6)] : '=';
    output[3] = (len > 2) ? base64_chars[input[2] & 0x3F] : '=';
}

void
Base64Url_encode_block(const unsigned char input[3], char output[4], int len)
{
    output[0] = base64url_chars[input[0] >> 2];
    output[1] = base64url_chars[((input[0] & 0x03) << 4) | (input[1] >> 4)];
    output[2] = (len > 1) ? base64url_chars[((input[1] & 0x0F) << 2) | (input[2] >> 6)] : '\0';
    output[3] = (len > 2) ? base64url_chars[input[2] & 0x3F] : '\0';
}

int
Base64_char_to_value(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int
Base64Url_char_to_value(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

Base64_Error
Base64_is_valid(const char* str)
{
    size_t len = strlen(str);
    if (len == 0 || len % 4 != 0) {
        return BASE64_INVALID;
    }

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '=') {
            if (i < len - 2 || (i == len - 2 && str[len - 1] != '=')) {
                return BASE64_INVALID;
            }
        } else if (!isalnum(str[i]) && str[i] != '+' && str[i] != '/') {
            return BASE64_INVALID;
        }
    }
    return BASE64_VALID;
}

Base64_Error
Base64Url_is_valid(const char* str)
{
    size_t len = strlen(str);

    if (len == 0 || len % 4 != 0) {
        return BASE64_INVALID;
    }

    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        
        if (!isalnum(c) && c != '-' && c != '_') {
            return BASE64_INVALID;
        }
    }

    return BASE64_VALID;
}

Base64_Error
Base64_encode(const unsigned char* data, size_t input_length, char** output)
{
    size_t output_length = 4 * ((input_length + 2) / 3);
    *output = (char*)malloc(output_length + 1);
    if (*output == NULL) {
        return BASE64_ERR_MEMORY_ALLOCATION;
    }

    size_t i = 0, j = 0;
    while (i < input_length) {
        unsigned char input[3] = {0, 0, 0};
        int len = 0;

        for (len = 0; len < 3 && i < input_length; len++, i++) {
            input[len] = data[i];
        }

        char output_block[4];
        Base64_encode_block(input, output_block, len);

        for (int k = 0; k < 4; k++) {
            (*output)[j++] = output_block[k];
        }
    }

    (*output)[output_length] = '\0';
    return BASE64_OK;
}

Base64_Error
Base64_decode(const char* encoded, size_t* output_length, unsigned char** output)
{
    size_t input_length = strlen(encoded);

    if (Base64_is_valid(encoded) == BASE64_INVALID) {
        *output_length = 0;
        return BASE64_INVALID;
    }

    size_t padding = 0;
    if (encoded[input_length - 1] == '=') padding++;
    if (encoded[input_length - 2] == '=') padding++;

    *output_length = (input_length / 4) * 3 - padding;

    *output = (unsigned char*)malloc(*output_length);
    if (*output == NULL) {
        return BASE64_ERR_MEMORY_ALLOCATION;
    }

    size_t i = 0, j = 0;
    while (i < input_length) {
        int values[4];
        for (int k = 0; k < 4; k++, i++) {
            values[k] = (encoded[i] == '=') ? 0 : Base64_char_to_value(encoded[i]);
        }

        (*output)[j++] = (values[0] << 2) | (values[1] >> 4);
        if (j < *output_length) (*output)[j++] = (values[1] << 4) | (values[2] >> 2);
        if (j < *output_length) (*output)[j++] = (values[2] << 6) | values[3];
    }

    return BASE64_OK;
}

Base64_Error
Base64Url_encode(const unsigned char* data, size_t input_length, char** output)
{
    size_t output_length = 4 * ((input_length + 2) / 3);
    *output = (char*)malloc(output_length + 1);
    if (*output == NULL) {
        return -1;
    }

    size_t i = 0, j = 0;
    while (i < input_length) {
        unsigned char input[3] = {0, 0, 0};
        int len = 0;

        for (len = 0; len < 3 && i < input_length; len++, i++) {
            input[len] = data[i];
        }

        char output_block[4];
        Base64Url_encode_block(input, output_block, len);

        for (int k = 0; k < 4; k++) {
            (*output)[j++] = output_block[k];
        }
    }

    (*output)[output_length] = '\0';
    return 0;
}

Base64_Error
Base64Url_decode(const char* encoded, size_t* output_length, unsigned char** output)
{
    size_t input_length = strlen(encoded);
    *output = (unsigned char*)malloc(input_length * 3 / 4);
    if (*output == NULL) {
        return -1;
    }

    size_t i = 0, j = 0;
    int values[4];
    while (i < input_length) {
        for (int k = 0; k < 4; k++, i++) {
            values[k] = (encoded[i] == '\0') ? 0 : Base64Url_char_to_value(encoded[i]);
        }

        (*output)[j++] = (values[0] << 2) | (values[1] >> 4);
        if (j < *output_length) (*output)[j++] = (values[1] << 4) | (values[2] >> 2);
        if (j < *output_length) (*output)[j++] = (values[2] << 6) | values[3];
    }

    *output_length = j;
    return 0;
}

#endif // BASE64_IMPLEMENTATION