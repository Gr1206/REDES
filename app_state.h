#ifndef STATE_H
#define STATE_H

#include <netdb.h>

typedef struct {
    // User state (login status, UID and password).
    User user;

    int peer_tcp_port; // TODO: What about this one? app start up vs login params?

    // DS address (set at app startup).
    struct addrinfo *ds_addr;

    // UDP file descriptor.
    int udp_fd;

} AppState;

#endif