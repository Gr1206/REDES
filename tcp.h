#ifndef TCP_H
#define TCP_H

/**
 * @file tcp.h
 * @brief TCP module: implements TCP communication with the DS.
 */

#include <netdb.h>
#include <sys/types.h>

/** @brief Seconds to wait for a DS send/receive before timing out. */
// TODO: Does not (yet) cover connect!
#define TCP_TIMEOUT 5

/**
 * @brief Resolves the DS's TCP address.
 *
 * Resolves the address, but unlike what happens with UDP, does not create a 
 * persistent socket, since TCP connections here are ephemeral (one fresh 
 * socket per exchange, not a single one reused for the program's lifetime).
 *
 * @param ds_ip The DS's IP address (or hostname).
 * @param ds_tcp_port The DS's TCP port, as a string.
 * @param addr_out Filled with the resolved DS address on success.
 * @return 0 on success, -1 on failure.
 */
int tcp_setup(const char *ds_ip, const char *ds_tcp_port, struct addrinfo **addr_out);

/**
 * @brief Performs one full request/reply exchange with the DS over TCP.
 *
 * Abstracts the connect + read + write logic required by every TCP command, 
 * so that callers only need to worry about building the request string and 
 * handling the subsequent exchange result.
 *
 * @param tcp_addr The resolved DS address.
 * @param request The request message to send.
 * @param reply_buffer Buffer where the reply is stored (null-terminated).
 * @param buffer_len Size of reply_buffer, including the null terminator.
 * @return Number of bytes received, or -1 on error/timeout/incomplete reply.
 */
ssize_t tcp_exchange(struct addrinfo *tcp_addr, const char *request, char *reply_buffer, size_t buffer_len);

/**
 * @brief Writes exactly len bytes to fd, looping over partial writes.
 *
 * Since write() may send fewer bytes than requested, a loop is required. 
 * Works on any file descriptor (i.e., sockets, files, etc). 
 *
 * @param fd The file descriptor to write to.
 * @param buf The bytes to write.
 * @param len Number of bytes to write.
 * @return 0 on success, -1 on failure.
 */
int write_all(int fd, const void *buf, size_t len);

#endif // TCP_H