#ifndef UDP_H
#define UDP_H

/**
 * @file udp.h
 * @brief UDP module: implements UDP features used to communicate with the DS.
 */

#include <netdb.h>
#include <sys/types.h>

/** @brief Seconds to wait for a DS reply before a timeout gets raised. */
#define UDP_TIMEOUT 5

/**
 * @brief Creates the UDP socket used to communicate with the DS.
 *
 * Resolves the DS's UDP address and creates a persistent UDP socket that is 
 * used for any UDP communications with the DS.
 *
 * @param ds_ip The DS's IP address (or hostname).
 * @param ds_port The DS's UDP port, as a string.
 * @param fd_out The created socket, set on success.
 * @param addr_out The resolved DS address, set on success.
 * @return 0 on success, -1 on failure.
 */
int udp_setup(const char *ds_ip, const char *ds_port, int *fd_out, struct addrinfo **addr_out);

/**
 * @brief Sends a request to the DS and waits for its reply.
 * 
 * Abstracts the sendto + recvfrom logic required by every UDP command
 * (login, logout, unregister, etc), so that callers only need to worry 
 * about building the request string and handling the subsequent reply.
 * 
 * @param fd The UDP socket file descriptor.
 * @param res The resolved DS address.
 * @param request The request message to send.
 * @param reply_buffer Buffer where the reply is stored (null-terminated).
 * @param buffer_len Size of reply_buffer, including the null terminator.
 * @return Number of bytes received, or -1 on error/timeout.
 */
ssize_t send_and_recv(int fd, struct addrinfo *res, char *request, char *reply_buffer, size_t buffer_len);

#endif // UDP_H