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

// Helper to fetch text from a textbox write element (moved from menu_maker)
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

// Moved from menu_maker.c
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
            open_menu(manager, make_lobby_menu(manager));
        }
    }

    if (ip_text)   free(ip_text);
    if (port_text) free(port_text);
}

