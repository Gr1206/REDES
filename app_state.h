#ifndef STATE_H
#define STATE_H

#include <netdb.h>

#define UID_LEN 6
#define PWD_LEN 8

typedef struct {
    // Login state.
    int logged_in;

    // User data (set at login).
    char username[UID_LEN + 1]; // TODO: What about larger/malformed user input sizes?
    char password[PWD_LEN + 1]; // TODO: What about larger/malformed user input sizes?

    int peer_tcp_port; // TODO: What about this one? app start up vs login params?

    // DS address (set at app startup).
    struct addrinfo *ds_addr;

    // UDP file descriptor.
    int udp_fd;
} AppState;

#endif