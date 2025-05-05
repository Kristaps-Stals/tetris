#include "protocol.h"
#include <stdlib.h>
#include <string.h>

msg_sync_board_t *make_sync_board_msg(tetris_board *board) {
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