#pragma once
#include <stdint.h>
#include "../client/tetris/board.h"
#include "../server/server_manager.h"

typedef enum {
    MSG_HELLO         = 0x00,
    MSG_WELCOME       = 0x01,
    MSG_DISCONNECT    = 0x02,
    MSG_PING          = 0x03,
    MSG_PONG          = 0x04,
    MSG_LEAVE         = 0x05,
    MSG_ERROR         = 0x06,

    MSG_SET_READY     = 0x10,
    MSG_MAKE_PLAYER   = 0x11, // request to become a player
    MSG_UNMAKE_PLAYER = 0x12, // request to unbecome a player
    MSG_SYNC_LOBBY    = 0x13, // sync lobby information

    MSG_REQ_SYNC      = 0x18,
    MSG_MOVE          = 0x19,
    MSG_ROTATE        = 0x1A,
    MSG_DROP          = 0x1B,

    MSG_SET_STATUS    = 0x20,
    MSG_SYNC_USERS    = 0x21,
    MSG_SYNC_BOARD    = 0x22,
    MSG_WINNER        = 0x23,

    MSG_UPDATE_BOARD  = 0x28

} MessageType;

#define PLAYER_ID_BROADCAST 255

#define MAX_CLIENTS 8
#define MAX_NAME_LEN 30

// HELLO
typedef struct __attribute__((packed)) {
    char client_id[20];
    char player_name[MAX_NAME_LEN];
} msg_hello_t;

// WELCOME
typedef struct __attribute__((packed)) {
    uint8_t player_id;
    uint8_t game_status;
    uint8_t length;
    char player_name[MAX_NAME_LEN];
} msg_welcome_t;

typedef struct __attribute__((packed)) {
    char player_names[MAX_CLIENTS][MAX_NAME_LEN];
    uint8_t player_1, player_2;
    uint8_t player_1_ready, player_2_ready;
} msg_sync_lobby_t;

typedef struct __attribute__((packed)) {
    int player_id;
    char state[40][10];
    tetromino active_tetromino;
    board_counters counters;
    tetris_bag now;
    tetris_bag next;
    int held_tetromino;
    int current_level;
    int armed_garbage;
    int queued_garbage;   
} msg_sync_board_t;

msg_sync_board_t *make_sync_board_msg(tetris_board *board);
void apply_sync_board_msg(tetris_board *board, msg_sync_board_t *msg);