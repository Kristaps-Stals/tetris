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
    menu_manager *mgr = make_menu_manager();
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
        if (user_input == ERR) user_input = -1;
    
        // Use select to ensure non-blocking socket operations
        if (mgr->server_socket >= 0) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(mgr->server_socket, &read_fds);
    
            struct timeval timeout = {0, 0};  // Non-blocking select
            int activity = select(mgr->server_socket + 1, &read_fds, NULL, NULL, &timeout);
    
            bool lobby_updated = false;
    
            if (activity > 0 && FD_ISSET(mgr->server_socket, &read_fds)) {
                uint8_t type, src;
                uint16_t psz;
                uint8_t buf[512];
    
                while (recv_message(mgr->server_socket, &type, &src, buf, &psz) == 0) {
                    switch(type) {
                        case MSG_WELCOME: {
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
                            lobby_updated = true;
                            break;
                        }
                        case MSG_HELLO: {
                            msg_hello_t *h = (msg_hello_t*)buf;
                            strncpy(mgr->slot_names[src-1], h->player_name, 31);
                            lobby_updated = true;
                            break;
                        }
                        case MSG_LEAVE: {
                            strcpy(mgr->slot_names[src - 1], "(empty)");
                            lobby_updated = true;
                            break;
                        }
                        case MSG_DISCONNECT: {
                            strcpy(mgr->slot_names[src-1], "(empty)");
                            lobby_updated = true;
                            break;
                        }
                        case MSG_SET_READY: {
                            bool ready = buf[0];
                            // Update UI here: Update your manager->slot_names or add a separate readiness array
                            mgr->slot_ready[src - 1] = ready;
                            lobby_updated = true;
                            break;
                        }
                        

                        default:
                            break;
                    }
                }
            }
    
            if (lobby_updated && mgr->top >= 0 && mgr->stack[mgr->top]->id == LOBBY_MENU_ID) {
                clear();
                werase(mgr->stack[mgr->top]->win);
                wrefresh(mgr->stack[mgr->top]->win);
                free_textbox(mgr->stack[mgr->top]);
                mgr->stack[mgr->top] = make_lobby_menu(mgr);
                draw_textbox(mgr->stack[mgr->top]);
                refresh();
                doupdate();
            }
        }
    
        int ret;
        switch(state) {
            case 0:
                ret = manage_menus(mgr, user_input);
                if (ret == 1) {
                    state = 1;
                    
                    tetris_board_settings *board_settings = malloc(sizeof(tetris_board_settings));
                    board_settings->bag_seed = 0;
                    board_settings->play_height = 40;
                    board_settings->play_width = 10;
                    board_settings->window_height = 22;
                    board_settings->window_width = 10;
                    board = construct_tetris_board(board_settings);
                    free(board_settings);
                }
                break;
            case 1:
                tetris_board_update *upd = malloc(sizeof(tetris_board_update));
                upd->board = board;
                upd->delta_time = delta_time;
                upd->user_input = user_input;
                ret = update_board(upd);
                free(upd);
                if (ret == 1) {
                    state = 0;
                    open_menu(mgr, make_endscreen(board));
                    if (board != NULL) deconstruct_tetris_board(board);
                    board = NULL;
                }
                break;
        }
        if (mgr->top < 0) break;
        
        nanosleep(&sleeptime, NULL);
    }
    
    if (mgr) free_menu_manager(mgr);
    if (board) deconstruct_tetris_board(board);
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
