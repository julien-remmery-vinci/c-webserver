#define _GNU_SOURCE // For sigaction incomplete struct error
#define DATA_STRUCTURES_IMPLEMENTATION
#include "data_structures.h"
#define JUTILS_IMPLEMENTATION
#include "jutils.h"
#define WEBSERVER_IMPLEMENTATION
#include "webserver.h"

volatile sig_atomic_t stop_server = 0;

int
init_server()
{
    int ret;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(sockfd > 0, "socket error");
    
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    CHECK(ret == 0, "bind error");

    ret = listen(sockfd, BACKLOG);
    CHECK(ret == 0, "listen error");

    return sockfd;
}

void
sigint_handler(int signum)
{
    (void)signum;
    stop_server = 1;
}

void
sigchld_handler(int signum)
{
    (void)signum;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int
handle_signal(int signum, void (*handler)(int signum))
{
    struct sigaction action;
    action.sa_handler = handler;
    int ret = sigfillset(&action.sa_mask);
    CHECK(ret == 0, "sigfillset error");
    action.sa_flags = 0;
    ret = sigaction (signum, &action, NULL);
    CHECK(ret == 0, "sigaction error");
    return ret;
}

// TODO : refactor to use new system
// void preload_file(Route* route, const char *filename) {
//     FILE *fp = fopen(filename, "rb");
//     if (fp == NULL) {
//         perror("Error opening file");
//         exit(EXIT_FAILURE);
//     }

//     fseek(fp, 0, SEEK_END);
//     route->file_size = ftell(fp);
//     rewind(fp);

//     route->file_buffer = (char *)malloc(route->file_size);
//     if (route->file_buffer == NULL) {
//         perror("Memory allocation failed");
//         exit(EXIT_FAILURE);
//     }

//     fread(route->file_buffer, 1, route->file_size, fp);
//     fclose(fp);

//     printf("File preloaded: %s %zu bytes\n", filename, route->file_size);
// }

int
route_get_root(Route* route, Request* req)
{
    req->response = HTTP_RES_OK;
    Ws_send_response(req->client_fd, &req->response);

    if (route->file_buffer != NULL && route->file_size > 0) {
        if (send(req->client_fd, route->file_buffer, route->file_size, 0) == -1) {
            perror("Error sending file");
        }
        return 0;
    }
    Ws_send_file(req, "static/index.html");
    return 0;
}

int
route_get_favicon(Route* route, Request* req)
{
    (void)route;
    req->response = HTTP_RES_NOT_FOUND;
    Ws_send_response(req->client_fd, &req->response);
    return 0;
}

int
route_get_users(Route* route, Request* req)
{
    (void)route;
    req->response = HTTP_RES_NOT_IMPLEMENTED;
    Ws_send_response(req->client_fd, &req->response);
    return 0;
}

Router
setup_router()
{
    Router router = {0};
    // Initialize the map to store routes with a size of 10
    router.routes = hm_create(HM_DEFAULT_SIZE);

    Ws_router_handle(&router, "/", HTTP_GET, route_get_root);
    Ws_router_handle(&router, "/favicon.ico", HTTP_GET, route_get_favicon);
    Ws_router_handle(&router, "/users", HTTP_GET, route_get_users);
    return router;
}

int 
main(void)
{
    int ret, shm_id;
    int *connection_count;
    sem_t *sem;

    handle_signal(SIGINT, sigint_handler);
    handle_signal(SIGCHLD, sigchld_handler);

    shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    connection_count = (int *)shmat(shm_id, NULL, 0);
    if (connection_count == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    *connection_count = 0;

    sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    INFO("Starting server");

    int sockfd = init_server();

    Router router = setup_router();

    INFO("Server ready");

    while(!stop_server) {
        sem_wait(sem);
        if(*connection_count == MAX_CONNECTIONS) {
            continue;
        }
        sem_post(sem);
        int client_fd = accept(sockfd, NULL, NULL);
        if(client_fd < 0) {
            continue;
        }

        Request request = {
            .client_fd = client_fd,
            .method = HTTP_NO_METHOD
        };

        printf("====================\n");
        Ws_start_request(&request);

        sem_wait(sem);
        (*connection_count)++;
        sem_post(sem);
        pid_t childId = fork();

        if(childId == 0) {
            char buf[BUFLEN+1];
            Ws_read_request(client_fd, buf);

            ret = Ws_parse_request(&request, buf);
            
            if(ret == HTTP_MALFORMED_ERROR) {
                Ws_send_response(request.client_fd, &HTTP_RES_HEADER_MALFORMED);
            } else if (ret == HTTP_NOT_IMPLEMENTED_ERROR) {
                Ws_send_response(request.client_fd, &HTTP_RES_NOT_IMPLEMENTED);
            } else {
                Ws_handle_request(&router, &request);
            }

            // Close connection
            close(client_fd);

            Ws_end_request(&request);
            Ws_log_request(&request);

            sem_wait(sem);
            (*connection_count)--;
            sem_post(sem);

            sem_close(sem);
            shmdt(connection_count);
            exit(0);
        } else {
            close(client_fd);
        }
    }

    int status;
    while (wait(&status) > 0);

    sem_close(sem);
    sem_unlink(SEM_NAME);
    shmdt(connection_count);
    shmctl(shm_id, IPC_RMID, NULL);

    for (size_t i = 0; i < router.routes.size; ++i) 
    {
        HashMapEntry* entry = router.routes.entries[i];
        if (entry != NULL)
        {
            free(entry->value);
        }
    }

    INFO("Stopping server");

    return EXIT_SUCCESS;
}   