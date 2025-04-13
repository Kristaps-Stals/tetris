#include "net.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <../shared/protocol.h>

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
