#pragma once
#include <stdint.h>
#include "../client/tetris/board.h"

typedef enum {
    MSG_HELLO         = 0x07, // DO NOT MAKE ANY ID ZERO !!!
    MSG_WELCOME       = 0x01,
    MSG_DISCONNECT    = 0x02,
    MSG_PING          = 0x03,
    MSG_PONG          = 0x04,
    MSG_LEAVE         = 0x05,
    MSG_ERROR         = 0x06,

    MSG_TOGGLE_READY  = 0x10,
    MSG_TOGGLE_PLAYER = 0x11, // request to become a player or unbecome a player
    MSG_SYNC_LOBBY    = 0x12, // sync lobby information
    MSG_START_GAME    = 0x13,
    MSG_REQ_LOBBY     = 0x14, // request to sync lobby

    // MSG_REQ_SYNC      = 0x18,
    // MSG_MOVE          = 0x19,
    // MSG_ROTATE        = 0x1A,
    // MSG_DROP          = 0x1B,

    MSG_SET_STATUS    = 0x20,
    MSG_SYNC_USERS    = 0x21,
    MSG_SYNC_BOARD    = 0x22,
    MSG_SEND_GARBAGE  = 0x23,
    MSG_REQ_BOARD     = 0x24,
    MSG_WINNER        = 0x25,
    MSG_SET_LOSE      = 0x26,

    // MSG_UPDATE_BOARD  = 0x28

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
    int8_t player_id;
    uint8_t game_status;
    char player_name[MAX_NAME_LEN];
} msg_welcome_t;

typedef struct __attribute__((packed)) {
    char player_names[MAX_CLIENTS][MAX_NAME_LEN];
    int8_t player_1, player_2;
    uint8_t player_1_ready, player_2_ready;
    uint8_t start_counter;
} msg_sync_lobby_t;

typedef struct __attribute__((packed)) {
    int8_t player_1, player_2;
    int32_t bag_seed;
} msg_start_game_t;

typedef struct __attribute__((packed)) {
    int8_t player_id;
    char state[40][10];
    tetromino active_tetromino;
    board_counters counters;
    tetris_bag now;
    tetris_bag next;
    int held_tetromino;
    int current_level;
    int armed_garbage;
    int queued_garbage;
    int8_t player_1, player_2;
    int32_t start_bag_seed;
} msg_sync_board_t;

// typedef struct __attribute__((packed)) {
//     int8_t garbage_amount;
// } msg_send_garbage_t;

typedef struct __attribute__((packed)) {
    int64_t total_time;
    int32_t score_player_1, score_player_2;
    int32_t winner;
    char player_names[2][MAX_NAME_LEN];
} msg_winner_t;

uint8_t *make_hdr(uint16_t payload_length, uint8_t type, uint8_t src);
void free_hdr(uint8_t *hdr);