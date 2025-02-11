#include "toki.h"
#include "sha.h"
#include "hmac.h"
#include <stdlib.h>

const char*
Toki_stralg(Toki_Alg alg)
{
    switch (alg) {
        case TOKI_ALG_HS256:
            return "HS256";
        case TOKI_ALG_HS384:
            return "HS384";
        case TOKI_ALG_HS512:
            return "HS512";
        default:
            return NULL;
    }
}

Toki_Error
Toki_token_init(Toki_Token* token, Toki_Alg algorithm)
{
    if (token == NULL) {
        return TOKI_ERR_NULL_PARAM;
    }

    memset(token, 0, sizeof(Toki_Token));

    token->algorithm = algorithm;

    const char* str_alg = Toki_stralg(algorithm);
    Jacon_Node* alg = calloc(1, sizeof(Jacon_Node));
    alg->type = JACON_VALUE_STRING;
    alg->name = "alg";
    alg->value.string_val = (char*)str_alg;

    Jacon_Node* typ = calloc(1, sizeof(Jacon_Node));
    typ->type = JACON_VALUE_STRING;
    typ->name = "typ";
    typ->value.string_val = "JWT";

    Jacon_append_child(&token->header, alg);
    Jacon_append_child(&token->header, typ);

    return TOKI_OK;
}

Toki_Error
Toki_add_claim(Toki_Token* token, Jacon_Node* claim)
{
    int ret;
    ret = Jacon_append_child(&token->payload, claim);
    if (ret != JACON_OK) return TOKI_ERR_ADD_CLAIM;
    return TOKI_OK;
}

Toki_Error
Toki_hash_params(Toki_Alg algorithm, size_t* block_size, size_t* digest_size, void** hash_func)
{
    switch (algorithm) {
        case TOKI_ALG_HS256:
            *block_size = SHA256_BLOCK_SIZE;
            *digest_size = SHA256_DIGEST_SIZE;
            *hash_func = sha256;
            return TOKI_OK;
        case TOKI_ALG_HS384:
            *block_size = SHA384_BLOCK_SIZE;
            *digest_size = SHA384_DIGEST_SIZE;
            *hash_func = sha384;
            return TOKI_OK;
        case TOKI_ALG_HS512:
            *block_size = SHA512_BLOCK_SIZE;
            *digest_size = SHA512_DIGEST_SIZE;
            *hash_func = sha512;
            return TOKI_OK;
    }
    return TOKI_ERR_UNSUPPORTED_ALGORITHM;
}

Toki_Error
Toki_sign_token(Toki_Token* token, const char* key, char** signed_token)
{
    int ret;
    char* header; 
    Jacon_serialize_unformatted(&token->header, &header);
    
    char* payload; 
    Jacon_serialize_unformatted(&token->payload, &payload);

    char* base64_header;
    Base64Url_encode((const unsigned char*)header, strlen(header), &base64_header); 
    char* base64_payload;
    Base64Url_encode((const unsigned char*)payload, strlen(payload), &base64_payload); 

    StringBuilder builder = {0};

    Ju_str_append_fmt_null(&builder, "%s.%s", base64_header, base64_payload);

    void* hash_func;
    size_t block_size = 0;
    size_t digest_size = 0;
    ret = Toki_hash_params(token->algorithm, &block_size, &digest_size, &hash_func);
    if (ret == TOKI_ERR_UNSUPPORTED_ALGORITHM) {
        return TOKI_ERR_UNSUPPORTED_ALGORITHM;
    }
    digest_size /= 8; // Size in bytes
    char* signature = hmac(builder.string, key, hash_func, block_size/ 8, digest_size);

    char* base64_signature;
    Base64Url_encode((const unsigned char*)signature, strlen(signature), &base64_signature);
    Ju_str_append_fmt_null(&builder, ".%s", base64_signature);

    *signed_token = strdup(builder.string);

    Ju_builder_free(&builder);
    free(header);
    free(payload);
    free(base64_header);
    free(base64_payload);
    free(signature);
    free(base64_signature);

    return TOKI_OK;
}

bool
Toki_validate_token(const char* token)
{
    char *last_dot = strrchr(token, '.');
    if (last_dot == NULL) return false;
    *last_dot = '\0';
    char *header_payload = (char*)token;
    char *signature = last_dot + 1;
    unsigned char *base64_decoded;
    size_t decoded_len;
    Base64Url_decode((const char*)signature, &decoded_len, &base64_decoded);

    // TODO : extract algorithm from token using Jacon to get right params
    char* verified_signature = hmac(header_payload, "secret", sha256, SHA256_BLOCK_SIZE / 8, SHA256_DIGEST_SIZE);
    return strcmp((char*)base64_decoded, verified_signature) == 0;
}

bool
Toki_verify_token(const char* token)
{
    if (!Toki_validate_token(token)) return false;
    
    // TODO : Additionnal verifications
    // expire time, ...
    
    return true;
}

void
Toki_free_token(Toki_Token* token)
{
    Jacon_free_node(&token->header);
    Jacon_free_node(&token->payload);
}

void
Toki_setup_env(Ws_Config* config)
{
    char* toki_secret = hm_get(config, "toki_secret");
    if (toki_secret == NULL) {
        setenv("toki_secret", "default_secret", 0); // SHOULD NEVER HAPPEN BRO
        return;
    }
    setenv("toki_secret", toki_secret, 0);
}