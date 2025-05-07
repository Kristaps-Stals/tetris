#include <stdlib.h>
#include "server_manager.h"
#include "message_handler.h"
#include "client_manager.h"
#include "time.h"

void set_menu_defaults(server_manager *mgr) {
    mgr->player_1 = -1;
    mgr->player_2 = -1;
    mgr->player_1_ready = 0;
    mgr->player_2_ready = 0;
    mgr->state = SERVER_STATE_LOBBY;
    mgr->last_time_char_sent = ' ';
    mgr->start_game_time_left = mgr->start_game_time_max;
}

server_manager* make_server_manager() {
    server_manager *mgr = malloc(sizeof(server_manager));
    mgr->start_game_time_max = 4*1e6; // 4 seconds
    set_menu_defaults(mgr);
    return mgr;
}
void free_server_manager(server_manager *s_manager) {
    free(s_manager);
}

void start_game(server_manager *s_manager) {
    s_manager->state = SERVER_STATE_GAME;
    msg_start_game_t msg;
    msg.player_1 = s_manager->player_1;
    msg.player_2 = s_manager->player_2;
    srand(time(NULL));
    msg.bag_seed = rand();
    uint8_t *hdr = make_hdr(sizeof(msg_start_game_t), MSG_START_GAME, PLAYER_ID_BROADCAST);
    client_manager_broadcast(hdr, 4, (void*)&msg, sizeof(msg_start_game_t), -1);
    free_hdr(hdr);
}

void handle_server_lobby(server_manager *s_manager, int64_t delta_time) {
    int8_t char_to_send = ' ';
    if (s_manager->player_1_ready && s_manager->player_2_ready) {
        s_manager->start_game_time_left -= delta_time; 
        if (s_manager->start_game_time_left <= 0) {
            start_game(s_manager);
            return;
        }
        int seconds = s_manager->start_game_time_left/1e6;
        char_to_send = '0'+seconds;
    } else {
        s_manager->start_game_time_left = s_manager->start_game_time_max;
    }
    if (char_to_send == s_manager->last_time_char_sent) return;
    s_manager->last_time_char_sent = char_to_send;
    sync_lobby(s_manager);
}

void declare_winner_versus(server_manager *server_manager, uint8_t winner) {
    printf("[server_manager] winner=%d\n", winner);
    uint8_t *hdr = make_hdr(0, MSG_WINNER, server_manager->player_2);
    client_manager_broadcast(hdr, 4, NULL, 0, -1);
    free_hdr(hdr);
    set_menu_defaults(server_manager);
    sync_lobby(server_manager);
}

void handle_server_game(server_manager *server_manager, int64_t delta_time) {
    (void) delta_time;
    if (server_manager->player_1 == -1) {
        // player 2 wins
        printf("[server_manager] in versus game, but player_1=-1\n");
        declare_winner_versus(server_manager, server_manager->player_2);
        return;
    }
    if (server_manager->player_2 == -1) {
        // player 1 wins
        printf("[server_manager] in versus game, but player_2=-1\n");
        declare_winner_versus(server_manager, server_manager->player_1);
        return;
    }
}

void handle_server(server_manager *s_manager, int64_t delta_time) {
    switch(s_manager->state) {
        case SERVER_STATE_LOBBY:
            handle_server_lobby(s_manager, delta_time);
            break;
        case SERVER_STATE_GAME:
            handle_server_game(s_manager, delta_time);
            break;
    }
}