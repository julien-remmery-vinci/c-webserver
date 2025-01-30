#include "root_routes.h"

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