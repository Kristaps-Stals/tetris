// server/connection.c
#include "connection.h"
#include "client_manager.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int connection_listen(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port)
    };
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, MAX_CLIENTS) < 0) {
        close(fd);
        return -1;
    }

    printf("[connection] Server listening on port %d\n", port);
    return fd;
}




void connection_loop(int listen_fd,
                     void (*on_new)(int, server_manager*),
                     void (*on_data)(int, server_manager*),
                     server_manager *s_manager) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(listen_fd, &rfds);
    int maxfd = listen_fd;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        const client_t *client = client_manager_get(i);
        if (client == NULL || client->exists == false) continue;
        int fd = client->sockfd;
        FD_SET(fd, &rfds);
        if (fd > maxfd) maxfd = fd;
        // printf("[connection] adding %d (max %d)\n", fd, maxfd);
    }

    struct timeval timeout = {0, 0};
    int ready = select(maxfd + 1, &rfds, NULL, NULL, &timeout);
    if (ready < 0) return;

    if (FD_ISSET(listen_fd, &rfds)) {
        struct sockaddr_in cliaddr;
        socklen_t addrlen = sizeof(cliaddr);
        int client_fd = accept(listen_fd,
                               (struct sockaddr*)&cliaddr,
                               &addrlen);
        if (client_fd >= 0) {
            printf("[connection] Accepted %s:%d\n",
                   inet_ntoa(cliaddr.sin_addr),
                   ntohs(cliaddr.sin_port));
            // fcntl(client_fd, F_SETFL, O_NONBLOCK);
            on_new(client_fd, s_manager);
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        const client_t *client = client_manager_get(i);
        if (client == NULL || client->exists == false) continue;
        int fd = client->sockfd;
        if (FD_ISSET(fd, &rfds)) {
            printf("[connection] data recieved from player_id=%d\n", i);
            on_data(fd, s_manager);
        }
    }
}
