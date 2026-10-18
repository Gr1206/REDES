#ifndef APP_STATE_H
#define APP_STATE_H

/**
 * @file app_state.h
 * @brief Representation of the current shared client state.
 */

#include <netdb.h>

#include "user.h"

/** @brief Longest valid filename (name, dot and 3-letter extension). */
// TODO: Shouldn't this use the value defined elsewhere instead?
#define FILENAME_MAX_LEN 24

/** @brief Maximum number of peers publishing a file (from the versions command). */
#define MAX_PEER_CANDIDATES 32

/** @brief A publisher returned by the last versions command. */
typedef struct {
    char uid[UID_LEN + 1];          /**< Peer's UID. */
    int available;                  /**< 1 if the DS reported it as available (AVL), 0 otherwise. */
} PeerCandidate;

/** @brief Peers remembered by the last successful versions command. */
typedef struct {
    char filename[FILENAME_MAX_LEN + 1];            /**< Resource the peers refer to. */
    PeerCandidate peers[MAX_PEER_CANDIDATES];       /**< Remembered peers. */
    int count;                                      /**< Number of valid entries in peers. */
} PeerList;

/**
 * @brief State of the running client.
 *
 * Includes all the data required by user commands: the logged in user's local 
 * state, the DS address and the peer TCP port (both set at startup), and the 
 * active UDP socket used to communicate with it.
 */
typedef struct {
    User user;                      /**< Logged in user state. */
    int peer_tcp_port;              /**< Peer-to-peer TCP port used by the client. */
    struct addrinfo *ds_addr;       /**< Resolved Directory Server UDP address. */
    struct addrinfo *ds_tcp_addr;   /**< Resolved Directory Server TCP address. */
    int udp_fd;                     /**< UDP socket used to communicate with the DS. */
    int peer_fd;                    /**< Listening socket for incoming peer connections. */
    PeerList peer_list;             /**< Peer candidates for the get command to pick from. */
} AppState;

#endif // APP_STATE_H