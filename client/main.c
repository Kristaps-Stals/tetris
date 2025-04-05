#define _XOPEN_SOURCE 700 // dont know what it does, but without it sigaction highlighting doesnt work
#include <signal.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "tetris/board.h"

typedef long long ll;

const int board_w = 10+2;
const int board_h = 22+2;
const int frame_time = 25; // desired time between frames in ms, too low causes flickering
const struct timespec sleeptime = {0, frame_time*1e6};

ll time_ll(struct timespec *x) {
    return (ll)1e9*x->tv_sec+x->tv_nsec;
}

// returns delta time in microseconds
ll get_delta_micro_s(struct timespec *now, struct timespec *bef) {
    return (ll)(time_ll(now)-time_ll(bef))/1e3;
}

void gameloop() {
    struct timespec now, last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    tetris_board_settings *board_settings = malloc(sizeof(tetris_board_settings));
    board_settings->bag_seed = 0;
    board_settings->play_height = 22;
    board_settings->play_width = 10;
    tetris_board *board = construct_tetris_board(board_settings);
    free(board_settings);

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        ll delta_time = get_delta_micro_s(&now, &last_time);
        last_time = now;
        if (delta_time == 0) delta_time = 1;

        // get input and update board
        int user_input = getch();
        tetris_board_update *upd = malloc(sizeof(tetris_board_update));
        upd->board = board;
        upd->delta_time = delta_time;
        upd->user_input = user_input;
        update_board(upd);
        free(upd);

        draw_tetris_board(board);
        nanosleep(&sleeptime, NULL);
    }

    deconstruct_tetris_board(board);
}

int main() {
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

    init_pair(1, COLOR_CYAN, COLOR_CYAN);
    init_pair(2, COLOR_BLUE, COLOR_BLUE);
    init_pair(3, 64, 64);
    init_pair(4, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(5, COLOR_GREEN, COLOR_GREEN);
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(7, COLOR_RED, COLOR_RED);

    init_pair(10, COLOR_RED, COLOR_BLACK); // used for warning

    cbreak(); // or raw(), disables input buffering
    // raw(); // disables signals like ctrl+c
    nodelay(stdscr, TRUE); // doesnt wait for user input
    keypad(stdscr, TRUE); // enables more keys
    noecho(); // dont print user input directly
    curs_set(0); // dont show cursor

    gameloop();

    endwin();
    return 0;
}