#ifndef REQUEST_H
#define REQUEST_H

#include "http.h"

typedef struct Response {
    char* version;
    enum HttpStatus status;
} Response;

typedef struct Request {
    int client_fd;
    enum HttpMethod method;
    char* path;
    char* version;
    Response* response;
} Request;

int rrequest(int fd, char* buf);
int sresponse(int fd, char* fmt, ...);
int parse_request(Request* req, char* reqstr);
void log_request(Request* req);
int build_response(Request* req);
int send_file(Request* req, char* filepath);

#endif // REQUEST_H