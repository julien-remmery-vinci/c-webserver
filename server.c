#define WEBSERVER_IMPLEMENTATION
#define HASHMAP_IMPLEMENTATION
#define JUTILS_IMPLEMENTATION
#define HTTP_IMPLEMENTATION
#define JACON_IMPLEMENTATION
#include "webserver.h"

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

int
route_get_users(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    res->status = HTTP_STATUS_NOT_IMPLEMENTED;
    Ws_send_response(req->client_fd, res);
    return 0;
}

int
authorized(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    (void)req;
    (void)res;
    return 0;
}

int
unauthorized(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    (void)req;
    (void)res;
    res->status = HTTP_STATUS_UNAUTHORIZED;
    Ws_send_response(req->client_fd, res);
    return 1;
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
    Ws_router_handle(&router, "/users", HTTP_METHOD_GET, route_get_users, authorized);
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