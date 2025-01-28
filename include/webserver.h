#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "jutils.h"
#include "http.h"
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

typedef struct Route Route;
typedef int (*Ws_Handler)(Route* route, Http_Request* request, Http_Response* res);

/**
 * Route struct conataining the informations about a Route
 */
struct Route {
    Http_Method method;
    char* path;
    Ws_Handler handler;
    Ws_Handler middleware; // Maybe make it so we can have multiple midllewares
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
    Http_Method method, 
    Ws_Handler handler,
    Ws_Handler middleware
);

/**
 * Server struct containing all informations about the server
 */
typedef struct Ws_Server {
    int* connection_count;
    int connection_count_shm_id;
    sem_t* connection_count_sem;
    int sock_fd;
    int max_connections;
    bool requests_logging;
    Ws_Config config;
    Ws_Router router;
} Ws_Server;

/**
 * Server setup function
 */
Ws_Server
Ws_server_setup(Ws_Config config, Ws_Router router);

/**
 * Enable requests logging on the server
 */
bool
Ws_server_enable_logging(Ws_Server* server);

/**
 * Disable requests logging on the server
 */
bool
Ws_server_disable_logging(Ws_Server* server);

/**
 * Run the server
 */
int
Ws_run_server(Ws_Server* server);

/**
 * Send an http response
 */
int
Ws_send_response(int fd, Http_Response* res);

int
Ws_send_response_with_file(int fd, Http_Response* res, const char* filepath);

#endif // WEBSERVER_H

/**
 * Web server implementation
 */
#ifdef WEBSERVER_IMPLEMENTATION

#define HTTP_IMPLEMENTATION

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
default_server_route(Route* route, Http_Request* req, Http_Response* res)
{
    (void)route;
    Ws_send_response_with_file(req->client_fd, res, "static/default.html");
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
    Ws_router_handle(&router, "/", HTTP_METHOD_GET, default_server_route, NULL);
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
Ws_send_response(int fd, Http_Response* res)
{
    const char* res_header = Http_get_status_header(res->status);

    StringBuilder builder = {0};
    Ju_str_append_null(&builder, res_header, "\r\n");

    write(fd, builder.string, builder.count);

    Ju_str_free(&builder);
    return 0;
}

int
Ws_send_response_with_file(int fd, Http_Response* res, const char* filepath)
{
    int filefd = open(filepath, O_RDONLY);
    if(filefd == -1) {
        // File paths are specified by the developper
        // And therefore should exist
        // We are not letting users say which file they want
        res->status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        Ws_send_response(fd, res);
        return 0;
    }
    Ws_send_response(fd, res);

    struct stat stat_buf;
    fstat(filefd, &stat_buf);

    StringBuilder builder = {0};
    Ju_str_append_fmt_null(&builder, "Content-Type: text/html\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", stat_buf.st_size);
    write(fd, builder.string, builder.count);

    off_t offset = 0;
    ssize_t bytes_sent = sendfile(fd, filefd, &offset, stat_buf.st_size);
    if (bytes_sent == -1) {
        perror("sendfile");
        return -1;
    }

    close(filefd);
    Ju_str_free(&builder);
    return 0;
}

/**
 * Log a request: time in ms, status, http method, path
 */
void
Ws_log_request(Http_Request* req, Http_Response* res) 
{
    INFO("%8.3f ms %-3d %-6s %s", req->request_timing, res->status, Http_strmethod(req->method), req->path);
}

Route*
Ws_create_route()
{
    Route* route = malloc(sizeof(Route));
    CHECK(route != NULL, "route alloc error");
    return route;
}

void 
Ws_router_handle(
    Ws_Router* router, 
    char* path, 
    Http_Method method, 
    Ws_Handler handler,
    Ws_Handler middleware
)
{
    CHECK(path != NULL, "add handler null path");
    CHECK(method >= 0 && method < HTTP_METHOD_INVALID, "add handler wrong method");
    
    Route* route = Ws_create_route(path, method, handler);
    route->path = path;
    route->method = method;
    route->handler = handler;
    route->middleware = middleware;

    StringBuilder builder = {0};
    Ju_str_append_null(&builder, Http_strmethod(method), path);
    hm_put(&router->routes, builder.string, route);
    Ju_str_free(&builder);
}

bool
Ws_server_enable_logging(Ws_Server* server)
{
  if (server == NULL) return false;
  server->requests_logging = true;
  return true;
}

bool
Ws_server_disable_logging(Ws_Server* server)
{
  if (server == NULL) return false;
  server->requests_logging = false;
  return true;
}

/**
 * Handle a request on an unsupported route
 */
void
handle_not_found_request(Http_Request* request, Http_Response* res)
{
    res->status = HTTP_STATUS_NOT_FOUND;
    Ws_send_response_with_file(request->client_fd, res, "static/not_found.html");
}

/**
 * Handle a request
 */
int
Ws_handle_request(Ws_Router* router, Http_Request* req, Http_Response* res)
{
    StringBuilder builder = {0};
    Ju_str_append_null(&builder, Http_strmethod(req->method), req->path);
    Route* route = hm_get(&router->routes, builder.string);
    Ju_str_free(&builder);
    if (route == NULL)
    {
        handle_not_found_request(req, res);
        return 0;
    }
    else
    {
        if (route->middleware != NULL) {
            if (route->middleware(route, req, res)) {
                return 0;
            }
        }
        return route->handler(route, req, res);
    }
}

/**
 * Time the start of a request
 */
void
Ws_start_request(Http_Request* request)
{
    gettimeofday(&request->start, NULL);
}

/**
 * Time the end of a request
 */
void
Ws_end_request(Http_Request* request)
{
    gettimeofday(&request->end, NULL);
    request->request_timing =
        (double)((request->end.tv_sec - request->start.tv_sec) * 1000000 + request->end.tv_usec - request->start.tv_usec) / 1000;
}

int
Ws_run_server(Ws_Server* server)
{
    int ret;
    // size_t max_request_len = Ws_config_get_value(&server->config, "max_req_size");

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

        Http_Request req = {
            .client_fd = client_fd,
            .method = HTTP_METHOD_INVALID
        };
        Http_Response res = {0};

        Ws_start_request(&req);

        sem_wait(server->connection_count_sem);
        (*server->connection_count)++;
        sem_post(server->connection_count_sem);
        pid_t childId = fork();

        if(childId == 0) {
            char buf[WS_BUFFER_MAX_LENGHT+1];
            Ws_read_request(client_fd, buf);

            ret = Http_parse_request(&req, buf, WS_BUFFER_MAX_LENGHT+1);
            
            if(ret == HTTP_ERR_MALFORMED_REQ) {
                res.status = HTTP_STATUS_BAD_REQUEST;
                res.content = "Malformed header in the request";
                Ws_send_response(req.client_fd, &res);
            } else {
                Ws_handle_request(&server->router, &req, &res);
            }

            // Close connection
            shutdown(client_fd, SHUT_RDWR);
            close(client_fd);

            Ws_end_request(&req);
            Ws_log_request(&req, &res);

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