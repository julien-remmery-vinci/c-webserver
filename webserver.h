#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/time.h>
#include <stddef.h>
#include <semaphore.h>

#define SEM_NAME "/sem_connection_count"
#define WS_BUFFER_MAX_LENGHT 2048

typedef struct Ws_parse_result {
    bool error;
    union {
        int int_val;
    };
} Ws_parse_result;

#define WS_DEFAULT_CONFIG_FILE_NAME "default_config.ini"
#define WS_CONFIG_MAX_LINE_LENGTH 256
#define WS_CONFIG_MAX_KEY_LENGTH 64
#define WS_CONFIG_MAX_VALUE_LENGTH 128
#define WS_CONFIG_DEFAULT_PORT 3000
#define WS_CONFIG_DEFAULT_BACKLOG 10
#define WS_CONFIG_DEFAULT_MAX_CONNECTIONS 10

typedef struct HashMap Ws_Config;

/**
 * Load server configuration from file at given path
 * if path is NULL, loads config from 'WS_CONFIG_FILE_NAME' file
 */
Ws_Config
Ws_load_config_from_file(const char* path);

/**
 * Get value for a given property key
 *  This function is a possible exit point if the given key
 *  was not found in the config file
 */
void*
Ws_config_get_value(Ws_Config* config, const char* key);

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

#define HTTP_MALFORMED_ERROR -1
#define HTTP_NOT_IMPLEMENTED_ERROR -2

/**
 * Response struct conataining the informations about a Response
 */
typedef struct Response {
    enum HttpStatus status;
    char* header;
} Response;

/**
 * Request struct conataining the informations about a Request
 */
typedef struct Request {
    int client_fd;
    enum HttpMethod method;
    char* path;
    char* http_version;
    struct timeval start;
    struct timeval end;
    double request_timing;
    Response response;
} Request;

typedef struct Route Route;

/**
 * Route struct conataining the informations about a Route
 */
struct Route {
    enum HttpMethod method;
    char* path;
    int (*handler)(Route* route, Request* request);
    char *file_buffer;
    size_t file_size;
};

/**
 * Router struct containg the informations about a router
 */
typedef struct Ws_Router {
    HashMap routes;
} Ws_Router;

/**
 * Add a route to handle to a router
 */
void 
Ws_router_handle(
    Ws_Router* router, 
    char* path, 
    enum HttpMethod method, 
    int (*handler)(Route* route, Request* request));

/**
 * Server struct containing all informations about the server
 */
typedef struct Ws_Server {
    int* connection_count;
    int connection_count_shm_id;
    sem_t* connection_count_sem;
    int sock_fd;
    int max_connections;
    Ws_Config config;
    Ws_Router router;
} Ws_Server;

/**
 * Server setup function
 */
Ws_Server
Ws_server_setup(Ws_Config config, Ws_Router router);

/**
 * Run the server
 */
int
Ws_run_server(Ws_Server* server);

#endif // WEBSERVER_H

/**
 * Web server implementation
 */
#ifdef WEBSERVER_IMPLEMENTATION

#include <ctype.h>
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

/**
 * Send an http response
 */
int
Ws_send_response(int fd, Response* res);

/**
 * Send a file to a client
 */
int
Ws_send_file(Request* req, char* filepath);

// Handles the stopping of the server when SIGINT is encountered, through 'sigint_handler()'
volatile sig_atomic_t stop_server = 0;

/**
 * Parses a int value from str
 *  Ws_parse_result.error set to true if can't parse the str
 *      true otherwise and the int_val is set to the parsed int
 */
Ws_parse_result
Ws_parse_int(const char* str)
{
    Ws_parse_result result = {
        .error = true
    };
    if (str == NULL || *str == '\0') {
        return result;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return result;
        }
    }

    result.int_val = atoi(str);
    result.error = false;
    return result;
}

/**
 * Utility function for config file parsing
 * Trims whitespace from a string
 */
char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str; // Empty string
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

/**
 * Utility funciton to parse a config file into a Ws_config struct
 */
void 
Ws_parse_config(Ws_Config* config, FILE* input_file)
{
    char line[WS_CONFIG_MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), input_file)) {
        char *trimmed_line = trim_whitespace(line);

        if (*trimmed_line == '\0' 
            || *trimmed_line == ';' 
            || *trimmed_line == '#' 
            || *trimmed_line == '[') continue;

        // Parse key-value pairs
        char *equals = strchr(trimmed_line, '=');
        if (equals) {
            *equals = '\0';
            char *key = trim_whitespace(trimmed_line);
            char *value = trim_whitespace(equals + 1);
            hm_put(config, key, value);
        } else {
            ERROR("Config parsing: Malformed line: %s\n", trimmed_line);
        }
    }
}

Ws_Config
Ws_default_config()
{
    FILE* config_file = fopen(WS_DEFAULT_CONFIG_FILE_NAME, "r");
    if(config_file == NULL)
    {
        ERROR("Error: missing mandatory default config file: %s", WS_DEFAULT_CONFIG_FILE_NAME);
        exit(EXIT_FAILURE);
    }
    Ws_Config config = hm_create(HM_DEFAULT_SIZE);
    Ws_parse_config(&config, config_file);
    fclose(config_file);
    return config;
}

Ws_Config
Ws_load_config_from_file(const char* path)
{
    FILE* config_file = fopen(path, "r");
    if(config_file == NULL)
    {
        ERROR("Error: missing config file\nUsing default config file");
        return Ws_default_config();
    }
    Ws_Config config = hm_create(HM_DEFAULT_SIZE);
    Ws_parse_config(&config, config_file);
    fclose(config_file);
    return config;
}

void*
Ws_config_get_value(Ws_Config* config, const char* key)
{
    void* value = hm_get(config, key);
    if (value == NULL)
    {
        ERROR("Error: property not found, %s", key);
        exit(EXIT_FAILURE);
    }
    return value;
}

/**
 * Default server root route
 */
int
default_server_route(Route* route, Request* req)
{
    req->response = HTTP_RES_OK;
    Ws_send_response(req->client_fd, &req->response);

    if (route->file_buffer != NULL && route->file_size > 0) {
        if (send(req->client_fd, route->file_buffer, route->file_size, 0) == -1) {
            ERROR("Error sending file to : %d", req->client_fd);
        }
        return 0;
    }
    Ws_send_file(req, "static/default.html");
    return 0;
}

/**
 * Default server router used when no router is provided
 */
Ws_Router
Ws_default_router()
{
    Ws_Router router = {0};
    router.routes = hm_create(1);
    Ws_router_handle(&router, "/", HTTP_GET, default_server_route);
    return router;
}

/**
 * Signal handler for SIGINT
 *  handles the stopping of the server
 */
void
sigint_handler(int signum)
{
    (void)signum;
    stop_server = 1;
}

/**
 * Add a signal handler
 */
int
Ws_handle_signal(int signum, void (*handler)(int signum))
{
    struct sigaction action;
    action.sa_handler = handler;
    int ret = sigfillset(&action.sa_mask);
    CHECK(ret == 0, "Ws_handle_signal :sigfillset error");
    action.sa_flags = 0;
    ret = sigaction(signum, &action, NULL);
    CHECK(ret == 0, "Ws_handle_signal :sigaction error");
    return ret;
}

/**
 * Initializes the server socket
 */
void
Ws_init_server_socket(Ws_Server* server)
{
    int ret;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(sockfd > 0, "Ws_init_server_socket : socket error");
    
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

    char* str_config_port = Ws_config_get_value(&server->config, "port");
    Ws_parse_result port = Ws_parse_int(str_config_port);
    if(port.error) port.int_val = WS_CONFIG_DEFAULT_PORT;

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port.int_val);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    CHECK(ret == 0, "Ws_init_server_socket : bind error");

    char* str_backlog = Ws_config_get_value(&server->config, "backlog");
    Ws_parse_result backlog = Ws_parse_int(str_backlog);
    if(backlog.error) backlog.int_val = WS_CONFIG_DEFAULT_BACKLOG;

    ret = listen(sockfd, backlog.int_val);
    CHECK(ret == 0, "Ws_init_server_socket : listen error");

    server->sock_fd = sockfd;
}

Ws_Server
Ws_server_setup(Ws_Config config, Ws_Router router)
{
    INFO("Starting server setup");
    Ws_Server server = {0};
    if(hm_isempty(config)) server.config = Ws_default_config();
    else server.config = config;

    if(hm_isempty(router.routes)) server.router = Ws_default_router();
    else server.router = router;

    int shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    CHECK(shm_id > 0, "Ws_server_setup : shmget error");
    server.connection_count_shm_id = shm_id;

    int* connection_count = (int *)shmat(shm_id, NULL, 0);
    CHECK(*connection_count != -1, "Ws_server_setup : shmat error");

    server.connection_count = connection_count;
    *server.connection_count = 0;

    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    CHECK(sem != SEM_FAILED, "Ws_server_setup : sem_open failed");
    server.connection_count_sem = sem;

    char* str_max_conn = Ws_config_get_value(&server.config, "max_conn");
    Ws_parse_result max_conn = Ws_parse_int(str_max_conn);
    if(max_conn.error) max_conn.int_val = WS_CONFIG_DEFAULT_MAX_CONNECTIONS;

    server.max_connections = max_conn.int_val;

    Ws_handle_signal(SIGINT, sigint_handler);

    Ws_init_server_socket(&server);
    INFO("Server setup done");
    return server;
}

/**
 * Returns the enum equivalent of an http method
 *  HTTP_NO_METHOD if invalid
 */
enum HttpMethod 
get_http_method(char* str)
{
    if(!strcmp(str, "GET")) return HTTP_GET;
    if(!strcmp(str, "POST")) return HTTP_POST;
    if(!strcmp(str, "PUT")) return HTTP_PUT;
    if(!strcmp(str, "PATCH")) return HTTP_PATCH;
    if(!strcmp(str, "DELETE")) return HTTP_DELETE;
    if(!strcmp(str, "HEAD")) return HTTP_HEAD;
    if(!strcmp(str, "OPTIONS")) return HTTP_OPTIONS;
    return HTTP_NO_METHOD;
}

/**
 * Returns the str equivalent of an http method
 */
char*
strmethod(enum HttpMethod method)
{
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

/**
 * Returns the str equivalent of an http status
 */
char*
strstatus(enum HttpStatus status)
{
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

/**
 * Read a request on a file descriptor
 */
int
Ws_read_request(int fd, char* buf) 
{
    int ret = read(fd, buf, WS_BUFFER_MAX_LENGHT);
    CHECK(ret > 0, "read empty request");
    return ret;
}

int
Ws_send_response(int fd, Response* res)
{
    write(fd, res->header, strlen(res->header));
    return 0;
}

/**
 * Parse an http request
 */
int
Ws_parse_request(Request* req, char* reqstr)
{
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

/**
 * Log a request: time in ms, status, http method, path
 */
void
Ws_log_request(Request* req) 
{
    INFO("%8.3f ms %-3d %-6s %s", req->request_timing, req->response.status, strmethod(req->method), req->path);
}

int
Ws_send_file(Request* req, char* filepath)
{
    int filefd = open(filepath, O_RDONLY);
    if(filefd == -1) {
        Ws_send_response(req->client_fd, &HTTP_RES_INTERNAL_SERVER_ERROR);
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

Route*
Ws_create_route(
    char* path, 
    enum HttpMethod method, 
    int (*handler)(Route* route, Request* request)
)
{
    Route* route = malloc(sizeof(Route));
    CHECK(route != NULL, "route alloc error");
    route->path = path;
    route->method = method;
    route->handler = handler;
    return route;
}

void 
Ws_router_handle(
    Ws_Router* router, 
    char* path, 
    enum HttpMethod method, 
    int (*handler)(Route* route, Request* request)
)
{
    CHECK(path != NULL, "add handler null path");
    CHECK(method >= 0 && method < HTTP_NO_METHOD, "add handler wrong method");
    
    Route* route = Ws_create_route(path, method, handler);

    StringBuilder builder = {0};
    Ju_str_append_null(&builder, strmethod(method), path);
    hm_put(&router->routes, builder.items, route);
    Ju_str_free(&builder);
}

/**
 * Handle a request on an unsupported route
 */
void
handle_not_found_request(Request* request)
{
    request->response = HTTP_RES_NOT_FOUND;
    Ws_send_response(request->client_fd, &request->response);
    Ws_send_file(request, "static/not_found.html");
}

/**
 * Handle a request
 */
int
Ws_handle_request(Ws_Router* router, Request* request)
{
    StringBuilder builder = {0};
    Ju_str_append_null(&builder, strmethod(request->method), request->path);
    Route* route = hm_get(&router->routes, builder.items);
    Ju_str_free(&builder);
    if (route == NULL)
    {
        handle_not_found_request(request);
        return 0;
    }
    else
    {
        return route->handler(route, request);
    }
}

/**
 * Time the start of a request
 */
void
Ws_start_request(Request* request)
{
    gettimeofday(&request->start, NULL);
}

/**
 * Time the end of a request
 */
void
Ws_end_request(Request* request)
{
    gettimeofday(&request->end, NULL);
    request->request_timing =
        (double)((request->end.tv_sec - request->start.tv_sec) * 1000000 + request->end.tv_usec - request->start.tv_usec) / 1000;
}

int
Ws_run_server(Ws_Server* server)
{
    int ret;
    INFO("Server ready, STOP with CTRL+C");

    while(!stop_server) {
        sem_wait(server->connection_count_sem);
        if(*server->connection_count == server->max_connections) {
            continue;
        }
        sem_post(server->connection_count_sem);
        int client_fd = accept(server->sock_fd, NULL, NULL);
        if(client_fd < 0) {
            continue;
        }

        Request request = {
            .client_fd = client_fd,
            .method = HTTP_NO_METHOD
        };

        Ws_start_request(&request);

        sem_wait(server->connection_count_sem);
        (*server->connection_count)++;
        sem_post(server->connection_count_sem);
        pid_t childId = fork();

        if(childId == 0) {
            char buf[WS_BUFFER_MAX_LENGHT+1];
            Ws_read_request(client_fd, buf);

            ret = Ws_parse_request(&request, buf);
            
            if(ret == HTTP_MALFORMED_ERROR) {
                Ws_send_response(request.client_fd, &HTTP_RES_HEADER_MALFORMED);
            } else if (ret == HTTP_NOT_IMPLEMENTED_ERROR) {
                Ws_send_response(request.client_fd, &HTTP_RES_NOT_IMPLEMENTED);
            } else {
                Ws_handle_request(&server->router, &request);
            }

            // Close connection
            close(client_fd);

            Ws_end_request(&request);
            Ws_log_request(&request);

            sem_wait(server->connection_count_sem);
            (*server->connection_count)--;
            sem_post(server->connection_count_sem);

            sem_close(server->connection_count_sem);
            ret = shmdt(server->connection_count);
            CHECK(ret == 0, "Ws_run_server : child shmdt error");
            exit(0);
        } else {
            close(client_fd);
        }
    }

    int status;
    while (wait(&status) > 0);

    sem_close(server->connection_count_sem);
    sem_unlink(SEM_NAME);
    shmdt(server->connection_count);
    shmctl(server->connection_count_shm_id, IPC_RMID, NULL);

    for (size_t i = 0; i < server->router.routes.size; ++i)
    {
        HashMapEntry* entry = server->router.routes.entries[i];
        if (entry != NULL)
        {
            free(entry->value);
        }
    }

    INFO("Stopping server");

    return EXIT_SUCCESS;
}

#endif // WEBSERVER_IMPLEMENTATION