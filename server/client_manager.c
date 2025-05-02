// server/client_manager.c
#include "client_manager.h"
#include "message_handler.h"
#include <unistd.h>
#include <string.h>

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
    uint8_t player_id = message_handler_lookup_id(sockfd);

    // Send MSG_LEAVE to all clients before removing
    char reason[] = "Disconnected";
    uint16_t reason_len = strlen(reason) + 1; // including null terminator

    uint16_t payload_len = reason_len;
    uint8_t hdr[4] = {
        (uint8_t)(payload_len >> 8),
        (uint8_t)(payload_len & 0xFF),
        MSG_LEAVE,
        player_id
    };

    client_manager_broadcast(hdr, 4, (uint8_t*)reason, reason_len);

    // Remove the client
    for (int i = 0; i < count; i++) {
        if (clients[i].sockfd == sockfd) {
            close(sockfd);
            message_handler_remove_client(sockfd);
            clients[i] = clients[--count];
            return;
        }
    }
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
