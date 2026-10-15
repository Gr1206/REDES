#ifndef PEER_H
#define PEER_H

/**
 * @file peer.h
 * @brief Peer module: implements the TCP peer-to-peer server side.
 */

#include "app_state.h"

/** @brief Seconds to wait for a peer send/receive before timing out. */
#define PEER_TIMEOUT 5

/** @brief Maximum number of peer connections that can wait to be accepted. */
#define PEER_BACKLOG 10

/**
 * @brief Serves one incoming peer connection.
 *
 * Reads a GET request and replies with RGT. The file is only served if the
 * request is well-formed, the user is logged in, the file exists locally, and
 * the DS confirms (through a WHR request) that this user publishes it.
 * Anything else is refused. Always closes connfd before returning, and
 * reports the outcome to the user.
 *
 * @param connfd The accepted peer connection.
 * @param state The shared app state object (user, UDP socket and DS address).
 */
void serve_peer(int connfd, AppState *state);

/**
 * @brief Creates the listening socket for incoming peer connections.
 *
 * Binds to the given TCP port on all interfaces and starts listening, so
 * peers that are advertised the port by the DS can connect to it.
 *
 * @param port The TCP port to listen on (the -m peerport).
 * @return The listening socket file descriptor, or -1 on failure.
 */
int peer_setup(int port);

#endif // PEER_H