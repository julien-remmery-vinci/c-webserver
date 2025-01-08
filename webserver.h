#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/time.h>
#include <stddef.h>

// TODO : Add config handling with config file & Config struct
#define PORT 3000
#define BACKLOG 10
#define BUFLEN 2048
#define MAX_CONNECTIONS 10
#define SEM_NAME "/sem_connection_count"

#define HTTP_VERSION "HTTP/1.1"

#define HTTP_HEADER_OK "HTTP/1.1 200 OK\n\n"
#define HTTP_HEADER_NO_CONTENT "HTTP/1.1 204 No Content\n\n"
#define HTTP_HEADER_BAD_REQUEST "HTTP/1.1 400 Bad Request\n\n"
#define HTTP_HEADER_UNAUTHORIZED "HTTP/1.1 401 Unauthorized\n\n"
#define HTTP_HEADER_FORBIDDEN "HTTP/1.1 403 Forbidden\n\n"
#define HTTP_HEADER_NOT_FOUND "HTTP/1.1 404 Not Found\n\n"
#define HTTP_HEADER_NOT_ALLOWED "HTTP/1.1 405 Not Allowed\n\n"
#define HTTP_HEADER_INTERNAL_SERVER_ERROR "HTTP/1.1 500 Internal Server Error\n\n"
#define HTTP_HEADER_NOT_IMPLEMENTED "HTTP/1.1 501 Not Implemented\n\n"
#define HTTP_HEADER_MALFORMED "HTTP/1.1 400 Bad Request\n\nMalformed headers in the request."

#define HTTP_RES_OK \
    (Response) { .status = HTTP_OK, .header = HTTP_HEADER_OK }
#define HTTP_RES_HEADER_MALFORMED \
    (Response) { .status = HTTP_BAD_REQUEST, .header = HTTP_HEADER_MALFORMED }
#define HTTP_RES_NOT_FOUND \
    (Response) { .status = HTTP_NOT_FOUND, .header = HTTP_HEADER_NOT_FOUND }
#define HTTP_RES_INTERNAL_SERVER_ERROR \
    (Response) { .status = HTTP_INTERNAL_SERVER_ERROR, .header = HTTP_HEADER_INTERNAL_SERVER_ERROR }
#define HTTP_RES_NOT_IMPLEMENTED \
    (Response) { .status = HTTP_NOT_IMPLEMENTED, .header = HTTP_HEADER_NOT_IMPLEMENTED }

enum HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_PATCH,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_NO_METHOD
};

enum HttpStatus {
    HTTP_OK = 200,
    HTTP_NO_CONTENT = 204,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_NOT_ALLOWED = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501
};

enum HttpMethod get_http_method(char* str);
char* strmethod(enum HttpMethod method);
char* strstatus(enum HttpStatus status);

#define HTTP_MALFORMED_ERROR -1
#define HTTP_NOT_IMPLEMENTED_ERROR -2

typedef struct Response {
    enum HttpStatus status;
    char* header;
} Response;

typedef struct Request {
    int client_fd;
    enum HttpMethod method;
    char* path;
    char* http_version;
    struct timeval start;
    struct timeval stop;
    Response response;
} Request;

int rrequest(int fd, char* buf);
int sresponse(int fd, Response* res);
int parse_request(Request* req, char* reqstr);
void log_request(Request* req);
int build_response(Request* req);
int send_file(Request* req, char* filepath);

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

#endif // WEBSERVER_H

/**
 * Web server implementation
 */
#ifdef WEBSERVER_IMPLEMENTATION

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <bits/waitflags.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

enum HttpMethod get_http_method(char* str) {
    if(!strcmp(str, "GET")) return HTTP_GET;
    if(!strcmp(str, "POST")) return HTTP_POST;
    if(!strcmp(str, "PUT")) return HTTP_PUT;
    if(!strcmp(str, "PATCH")) return HTTP_PATCH;
    if(!strcmp(str, "DELETE")) return HTTP_DELETE;
    if(!strcmp(str, "HEAD")) return HTTP_HEAD;
    if(!strcmp(str, "OPTIONS")) return HTTP_OPTIONS;
    return HTTP_NO_METHOD;
}

char* strmethod(enum HttpMethod method) {
    switch (method) {
    case HTTP_GET:
        return "GET";
    case HTTP_POST:
        return "POST";
    case HTTP_PUT:
        return "PUT";
    case HTTP_PATCH:
        return "PATCH";
    case HTTP_DELETE:
        return "DELETE";
    default:
        return NULL;
    }
}

char* strstatus(enum HttpStatus status) {
    switch(status) {
        case(HTTP_OK):
            return "Success";
        case(HTTP_NO_CONTENT):
            return "No Content";
        case(HTTP_BAD_REQUEST):
            return "Bad Request";
        case(HTTP_UNAUTHORIZED):
            return "Unauthorized";
        case(HTTP_FORBIDDEN):
            return "Forbidden";
        case(HTTP_NOT_FOUND):
            return "Not Found";
        case(HTTP_INTERNAL_SERVER_ERROR):
            return "Internal Server Error";
        default:
            return NULL;
    }
}

int rrequest(int fd, char* buf) {
    int ret = read(fd, buf, BUFLEN);
    CHECK(ret > 0);
    return ret;
}

int sresponse(int fd, Response* res) {
    write(fd, res->header, strlen(res->header));
    return 0;
}

int parse_request(Request* req, char* reqstr) {
    char* method_str = strtok(reqstr, " \r\n");
    char* path = strtok(NULL, " \r\n");
    char* version = strtok(NULL, " \r\n");

    if (method_str == NULL || path == NULL || version == NULL) {
        return HTTP_MALFORMED_ERROR;
    }

    req->method = get_http_method(method_str);
    if (req->method == HTTP_NO_METHOD) {
        WARN("Unhandled HTTP Method received: %d", req->method);
        return HTTP_NOT_IMPLEMENTED_ERROR;
    }

    req->path = path;
    req->http_version = version;
    return 0;
}

void log_request(Request* req) {
    // Time from start of request processing to its end in milliseconds
    double req_time_ms = (double)((req->stop.tv_sec - req->start.tv_sec) * 1000000 + req->stop.tv_usec - req->start.tv_usec)/1000;
    INFO("%8.3f ms %-3d %-6s %s", req_time_ms, req->response.status, strmethod(req->method), req->path);
}

int build_response(Request* req) {
    (void)req;
    return -1;
}

int send_file(Request* req, char* filepath) {
    int filefd = open(filepath, O_RDONLY);
    if(filefd == -1) {
        sresponse(req->client_fd, &HTTP_RES_INTERNAL_SERVER_ERROR);
        return 0;
    }
    off_t offset = 0;
    struct stat stat_buf;
    fstat(filefd, &stat_buf);

    ssize_t bytes_sent = sendfile(req->client_fd, filefd, &offset, stat_buf.st_size);
    if (bytes_sent == -1) {
        perror("sendfile");
        return -1;
    }
    close(filefd);
    return 0;
}

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
    
    req->response = HTTP_RES_NOT_FOUND;
    sresponse(req->client_fd, &req->response);
    send_file(req, "static/not_found.html");
    return 0;
}

#endif // WEBSERVER_IMPLEMENTATION