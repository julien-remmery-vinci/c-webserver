#ifndef HTTP_H
#define HTTP_H

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

#endif // HTTP_H