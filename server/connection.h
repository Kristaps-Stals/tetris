// server/connection.h
#pragma once

// Start listening on 'port'. Returns listen_fd or -1 on error.
int connection_listen(int port);

// Run one select()‐iteration. Calls on_new(new_fd) for each freshly accepted client. 
// Calls on_data(fd) for each client socket ready to read.
void connection_loop(int listen_fd, void (*on_new)(int new_fd), void (*on_data)(int client_fd));
