// server/client_manager.h
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

typedef struct {
    int      sockfd;
    uint8_t  player_id;
    char     name[30];
    bool     ready;
} client_t;

// initialize client table
void client_manager_init(void);

// close all client sockets
void client_manager_teardown(void);

int client_manager_count(void); // active clients

// read-only access to client at index
const client_t* client_manager_get(int index);

// add a new client (after successful HELLO)
void client_manager_add(int sockfd, uint8_t player_id, const char *name);

// remove a client (on disconnect or error)
void client_manager_remove(int sockfd);

// send a raw [hdr,len=hdr_len] + [payload,len=payload_len] to all clients
void client_manager_broadcast(const uint8_t *hdr, int hdr_len, const uint8_t *payload, int payload_len);

// update client’s ready flag
void client_manager_set_ready(uint8_t player_id, bool ready);

// count how many clients are ready
int  client_manager_count_ready(void);
