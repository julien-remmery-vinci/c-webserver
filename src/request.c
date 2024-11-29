#include "request.h"
#include "jutils.h"
#include "config.h"
#include "httperror.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/sendfile.h>

int rrequest(int fd, char* buf) {
    int ret = read(fd, buf, BUFLEN);
    CHECK(ret > 0);
    return ret;
}

int sresponse(int fd, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vdprintf(fd, fmt, args);
    va_end(args);
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
    req->version = version;
    return 0;
}

void log_request(Request* req) {
    INFO("%s %s", strmethod(req->method), req->path);
}

int build_response(Request* req) {
    return -1;
}

int send_file(Request* req, char* filepath) {
    int filefd = open(filepath, O_RDONLY);
    if(filefd == -1) {
        sresponse(req->client_fd, HTTP_HEADER_INTERNAL_SERVER_ERROR);
        return 0;
    }
    if(req->response->status == HTTP_NOT_FOUND) {
        sresponse(req->client_fd, HTTP_HEADER_NOT_FOUND);
    } else {
        sresponse(req->client_fd, HTTP_HEADER_OK);
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