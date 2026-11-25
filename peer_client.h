#ifndef PEER_CLIENT_H
#define PEER_CLIENT_H

/**
 * @file peer_client.h
 * @brief Peer client module: implements the TCP peer-to-peer client side.
 */

#include "app_state.h"

/** @brief peer_download result: file downloaded and saved successfully. */
#define PEER_DL_OK        0

/** @brief peer_download result: received NOK from the peer. */
#define PEER_DL_REFUSED   1

/** @brief peer_download result: other failures (connection, protocol, I/O, timeout, etc). */
#define PEER_DL_FAILED   -1

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

#endif // PEER_CLIENT_H