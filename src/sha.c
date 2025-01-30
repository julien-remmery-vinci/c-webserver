#include "sha.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t
rotate_right(uint32_t value, size_t size, int shift)
{
    return (value >> shift) | (value << (size - shift));
}

void
pad_message(const uint8_t *input, size_t input_len, uint8_t **padded_message, size_t *padded_len)
{
    size_t bit_len = input_len * 8;
    size_t pad_len = ((bit_len + 1 + 64 + 511) / 512) * 64 - input_len;
    *padded_len = input_len + pad_len;
    *padded_message = (uint8_t*)malloc(*padded_len);

    memcpy(*padded_message, input, input_len);
    (*padded_message)[input_len] = 0x80;
    memset(*padded_message + input_len + 1, 0, pad_len - 1 - 8);

    for (int i = 0; i < 8; i++) {
        (*padded_message)[*padded_len - 1 - i] = (bit_len >> (8 * i)) & 0xFF;
    }
}

int
sha256(uint8_t *output, const uint8_t *input, size_t input_len)
{
    uint8_t *padded_message;
    size_t padded_len;
    pad_message(input, input_len, &padded_message, &padded_len);

    uint32_t h[8];
    memcpy(h, H, sizeof(H));

    for (size_t i = 0; i < padded_len; i += 64) {
        uint32_t w[64] = {0};
        for (size_t j = 0; j < 16; j++) {
            w[j] = (padded_message[i + 4 * j + 0] << 24) |
                   (padded_message[i + 4 * j + 1] << 16) |
                   (padded_message[i + 4 * j + 2] << 8) |
                   (padded_message[i + 4 * j + 3]);
        }

        for (size_t j = 16; j < 64; j++) {
            uint32_t s0 = rotate_right(w[j-15], SHA256_WORD_SIZE, 7) 
                ^ rotate_right(w[j-15], SHA256_WORD_SIZE, 18) 
                ^ (w[j-15] >> 3);

            uint32_t s1 = rotate_right(w[j-2], SHA256_WORD_SIZE, 17) 
                ^ rotate_right(w[j-2], SHA256_WORD_SIZE, 19) 
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
            uint32_t S1 = rotate_right(e, SHA256_WORD_SIZE, 6) 
                ^ rotate_right(e, SHA256_WORD_SIZE, 11)
                ^ rotate_right(e, SHA256_WORD_SIZE, 25);

            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + S1 + ch + K[j] + w[j];

            uint32_t S0 = rotate_right(a, SHA256_WORD_SIZE, 2) 
                ^ rotate_right(a, SHA256_WORD_SIZE, 13)
                ^ rotate_right(a, SHA256_WORD_SIZE, 22);
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