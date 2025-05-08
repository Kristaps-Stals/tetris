#pragma once
#include <stdint.h>

enum {
    SERVER_STATE_LOBBY = 0,
    SERVER_STATE_GAME = 1,
};

typedef struct {
    int state;
    int player_1, player_2;
    int player_1_ready, player_2_ready;

    int64_t start_game_time_left; // in microseconds
    int64_t start_game_time_max;
    int last_time_char_sent;
    int last_winner;
} server_manager;

server_manager* make_server_manager();
void free_server_manager(server_manager* s_manager);

void handle_server(server_manager *s_manager, int64_t delta_time);
void declare_winner_versus(server_manager *server_manager, uint8_t winner);