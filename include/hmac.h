#ifdef HMAC_H
#define HMAC_H

#include <stdint.h>
#include <stddef.h>

char*
hmac(char* message, char* key, 
    int (*hash)(uint8_t *output, const uint8_t *input, size_t input_len), 
    size_t blocksize, size_t output_size);

#endif // HMAC_H

#define HMAC_IMPLEMENTATION
#ifdef HMAC_IMPLEMENTATION

#include <stdint.h>

char*
pad(char* key, size_t blocksize)
{
    char* new_key = calloc(blocksize + 1, 1);
    memcpy(new_key, key, strlen(key));
    memset(new_key + strlen(new_key), 0, blocksize - strlen(new_key));
    new_key[blocksize] = '\0';
    return new_key;
}

char*
computeBlockSizedKey(char* key,
    int (*hash)(uint8_t *output, const uint8_t *input, size_t input_len),
    size_t blocksize)
{
    char* new_key = NULL;
    size_t key_len = strlen(key);

    if (strlen(key) > blocksize) {
        new_key  = calloc(blocksize + 1, 1);
        uint8_t* output = calloc(blocksize, sizeof(uint8_t));
        hash(output, (uint8_t*)key, strlen(key));
        memcpy(new_key, output, blocksize);
        new_key[blocksize] = '\0';
        free(output);
    }

    else if (key_len < blocksize) {
        return pad(key, blocksize);
    }
    else {
        new_key = strdup(key);
    }

    return new_key;
}

char*
hmac(char* message, char* key,
    int (*hash)(uint8_t *output, const uint8_t *input, size_t input_len), 
    size_t blocksize, size_t output_size)
{
    char* block_sized_key = computeBlockSizedKey(key, hash, blocksize);

    uint8_t* o_key_pad = (uint8_t*)calloc(blocksize, sizeof(uint8_t));
    uint8_t* i_key_pad = (uint8_t*)calloc(blocksize, sizeof(uint8_t));

    for (size_t i = 0; i < blocksize; i++) {
        o_key_pad[i] = block_sized_key[i] ^ 0x5c;
        i_key_pad[i] = block_sized_key[i] ^ 0x36;
    }

    size_t message_len = strlen(message);
    uint8_t* key_pad_msg = (uint8_t*)calloc(blocksize + message_len, sizeof(uint8_t));
    memcpy(key_pad_msg, i_key_pad, blocksize);
    memcpy(key_pad_msg + blocksize, message, message_len);

    size_t key_pad_msg_len = blocksize + message_len;

    uint8_t* hashed = (uint8_t*)calloc(output_size, sizeof(uint8_t));
    hash(hashed, key_pad_msg, key_pad_msg_len);

    uint8_t* key_pad_hash = (uint8_t*)calloc(blocksize + output_size, sizeof(uint8_t));
    memcpy(key_pad_hash, o_key_pad, blocksize);
    memcpy(key_pad_hash + blocksize, hashed, output_size);

    size_t key_pad_hash_len = blocksize + output_size;

    uint8_t* final_hashed = (uint8_t*)calloc(output_size, sizeof(uint8_t));
    hash(final_hashed, key_pad_hash, key_pad_hash_len);

    char* str_hash = (char*)calloc(output_size + 1, sizeof(char));
    memcpy(str_hash, final_hashed, output_size);
    str_hash[output_size] = '\0';

    free(block_sized_key);
    free(o_key_pad);
    free(i_key_pad);
    free(key_pad_msg);
    free(hashed);
    free(key_pad_hash);
    free(final_hashed);

    return str_hash;
}

#endif // HMAC_IMPLEMENTATION