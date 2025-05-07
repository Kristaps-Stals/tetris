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

int recv_payload(int sockfd, uint16_t length, void *out_payload) {
    if (sockfd < 0) return -1;
    ssize_t r;
    if (length > 0) {
        r = read(sockfd, out_payload, length);
        if (r != length) return -1;
    }
    return 0;
}

void skip_msg(uint16_t length, int client_fd) {
    int toskip = length;
    char buf[1024];
    while (toskip > 0) {
        int byte_amount = sizeof buf;
        if (toskip < byte_amount) byte_amount = toskip;
        ssize_t r = read(client_fd, buf, byte_amount);
        if (r <= 0) break;
        toskip -= r;
    }
}

msg_sync_lobby_t *make_sync_lobby_msg(server_manager *s_manager) {
    msg_sync_lobby_t *msg = malloc(sizeof(msg_sync_lobby_t));
    msg->player_1 = s_manager->player_1;
    msg->player_2 = s_manager->player_2;
    msg->player_1_ready = s_manager->player_1_ready;
    msg->player_2_ready = s_manager->player_2_ready;
    msg->start_counter = s_manager->last_time_char_sent;
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

    client_manager_broadcast(sync_hdr, 4, (void*)msg_sync, sizeof(msg_sync_lobby_t), -1);

    free_hdr(sync_hdr);
    free(msg_sync);
}

// sus
void disconnect_player(int client_fd, const char* reason, server_manager *s_manager) {
    printf("[message_handler] disconnecting client_fd = %d\n", client_fd);
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

    printf("[message_handler] Added player: player_id=%d, name=%s\n", player_id, hello.player_name);
    sync_lobby(s_manager);
}

void handle_msg_leave(uint16_t length, int client_fd, server_manager *s_manager) {
    skip_msg(length, client_fd);
    client_manager_remove(client_fd, s_manager);
    sync_lobby(s_manager);
}

void handle_msg_toggle_ready(uint16_t length, int client_fd, server_manager *s_manager) {
    skip_msg(length, client_fd);
    int8_t player_id = get_player_id_from_fd(client_fd);
    if (player_id == -1) return;

    bool changed = false;
    if (s_manager->player_1 == player_id) {
        s_manager->player_1_ready = !s_manager->player_1_ready;
        changed = true;
        printf("[message_handler] player_1_ready %d->%d\n", !s_manager->player_1_ready, s_manager->player_1_ready);
    }
    if (s_manager->player_2 == player_id) {
        s_manager->player_2_ready = !s_manager->player_2_ready;
        changed = true;
        printf("[message_handler] player_2_ready %d->%d\n", !s_manager->player_2_ready, s_manager->player_2_ready);
    }

    if (changed) sync_lobby(s_manager);
}

void handle_msg_toggle_player(uint16_t length, int client_fd, server_manager *s_manager) {
    skip_msg(length, client_fd);
    int client_id = get_player_id_from_fd(client_fd);
    if (client_id == -1) return;
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

void handle_msg_sync_board(uint16_t length, int client_fd) {
    msg_sync_board_t msg;
    if (recv_payload(client_fd, length, &msg) != 0) return;
    printf("[message_handler] sending board sync, player_id=%d, score=%d\n", msg.player_id, msg.counters.score);
    uint8_t *hdr = make_hdr(sizeof(msg_sync_board_t), MSG_SYNC_BOARD, PLAYER_ID_BROADCAST);
    client_manager_broadcast(hdr, 4, (void*)&msg, sizeof(msg_sync_board_t), client_fd);
    free_hdr(hdr);
}

void handle_msg_send_garbage(uint16_t length, int client_fd, server_manager *s_manager) {
    msg_send_garbage_t msg;
    if (recv_payload(client_fd, length, &msg) != 0) return;

    int8_t player_id = get_player_id_from_fd(client_fd);
    printf("[message_handler] garbage from player_id=%d, garbage=%d", player_id, msg.garbage_amount);
    if (player_id == s_manager->player_1) {
        // from player 1 to player 2
        uint8_t *hdr = make_hdr(sizeof(msg_send_garbage_t), MSG_SEND_GARBAGE, PLAYER_ID_BROADCAST);
        client_manager_send(hdr, 4, (void*)&msg, sizeof(msg_send_garbage_t), s_manager->player_2);
        free_hdr(hdr);
    }

    if (player_id == s_manager->player_2) {
        // from player 2 to player 1
        uint8_t *hdr = make_hdr(sizeof(msg_send_garbage_t), MSG_SEND_GARBAGE, PLAYER_ID_BROADCAST);
        client_manager_send(hdr, 4, (void*)&msg, sizeof(msg_send_garbage_t), s_manager->player_1);
        free_hdr(hdr);
    }
}

void handle_msg_req_board(int client_fd, server_manager *s_manager) {
    (void)s_manager;
    int8_t player_id = get_player_id_from_fd(client_fd);
    printf("[message_handler] req_board player_id=%d\n", player_id);
}

void handle_msg_set_lose(int client_fd, server_manager *s_manager) {
    int8_t loser_id = get_player_id_from_fd(client_fd);
    int8_t winner_id = -1;

    if (s_manager->player_1 == loser_id) {
        winner_id = s_manager->player_2;
    }
    if (s_manager->player_2 == loser_id) {
        winner_id = s_manager->player_1;
    }

    if (winner_id == -1) return;
    declare_winner_versus(s_manager, winner_id);
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

    (void)src;

    switch(type) {
        case MSG_LEAVE:
            handle_msg_leave(length, client_fd, s_manager);
            break;
        case MSG_TOGGLE_READY:
            handle_msg_toggle_ready(length, client_fd, s_manager);
            break;
        case MSG_TOGGLE_PLAYER:
            handle_msg_toggle_player(length, client_fd, s_manager);
            sync_lobby(s_manager);
            break;
        case MSG_SYNC_BOARD:
            handle_msg_sync_board(length, client_fd);
            break;
        case MSG_SEND_GARBAGE:
            handle_msg_send_garbage(length, client_fd, s_manager);
            break;
        case MSG_REQ_BOARD:
            handle_msg_req_board(client_fd, s_manager);
            break;
        case MSG_SET_LOSE:
            handle_msg_set_lose(client_fd, s_manager);
            break;
        default:
            skip_msg(length, client_fd);
            break;
    }
}
