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

int
Ws_send_response_with_content(int fd, Http_Response* res, Http_ContentType type);

#endif // WEBSERVER_H