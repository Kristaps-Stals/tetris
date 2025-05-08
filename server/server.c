// server/server.c
#define _XOPEN_SOURCE 700 // dont know what it does, but without it sigaction highlighting doesnt work
#include "server.h"
#include "connection.h"
#include "client_manager.h"
#include "message_handler.h"
#include <signal.h>
#include <unistd.h>
#include "server_manager.h"
#include <time.h>

const int frame_time = 25; // desired time between frames in ms, too low causes flickering
const struct timespec sleeptime = {0, frame_time*1e6};

// converts timespec struct into a long long
int64_t time_ll(struct timespec *x) {
    return (int64_t)1e9*x->tv_sec+x->tv_nsec;
}

// returns delta time in microseconds
int64_t get_delta_micro_s(struct timespec *now, struct timespec *bef) {
    return (int64_t)(time_ll(now)-time_ll(bef))/1e3;
}

static int listen_fd;

int server_init(int port) {
    // Don’t die on SIGPIPE if a client disconnects mid‐write
    signal(SIGPIPE, SIG_IGN);

    listen_fd = connection_listen(port);
    if (listen_fd < 0) return -1;

    client_manager_init();
    return 0;
}

void server_run(void) {
    server_manager *s_manager = make_server_manager();

    struct timespec now, last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t delta_time = get_delta_micro_s(&now, &last_time);
        last_time = now;
        if (delta_time == 0) delta_time = 1;

        handle_server(s_manager, delta_time);
        connection_loop(
            listen_fd,
            message_handler_handle_hello,
            message_handler_dispatch,
            s_manager
        );

        nanosleep(&sleeptime, NULL);
    }
    free_server_manager(s_manager);
}

void server_shutdown(void) {
    client_manager_teardown();
    close(listen_fd);
}
