#ifndef TOKI_H
#define TOKI_H

#include "base64.h"
#include "hmac.h"
#include "SHA.h"
#include "hashmap.h"
#include "jacon.h"
#include "jutils.h"

typedef enum {
    TOKI_OK,
    TOKI_INVALID_TOKEN,
    TOKI_ERR_NULL_PARAM,
} Toki_Error;

typedef Jacon_Node Toki_Claims;
typedef Jacon_Node Toki_Payload;

typedef struct Toki_Token {
    Toki_Claims header;
    Toki_Payload payload;
} Toki_Token;

typedef enum {
    TOKI_ALG_HS256, 
} Toki_Alg;

/**
 * Initializes a token with the provided algorithm
 * Sets the token's header to
 *      {
 *          "alg": "provided_alg",
 *          "typ": "JWT"
 *      }
 */
Toki_Error
Toki_token_init(Toki_Token* token, Toki_Alg algorithm);

/**
 * Signs a token and sets signed_token to the signed token value
 */
Toki_Error
Toki_sign_token(Toki_Token* token, char** signed_token);

/**
 * Verify a token
 * Based on token signature and optionnal registered claims
 *  (iss, sub, aud, exp, nbf, iat, jti)
 */
bool
Toki_verify_token(const char* token);

/**
 * Free allocated ressources
 */
void
Toki_free_token(Toki_Token* token);

#endif // TOKI_H

#ifdef TOKI_IMPLEMENTATION

const char*
Toki_stralg(Toki_Alg alg)
{
    switch (alg) {
        case TOKI_ALG_HS256:
            return "HS256";
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
    Jacon_append_child(&token->payload, claim);
}

Toki_Error
Toki_sign_token(Toki_Token* token, char** signed_token)
{
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
    char* signature = hmac(builder.string, "secretkey", sha256, SHA256_BLOCK_SIZE / 8, SHA256_DIGEST_SIZE);
    char* base64_signature;
    Base64Url_encode((const unsigned char*)signature, strlen(signature), &base64_signature);
    Ju_str_append_fmt_null(&builder, ".%s", base64_signature);

    *signed_token = strdup(builder.string);

    Ju_str_free(&builder);
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
    const char *signature = last_dot + 1;
    unsigned char *base64_decoded;
    size_t decoded_len;
    Base64Url_decode((const unsigned char*)signature, &decoded_len, &base64_decoded);

    char* verified_signature = hmac(header_payload, "secretkey", sha256, SHA256_BLOCK_SIZE / 8, SHA256_DIGEST_SIZE);
    return strcmp(base64_decoded, verified_signature) == 0;
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

#endif // TOKI_IMPLEMENTATION