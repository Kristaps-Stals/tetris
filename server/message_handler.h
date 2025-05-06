// server/message_handler.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "server_manager.h"

// called immediately after accept(); does HELLO→WELCOME handshake & registers client
void message_handler_handle_hello(int client_fd, server_manager *s_manager);

// called whenever a client_fd is ready for reading; processes SET_READY, etc.
void message_handler_dispatch(int client_fd, server_manager *s_manager);

void sync_lobby(server_manager *s_manager);