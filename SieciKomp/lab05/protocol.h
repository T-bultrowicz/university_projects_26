#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

typedef struct __attribute__((__packed__)) {
    uint32_t file_length;
    uint16_t name_length;
} data_header;

typedef struct {
    char const * client_ip;
    int client_fd;
    uint16_t client_port;
} client_data;

#endif 
