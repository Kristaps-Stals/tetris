// server/message_handler.c
#include "message_handler.h"
#include "client_manager.h"
#include "../shared/protocol.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "server_manager.h"
// #include "../client/net/net.h" 
// TODO: MOVE TO SHARED. you can implement building payloads not with write, but with send_message() from net.h

static uint8_t next_id = 1;
static struct { int fd; uint8_t player_id; } fd_map[MAX_CLIENTS];
static int map_count;

static uint8_t find_free_id(void) {
    for (uint8_t cand = 1; cand <= MAX_CLIENTS; cand++) {
        bool used = false;
        for (int i = 0; i < map_count; i++) {
            if (fd_map[i].player_id == cand) {
                used = true;
                break;
            }
        }
        if (!used) return cand;
    }
    return 0;  // no slots free
}

void message_handler_init(void) {
    next_id   = 1;
    map_count = 0;
}

uint8_t message_handler_lookup_id(int fd) {
    for (int i = 0; i < map_count; i++)
        if (fd_map[i].fd == fd)
            return fd_map[i].player_id;
    return 0;
}

void message_handler_remove_client(int fd) {
    for (int i = 0; i < map_count; i++) {
        if (fd_map[i].fd == fd) {
            fd_map[i] = fd_map[--map_count];
            return;
        }
    }
}

msg_sync_lobby_t *make_sync_lobby_msg(server_manager *s_manager) {
    msg_sync_lobby_t *msg = malloc(sizeof(msg_sync_lobby_t));
    msg->player_1 = s_manager->player_1;
    msg->player_2 = s_manager->player_2;
    msg->player_1_ready = 0;
    msg->player_2_ready = 0;
    for (int i = 0; i < 8; i++) {
        const client_t *client = client_manager_get(i);
        if (client == NULL) {
            sprintf(msg->player_names[i], "%s", "NULL");
            continue;
        }
        sprintf(msg->player_names[i], "%s", client->name);
    }
    return msg;
}

void message_handler_handle_hello(int client_fd) {
    uint8_t hdr[4];
    if (read(client_fd, hdr, 4) != 4) {
        client_manager_remove(client_fd);
        return;
    }
    uint8_t type = hdr[2];
    if (type != MSG_HELLO) {
        client_manager_remove(client_fd);
        return;
    }

    msg_hello_t hello;
    if (read(client_fd, &hello, sizeof(hello)) != sizeof(hello)) {
        client_manager_remove(client_fd);
        return;
    }

    // pick the first free slot (1..MAX_CLIENTS)
    uint8_t id = find_free_id();
    if (id == 0) {
        const char *reason = "Server full";
        uint16_t payload_len = strlen(reason) + 1; // include '\0'
        uint16_t length = 1 + 1 + payload_len;    // type + source + payload

        uint8_t hdr[4] = {
            (uint8_t)(length >> 8),
            (uint8_t)(length & 0xFF),
            MSG_DISCONNECT,
            PLAYER_ID_BROADCAST
        };
        // send the disconnect
        write(client_fd, hdr, sizeof hdr);
        write(client_fd, reason, payload_len);

        close(client_fd);
        return;
    }

    // store mapping
    fd_map[map_count].fd         = client_fd;
    fd_map[map_count].player_id  = id;
    map_count++;

    // 1) Forward HELLO to existing clients
    uint8_t forward_hdr[4] = {
        hdr[0], hdr[1], MSG_HELLO, id
    };
    for (int i = 0; i < client_manager_count(); i++) {
        const client_t *c = client_manager_get(i);
        write(c->sockfd, forward_hdr, 4);
        write(c->sockfd, &hello, sizeof(hello));
    }

    // 2) Build and send WELCOME
    int existing = client_manager_count();
    msg_welcome_t welcome = {
        .player_id   = id,
        .game_status = 0,
        .length      = existing,
    };
    memcpy(welcome.player_name, hello.player_name, sizeof(welcome.player_name));

    uint16_t payload_len = sizeof(welcome) + existing * (1 + 1 + 30);
    uint8_t resp_hdr[4] = {
        (uint8_t)(payload_len >> 8),
        (uint8_t)(payload_len & 0xFF),
        MSG_WELCOME,
        id
    };
    write(client_fd, resp_hdr, 4);
    write(client_fd, &welcome, sizeof(welcome));

    // send each existing client’s (id, ready, name)
    for (int i = 0; i < existing; i++) {
        const client_t *c = client_manager_get(i);
        uint8_t entry[1 + 1 + 30];
        entry[0] = c->player_id;
        entry[1] = c->ready ? 1 : 0;
        memcpy(entry + 2, c->name, 30);
        write(client_fd, entry, sizeof(entry));
    }

    printf("Sent MSG_WELCOME: player_id=%d, name=%s, length=%d\n", welcome.player_id, welcome.player_name, welcome.length);
    for (int i = 0; i < existing; i++) {
        const client_t *c = client_manager_get(i);
        printf("  entry %d: id=%d, ready=%d, name=%s\n", i, c->player_id, c->ready, c->name);
    }
    fflush(stdout);

    // 3) register newcomer
    client_manager_add(client_fd, id, hello.player_name);
}

void skip_msg(uint16_t length, int client_fd) {
    int toskip = length - 2;
    char buf[256];
    while (toskip > 0) {
        int byte_amount = sizeof buf;
        if (toskip < byte_amount) byte_amount = toskip;
        ssize_t r = read(client_fd, buf, byte_amount);
        if (r <= 0) break;
        toskip -= r;
    }
}

void handle_msg_leave(uint16_t length, int client_fd) {
    skip_msg(length, client_fd);
    client_manager_remove(client_fd);
}

void handle_msg_set_ready(uint16_t length, int client_fd, uint8_t src) {
    if (length != 3) {
        skip_msg(length, client_fd);
        return;
    }

    uint8_t flag;
    if (read(client_fd, &flag, 1) != 1) {
        client_manager_remove(client_fd);
        return;
    }
    bool ready = (flag != 0);
    client_manager_set_ready(src, ready);

    uint8_t ready_hdr[4] = {0, 3, MSG_SET_READY, src};
    client_manager_broadcast(ready_hdr, 4, &flag, 1);

    if (client_manager_count_ready() == 2) {
        uint8_t stat_hdr[4]    = {0, 3, MSG_SET_STATUS, PLAYER_ID_BROADCAST};
        uint8_t stat_payload[1] = {1};
        client_manager_broadcast(stat_hdr, 4, stat_payload, 1);
    }
}

void handle_msg_make_player(uint16_t length, int client_fd, uint8_t src, server_manager *s_manager) {
    if (s_manager->player_1 == -1) {
        
        return;
    }
    if (s_manager->player_2 == -1) {

        return;
    }
}

void message_handler_dispatch(int client_fd, server_manager *s_manager) {
    uint8_t hdr[4];
    ssize_t n = read(client_fd, hdr, sizeof hdr);

    if (n == 0) {
        client_manager_remove(client_fd); // peer closed cleanly
        return;
    }
    if (n < 0) {
        // skip if no data
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        // real error
        client_manager_remove(client_fd);
        return;
    }
    if (n < (ssize_t)sizeof hdr) {
        // partial header, give up on this client
        client_manager_remove(client_fd);
        return;
    }

    uint16_t length = (hdr[0] << 8) | hdr[1];
    uint8_t  type   = hdr[2];
    uint8_t  src    = message_handler_lookup_id(client_fd);
    
    (void)s_manager;

    switch(type) {
        case MSG_LEAVE:
            handle_msg_leave(length, client_fd);
            break;
        case MSG_SET_READY:
            handle_msg_set_ready(length, client_fd, src);
            break;
        case MSG_MAKE_PLAYER:
            handle_msg_make_player(length, client_fd, src, s_manager);
            break;
        default:
            skip_msg(length, client_fd);
            break;
    }
}
