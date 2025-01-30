#ifndef HTTP_H
#define HTTP_H

#include "hashmap.h"
#include "jacon.h"
#include "jutils.h"
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

#define HTTP_HEADER_OK "HTTP/1.1 200 OK"
#define HTTP_HEADER_NO_CONTENT "HTTP/1.1 204 No Content"
#define HTTP_HEADER_BAD_REQUEST "HTTP/1.1 400 Bad Request"
#define HTTP_HEADER_UNAUTHORIZED "HTTP/1.1 401 Unauthorized"
#define HTTP_HEADER_FORBIDDEN "HTTP/1.1 403 Forbidden"
#define HTTP_HEADER_NOT_FOUND "HTTP/1.1 404 Not Found"
#define HTTP_HEADER_NOT_ALLOWED "HTTP/1.1 405 Not Allowed"
#define HTTP_HEADER_INTERNAL_SERVER_ERROR "HTTP/1.1 500 Internal Server Error"
#define HTTP_HEADER_NOT_IMPLEMENTED "HTTP/1.1 501 Not Implemented"

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
    HTTP_CONTENTTYPE_JSON,
    HTTP_CONTENTTYPE_UNSUPPORTED,
} Http_ContentType;

typedef struct HashMap Http_Headers;

/**
 * Response struct conataining the informations about a Response
 */
typedef struct Http_Response {
    Http_Status status;
    char* content;
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

/**
 * Get the header string representation of a status code
 */
const char*
Http_get_status_header(Http_Status status);

/**
 * Get string representation of a content type value
 */
const char*
Http_get_content_type(Http_ContentType type);

#endif // HTTP_H