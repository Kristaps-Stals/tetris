// server/main.c
#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "Port must be 1024–65535\n");
        return 1;
    }

    if (server_init(port) != 0) {
        fprintf(stderr, "Failed to initialize server on port %d\n", port);
        return 1;
    }

    server_run();
    server_shutdown();
    return 0;
}
