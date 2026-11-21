#ifndef PEER_SERVER_H
#define PEER_SERVER_H

/**
 * @file peer_server.h
 * @brief Peer server module: implements the TCP peer-to-peer server side.
 */

#include "app_state.h"

/**
 * @brief Serves one incoming peer connection.
 *
 * Reads a GET request and replies with RGT. The file is only served if the
 * request is well-formed, the user is logged in, the file exists locally, and
 * the DS confirms that the user is actively publishing the file. If that's not 
 * the case, the request gets refused. Always closes the file descriptor before 
 * returning, and reports the outcome to the user.
 *
 * @param connfd The accepted peer connection.
 * @param state The shared app state object.
 */
void serve_peer(int connfd, AppState *state);

/**
 * @brief Creates the listening socket for incoming peer connections.
 *
 * Binds to the given TCP port on all interfaces and starts listening, so
 * that peers can connect to it.
 *
 * @param port The TCP port to listen on (corresponding to the -m TCP peer port).
 * @return The listening socket file descriptor, or -1 on failure.
 */
int peer_setup(int port);

#endif // PEER_SERVER_H