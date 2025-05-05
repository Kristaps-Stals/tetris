// server/client_manager.c
#include "client_manager.h"
#include "message_handler.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static client_t clients[MAX_CLIENTS];
static int      count;

void client_manager_init(void) {
    count = 0;
}

void client_manager_teardown(void) {
    for (int i = 0; i < count; i++) {
        close(clients[i].sockfd);
    }
    count = 0;
}

int client_manager_count(void) {
    return count;
}

const client_t* client_manager_get(int index) {
    if (index < 0 || index >= count) return NULL;
    return &clients[index];
}

void client_manager_add(int sockfd, uint8_t player_id, const char *name) {
    if (count >= MAX_CLIENTS) return;
    clients[count].sockfd    = sockfd;
    clients[count].player_id = player_id;
    memcpy(clients[count].name, name, 30);
    clients[count].ready     = false;
    count++;
}

void client_manager_remove(int sockfd) {
    printf("[client_manager] removing fd %d (count was %d)\n", sockfd, count);
    // 1) find & save the player_id (for your LEAVE broadcast)
    uint8_t player_id = 0;
    for (int i = 0; i < count; i++) {
      if (clients[i].sockfd == sockfd) {
        player_id = clients[i].player_id;
        break;
      }
    }

    // 2) broadcast the zero‑payload LEAVE if we have a valid id
    if (player_id != 0) {
      uint8_t leave_hdr[4] = { 0, 2, MSG_LEAVE, player_id };
      client_manager_broadcast(leave_hdr, 4, NULL, 0);
    }

    // 3) remove sockfd from clients[]
    for (int i = 0; i < count; i++) {
      if (clients[i].sockfd == sockfd) {
        close(clients[i].sockfd);
        clients[i] = clients[--count];
        break;
      }
    }

    // 4) and finally remove it from your fd_map
    message_handler_remove_client(sockfd);
}

void client_manager_broadcast(const uint8_t *hdr, int hdr_len,
                              const uint8_t *payload, int payload_len) {
    for (int i = 0; i < count; i++) {
        write(clients[i].sockfd, hdr, hdr_len);
        if (payload_len > 0)
            write(clients[i].sockfd, payload, payload_len);
    }
}

void client_manager_set_ready(uint8_t player_id, bool ready) {
    for (int i = 0; i < count; i++) {
        if (clients[i].player_id == player_id) {
            clients[i].ready = ready;
            return;
        }
    }
}

int client_manager_count_ready(void) {
    int r = 0;
    for (int i = 0; i < count; i++)
        if (clients[i].ready) r++;
    return r;
}
