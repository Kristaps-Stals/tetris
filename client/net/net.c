#include "net.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <../shared/protocol.h>
#include <fcntl.h>
#include <sys/time.h>
#include "../menus/menu_maker.h"
#include "../state_manager.h"
#include "../tetris/board.h"
#include "../menus/settings.h"

char *copy_text(const char *src);

int connect_to_server(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(ip)
    };

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int send_message(int sockfd, uint8_t type, uint8_t player_id, const void *payload, uint16_t payload_size) {
    if (sockfd < 0) return -1;

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    struct timeval timeout = {0, 0};
    select(sockfd+1, NULL, &wfds, NULL, &timeout);
    if (!FD_ISSET(sockfd, &wfds)) return -1;

    uint16_t length = 4 + payload_size; // header + payload
    uint8_t *buffer = malloc(length);
    if (!buffer) return -1;

    // length in big-endian
    buffer[0] = (payload_size >> 8) & 0xFF;
    buffer[1] = payload_size & 0xFF;
    buffer[2] = type;
    buffer[3] = player_id;

    if (payload_size > 0 && payload) {
        memcpy(buffer + 4, payload, payload_size);
    }

    int sent = send(sockfd, buffer, length, MSG_NOSIGNAL);
    // write(sockfd, buffer, length);
    free(buffer);
    return sent;
}

int send_hello(int sockfd, const char *client_id, const char *player_name) {
    msg_hello_t msg = {0};
    strncpy(msg.client_id, client_id, sizeof(msg.client_id));
    strncpy(msg.player_name, player_name, sizeof(msg.player_name));
    return send_message(sockfd, MSG_HELLO, PLAYER_ID_BROADCAST, &msg, sizeof(msg));
}

int recv_message(int sockfd, uint8_t *out_type, uint8_t *out_source, void *out_payload, uint16_t *out_payload_size) {
    if (sockfd < 0) return -1;

    uint8_t hdr[4];
    ssize_t r = recv(sockfd, hdr, 4, MSG_NOSIGNAL);
    if (r != 4) return -1;
    uint16_t length = (hdr[0] << 8) | hdr[1];
    *out_type   = hdr[2];
    *out_source = hdr[3];

    uint16_t payload_len = length;
    if (payload_len > 0 && out_payload && out_payload_size) {
        r = recv(sockfd, out_payload, payload_len, MSG_NOSIGNAL);
        if (r != payload_len) return -1;
    }
    *out_payload_size = payload_len;
    return 0;
}

int parse_connection_args(int argc, char **argv, const char **host, int *port) {
    *host = "127.0.0.1";
    *port = 0;

    int opt;
    while ((opt = getopt(argc, argv, "p:h:")) != -1) {
        switch (opt) {
            case 'p':
                *port = atoi(optarg);
                break;
            case 'h':
                *host = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -p <port> [-h <host>]\n", argv[0]);
                return 0;
        }
    }
    return 1;
}

char* fetch_text_from_element(menu_manager *manager, int write_id, int *length) {
    textbox **stack = manager->stack;
    int top = manager->top;
    for (int i = 0; i < stack[top]->element_count; i++) {
        if (stack[top]->elements[i]->type == WRITE_ELEMENT_ID) {
            textbox_write *info = stack[top]->elements[i]->info;
            if (info->write_id != write_id || info->text == NULL) continue;
            *length = info->curr_len;
            return copy_text(info->text);
        }
    }
    *length = 0;
    return NULL;
}

void attempt_join_lobby(menu_manager *manager) {
    int ip_len, port_len;
    char *ip_text   = fetch_text_from_element(manager, WRITE_ID_JOIN_IP,   &ip_len);
    char *port_text = fetch_text_from_element(manager, WRITE_ID_JOIN_PORT, &port_len);

    if (ip_text && port_text) {
        int port = atoi(port_text);
        int sockfd = connect_to_server(ip_text, port);
        if (sockfd < 0) {
            mvprintw(0, 0, "Failed to connect to %s:%d", ip_text, port);
            refresh();
            sleep(1);
        } else {
            fcntl(sockfd, F_SETFL, O_NONBLOCK);
            manager->server_socket = sockfd;
            send_hello(sockfd, "TetrisClient 1.0", get_nickname());
            open_menu(manager, make_lobby_menu());
            update_lobby_menu(manager);
        }
    }

    if (ip_text)   free(ip_text);
    if (port_text) free(port_text);
}

void handle_msg_welcome(menu_manager *mgr, uint8_t *buf) {
    msg_welcome_t *msg = (msg_welcome_t*)buf;
    for(int i=0; i<8; i++){
        sprintf(mgr->slot_names[i], "(empty)");
    }
    // int me = w->player_id;
    // sprintf(mgr->slot_names[me], "%s", w->player_name);
    mgr->player_id = msg->player_id;
    // mvprintw(1, 1, "%d", mgr->player_id);
}

void handle_msg_hello(menu_manager *mgr, uint8_t *buf, uint8_t src) {
    msg_hello_t *h = (msg_hello_t*)buf;
    strncpy(mgr->slot_names[src-1], h->player_name, 31);
}

void handle_msg_leave(menu_manager *mgr, uint8_t src) {
    (void)mgr;
    (void)src;
    // strcpy(mgr->slot_names[src - 1], "(empty)");
}

void handle_msg_disconnect(menu_manager *mgr, uint8_t src) {
    (void)src;
    if (mgr->stack[mgr->top]->id == LOBBY_MENU_ID) pop_menu_stack(mgr);
}

void handle_msg_sync_lobby(menu_manager *mgr, uint8_t *buf, uint8_t src) {
    (void)src;
    msg_sync_lobby_t *msg = (msg_sync_lobby_t*)buf;

    for (int i = 0; i < 8; i++) {  
        strcpy(mgr->slot_names[i], msg->player_names[i]);
    }
    mgr->player_1 = msg->player_1;
    mgr->player_2 = msg->player_2;
    mgr->player_1_ready = msg->player_1_ready;
    mgr->player_2_ready = msg->player_2_ready;
    mgr->start_counter = msg->start_counter;
}

void handle_msg_start_game(menu_manager *mgr, uint8_t *buf) {
    msg_start_game_t *msg = (msg_start_game_t*)buf;
    mgr->player_1 = msg->player_1;
    mgr->player_2 = msg->player_2;
    mgr->bag_seed = msg->bag_seed;
    start_game_versus(mgr->parent);
}

void handle_msg_sync_board(menu_manager *mgr, uint8_t *buf) {
    msg_sync_board_t *msg = (msg_sync_board_t*)buf;
    state_manager *s_manager = (state_manager*)mgr->parent;
    if (s_manager->state != STATE_GAME_VERSUS) {
        s_manager->menu_manager->player_1 = msg->player_1;
        s_manager->menu_manager->player_2 = msg->player_2;
        s_manager->menu_manager->bag_seed = msg->start_bag_seed;
        start_game_versus(s_manager);
    }
    
    if (msg->player_id == mgr->player_1 && mgr->player_id != mgr->player_1) {
        apply_sync_board_msg(s_manager->board_1, msg);
    }
    if (msg->player_id == mgr->player_2 && mgr->player_id != mgr->player_2) {
        apply_sync_board_msg(s_manager->board_2, msg);
    }
}

void handle_msg_send_garbage(menu_manager *mgr, uint8_t *buf, uint8_t garbage) {
    (void)buf;
    state_manager *s_manager = mgr->parent;
    
    if (s_manager->board_1->is_controlled) {
        add_garbage(s_manager->board_1, garbage);
    }
    if (s_manager->board_2->is_controlled) {
        add_garbage(s_manager->board_2, garbage);
    }
}

void handle_msg_winner(menu_manager *mgr, uint8_t *buf) {
    msg_winner_t *msg = (msg_winner_t *)buf;
    // if (src >= 8) return;
    state_manager *s_manager = mgr->parent;
    handle_winner_versus(s_manager, msg);
}

void req_lobby_sync(menu_manager *mgr) {
    if (mgr->server_socket <= 0) return;
    send_message(mgr->server_socket, MSG_REQ_LOBBY, PLAYER_ID_BROADCAST, NULL, 0);
}

// static int tmp = 0;
void handle_msg(menu_manager *mgr, uint8_t type, uint8_t src, uint16_t psz, uint8_t *buf) {
    bool lobby_updated = false;
    (void)psz; // unused for now?
    // mvprintw(2, 2, "%d, %d", tmp, type);
    // refresh();
    // tmp++;

    switch(type) {
        case MSG_WELCOME:
            handle_msg_welcome(mgr, buf);
            lobby_updated = true;
            break;
        case MSG_HELLO:
            handle_msg_hello(mgr, buf, src);
            lobby_updated = true;
            break;
        case MSG_LEAVE:
            handle_msg_leave(mgr, src);
            lobby_updated = true;
            break;
        case MSG_DISCONNECT:
            handle_msg_disconnect(mgr, src);
            lobby_updated = true;
            break;
        case MSG_SYNC_LOBBY:
            handle_msg_sync_lobby(mgr, buf, src);
            lobby_updated = true;
            break;
        case MSG_START_GAME:
            handle_msg_start_game(mgr, buf);
            break;
        case MSG_SYNC_BOARD:
            handle_msg_sync_board(mgr, buf);
            break;
        case MSG_SEND_GARBAGE:
            handle_msg_send_garbage(mgr, buf, src);
            break;
        case MSG_WINNER:
            handle_msg_winner(mgr, buf);
            break;
        default:
            break;
    }

    if (lobby_updated && mgr->top >= 0 && mgr->stack[mgr->top]->id == LOBBY_MENU_ID) {
        update_lobby_menu(mgr);
    }
}

void recieve_all_messages(menu_manager *mgr) {
    uint8_t type, src;
    uint16_t psz;
    uint8_t buf[1024];

    while (recv_message(mgr->server_socket, &type, &src, buf, &psz) == 0) {
        handle_msg(mgr, type, src, psz, buf);
    }
}

// handles all incoming messages
void process_server_messages(menu_manager *mgr) {
    if (mgr->server_socket < 0) return;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(mgr->server_socket, &read_fds);

    struct timeval timeout = {0, 0};
    int activity = select(mgr->server_socket + 1, &read_fds, NULL, NULL, &timeout);

    if (activity > 0 && FD_ISSET(mgr->server_socket, &read_fds)) {
        recieve_all_messages(mgr);
    }
}