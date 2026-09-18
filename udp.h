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
 * @brief Sends a request to the DS and waits for its reply.
 * 
 * Abstracts the sendto + recvfrom logic required by every UDP command
 * (login, logout, unregister), so that callers only need to worry about 
 * building the request string and handling the subsequent reply.
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