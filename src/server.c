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

int route_get_root(Request* req) {
    return send_file(req, "static/index.html");
}

int route_get_users(Request* req) {
    return sresponse(req->client_fd, HTTP_HEADER_NOT_IMPLEMENTED);
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
    add_route(&router, &get_root);
    
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

        sem_wait(sem);
        (*connection_count)++;
        sem_post(sem);
        pid_t childId = fork();

        if(childId == 0) {
            char buf[BUFLEN+1];
            rrequest(client_fd, buf);

            Request req = {
                .client_fd = client_fd,
                .method = HTTP_NO_METHOD
            };

            Response res = {};
            req.response = &res;

            ret = parse_request(&req, buf);
            
            if(ret == HTTP_MALFORMED_ERROR) {
                sresponse(req.client_fd, HTTP_HEADER_MALFORMED);
            } else if (ret == HTTP_NOT_IMPLEMENTED_ERROR) {
                sresponse(req.client_fd, HTTP_HEADER_NOT_IMPLEMENTED);
            } else {
                log_request(&req);
                handle_request(&router, &req);
            }

            close(client_fd);

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

    INFO("Stopping server");

    return EXIT_SUCCESS;
}   