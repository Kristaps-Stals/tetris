#pragma once
#include <stdint.h>

typedef enum {
    MSG_HELLO         = 0x00,
    MSG_WELCOME       = 0x01,
    MSG_DISCONNECT    = 0x02,
    MSG_PING          = 0x03,
    MSG_PONG          = 0x04,
    MSG_LEAVE         = 0x05,
    MSG_ERROR         = 0x06,

    MSG_SET_READY     = 0x10,

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


// HELLO
typedef struct __attribute__((packed)) {
    char client_id[20];
    char player_name[30];
} msg_hello_t;


// WELCOME
typedef struct __attribute__((packed)) {
    uint8_t player_id;
    uint8_t game_status;
    uint8_t length;
    char player_name[30];
} msg_welcome_t;

