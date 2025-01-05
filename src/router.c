#include "router.h"
#include "jutils.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int add_route(Router* router, Route* route) {
    if(router->routes == NULL) {
        router->routes = (Route*)malloc(sizeof(Route));
        if (router->routes == NULL) {
            return -1; // Échec d'allocation
        }
    } else {
        Route* temp = (Route*)realloc(router->routes, (router->nb_routes + 1) * sizeof(Route));
        if (temp == NULL) {
            perror("realloc failed");
            return -1; // Échec d'allocation
        }
        router->routes = temp;
    }
    router->routes[router->nb_routes++] = *route;
    return 0;
}

int handle_request(Router* router, Request* req) {
    for (size_t i = 0; i < router->nb_routes; i++)
    {
        Route route = router->routes[i];
        if(strcmp(route.path, req->path) == 0
            && route.method == req->method) {
            return route.handler(&route, req);
        }
    }
    
    req->response.status = HTTP_NOT_FOUND;
    sresponse(req->client_fd, &HTTP_RES_NOT_FOUND);
    send_file(req, "static/not_found.html");
    return 0;
}