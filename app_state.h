#ifndef APP_STATE_H
#define APP_STATE_H

/**
 * @file app_state.h
 * @brief Representation of the current shared client state.
 */

#include <netdb.h>

#include "user.h"

/**
 * @brief State of the running client.
 *
 * Includes all the data required by user commands: the logged in user's local 
 * state, the DS address and the peer TCP port (both set at startup), and the 
 * active UDP socket used to communicate with it.
 */
typedef struct {
    User user;                  /**< Logged in user state. */
    int peer_tcp_port;          /**< Peer-to-peer TCP port used by the client. */
    struct addrinfo *ds_addr;   /**< Directory Server address. */
    int udp_fd;                 /**< UDP socket used to communicate with the DS. */
} AppState;

#endif // APP_STATE_H