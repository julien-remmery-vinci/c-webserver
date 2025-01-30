#ifndef DASHBOARD_ROUTES_H
#define DASHBOARD_ROUTES_H

#include "server.h"
#include "http.h"

int
route_get_dashboard(Route* route, Http_Request* req, Http_Response* res);

#endif // DASHBOARD_ROUTES_H