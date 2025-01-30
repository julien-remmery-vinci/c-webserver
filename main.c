#include "server.h"
#include "toki.h"
#include <time.h>

int
route_get_root(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    res->status = HTTP_STATUS_OK;
    Ws_send_response_with_file(req->client_fd, res, "static/index.html");
    return 0;
}
int
route_get_root_js(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    res->status = HTTP_STATUS_OK;
    Ws_send_response_with_file(req->client_fd, res, "static/index.js");
    return 0;
}
int
route_get_root_css(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    res->status = HTTP_STATUS_OK;
    Ws_send_response_with_file(req->client_fd, res, "static/index.css");
    return 0;
}

int
route_get_favicon(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    res->status = HTTP_STATUS_NOT_FOUND;
    Ws_send_response(req->client_fd, res);
    return 0;
}

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

int
route_get_dashboard(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    res->status = HTTP_STATUS_OK;
    Ws_send_response_with_file(req->client_fd, res, "static/dashboard.html");
    return 0;
}

int
authorize(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    char* auth_header = hm_get(&req->headers, "authorization");
    if (auth_header == NULL) {
        res->status = HTTP_STATUS_UNAUTHORIZED;
        Ws_send_response(req->client_fd, res);
        return 1;
    }

    if (!Toki_verify_token(auth_header)) {
        res->status = HTTP_STATUS_UNAUTHORIZED;
        Ws_send_response(req->client_fd, res);
        return 1;
    }

    // Additional work on token data
    
    return 0;
}

Ws_Router
setup_router()
{
    Ws_Router router = {0};
    // Initialize the map to store routes with a size of 10
    router.routes = hm_create(HM_DEFAULT_SIZE);

    Ws_router_handle(&router, "/", HTTP_METHOD_GET, route_get_root, NULL);
    Ws_router_handle(&router, "/index.js", HTTP_METHOD_GET, route_get_root_js, NULL);
    Ws_router_handle(&router, "/index.css", HTTP_METHOD_GET, route_get_root_css, NULL);
    Ws_router_handle(&router, "/favicon.ico", HTTP_METHOD_GET, route_get_favicon, NULL);
    Ws_router_handle(&router, "/dashboard", HTTP_METHOD_GET, route_get_dashboard, authorize);
    Ws_router_handle(&router, "/api/login", HTTP_METHOD_POST, route_post_login, NULL);
    return router;
}

int 
main(void)
{
    Ws_Config config = Ws_load_config_from_file(NULL);
    Ws_Router router = setup_router();
    Ws_Server server = Ws_server_setup(config, router);
    Ws_server_enable_logging(&server);
    return Ws_run_server(&server);
}