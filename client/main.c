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

int parse_connection_args(int argc, char **argv, const char **host, int *port) { // todo: move to net?
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

    // if no port, just run singe player :)
    return 1;
}

void gameloop(const char *host, int port) {
    struct timespec now, last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    init_binds();
    menu_manager *menu_manager_ = make_menu_manager();
    tetris_board *board = NULL;
    int state = 0; // 0 = in menus, 1 = playing
    mvprintw(LINES-9, 0, "Game:");
    mvprintw(LINES-8, 0, "  Arrow keys to move right/down/left");
    mvprintw(LINES-7, 0, "  Z/X - rotate");
    mvprintw(LINES-6, 0, "  C - hold");
    mvprintw(LINES-5, 0, "  SPACE - hard drop");
    mvprintw(LINES-4, 0, "Menu:");
    mvprintw(LINES-3, 0, "  Arrow keys to move up/right/down/left");
    mvprintw(LINES-2, 0, "  Z/ENTER - select option");
    mvprintw(LINES-1, 0, "  X - back");

    refresh();

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        ll delta_time = get_delta_micro_s(&now, &last_time);
        last_time = now;
        if (delta_time == 0) delta_time = 1;
        int user_input = getch();

        int ret;
        switch(state) {
            case 0:
                ret = manage_menus(menu_manager_, user_input);
                if (ret == 1) {
                    state = 1;

                    // connect
                    if (port != 0) {
                        int server_socket = connect_to_server(host, port);
                        if (server_socket < 0) {
                            mvprintw(0, 0, "Failed to connect to server at %s:%d", host, port);
                            refresh();
                            sleep(2);
                            break;
                        }
                        if (send_hello(server_socket, "TetrisClient 1.0", "PlayerOne") < 0) {
                            mvprintw(0, 0, "Failed to send HELLO message.");
                            refresh();
                            sleep(2);
                            close(server_socket);
                            break;
                        }
                    }
                
                    // setup tetris
                    tetris_board_settings *board_settings = malloc(sizeof(tetris_board_settings));
                    board_settings->bag_seed = 0;
                    board_settings->play_height = 22;
                    board_settings->play_width = 10;
                    board = construct_tetris_board(board_settings);
                    free(board_settings);
                }
                break;
            case 1:
                // update board
                tetris_board_update *upd = malloc(sizeof(tetris_board_update));
                upd->board = board;
                upd->delta_time = delta_time;
                upd->user_input = user_input;
                ret = update_board(upd);
                free(upd);
                if (ret == 1) {
                    // go back to menus
                    state = 0;
                    open_menu(menu_manager_, make_endscreen(board));
                    if (board != NULL) deconstruct_tetris_board(board);
                }
                break;
        }
        if (menu_manager_->top < 0) break; // if close all menus quit game
        
        nanosleep(&sleeptime, NULL);
    }

    if (menu_manager_ != NULL) free_menu_manager(menu_manager_);
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

    init_pair(1, COLOR_CYAN, COLOR_CYAN); // I piece
    init_pair(2, COLOR_BLUE, COLOR_BLUE); // J piece
    init_pair(3, 64, 64); // L piece
    init_pair(4, COLOR_YELLOW, COLOR_YELLOW); // O piece
    init_pair(5, COLOR_GREEN, COLOR_GREEN); // S piece
    init_pair(6, COLOR_MAGENTA, COLOR_MAGENTA); // T piece
    init_pair(7, COLOR_RED, COLOR_RED); // Z piece

    init_pair(10, COLOR_RED, COLOR_BLACK); // used for warning

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