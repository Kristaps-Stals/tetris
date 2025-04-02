#define _XOPEN_SOURCE 700 // dont know what it does, but without it sigaction highlighting doesnt work
#include <signal.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include "test.h"

typedef long long ll;

const int board_w = 10+2;
const int board_h = 20+2;
const int frame_time = 20; // desired time between frames in ms, too low causes flickering
const struct timespec sleeptime = {0, frame_time*1e6};
int max_x, max_y;

ll time_ll(struct timespec *x) {
    return (ll)1e9*x->tv_sec+x->tv_nsec;
}

WINDOW *mainscr;
int bx = 10;
int by = 0;
ll block_time_passed = 0;
ll const fall_interval = 500; // in ms

void block_fall(ll delta_time){
    block_time_passed += delta_time;
    while (block_time_passed > fall_interval) {
        block_time_passed -= fall_interval;
        by++;
    }
}

void draw_all(ll delta_time) {
    clear();
    wclear(mainscr);

    if (delta_time == 0) delta_time = 1;
    mvprintw(0, 0, "fps: %lli", (int)1000/delta_time);
    mvaddch(by, bx, 'O');
    box(mainscr, 0, 0);

    refresh();
    wrefresh(mainscr);
}

// returns delta time in miliseconds
ll get_delta_ms(struct timespec *now, struct timespec *bef) {
    ll time_now = (ll)1e9*now->tv_sec+now->tv_nsec;
    ll time_bef = (ll)1e9*bef->tv_sec+bef->tv_nsec;
    return (ll)(time_now-time_bef)/1e6;
}

void gameloop() {
    struct timespec now, last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        ll delta_time = get_delta_ms(&now, &last_time);
        last_time = now;
        if (delta_time == 0) delta_time = 1;
        
        char usr_inp = getch();
        if (usr_inp == 'd') bx++;
        if (usr_inp == 'a') bx--;
        if (usr_inp == 'q') break;
        
        block_fall(delta_time);
        
        wrefresh(mainscr);
        draw_all(delta_time);
        nanosleep(&sleeptime, NULL);
    }
}

void on_resize() {
    // do on resize
    // getmaxyx(stdscr, max_y, max_x);
    // // move(0, 0);
    // // printw("              ");
    // // move(0, 0);
    // printw("%d %d", COLS, LINES);
    // refresh();
}

void attach_signals() {
    struct sigaction act;
    act.sa_handler = on_resize;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGWINCH, &act, NULL); // SIGWINCH = resize
}

int main() {
    attach_signals();

    initscr();
    // raw(); // disables signals like ctrl+c
    nodelay(stdscr, TRUE); // doesnt wait for user input
    keypad(stdscr, TRUE); // enables more keys
    noecho(); // dont print user input directly
    curs_set(0); // dont show cursor

    int main_x = (COLS-board_w)/2;
    int main_y = (LINES-board_h)/2;
    mainscr = newwin(board_h, board_w, main_y, main_x);
    
    gameloop();

    endwin();
    return 0;
}