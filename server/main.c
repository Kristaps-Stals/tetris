#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <../shared/protocol.h>

int main(int argc, char **argv) {
    int sockfd = -1;
    int client_sockfd = -1;

    // parse port
    int port = 0;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port < 1024 || port > 65535) {
            fprintf(stderr, "Invalid port number. Must be between 1024 and 65535.\n");
            return 1;
        }   
    } else {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // create tcp socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        goto cleanup;
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        goto cleanup;
    }

    printf("Server listening on port %d\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sockfd < 0) {
        perror("accept");
        goto cleanup;
    }

    printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    uint8_t header[4];
    if (read(client_sockfd, header, 4) != 4) {
        perror("read header");
        goto cleanup;
    }

    uint16_t length = (header[0] << 8) | header[1];
    uint8_t msg_type = header[2];
    uint8_t player_id = header[3];

    printf("Received message type: 0x%02X from ID: %d with length: %d\n", msg_type, player_id, length);

    if (msg_type == MSG_HELLO) {
        msg_hello_t hello;
        if (read(client_sockfd, &hello, sizeof(hello)) != sizeof(hello)) {
            perror("read hello payload");
            goto cleanup;
        }

        printf("HELLO received!\n");
        printf("Client ID: %.20s\n", hello.client_id);
        printf("Player Name: %.30s\n", hello.player_name);

        // TODO: send WELCOME response here
    } else {
        printf("Unexpected message type: 0x%02X\n", msg_type);
    }

cleanup:
    if (client_sockfd >= 0) close(client_sockfd);
    if (sockfd >= 0) close(sockfd);
    return 0;
}
