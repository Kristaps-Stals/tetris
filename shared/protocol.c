#include "protocol.h"
#include <stdlib.h>
#include <string.h>

uint8_t* make_hdr(uint16_t payload_length, uint8_t type, uint8_t src) {
    uint8_t *hdr = malloc(4*sizeof(uint8_t));
    hdr[0] = (payload_length >> 8);
    hdr[1] = (payload_length & 0xFF);
    hdr[2] = type;
    hdr[3] = src;
    return hdr;
}
void free_hdr(uint8_t *hdr) {
    free(hdr);
}