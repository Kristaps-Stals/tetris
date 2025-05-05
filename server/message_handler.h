// server/message_handler.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "server_manager.h"

// set up ID mapping
void message_handler_init(void);

// lookup assigned player_id for a given socket fd (0 if none)
uint8_t message_handler_lookup_id(int fd);

// remove mapping when client disconnects
void message_handler_remove_client(int fd);

// called immediately after accept(); does HELLO→WELCOME handshake & registers client
void message_handler_handle_hello(int client_fd);

// called whenever a client_fd is ready for reading; processes SET_READY, etc.
void message_handler_dispatch(int client_fd, server_manager *s_manager);
