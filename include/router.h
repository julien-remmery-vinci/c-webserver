#ifndef ROUTER_H_
#define ROUTER_H_

#include "http.h"
#include "request.h"
#include <stddef.h>

typedef struct Route {
    enum HttpMethod method;
    char* path;
    int (*handler)(Request* req);
} Route;

typedef struct Router {
    Route* routes;
    size_t nb_routes;
} Router;

int add_route(Router* router, Route* route);
int handle_request(Router* router, Request* req);

#endif // ROUTER_H_