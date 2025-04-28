// server/server.h
#pragma once

// Initialize listening socket, client manager, message handler.
// Returns 0 on success, -1 on failure.
int server_init(int port);

// Main loop: blocks until server_shutdown is called or fatal error.
void server_run(void);

// Cleanly tear down all resources.
void server_shutdown(void);
