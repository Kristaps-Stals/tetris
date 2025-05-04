#define _XOPEN_SOURCE 700 // dont know what it does, but without it sigaction highlighting doesnt work
#include <signal.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "tetris/board.h"
#include "menus/textbox.h"
#include "menus/menu_maker.h"
#include <getopt.h>
#include <stdio.h>
#include "net/net.h" 
#include "menus/keyboard_manager.h"
#include "../shared/protocol.h"  // for MSG_WELCOME, MSG_HELLO, MSG_DISCONNECT
#include <string.h>
#include <sys/select.h>
#include <fcntl.h>
#include "state_manager.h"

typedef long long ll;

const int board_w = 10+2;
const int board_h = 22+2;
const int frame_time = 25; // desired time between frames in ms, too low causes flickering
const struct timespec sleeptime = {0, frame_time*1e6};

// converts timespec struct into a long long
ll time_ll(struct timespec *x) {
    return (ll)1e9*x->tv_sec+x->tv_nsec;
}

// returns delta time in microseconds
ll get_delta_micro_s(struct timespec *now, struct timespec *bef) {
    return (ll)(time_ll(now)-time_ll(bef))/1e3;
}

void gameloop(const char *host, int port) {
    (void)host; // unused
    (void)port; // unused
    struct timespec now, last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    init_binds();
    state_manager *state = make_state_manager();
    // mvprintw(LINES-9, 0, "Game:");
    // mvprintw(LINES-8, 0, "  Arrow keys to move right/down/left");
    // mvprintw(LINES-7, 0, "  Z/X - rotate");
    // mvprintw(LINES-6, 0, "  C - hold");
    // mvprintw(LINES-5, 0, "  SPACE - hard drop");
    // mvprintw(LINES-4, 0, "Menu:");
    // mvprintw(LINES-3, 0, "  Arrow keys to move up/right/down/left");
    // mvprintw(LINES-2, 0, "  Z/ENTER - select option");
    // mvprintw(LINES-1, 0, "  X - back");
    // refresh();

    while (true) {
        // get delta time
        clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t delta_time = get_delta_micro_s(&now, &last_time);
        last_time = now;
        if (delta_time == 0) delta_time = 1;

        // debug
        mvprintw(0, 0, "%ld, %d", delta_time, state->menu_manager->server_socket);
        refresh();

        int user_input = getch();

        process_server_messages(state->menu_manager);
        
        state->user_input = user_input;
        state->delta_time = delta_time;
        handle_state(state);

        if (state->menu_manager->top < 0) break; // closing all menus causes game to close
        
        nanosleep(&sleeptime, NULL);
    }
    
    free_state_manager(state);
}

int main(int argc, char **argv) {
    const char *host;
    int port;

    if (!parse_connection_args(argc, argv, &host, &port)) {
        return 1; // Invalid args
    }

    initscr();

    start_color();
    if (!has_colors()) {
        printw("Screen doesnt support colors...\n");
        refresh();
        getch();
        endwin();
        return 0;   
    }

    init_color(COLOR_CYAN, 0, 1000, 1000);
    init_color(COLOR_BLUE, 0, 0, 1000);
    init_color(COLOR_YELLOW, 1000, 1000, 0);
    init_color(64, 1000, 666, 0); // orange
    init_color(COLOR_GREEN, 0, 1000, 0);
    init_color(COLOR_MAGENTA, 600, 0, 1000);
    init_color(COLOR_RED, 1000, 0, 0);
    init_color(65, 500, 100, 50); // dark red
    init_color(66, 400, 400, 400); // gray

    init_pair(1, COLOR_CYAN, COLOR_CYAN); // I piece
    init_pair(2, COLOR_BLUE, COLOR_BLUE); // J piece
    init_pair(3, 64, 64); // L piece
    init_pair(4, COLOR_YELLOW, COLOR_YELLOW); // O piece
    init_pair(5, COLOR_GREEN, COLOR_GREEN); // S piece
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA); // T piece
    init_pair(7, COLOR_RED, COLOR_RED); // Z piece
    init_pair(8, 66, 66); // gray for garbage

    init_pair(10, COLOR_RED, COLOR_BLACK); // used for warning
    init_pair(11, COLOR_RED, COLOR_RED); // used for armed garbage
    init_pair(12, 65, 65); // used for unarmed garbage

    cbreak(); // or raw(), disables input buffering
    // raw(); // disables signals like ctrl+c
    nodelay(stdscr, TRUE); // doesnt wait for user input
    keypad(stdscr, TRUE); // enables more keys
    noecho(); // dont print user input directly
    curs_set(0); // dont show cursor

    gameloop(host, port);

    endwin();
    return 0;
}
