#ifndef ROOT_ROUTES_H
#define ROOT_ROUTES_H

#include "server.h"
#include "http.h"

int
route_get_root(Route* route, Http_Request* req, Http_Response* res);

int
route_get_root_js(Route* route, Http_Request* req, Http_Response* res);

int
route_get_root_css(Route* route, Http_Request* req, Http_Response* res);

int
route_get_favicon(Route* route, Http_Request* req, Http_Response* res);

#endif // ROOT_ROUTES_H