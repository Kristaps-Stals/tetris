#pragma once

enum {
    SERVER_STATE_LOBBY = 0,
    SERVER_STATE_GAME = 1,
};

typedef struct {
    int state;
    int player_1, player_2;
    int player_1_ready, player_2_ready;
} server_manager;

server_manager* make_server_manager();
void free_server_manager(server_manager* s_manager);

void handle_server(server_manager *s_manager);