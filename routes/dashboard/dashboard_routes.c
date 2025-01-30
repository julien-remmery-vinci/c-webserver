#include "dashboard_routes.h"

int
route_get_dashboard(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    res->status = HTTP_STATUS_OK;
    Ws_send_response_with_file(req->client_fd, res, "static/dashboard.html");
    return 0;
}