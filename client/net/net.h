#pragma once 
#include <stdint.h>
#include <stdbool.h>
#include <../shared/protocol.h> // todo make this better in makefile too?

// connects to server at <ip>:<port>, return socket df or -1 on error
int connect_to_server(const char *ip, int port);

// wrapper for sending messages. return num of sent bytes or -1
int send_message(int sockfd, uint8_t type, uint8_t player_id, const void *payload, uint16_t payload_size);

int recv_message(int sockfd, uint8_t *out_type, uint8_t *out_source, void *out_payload, uint16_t *out_payload_size);

int send_hello(int sockfd, const char *client_id, const char *player_name);
