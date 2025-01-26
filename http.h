#ifndef HTTP_H
#define HTTP_H

#include "data_structures.h"
#include "jacon.h"
#include <sys/time.h>
#include <string.h>

typedef enum {
    HTTP_OK,
    HTTP_ERR_MALFORMED_REQ,
    HTTP_UNSUPPORTED_METHOD,
    HTTP_ERR_INVALID_PATH,
    HTTP_UNSUPPORTED_VERSION,
    HTTP_ERR_NULL_PARAM,
    HTTP_ERR_MEMORY_ALLOCATION,
} Http_Error;

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

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_INVALID
} Http_Method;

typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_NOT_ALLOWED = 405,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501
} Http_Status;

typedef enum {
    HTTP_VERSION_1_1,
    HTTP_VERSION_INVALID
} Http_Version;

typedef enum {
    HTTP_CONTENTTYPE_APP_JSON,
    HTTP_CONTENTTYPE_UNSUPPORTED,
} Http_ContentType;

typedef struct HashMap Http_Headers;

/**
 * Response struct conataining the informations about a Response
 */
typedef struct Http_Response {
    Http_Status status;
    char* header;
} Http_Response;

/**
 * Request struct conataining the informations about a Request
 */
typedef struct Http_Request {
    Http_Method method;
    char* path;
    Http_Version version;
    Http_Headers headers;
    union {
        Jacon_content body;
    };
    int client_fd;
    struct timeval start;
    struct timeval end;
    double request_timing;
} Http_Request;

/**
 * Parse a string into an Http_Request struct
 */
Http_Error
Http_parse_request(Http_Request* req, const char* reqstr, const size_t header_len);

/**
 * Parse a request's headers
 */
Http_Error
Http_parse_headers(Http_Headers* headers, const char* headers_str, char** headers_last, const size_t header_len);

/**
 * Validate a user input path
 */
bool
Http_validate_path(const char* path);

/**
 * Returns the enum equivalent of an http method
 *  HTTP_METHOD_INVALID if invalid
 */
Http_Method 
Http_parse_method(const char* str);

/**
 * Returns de enum equivalent of an http version
 *  HTTP_VERSION_INVALID if invalid
 */
Http_Version 
Http_parse_version(const char* str);

/**
 * Returns the str equivalent of an http method
 */
char*
Http_strmethod(Http_Method method);

/**
 * Returns the str equivalent of an http status
 */
char*
Http_strstatus(Http_Status status);

/**
 * Free ressources allocated for a request struct
 */
void
Http_free_request(Http_Request* req);

#endif // HTTP_H

#ifdef HTTP_IMPLEMENTATION

Http_Error
Http_parse_request(Http_Request* req, const char* reqstr, const size_t header_len)
{
    int ret;
    char* method_end = strchr(reqstr, ' ');
    if (method_end == NULL) {
        return HTTP_ERR_MALFORMED_REQ;
    }
    *method_end = '\0';
    const char* method_str = reqstr;

    char* path_start = method_end + 1;
    char* path_end = strchr(path_start, ' ');
    if (path_end == NULL) {
        return HTTP_ERR_MALFORMED_REQ;
    }
    *path_end = '\0';
    const char* path = path_start;

    char* version_start = path_end + 1;
    char* version_end = strstr(version_start, "\r\n");
    if (version_end == NULL) {
        return HTTP_ERR_MALFORMED_REQ;
    }
    *version_end = '\0';
    const char* version = version_start;

    req->method = Http_parse_method(method_str);
    if (req->method == HTTP_METHOD_INVALID) {
        return HTTP_UNSUPPORTED_METHOD;
    }

    if (!Http_validate_path(path)) {
        return HTTP_ERR_INVALID_PATH;
    }
    req->path = strdup(path);

    req->version = Http_parse_version(version);
    if (req->version == HTTP_VERSION_INVALID) {
        return HTTP_UNSUPPORTED_VERSION;
    }

    req->headers = hm_create(10);
    char* headers_last; // NULL or at body start after parsing
    ret = Http_parse_headers(&req->headers, version_end + 2, &headers_last, header_len);
    if (ret != HTTP_OK) return ret;

    if(hm_exists(&req->headers, "Content-Type")) {
        if (strcmp(hm_get(&req->headers, "Content-Type"), "application/json") == 0) {
            Jacon_init_content(&req->body);
            Jacon_deserialize(&req->body, headers_last);
        }
    }

    return HTTP_OK;
}

Http_Error
Http_parse_headers(Http_Headers* headers, const char* headers_str, char** headers_last, const size_t header_len) {
    if (headers == NULL || headers_last == NULL)
        return HTTP_ERR_NULL_PARAM;
    if (headers_str == NULL || *headers_str == '\0')
        return HTTP_OK;

    const char *headers_end = headers_str + header_len;
    const char *line_start = headers_str;
    const char *line_end;

    while (line_start < headers_end) {
        line_end = strstr(line_start, "\r\n");
        if (line_end == NULL || line_end >= headers_end) {
            return HTTP_ERR_MALFORMED_REQ;
        }

        if (line_start == line_end) {
            *headers_last = (char*)(line_end + 2);
            return HTTP_OK;
        }

        const char* sep = memchr(line_start, ':', line_end - line_start);
        if (sep == NULL) {
            return HTTP_ERR_MALFORMED_REQ;
        }

        char* key = strndup(line_start, sep - line_start);
        if (key == NULL) {
            return HTTP_ERR_MEMORY_ALLOCATION;
        }

        const char* value_start = sep + 1;
        while (value_start < line_end && *value_start == ' ') value_start++;

        char* value = strndup(value_start, line_end - value_start);
        if (value == NULL) {
            free(key);
            return HTTP_ERR_MEMORY_ALLOCATION;
        }

        if (hm_put(headers, key, (void*)value) != 0) {
            free(key);
            free(value);
            return HTTP_ERR_MEMORY_ALLOCATION;
        }
        free(key);

        line_start = line_end + 2;
    }

    return HTTP_ERR_MALFORMED_REQ;
}


bool
Http_validate_path(const char* path)
{
    return true;
}

Http_Method 
Http_parse_method(const char* str)
{
    if(!strcmp(str, "GET")) return HTTP_METHOD_GET;
    if(!strcmp(str, "POST")) return HTTP_METHOD_POST;
    if(!strcmp(str, "PUT")) return HTTP_METHOD_PUT;
    if(!strcmp(str, "PATCH")) return HTTP_METHOD_PATCH;
    if(!strcmp(str, "DELETE")) return HTTP_METHOD_DELETE;
    if(!strcmp(str, "HEAD")) return HTTP_METHOD_HEAD;
    if(!strcmp(str, "OPTIONS")) return HTTP_METHOD_OPTIONS;
    return HTTP_METHOD_INVALID;
}

Http_Version 
Http_parse_version(const char* str)
{
    if(!strcmp(str, "HTTP/1.1")) return HTTP_VERSION_1_1;
    return HTTP_VERSION_INVALID;
}

char*
Http_strmethod(Http_Method method)
{
    switch (method) {
    case HTTP_METHOD_GET:
        return "GET";
    case HTTP_METHOD_POST:
        return "POST";
    case HTTP_METHOD_PUT:
        return "PUT";
    case HTTP_METHOD_PATCH:
        return "PATCH";
    case HTTP_METHOD_DELETE:
        return "DELETE";
    default:
        return NULL;
    }
}

char*
Http_strstatus(Http_Status status)
{
    switch(status) {
        case(HTTP_STATUS_OK):
            return "Success";
        case(HTTP_STATUS_NO_CONTENT):
            return "No Content";
        case(HTTP_STATUS_BAD_REQUEST):
            return "Bad Request";
        case(HTTP_STATUS_UNAUTHORIZED):
            return "Unauthorized";
        case(HTTP_STATUS_FORBIDDEN):
            return "Forbidden";
        case(HTTP_STATUS_NOT_FOUND):
            return "Not Found";
        case(HTTP_STATUS_INTERNAL_SERVER_ERROR):
            return "Internal Server Error";
        default:
            return NULL;
    }
}

void
Http_free_request(Http_Request* req)
{
    hm_free(&req->headers);
    if (req->path != NULL) free(req->path);
}

#endif //HTTP_IMPLEMENTATION