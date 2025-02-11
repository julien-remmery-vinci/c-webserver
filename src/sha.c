#include "sha.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t
rotru32(uint32_t value, size_t size, int shift)
{
    return (value >> shift) | (value << (size - shift));
}

uint64_t
rotru64(uint64_t value, size_t size, int shift)
{
    return (value >> shift) | (value << (size - shift));
}

void
shapad(const uint8_t *input, size_t input_len, 
    uint8_t **padded_message, size_t *padded_len, const size_t block_size)
{
    size_t bit_len = input_len * 8;
    size_t pad_len = ((bit_len + block_size + (block_size / 8)) 
        / block_size) * (block_size / 8) - input_len;
    *padded_len = input_len + pad_len;
    *padded_message = (uint8_t*)malloc(*padded_len);

    memcpy(*padded_message, input, input_len);
    (*padded_message)[input_len] = 0x80;
    memset(*padded_message + input_len + 1, 0, pad_len - 1 - 8);

    for (int i = 0; i < 8; i++) {
        (*padded_message)[*padded_len - 1 - i] 
            = (bit_len >> (8 * i)) & 0xFF;
    }
}

int
sha256(uint8_t *output, const uint8_t *input, size_t input_len)
{
    uint8_t *padded_message;
    size_t padded_len;
    shapad(input, input_len, &padded_message, &padded_len, SHA256_BLOCK_SIZE);

    uint32_t h[8];
    memcpy(h, sha_256_H, sizeof(sha_256_H));

    for (size_t i = 0; i < padded_len; i += 64) {
        uint32_t w[64] = {0};
        for (size_t j = 0; j < 16; j++) {
            w[j] = (padded_message[i + 4 * j + 0] << 24) |
                   (padded_message[i + 4 * j + 1] << 16) |
                   (padded_message[i + 4 * j + 2] << 8) |
                   (padded_message[i + 4 * j + 3]);
        }

        for (size_t j = 16; j < 64; j++) {
            uint32_t s0 = rotru32(w[j-15], SHA256_WORD_SIZE, 7) 
                ^ rotru32(w[j-15], SHA256_WORD_SIZE, 18) 
                ^ (w[j-15] >> 3);

            uint32_t s1 = rotru32(w[j-2], SHA256_WORD_SIZE, 17) 
                ^ rotru32(w[j-2], SHA256_WORD_SIZE, 19) 
                ^ (w[j-2] >> 10);

            w[j] = w[j-16] + s0 + w[j-7] + s1;
        }

        uint32_t a = h[0];
        uint32_t b = h[1];
        uint32_t c = h[2];
        uint32_t d = h[3];
        uint32_t e = h[4];
        uint32_t f = h[5];
        uint32_t g = h[6];
        uint32_t hh = h[7];
        
        for (size_t j = 0; j < 64; j++) {
            uint32_t S1 = rotru32(e, SHA256_WORD_SIZE, 6) 
                ^ rotru32(e, SHA256_WORD_SIZE, 11)
                ^ rotru32(e, SHA256_WORD_SIZE, 25);

            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + S1 + ch + sha_256_K[j] + w[j];

            uint32_t S0 = rotru32(a, SHA256_WORD_SIZE, 2) 
                ^ rotru32(a, SHA256_WORD_SIZE, 13)
                ^ rotru32(a, SHA256_WORD_SIZE, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    free(padded_message);

    for (int i = 0; i < 8; i++) {
        output[4 * i + 0] = (h[i] >> 24) & 0xFF;
        output[4 * i + 1] = (h[i] >> 16) & 0xFF;
        output[4 * i + 2] = (h[i] >> 8) & 0xFF;
        output[4 * i + 3] = h[i] & 0xFF;
    }

    return 0;
}

int
sha384(uint8_t *output, const uint8_t *input, size_t input_len)
{
    uint8_t *padded_message;
    size_t padded_len;
    shapad(input, input_len, &padded_message, &padded_len, SHA512_BLOCK_SIZE);

    uint64_t h[8];
    memcpy(h, sha_384_H, sizeof(sha_384_H));

    for (size_t i = 0; i < padded_len; i += 128) {
        uint64_t w[80] = {0};
        for (size_t j = 0; j < 16; j++) {
            w[j] = ((uint64_t)padded_message[i + 8 * j + 0] << 56) |
                   ((uint64_t)padded_message[i + 8 * j + 1] << 48) |
                   ((uint64_t)padded_message[i + 8 * j + 2] << 40) |
                   ((uint64_t)padded_message[i + 8 * j + 3] << 32) |
                   ((uint64_t)padded_message[i + 8 * j + 4] << 24) |
                   ((uint64_t)padded_message[i + 8 * j + 5] << 16) |
                   ((uint64_t)padded_message[i + 8 * j + 6] << 8) |
                   ((uint64_t)padded_message[i + 8 * j + 7]);
        }

        for (size_t j = 16; j < 80; j++) {
            uint64_t s0 = rotru64(w[j-15], SHA512_WORD_SIZE, 1) 
                ^ rotru64(w[j-15], SHA512_WORD_SIZE, 8) 
                ^ (w[j-15] >> 7);

            uint64_t s1 = rotru64(w[j-2], SHA512_WORD_SIZE, 19) 
                ^ rotru64(w[j-2], SHA512_WORD_SIZE, 61) 
                ^ (w[j-2] >> 6);

            w[j] = w[j-16] + s0 + w[j-7] + s1;
        }

        uint64_t a = h[0];
        uint64_t b = h[1];
        uint64_t c = h[2];
        uint64_t d = h[3];
        uint64_t e = h[4];
        uint64_t f = h[5];
        uint64_t g = h[6];
        uint64_t hh = h[7];
        
        for (size_t j = 0; j < 80; j++) {
            uint64_t S1 = rotru64(e, SHA512_WORD_SIZE, 14) 
                ^ rotru64(e, SHA512_WORD_SIZE, 18)
                ^ rotru64(e, SHA512_WORD_SIZE, 41);

            uint64_t ch = (e & f) ^ ((~e) & g);
            uint64_t temp1 = hh + S1 + ch + sha_512_K[j] + w[j];

            uint64_t S0 = rotru64(a, SHA512_WORD_SIZE, 28) 
                ^ rotru64(a, SHA512_WORD_SIZE, 34)
                ^ rotru64(a, SHA512_WORD_SIZE, 39);
            uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint64_t temp2 = S0 + maj;

            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    free(padded_message);

    for (int i = 0; i < 6; i++) {
        output[8 * i + 0] = (h[i] >> 56) & 0xFF;
        output[8 * i + 1] = (h[i] >> 48) & 0xFF;
        output[8 * i + 2] = (h[i] >> 40) & 0xFF;
        output[8 * i + 3] = (h[i] >> 32) & 0xFF;
        output[8 * i + 4] = (h[i] >> 24) & 0xFF;
        output[8 * i + 5] = (h[i] >> 16) & 0xFF;
        output[8 * i + 6] = (h[i] >> 8) & 0xFF;
        output[8 * i + 7] = h[i] & 0xFF;
    }

    return 0;
}

int
sha512(uint8_t *output, const uint8_t *input, size_t input_len)
{
    uint8_t *padded_message;
    size_t padded_len;
    shapad(input, input_len, &padded_message, &padded_len, SHA512_BLOCK_SIZE);

    uint64_t h[8];
    memcpy(h, sha_512_H, sizeof(sha_512_H));

    for (size_t i = 0; i < padded_len; i += 128) {
        uint64_t w[80] = {0};
        for (size_t j = 0; j < 16; j++) {
            w[j] = ((uint64_t)padded_message[i + 8 * j + 0] << 56) |
                   ((uint64_t)padded_message[i + 8 * j + 1] << 48) |
                   ((uint64_t)padded_message[i + 8 * j + 2] << 40) |
                   ((uint64_t)padded_message[i + 8 * j + 3] << 32) |
                   ((uint64_t)padded_message[i + 8 * j + 4] << 24) |
                   ((uint64_t)padded_message[i + 8 * j + 5] << 16) |
                   ((uint64_t)padded_message[i + 8 * j + 6] << 8) |
                   ((uint64_t)padded_message[i + 8 * j + 7]);
        }

        for (size_t j = 16; j < 80; j++) {
            uint64_t s0 = rotru64(w[j-15], SHA512_WORD_SIZE, 1) 
                ^ rotru64(w[j-15], SHA512_WORD_SIZE, 8) 
                ^ (w[j-15] >> 7);

            uint64_t s1 = rotru64(w[j-2], SHA512_WORD_SIZE, 19) 
                ^ rotru64(w[j-2], SHA512_WORD_SIZE, 61) 
                ^ (w[j-2] >> 6);

            w[j] = w[j-16] + s0 + w[j-7] + s1;
        }

        uint64_t a = h[0];
        uint64_t b = h[1];
        uint64_t c = h[2];
        uint64_t d = h[3];
        uint64_t e = h[4];
        uint64_t f = h[5];
        uint64_t g = h[6];
        uint64_t hh = h[7];
        
        for (size_t j = 0; j < 80; j++) {
            uint64_t S1 = rotru64(e, SHA512_WORD_SIZE, 14) 
                ^ rotru64(e, SHA512_WORD_SIZE, 18)
                ^ rotru64(e, SHA512_WORD_SIZE, 41);

            uint64_t ch = (e & f) ^ ((~e) & g);
            uint64_t temp1 = hh + S1 + ch + sha_512_K[j] + w[j];

            uint64_t S0 = rotru64(a, SHA512_WORD_SIZE, 28) 
                ^ rotru64(a, SHA512_WORD_SIZE, 34)
                ^ rotru64(a, SHA512_WORD_SIZE, 39);
            uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint64_t temp2 = S0 + maj;

            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    free(padded_message);

    for (int i = 0; i < 8; i++) {
        output[8 * i + 0] = (h[i] >> 56) & 0xFF;
        output[8 * i + 1] = (h[i] >> 48) & 0xFF;
        output[8 * i + 2] = (h[i] >> 40) & 0xFF;
        output[8 * i + 3] = (h[i] >> 32) & 0xFF;
        output[8 * i + 4] = (h[i] >> 24) & 0xFF;
        output[8 * i + 5] = (h[i] >> 16) & 0xFF;
        output[8 * i + 6] = (h[i] >> 8) & 0xFF;
        output[8 * i + 7] = h[i] & 0xFF;
    }

    return 0;
}