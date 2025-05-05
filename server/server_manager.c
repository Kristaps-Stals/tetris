#include <stdlib.h>
#include "server_manager.h"

server_manager* make_server_manager() {
    server_manager *mgr = malloc(sizeof(server_manager));
    mgr->state = SERVER_STATE_LOBBY;
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