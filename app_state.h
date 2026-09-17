#ifndef STATE_H
#define STATE_H

#include <netdb.h>

#include "user.h"

typedef struct {
    // User state (login status, UID and password).
    User user;

    int peer_tcp_port;

    // DS address (set at app startup).
    struct addrinfo *ds_addr;

    // UDP file descriptor.
    int udp_fd;
} AppState;

#endif