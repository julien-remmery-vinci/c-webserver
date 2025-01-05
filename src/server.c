#include "jutils.h"
#include "http.h"
#include "request.h"
#include "router.h"
#include "config.h"
#include "httperror.h"
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t stop_server = 0;

int init_server() {
    int ret;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(sockfd > 0);
    
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    CHECK(ret == 0);

    ret = listen(sockfd, BACKLOG);
    CHECK(ret == 0);

    return sockfd;
}

void sigint_handler(int signum) {
    stop_server = 1;
}

void sigchld_handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int handle_signal(int signum, void (*handler)(int signum)) {
    struct sigaction action;
    action.sa_handler = handler;
    int ret = sigfillset(&action.sa_mask);
    CHECK(ret == 0);
    action.sa_flags = 0;
    ret = sigaction (signum, &action, NULL);
    CHECK(ret == 0);
    return ret;
}

void preload_file(Route* route, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    route->file_size = ftell(fp);
    rewind(fp);

    route->file_buffer = (char *)malloc(route->file_size);
    if (route->file_buffer == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    fread(route->file_buffer, 1, route->file_size, fp);
    fclose(fp);

    printf("File preloaded: %s %zu bytes\n", filename, route->file_size);
}

int route_get_root(Route* route, Request* req) {
    req->response = HTTP_RES_OK;
    sresponse(req->client_fd, &req->response);

    if (route->file_buffer != NULL && route->file_size > 0) {
        if (send(req->client_fd, route->file_buffer, route->file_size, 0) == -1) {
            perror("Error sending file");
        }
        return 0;
    }
    send_file(req, "static/index.html");
    return 0;
}

int route_get_favicon(Route* route, Request* req) {
    sresponse(req->client_fd, &HTTP_RES_NOT_FOUND);
    return 0;
}

int route_get_users(Route* route, Request* req) {
    sresponse(req->client_fd, &HTTP_RES_NOT_IMPLEMENTED);
    return 0;
}

int main(void) {
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

    Router router = {
        .routes = NULL,
        .nb_routes = 0
    };

    Route get_root = {
        .method = HTTP_GET,
        .path = "/",
        .handler = route_get_root
    };
    preload_file(&get_root, "static/index.html");
    add_route(&router, &get_root);

    Route get_favicon = {
        .method = HTTP_GET,
        .path = "/favicon.ico",
        .handler = route_get_favicon
    };
    add_route(&router, &get_favicon);
    
    Route get_users = {
        .method = HTTP_GET,
        .path = "/users",
        .handler = route_get_users
    };
    add_route(&router, &get_users);

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

        Request req = {
            .client_fd = client_fd,
            .method = HTTP_NO_METHOD
        };

        printf("====================\n");
        gettimeofday(&req.start, NULL);

        sem_wait(sem);
        (*connection_count)++;
        sem_post(sem);
        pid_t childId = fork();

        if(childId == 0) {
            char buf[BUFLEN+1];
            rrequest(client_fd, buf);

            ret = parse_request(&req, buf);
            
            if(ret == HTTP_MALFORMED_ERROR) {
                sresponse(req.client_fd, &HTTP_RES_HEADER_MALFORMED);
            } else if (ret == HTTP_NOT_IMPLEMENTED_ERROR) {
                sresponse(req.client_fd, &HTTP_RES_NOT_IMPLEMENTED);
            } else {
                handle_request(&router, &req);
            }

            // Close connection
            close(client_fd);

            gettimeofday(&req.stop, NULL);
            log_request(&req);

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

    for (int i = 0; i < router.nb_routes; ++i) {
        if (router.routes[i].file_buffer != NULL) {
            free(router.routes[i].file_buffer);
        }
    }

    INFO("Stopping server");

    return EXIT_SUCCESS;
}   