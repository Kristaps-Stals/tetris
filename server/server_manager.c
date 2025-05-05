#include <stdlib.h>
#include "server_manager.h"

server_manager* make_server_manager() {
    server_manager *mgr = malloc(sizeof(server_manager));
    mgr->state = SERVER_STATE_LOBBY;
    mgr->player_1 = -1;
    mgr->player_2 = -1;
    mgr->player_1_ready = 0;
    mgr->player_2_ready = 0;
    return mgr;
}
void free_server_manager(server_manager* s_manager) {
    free(s_manager);
}

void handle_server(server_manager *s_manager) {
    switch(s_manager->state) {
        case SERVER_STATE_LOBBY:
            // handle_server_lobby(s_manager);
            break;
        case SERVER_STATE_GAME:
            // handle_server_game(s_manager);
            break;
    }
}