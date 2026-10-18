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
#define PEER_BACKLOG 16

/** @brief peer_download result: file downloaded and saved successfully. */
#define PEER_DL_OK        0

/** @brief peer_download result: received NOK from the peer. */
#define PEER_DL_REFUSED   1

/** @brief peer_download result: other failures (connection, protocol, I/O, timeout, etc). */
#define PEER_DL_FAILED   -1

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

/**
 * @brief Downloads a file from a peer, using the GET/RGT protocol.
 *
 * Connects to the peer, requests the file and streams the contents into
 * "<filename>.part", which replaces <filename> only once every byte has been 
 * received. On any failure the partial file is deleted and the existing 
 * <filename> is left as it was.
 *
 * @param ip The peer's IPv4 address (already validated).
 * @param port The peer's TCP port.
 * @param filename The resource name (must already be validated).
 * @param size_out Out-parameter: size of the saved file, set on PEER_DL_OK only.
 * @return PEER_DL_OK, PEER_DL_REFUSED or PEER_DL_FAILED.
 */
int peer_download(const char *ip, int port, const char *filename, long *size_out);

#endif // PEER_H