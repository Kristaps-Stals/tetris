// server/client_manager.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

typedef struct {
    bool     exists;
    int      sockfd;
    char     name[30];
} client_t;

// initialize client table
void client_manager_init(void);

// close all client sockets
void client_manager_teardown(void);

int client_manager_count(void); // active clients

// read-only access to client at index
const client_t* client_manager_get(int index);

// add a new client (after successful HELLO)
bool client_manager_add(int sockfd, const char *name);

// remove a client (on disconnect or error)
void client_manager_remove(int sockfd, server_manager *s_manager);

// send a raw [hdr,len=hdr_len] + [payload,len=payload_len] to [player_id]
void client_manager_send(const uint8_t *hdr, int hdr_len, const uint8_t *payload, int payload_len, int player_id);

// send a raw [hdr,len=hdr_len] + [payload,len=payload_len] to all clients except [except_socketfd]
void client_manager_broadcast(const uint8_t *hdr, int hdr_len, const uint8_t *payload, int payload_len, int except_socketfd);

// gets player index from sockfd, returns -1 if not found
int8_t get_player_id_from_fd(int sockfd);
