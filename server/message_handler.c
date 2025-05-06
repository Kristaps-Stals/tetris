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

msg_sync_lobby_t *make_sync_lobby_msg(server_manager *s_manager) {
    msg_sync_lobby_t *msg = malloc(sizeof(msg_sync_lobby_t));
    msg->player_1 = s_manager->player_1;
    msg->player_2 = s_manager->player_2;
    msg->player_1_ready = s_manager->player_1_ready;
    msg->player_2_ready = s_manager->player_2_ready;
    for (int i = 0; i < 8; i++) {
        const client_t *client = client_manager_get(i);
        memset(&msg->player_names[i], 0, 30);
        if (client == NULL || client->exists == false) {
            sprintf(msg->player_names[i], "(empty)");
            continue;
        }
        sprintf(msg->player_names[i], "%s", client->name);
    }
    return msg;
}

void sync_lobby(server_manager *s_manager) {
    msg_sync_lobby_t *msg_sync = make_sync_lobby_msg(s_manager);
    uint8_t *sync_hdr = make_hdr(sizeof(msg_sync_lobby_t), MSG_SYNC_LOBBY, PLAYER_ID_BROADCAST);

    client_manager_broadcast(sync_hdr, 4, (void*)msg_sync, sizeof(msg_sync_lobby_t));

    free_hdr(sync_hdr);
    free(msg_sync);
}

void disconnect_player(int client_fd, const char* reason, server_manager *s_manager) {
    printf("disconnecting client_fd = %d\n", client_fd);
    uint16_t payload_len = strlen(reason) + 1; // include '\0'

    // send disconnect
    uint8_t *hdr = make_hdr(payload_len, MSG_DISCONNECT, PLAYER_ID_BROADCAST);
    write(client_fd, hdr, 4);
    write(client_fd, reason, payload_len);
    free_hdr(hdr);

    close(client_fd);
    client_manager_remove(client_fd, s_manager);
    sync_lobby(s_manager);
}

void message_handler_handle_hello(int client_fd, server_manager *s_manager) {
    uint8_t hdr[4];
    if (read(client_fd, hdr, 4) != 4) {
        disconnect_player(client_fd, "Header less than 4 bytes", s_manager);
        return;
    }
    uint8_t type = hdr[2];
    if (type != MSG_HELLO) {
        disconnect_player(client_fd, "Invalid message type (is not MSG_HELLO)", s_manager);
        return;
    }

    msg_hello_t hello;
    if (read(client_fd, &hello, sizeof(hello)) != sizeof(hello)) {
        disconnect_player(client_fd, "Invalid hello size", s_manager);
        return;
    }

    // try to add client
    if (client_manager_add(client_fd, hello.player_name) == false) {
        disconnect_player(client_fd, "Server full", s_manager);
        return;
    }
    
    int8_t player_id = get_player_id_from_fd(client_fd);

    // 1) Forward HELLO to existing clients
    // uint8_t forward_hdr[4] = {
    //     hdr[0], hdr[1], MSG_HELLO, id
    // };
    // for (int i = 0; i < client_manager_count(); i++) {
    //     const client_t *c = client_manager_get(i);
    //     write(c->sockfd, forward_hdr, 4);
    //     write(c->sockfd, &hello, sizeof(hello));
    // }

    // Build and send WELCOME
    // int existing = client_manager_count();
    msg_welcome_t welcome;
    welcome.player_id = player_id;
    welcome.game_status = 0;
    memcpy(welcome.player_name, hello.player_name, sizeof(welcome.player_name));

    uint8_t *resp_hdr = make_hdr(sizeof(welcome), MSG_WELCOME, player_id);
    write(client_fd, resp_hdr, 4);
    write(client_fd, &welcome, sizeof(welcome));
    free_hdr(resp_hdr);

    // printf("Sent MSG_WELCOME: player_id=%d, name=%s, length=%d\n", welcome.player_id, welcome.player_name, welcome.length);
    // for (int i = 0; i < existing; i++) {
    //     const client_t *c = client_manager_get(i);
    //     printf("  entry %d: id=%d, ready=%d, name=%s\n", i, c->player_id, c->ready, c->name);
    // }
    // fflush(stdout);

    printf("Added player: player_id=%d, name=%s\n", player_id, hello.player_name);
    sync_lobby(s_manager);
}

void skip_msg(uint16_t length, int client_fd) {
    int toskip = length;
    char buf[256];
    while (toskip > 0) {
        int byte_amount = sizeof buf;
        if (toskip < byte_amount) byte_amount = toskip;
        ssize_t r = read(client_fd, buf, byte_amount);
        if (r <= 0) break;
        toskip -= r;
    }
}

void handle_msg_leave(uint16_t length, int client_fd, server_manager *s_manager) {
    skip_msg(length, client_fd);
    client_manager_remove(client_fd, s_manager);
}

void handle_msg_set_ready(uint16_t length, int client_fd, uint8_t src, server_manager *s_manager) {
    if (length != 3) {
        skip_msg(length, client_fd);
        return;
    }

    uint8_t flag;
    if (read(client_fd, &flag, 1) != 1) {
        client_manager_remove(client_fd, s_manager);
        return;
    }
    // bool ready = (flag != 0);
    // client_manager_set_ready(src, ready);

    uint8_t ready_hdr[4] = {0, 3, MSG_SET_READY, src};
    client_manager_broadcast(ready_hdr, 4, &flag, 1);

    // if (client_manager_count_ready() == 2) {
    //     uint8_t stat_hdr[4]    = {0, 3, MSG_SET_STATUS, PLAYER_ID_BROADCAST};
    //     uint8_t stat_payload[1] = {1};
    //     client_manager_broadcast(stat_hdr, 4, stat_payload, 1);
    // }
}

void handle_msg_toggle_player(uint16_t length, int client_fd, uint8_t src, server_manager *s_manager) {
    (void)src;
    (void)length;
    // skip_msg(length, client_fd);
    int client_id = get_player_id_from_fd(client_fd);
    if (client_id == -1) return;
    // printf("%d!\n", client_id);
    if (s_manager->player_1 == client_id) {
        printf("[message_handler] set player_1 to -1\n");
        s_manager->player_1 = -1;
        s_manager->player_1_ready = 0;
        return;
    }
    if (s_manager->player_2 == client_id) {
        printf("[message_handler] set player_2 to -1\n");
        s_manager->player_2 = -1;
        s_manager->player_2_ready = 0;
        return;
    }

    if (s_manager->player_1 == -1) {
        printf("[message_handler] set player_1 to %d\n", client_id);
        s_manager->player_1 = client_id;
        return;
    }
    if (s_manager->player_2 == -1) {
        printf("[message_handler] set player_2 to %d\n", client_id);
        s_manager->player_2 = client_id;
        return;
    }
}

void message_handler_dispatch(int client_fd, server_manager *s_manager) {
    uint8_t hdr[4];
    ssize_t n = read(client_fd, hdr, sizeof hdr);

    if (n == 0) {
        client_manager_remove(client_fd, s_manager); // peer closed cleanly
        sync_lobby(s_manager);
        return;
    }
    if (n < 0) {
        // skip if no data
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        // real error
        client_manager_remove(client_fd, s_manager);
        sync_lobby(s_manager);
        return;
    }
    if (n < (ssize_t)sizeof hdr) {
        // partial header, give up on this client
        client_manager_remove(client_fd, s_manager);
        sync_lobby(s_manager);
        return;
    }

    uint16_t length = (hdr[0] << 8) | hdr[1];
    uint8_t  type   = hdr[2];
    uint8_t  src    = get_player_id_from_fd(client_fd);
    
    (void)s_manager;

    switch(type) {
        case MSG_LEAVE:
            handle_msg_leave(length, client_fd, s_manager);
            break;
        case MSG_SET_READY:
            handle_msg_set_ready(length, client_fd, src, s_manager);
            break;
        case MSG_TOGGLE_PLAYER:
            handle_msg_toggle_player(length, client_fd, src, s_manager);
            break;
        default:
            skip_msg(length, client_fd);
            break;
    }
    sync_lobby(s_manager);
}
