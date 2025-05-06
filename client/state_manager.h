#pragma once
#include "menus/menu_maker.h"
#include "tetris/board.h"

enum {
    STATE_MENU = 0,
    STATE_GAME_SOLO = 1,
    STATE_GAME_VERSUS = 2,
};

typedef struct {
    menu_manager *menu_manager;
    tetris_board *board_1, *board_2;
    int state;
    int user_input;
    int64_t delta_time;
} state_manager;

state_manager* make_state_manager();
void free_state_manager(state_manager* s_manager);

void handle_state(state_manager *s_manager);

void start_game_versus(state_manager *s_manager);