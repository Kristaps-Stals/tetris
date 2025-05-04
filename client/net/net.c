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
    uint16_t length = 1 + 1 + payload_size; // type + player_id + payload
    uint8_t *buffer = malloc(2 + length);
    if (!buffer) return -1;

    // length in big-endian
    buffer[0] = (length >> 8) & 0xFF;
    buffer[1] = length & 0xFF;
    buffer[2] = type;
    buffer[3] = player_id;

    if (payload_size > 0 && payload) {
        memcpy(buffer + 4, payload, payload_size);
    }

    int sent = send(sockfd, buffer, 2 + length, 0);
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
    uint8_t hdr[4];
    ssize_t r = read(sockfd, hdr, 4);
    if (r != 4) return -1;
    uint16_t length = (hdr[0] << 8) | hdr[1];
    *out_type   = hdr[2];
    *out_source = hdr[3];

    uint16_t payload_len = length - 2;
    if (payload_len > 0 && out_payload && out_payload_size) {
        r = read(sockfd, out_payload, payload_len);
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
            sleep(2);
        } else {
            fcntl(sockfd, F_SETFL, O_NONBLOCK);
            manager->server_socket = sockfd;
            send_hello(sockfd, "TetrisClient 1.0", "PlayerOne");
            open_menu(manager, make_lobby_menu());
            update_lobby_menu(manager);
        }
    }

    if (ip_text)   free(ip_text);
    if (port_text) free(port_text);
}

void handle_msg_welcome(menu_manager *mgr, uint8_t *buf) {
    msg_welcome_t *w = (msg_welcome_t*)buf;
    for(int i=0; i<8; i++) 
        strcpy(mgr->slot_names[i], "(empty)");
    int me = w->player_id - 1;
    strncpy(mgr->slot_names[me], w->player_name, 31);
    uint8_t *p = buf + sizeof(*w);
    for(int i = 0; i < w->length && i < MAX_CLIENTS; i++) {
        uint8_t pid = p[0];
        char *nm = (char*)(p+2);
        strncpy(mgr->slot_names[pid-1], nm, 31);
        p += 1 + 1 + 30;
    }
}

void handle_msg_hello(menu_manager *mgr, uint8_t *buf, uint8_t src) {
    msg_hello_t *h = (msg_hello_t*)buf;
    strncpy(mgr->slot_names[src-1], h->player_name, 31);
}

void handle_msg_leave(menu_manager *mgr, uint8_t src) {
    strcpy(mgr->slot_names[src - 1], "(empty)");
}

void handle_msg_disconnect(menu_manager *mgr, uint8_t src) {
    strcpy(mgr->slot_names[src-1], "(empty)");
}

void handle_msg_set_ready(menu_manager *mgr, uint8_t *buf, uint8_t src) {
    bool ready = buf[0];
    mgr->slot_ready[src - 1] = ready;
}

void handle_msg(menu_manager *mgr, uint8_t type, uint8_t src, uint16_t psz, uint8_t *buf) {
    bool lobby_updated = false;
    (void)psz; // unused for now?

    switch(type) {
        case MSG_WELCOME: {
            handle_msg_welcome(mgr, buf);
            lobby_updated = true;
            break;
        }
        case MSG_HELLO: {
            handle_msg_hello(mgr, buf, src);
            lobby_updated = true;
            break;
        }
        case MSG_LEAVE: {
            handle_msg_leave(mgr, src);
            lobby_updated = true;
            break;
        }
        case MSG_DISCONNECT: {
            handle_msg_disconnect(mgr, src);
            lobby_updated = true;
            break;
        }
        case MSG_SET_READY: {
            handle_msg_set_ready(mgr, buf, src);
            lobby_updated = true;
            break;
        }
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
    uint8_t buf[512];
    while (recv_message(mgr->server_socket, &type, &src, buf, &psz) == 0) {
        handle_msg(mgr, type, src, psz, buf);
    }
}

// handles all incoming lobby-related messages and updates the menu_manager
// returns true if the lobby state was updated and needs a redraw
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