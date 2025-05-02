// server/message_handler.c
#include "message_handler.h"
#include "client_manager.h"
#include "../shared/protocol.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static uint8_t next_id = 1;
static struct { int fd; uint8_t player_id; } fd_map[MAX_CLIENTS];
static int map_count;

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

void message_handler_handle_hello(int client_fd) {
    uint8_t hdr[4];
    if (read(client_fd, hdr, 4) != 4) {
        close(client_fd);
        return;
    }
    uint8_t type = hdr[2];
    if (type != MSG_HELLO) {
        close(client_fd);
        return;
    }

    msg_hello_t hello;
    if (read(client_fd, &hello, sizeof(hello)) != sizeof(hello)) {
        close(client_fd);
        return;
    }

    uint8_t id = next_id++;
    if (id == 0) id = next_id = 1;

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

void message_handler_dispatch(int client_fd) {
    uint8_t hdr[4];
    if (read(client_fd, hdr, 4) != 4) {
        client_manager_remove(client_fd);
        return;
    }
    uint16_t length = (hdr[0] << 8) | hdr[1];
    uint8_t  type   = hdr[2];
    uint8_t  src    = message_handler_lookup_id(client_fd);

    if (type == MSG_SET_READY && length == 3) {
        uint8_t flag;
        if (read(client_fd, &flag, 1) != 1) {
            client_manager_remove(client_fd);
            return;
        }
        bool ready = (flag != 0);
        client_manager_set_ready(src, ready);

        // broadcast SET_READY
        uint8_t ready_hdr[4] = {0, 3, MSG_SET_READY, src};
        client_manager_broadcast(ready_hdr, 4, &flag, 1);

        // if exactly two are ready → start game
        if (client_manager_count_ready() == 2) {
            uint8_t stat_hdr[4]   = {0, 3, MSG_SET_STATUS, PLAYER_ID_BROADCAST};
            uint8_t stat_payload[1] = {1};
            client_manager_broadcast(stat_hdr, 4, stat_payload, 1);
            // TODO: send initial SYNC_BOARD
        }
    } else {
        // skip over unknown payload
        int toskip = length - 2;
        char buf[256];
        while (toskip > 0) {
            int r = read(client_fd, buf, toskip < sizeof(buf) ? toskip : sizeof(buf));
            if (r <= 0) break;
            toskip -= r;
        }
    }
}
