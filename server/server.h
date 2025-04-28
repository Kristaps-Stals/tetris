// server/server.h
#pragma once

// initialize listening socket, client manager, message handler
int server_init(int port);

// main loop, blocks until server_shutdown is called or fatal error
void server_run(void);

//clean all resources.
void server_shutdown(void);
