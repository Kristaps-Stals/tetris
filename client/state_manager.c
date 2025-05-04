#define _XOPEN_SOURCE 700 // dont know what it does, but without it sigaction highlighting doesnt work
#include <signal.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "tetris/board.h"
#include "menus/textbox.h"
#include "menus/menu_maker.h"
#include <stdio.h>
#include "net/net.h" 
#include "menus/keyboard_manager.h"
#include <string.h>
#include "state_manager.h"

state_manager* make_state_manager() {
    state_manager *s_manager = malloc(sizeof(state_manager));
    s_manager->menu_manager = make_menu_manager();
    s_manager->board_1 = NULL;
    s_manager->board_2 = NULL;
    s_manager->state = STATE_MENU;
    s_manager->delta_time = 1;
    s_manager->user_input = -1;
    return s_manager;
}
void free_state_manager(state_manager* s_manager) {
    if (s_manager->menu_manager) free_menu_manager(s_manager->menu_manager);
    if (s_manager->board_1) deconstruct_tetris_board(s_manager->board_1);
    if (s_manager->board_2) deconstruct_tetris_board(s_manager->board_2);
    free(s_manager);
}

void start_game_solo(state_manager *s_manager) {
    s_manager->state = STATE_GAME_SOLO;    
    tetris_board_settings *board_settings = malloc(sizeof(tetris_board_settings));
    board_settings->bag_seed = 0;
    board_settings->play_height = 40;
    board_settings->play_width = 10;
    board_settings->window_height = 22;
    board_settings->window_width = 10;
    board_settings->win_y = (LINES-(22+2))/2;
    board_settings->win_x = (COLS-(10*2+2))/2;
    board_settings->controlled = true;
    board_settings->player_id = 0;
    s_manager->board_1 = construct_tetris_board(board_settings);
    free(board_settings);

    clear();
    refresh();
}

void start_game_versus(state_manager *s_manager) {
    s_manager->state = STATE_GAME_VERSUS;

    const int space_between_boards = 50;
    int center_x_board_1 = (COLS-space_between_boards)/2;
    int center_x_board_2 = (COLS+space_between_boards)/2;

    // board 1 (left)
    tetris_board_settings *board_settings = malloc(sizeof(tetris_board_settings));
    board_settings->bag_seed = 0;
    board_settings->play_height = 40;
    board_settings->play_width = 10;
    board_settings->window_height = 22;
    board_settings->window_width = 10;
    board_settings->win_x = (center_x_board_1-11);
    board_settings->win_y = (LINES-(22+2))/2;
    board_settings->controlled = true;
    board_settings->player_id = 0;
    s_manager->board_1 = construct_tetris_board(board_settings);
    free(board_settings);

    // board 2 (right)
    board_settings = malloc(sizeof(tetris_board_settings));
    board_settings->bag_seed = 0;
    board_settings->play_height = 40;
    board_settings->play_width = 10;
    board_settings->window_height = 22;
    board_settings->window_width = 10;
    board_settings->win_x = (center_x_board_2-11);
    board_settings->win_y = (LINES-(22+2))/2;
    board_settings->controlled = false;
    board_settings->player_id = 1;
    s_manager->board_2 = construct_tetris_board(board_settings);
    free(board_settings);

    clear();
    refresh();
}

void handle_state_menu(state_manager *s_manager) {
    int ret = manage_menus(s_manager->menu_manager, s_manager->user_input);

    switch(ret) {
        case UPDSTATE_SOLO:
            start_game_versus(s_manager); // temp
            // start_game_solo(s_manager);
            break;
        case UPDSTATE_VERSUS:
            start_game_versus(s_manager);
            break;
    }
}

void handle_state_game_solo(state_manager *s_manager) {
    tetris_board_update *upd = malloc(sizeof(tetris_board_update));
    upd->board = s_manager->board_1;
    upd->delta_time = s_manager->delta_time;
    upd->user_input = s_manager->user_input;
    int ret = update_board(upd);
    free(upd);
    if (ret == 1) {
        s_manager->state = STATE_MENU;
        open_menu(s_manager->menu_manager, make_endscreen(s_manager->board_1));
        if (s_manager->board_1) deconstruct_tetris_board(s_manager->board_1);
        s_manager->board_1 = NULL;
    }
}

void handle_state_game_versus(state_manager *s_manager) {
    tetris_board_update *upd = malloc(sizeof(tetris_board_update));
    upd->board = s_manager->board_1;
    upd->delta_time = s_manager->delta_time;
    upd->user_input = s_manager->user_input;
    int ret = update_board(upd);
    free(upd);
    if (ret == 1) {
        s_manager->state = STATE_MENU;
        open_menu(s_manager->menu_manager, make_endscreen(s_manager->board_1));
        if (s_manager->board_1) deconstruct_tetris_board(s_manager->board_1);
        s_manager->board_1 = NULL;
        return;
    }

    upd = malloc(sizeof(tetris_board_update));
    upd->board = s_manager->board_2;
    upd->delta_time = s_manager->delta_time;
    upd->user_input = s_manager->user_input;
    ret = update_board(upd);
    free(upd);
    if (ret == 1) {
        s_manager->state = STATE_MENU;
        open_menu(s_manager->menu_manager, make_endscreen(s_manager->board_1));
        if (s_manager->board_1) deconstruct_tetris_board(s_manager->board_1);
        s_manager->board_1 = NULL;
        return;
    }
}

void handle_state(state_manager *s_manager) {
    switch(s_manager->state) {
        case STATE_MENU:
            handle_state_menu(s_manager);
            break;
        case STATE_GAME_SOLO:
            handle_state_game_solo(s_manager);
            break;
        case STATE_GAME_VERSUS:
            handle_state_game_versus(s_manager);
            break;
    }
}