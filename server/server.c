// server/server.c
#include "server.h"
#include "connection.h"
#include "client_manager.h"
#include "message_handler.h"
#include <signal.h>
#include <unistd.h>

static int listen_fd;

int server_init(int port) {
    // Don’t die on SIGPIPE if a client disconnects mid‐write
    signal(SIGPIPE, SIG_IGN);

    listen_fd = connection_listen(port);
    if (listen_fd < 0) return -1;

    client_manager_init();
    message_handler_init();
    return 0;
}

void server_run(void) {
    while (1) {
        connection_loop(
            listen_fd,
            message_handler_handle_hello,
            message_handler_dispatch
        );
    }
}

void server_shutdown(void) {
    client_manager_teardown();
    close(listen_fd);
}
