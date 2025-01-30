#ifndef LOGIN_ROUTES_H
#define LOGIN_ROUTES_H

#include "server.h"
#include "http.h"

int
route_post_login(Route* route, Http_Request* req, Http_Response* res);

#endif // LOGIN_ROUTES_H