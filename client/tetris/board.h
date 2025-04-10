#pragma once
#include <ncurses.h>

// actively falling tetromino
typedef struct tetromino {
    int type;
    int x, y; // pos
    int rotation; // 0 - 3
} tetromino;

typedef struct board_counters {
    long long time_since_gravity;
    int hold_count; // times hold used, resets on hard drop
    long long total_time_elapsed;
    int score;
} board_counters;

// a single bag
typedef struct tetris_bag {
    int stack[7];
    int top;
} tetris_bag;

typedef struct tetris_bag_manager {
    WINDOW *hold, *upcoming;
    int hold_x, hold_y, hold_w, hold_h;
    int upcoming_x, upcoming_y, upcoming_w, upcoming_h;
    tetris_bag *now, *next; // now - current bag, next - bag that will be used after <now> is empty.
    int held_tetromino; // -1 means not hold
    int bag_seed; // current bad seed to ensure everyone has the same bags
} tetris_bag_manager;

// contains all info about a board
typedef struct tetris_board {
    WINDOW *win;
    int win_x, win_y; // pos of the top left corner of window
    int width, height; // width and height of the play area NOT THE WINDOW!
    int win_w, win_h; // width and height of THE WINDOW!
    int **state; // state of each position on the board: -1 = empty, 0-6 = tetrominos
    int highest_tetromino; // highest placed tetromino

    tetromino *active_tetromino; // actively falling tetromino
    board_counters *counters, *limits; // counters count, triggers action when counter hits limits
    tetris_bag_manager *bag_manager;
} tetris_board;

// settings, which are used to construct a tetris board
typedef struct tetris_board_settings {
    int play_width;
    int play_height;
    int bag_seed;
} tetris_board_settings;

// information used when updating tetris board
typedef struct tetris_board_update {
    tetris_board *board;
    int user_input;
    long long delta_time; // time since last update in microseconds
} tetris_board_update;

// used to construct an active tetromino
typedef struct tetromino_construct_info {
    int id; // tetromino's id
    tetris_board *board; // boards play width
} tetromino_construct_info;

tetris_board *construct_tetris_board(const tetris_board_settings *settings);
void deconstruct_tetris_board(tetris_board *board); 
int update_board(tetris_board_update *update);
void draw_tetris_board(tetris_board *board);
bool valid_pos(tetromino *test, tetris_board *board);
tetromino *deepcpy_tetromino(tetromino *a);