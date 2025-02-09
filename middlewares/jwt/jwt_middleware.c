#include "toki.h"
#include "jwt_middleware.h"

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