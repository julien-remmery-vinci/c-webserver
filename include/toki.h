#ifndef TOKI_H
#define TOKI_H

#include "base64.h"
#include "hmac.h"
#include "sha.h"
#include "hashmap.h"
#include "jacon.h"
#include "jutils.h"

typedef enum {
    TOKI_OK,
    TOKI_INVALID_TOKEN,
    TOKI_ERR_NULL_PARAM,
    TOKI_ERR_ADD_CLAIM,
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

Toki_Error
Toki_add_claim(Toki_Token* token, Jacon_Node* claim);

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