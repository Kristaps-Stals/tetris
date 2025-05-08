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
#include "../shared/protocol.h"
#include "menus/settings.h"

state_manager* make_state_manager() {
    state_manager *s_manager = malloc(sizeof(state_manager));
    s_manager->menu_manager = make_menu_manager(s_manager);
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

msg_sync_board_t *make_sync_board_msg(tetris_board *board, state_manager *s_manager) {
    msg_sync_board_t *msg = malloc(sizeof(msg_sync_board_t));
    msg->active_tetromino = *board->active_tetromino;
    msg->armed_garbage = board->garbage_manager->armed_garbage;
    msg->counters = *board->counters;
    msg->current_level = board->difficulty_manager->current_level;
    msg->held_tetromino = board->bag_manager->held_tetromino;
    msg->next = *board->bag_manager->next;
    msg->now = *board->bag_manager->now;
    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 10; j++) {
            msg->state[i][j] = board->state[i][j];
        }
    }
    msg->queued_garbage = 0;
    for (int i = 0; i < board->garbage_manager->max_garbage_in_queue; i++) {
        msg->queued_garbage += board->garbage_manager->queue_amount[i];
    }
    msg->player_id = board->player_id;

    msg->player_1 = s_manager->menu_manager->player_1;
    msg->player_2 = s_manager->menu_manager->player_2;
    msg->start_bag_seed = s_manager->menu_manager->bag_seed;
    return msg;
}

void apply_sync_board_msg(tetris_board *board, msg_sync_board_t *msg) {
    memcpy(board->active_tetromino, &msg->active_tetromino, sizeof(tetromino));
    board->garbage_manager->armed_garbage = msg->armed_garbage;
    memcpy(board->counters, &msg->counters, sizeof(board_counters));
    board->difficulty_manager->current_level = msg->current_level;
    board->bag_manager->held_tetromino = msg->held_tetromino;
    memcpy(board->bag_manager->next, &msg->next, sizeof(tetris_bag));
    memcpy(board->bag_manager->now, &msg->now, sizeof(tetris_bag));
    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 10; j++) {
            board->state[i][j] = msg->state[i][j];
        }
    }
    board->garbage_manager->queue_amount[0] = msg->queued_garbage;
    draw_tetris_board(board);
}

void sync_board(tetris_board *board, state_manager *s_manager) {
    msg_sync_board_t *msg = make_sync_board_msg(board, s_manager);
    send_message(
        s_manager->menu_manager->server_socket,
        MSG_SYNC_BOARD,
        s_manager->menu_manager->player_id,
        (void*)msg,
        sizeof(msg_sync_board_t)
    );
    free(msg);
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
    board_settings->sockfd = -1;
    board_settings->player_id = 0;
    sprintf(board_settings->player_name, "%s", get_nickname());
    s_manager->board_1 = construct_tetris_board(board_settings);
    free(board_settings);

    clear();
    refresh();
}

void start_game_versus(state_manager *s_manager) {
    menu_manager *m_manager = s_manager->menu_manager;
    s_manager->state = STATE_GAME_VERSUS;

    const int space_between_boards = 50;
    int center_x_board_1 = (COLS-space_between_boards)/2;
    int center_x_board_2 = (COLS+space_between_boards)/2;

    // board 1 (left)
    tetris_board_settings *board_settings = malloc(sizeof(tetris_board_settings));
    board_settings->bag_seed = m_manager->bag_seed;
    board_settings->play_height = 40;
    board_settings->play_width = 10;
    board_settings->window_height = 22;
    board_settings->window_width = 10;
    board_settings->win_x = (center_x_board_1-11);
    board_settings->win_y = (LINES-(22+2))/2;
    board_settings->controlled = false;
    board_settings->sockfd = -1;
    if (m_manager->player_id == m_manager->player_1) {
        board_settings->controlled = true;
        board_settings->sockfd = m_manager->server_socket;
    }
    board_settings->player_id = m_manager->player_1;
    sprintf(board_settings->player_name, "%s", m_manager->slot_names[m_manager->player_1]);
    s_manager->board_1 = construct_tetris_board(board_settings);
    free(board_settings);

    // board 2 (right)
    board_settings = malloc(sizeof(tetris_board_settings));
    board_settings->bag_seed = m_manager->bag_seed;
    board_settings->play_height = 40;
    board_settings->play_width = 10;
    board_settings->window_height = 22;
    board_settings->window_width = 10;
    board_settings->win_x = (center_x_board_2-11);
    board_settings->win_y = (LINES-(22+2))/2;
    board_settings->controlled = false;
    board_settings->sockfd = -1;
    if (m_manager->player_id == m_manager->player_2) {
        board_settings->controlled = true;
        board_settings->sockfd = m_manager->server_socket;
    }
    board_settings->player_id = m_manager->player_2;
    sprintf(board_settings->player_name, "%s", m_manager->slot_names[m_manager->player_2]);
    s_manager->board_2 = construct_tetris_board(board_settings);
    free(board_settings);

    clear();
    refresh();

    draw_tetris_board(s_manager->board_1);
    draw_tetris_board(s_manager->board_2);
}

void handle_state_menu(state_manager *s_manager) {
    int ret = manage_menus(s_manager->menu_manager, s_manager->user_input);

    switch(ret) {
        case UPDSTATE_SOLO:
            start_game_solo(s_manager);
            break;
        // case UPDSTATE_VERSUS: // unused
        //     start_game_versus(s_manager);
        //     break;
    }
}

void handle_state_game_solo(state_manager *s_manager) {
    tetris_board_update *upd = malloc(sizeof(tetris_board_update));
    upd->board = s_manager->board_1;
    upd->delta_time = s_manager->delta_time;
    upd->user_input = s_manager->user_input;
    bool is_changed = false;
    int ret = update_board(upd, &is_changed);
    free(upd);
    if (ret == 1) {
        s_manager->state = STATE_MENU;
        open_menu(s_manager->menu_manager, make_endscreen(s_manager->board_1));
        if (s_manager->board_1) deconstruct_tetris_board(s_manager->board_1);
        s_manager->board_1 = NULL;
    }
}

void control_board_versus(tetris_board *board, state_manager *s_manager) {
    tetris_board_update *upd = malloc(sizeof(tetris_board_update));
    upd->board = board;
    upd->delta_time = s_manager->delta_time;
    upd->user_input = s_manager->user_input;
    bool is_changed = false;
    int ret = update_board(upd, &is_changed);
    free(upd);
    
    static int64_t cooldown = 100*1e3;
    cooldown -= s_manager->delta_time;
    if (is_changed && cooldown <= 0 && ret != 1) {    
        sync_board(board, s_manager);
        cooldown = 100*1e3; // 200 ms
    }

    if (ret == 1) {
        // lose
        send_message(
            s_manager->menu_manager->server_socket,
            MSG_SET_LOSE,
            s_manager->menu_manager->player_id,
            NULL,
            0
        );
    }
}

void handle_state_game_versus(state_manager *s_manager) {
    if (s_manager->board_1->is_controlled) {
        control_board_versus(s_manager->board_1, s_manager);
    }
    
    if (s_manager->board_2->is_controlled) {
        control_board_versus(s_manager->board_2, s_manager);
    }
}

void handle_winner_versus(state_manager *s_manager, msg_winner_t *msg) {
    open_menu(s_manager->menu_manager, make_endscreen_versus(msg));
    if (s_manager->board_1) deconstruct_tetris_board(s_manager->board_1);
    s_manager->board_1 = NULL;
    if (s_manager->board_2) deconstruct_tetris_board(s_manager->board_2);
    s_manager->board_2 = NULL;
    s_manager->state = STATE_MENU;
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