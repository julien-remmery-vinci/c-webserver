#include "login_routes.h"
#include "toki.h"
#include <time.h>

bool
validate_credentials(const char* login, const char* password)
{
    return login != NULL 
        && password != NULL 
        && strlen(login) != 0 
        && strlen(password) != 0;
}

char*
create_token(const char* login)
{
    Toki_Token token = {0};
    Toki_token_init(&token, TOKI_ALG_HS256);
    Jacon_Node iss = Jacon_string_prop("iss", "Conrad");
    Jacon_Node exp = Jacon_int_prop("exp", (int)time(NULL));
    Jacon_Node login_node = Jacon_string_prop("login", login);
    Toki_add_claim(&token, &iss);
    Toki_add_claim(&token, &exp);
    Toki_add_claim(&token, &login_node);

    char* signed_token;
    Toki_sign_token(&token, &signed_token);
    return signed_token;
}

int
route_post_login(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    char* login;
    Jacon_get_string_by_name(&req->body, "login", &login);
    char* password;
    Jacon_get_string_by_name(&req->body, "password", &password);

    if (!validate_credentials(login, password)) {
        res->status = HTTP_STATUS_BAD_REQUEST;
        return Ws_send_response(req->client_fd, res);
    }

    char* signed_token = create_token(login);

    Jacon_Node json_object = Jacon_object();
    Jacon_Node token_node = Jacon_string_prop("token", signed_token);
    Jacon_append_child(&json_object, &token_node);

    res->status = HTTP_STATUS_OK;
    Jacon_serialize(&json_object, &res->content);

    free(login);
    free(password);
    free(signed_token);

    return Ws_send_response_with_content(req->client_fd, res, HTTP_CONTENTTYPE_JSON);
}