#include "protocol.h"
#include <stdlib.h>

// unfinished
msg_sync_board_t *make_sync_board_msg(tetris_board *board) {
    msg_sync_board_t *msg = malloc(sizeof(msg_sync_board_t));
    (void)board;
    return msg;
}
// unfinished
void apply_sync_board_msg(tetris_board *board, msg_sync_board_t msg) {
    (void)board;
    (void)msg;
}