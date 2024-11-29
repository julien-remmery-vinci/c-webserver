#include "http.h"
#include <string.h>

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