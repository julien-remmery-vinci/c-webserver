#include "http.h"

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

    if (req->method == HTTP_METHOD_GET) return HTTP_OK;

    if(hm_exists(&req->headers, "Content-Type")) {
        char* ctype = hm_get(&req->headers, "Content-Type");
        if (strcmp(ctype, "application/json") == 0) {
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
    (void)path;
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
    case HTTP_METHOD_HEAD:
        return "HEAD";
    case HTTP_METHOD_OPTIONS:
        return "OPTIONS";
    case HTTP_METHOD_INVALID:
    default:
        return NULL;
    }
}

char*
Http_strstatus(Http_Status status)
{
    switch(status) {
        case HTTP_STATUS_OK:
            return "Success";
        case HTTP_STATUS_NO_CONTENT:
            return "No Content";
        case HTTP_STATUS_BAD_REQUEST:
            return "Bad Request";
        case HTTP_STATUS_UNAUTHORIZED:
            return "Unauthorized";
        case HTTP_STATUS_FORBIDDEN:
            return "Forbidden";
        case HTTP_STATUS_NOT_FOUND:
            return "Not Found";
        case HTTP_STATUS_NOT_ALLOWED:
            return "Not Allowed";
        case HTTP_STATUS_INTERNAL_SERVER_ERROR :
            return "Internal Server Error";
        case HTTP_STATUS_NOT_IMPLEMENTED:
            return "Not Implemented";
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

const char*
Http_get_status_header(Http_Status status)
{
    switch (status) {
        case HTTP_STATUS_OK:
            return HTTP_HEADER_OK;
        case HTTP_STATUS_NO_CONTENT:
            return HTTP_HEADER_NO_CONTENT;
        case HTTP_STATUS_BAD_REQUEST:
            return HTTP_HEADER_BAD_REQUEST;
        case HTTP_STATUS_UNAUTHORIZED:
            return HTTP_HEADER_UNAUTHORIZED;
        case HTTP_STATUS_FORBIDDEN:
            return HTTP_HEADER_FORBIDDEN;
        case HTTP_STATUS_NOT_FOUND:
            return HTTP_HEADER_NOT_FOUND;
        case HTTP_STATUS_NOT_ALLOWED:
            return HTTP_HEADER_NOT_ALLOWED;
        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
            return HTTP_HEADER_INTERNAL_SERVER_ERROR;
        case HTTP_STATUS_NOT_IMPLEMENTED:
            return HTTP_HEADER_NOT_IMPLEMENTED;
        default:
            return NULL;
    }
}

const char*
Http_get_content_type(Http_ContentType type)
{
    switch (type) {
        case HTTP_CONTENTTYPE_JSON:
            return "application/json";
        case HTTP_CONTENTTYPE_UNSUPPORTED:
        default:
            return NULL;
    }
}