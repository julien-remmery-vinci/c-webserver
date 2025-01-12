#define DATA_STRUCTURES_IMPLEMENTATION
#include "data_structures.h"
#define JUTILS_IMPLEMENTATION
#include "jutils.h"
#define WEBSERVER_IMPLEMENTATION
#include "webserver.h"

int
route_get_root(Route* route, Request* req)
{
    req->response = HTTP_RES_OK;
    Ws_send_response(req->client_fd, &req->response);

    if (route->file_buffer != NULL && route->file_size > 0) {
        if (send(req->client_fd, route->file_buffer, route->file_size, 0) == -1) {
            perror("Error sending file");
        }
        return 0;
    }
    Ws_send_file(req, "static/index.html");
    return 0;
}

int
route_get_indexjs(Route* route, Request* req)
{
    req->response = HTTP_RES_OK;
    Ws_send_response(req->client_fd, &req->response);

    if (route->file_buffer != NULL && route->file_size > 0) {
        if (send(req->client_fd, route->file_buffer, route->file_size, 0) == -1) {
            perror("Error sending file");
        }
        return 0;
    }
    Ws_send_file(req, "static/index.js");
    return 0;
}

int
route_get_indexcss(Route* route, Request* req)
{
    req->response = HTTP_RES_OK;
    Ws_send_response(req->client_fd, &req->response);

    if (route->file_buffer != NULL && route->file_size > 0) {
        if (send(req->client_fd, route->file_buffer, route->file_size, 0) == -1) {
            perror("Error sending file");
        }
        return 0;
    }
    Ws_send_file(req, "static/index.css");
    return 0;
}

int
route_get_favicon(Route* route, Request* req)
{
    (void)route;
    req->response = HTTP_RES_NOT_FOUND;
    Ws_send_response(req->client_fd, &req->response);
    return 0;
}

int
route_get_users(Route* route, Request* req)
{
    (void)route;
    req->response = HTTP_RES_NOT_IMPLEMENTED;
    Ws_send_response(req->client_fd, &req->response);
    return 0;
}

Ws_Router
setup_router()
{
    Ws_Router router = {0};
    // Initialize the map to store routes with a size of 10
    router.routes = hm_create(HM_DEFAULT_SIZE);

    Ws_router_handle(&router, "/", HTTP_GET, route_get_root);
    Ws_router_handle(&router, "/index.js", HTTP_GET, route_get_indexjs);
    Ws_router_handle(&router, "/index.css", HTTP_GET, route_get_indexcss);
    Ws_router_handle(&router, "/favicon.ico", HTTP_GET, route_get_favicon);
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