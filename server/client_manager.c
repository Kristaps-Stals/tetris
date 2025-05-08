// server/client_manager.c
#include "client_manager.h"
#include "message_handler.h"
#include "server_manager.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>

static client_t clients[MAX_CLIENTS];
static int      count;

void client_manager_init(void) {
    count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        sprintf(clients[i].name, "(empty)");
        clients[i].exists = false;
        clients[i].sockfd = 0;
    }
}

void client_manager_teardown(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].exists) close(clients[i].sockfd);
    }
    count = 0;
}

int client_manager_count(void) {
    return count;
}

const client_t* client_manager_get(int index) {
    if (index < 0 || index >= MAX_CLIENTS) return NULL;
    return &clients[index];
}

int8_t get_player_id_from_fd(int sockfd) {
    for (int8_t i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].exists && clients[i].sockfd == sockfd) return i;
    }
    return -1;
}

int8_t find_free_client() {
    for (int8_t i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].exists == false) {
            return i;
        } 
    };
    return -1;
}

bool client_manager_add(int sockfd, const char *name) {
    int8_t player_id = find_free_client();
    if (player_id == -1) return false;
    printf("[client_manager] adding fd %d, player_id = %d, count %d->%d, name '%s'\n", sockfd, player_id, count, count+1, name);
    clients[player_id].exists = true;
    clients[player_id].sockfd = sockfd;
    memcpy(clients[player_id].name, name, 30);
    count++;
    return true;
}

void client_manager_remove(int sockfd, server_manager *s_manager) {
    int8_t player_id = get_player_id_from_fd(sockfd);
    if (player_id == -1) return;
    printf("[client_manager] removing fd %d, player_id = %d, count %d->%d\n", sockfd, player_id, count, count-1);

    if (s_manager->player_1 == player_id) {
        printf("[client_manager] set player_1 to -1\n");
        s_manager->player_1 = -1;
        s_manager->player_1_ready = false;
    }
    if (s_manager->player_2 == player_id) {
        printf("[client_manager] set player_2 to -1\n");
        s_manager->player_2 = -1;
        s_manager->player_2_ready = false;
    }

    uint8_t *leave_hdr = make_hdr(0, MSG_LEAVE, player_id);
    client_manager_broadcast(leave_hdr, 4, NULL, 0, -1);
    free_hdr(leave_hdr);

    clients[player_id].exists = false;
    close(clients[player_id].sockfd);
    count--;
}



void client_manager_send(const uint8_t *hdr, int hdr_len,
                         const uint8_t *payload, int payload_len,
                         int player_id) {
    const client_t *client = client_manager_get(player_id);
    if (client == NULL || client->exists == false) return;

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(client->sockfd, &wfds);
    struct timeval timeout = {0, 0};
    select(client->sockfd+1, NULL, &wfds, NULL, &timeout);
    if (!FD_ISSET(client->sockfd, &wfds)) return;

    // memcpy(gbuf, hdr, 4);
    // if (payload_len > 0) memcpy(gbuf+4, payload, payload_len);
    // send(client->sockfd, gbuf, hdr_len+payload_len, MSG_NOSIGNAL);
    send(client->sockfd, hdr, hdr_len, MSG_NOSIGNAL);
    if (payload_len > 0){
        send(client->sockfd, payload, payload_len, MSG_NOSIGNAL);
    }
}

void client_manager_broadcast(const uint8_t *hdr, int hdr_len,
                              const uint8_t *payload, int payload_len,
                              int except_socketfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        const client_t *client = client_manager_get(i);
        if (client->sockfd == except_socketfd) continue;
        client_manager_send(hdr, hdr_len, payload, payload_len, i);
    }
}