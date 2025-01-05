#ifndef ROUTER_H_
#define ROUTER_H_

#include "http.h"
#include "request.h"
#include <stddef.h>

struct Route;

typedef int (*RouteHandler)(struct Route* route, Request* request);

typedef struct Route {
    enum HttpMethod method;
    char* path;
    RouteHandler handler;
    char *file_buffer;
    size_t file_size;
} Route;

typedef struct Router {
    Route* routes;
    size_t nb_routes;
} Router;

int add_route(Router* router, Route* route);
int handle_request(Router* router, Request* req);

#endif // ROUTER_H_