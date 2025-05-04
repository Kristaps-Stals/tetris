// server/connection.c
#include "connection.h"
#include "client_manager.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

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

    printf("Server listening on port %d\n", port);
    return fd;
}

void connection_loop(int listen_fd,
                     void (*on_new)(int),
                     void (*on_data)(int)) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(listen_fd, &rfds);
    int maxfd = listen_fd;

    for (int i = 0; i < client_manager_count(); i++) {
        int fd = client_manager_get(i)->sockfd;
        FD_SET(fd, &rfds);
        if (fd > maxfd) maxfd = fd;
    }

    int ready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
    if (ready < 0) return;

    if (FD_ISSET(listen_fd, &rfds)) {
        struct sockaddr_in cliaddr;
        socklen_t addrlen = sizeof(cliaddr);
        int client_fd = accept(listen_fd,
                               (struct sockaddr*)&cliaddr,
                               &addrlen);
        if (client_fd >= 0) {
            printf("Accepted %s:%d\n",
                   inet_ntoa(cliaddr.sin_addr),
                   ntohs(cliaddr.sin_port));
            on_new(client_fd);
        }
        if (--ready <= 0) return;
    }

    int i = 0;
    while (i < client_manager_count() && ready > 0) {
        int fd = client_manager_get(i)->sockfd;
        if (FD_ISSET(fd, &rfds)) {
            on_data(fd);
            if (client_manager_get(i) && client_manager_get(i)->sockfd == fd) {
                i++;  // Only increment if the client was NOT removed.
            }
            ready--;
        } else {
            i++; // increment normally if fd was not set
        }
    }
}
