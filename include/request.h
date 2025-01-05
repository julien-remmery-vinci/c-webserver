#ifndef REQUEST_H
#define REQUEST_H

#include "http.h"
#include <sys/time.h>

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

#endif // REQUEST_H