#ifndef CLIENT_H
#define CLIENT_H

/**
 * @file client.h
 * @brief Client module: implements signal handling, controlled exit, and
 * input validation.
 */

#include <netdb.h>

/**
 * @brief Signal handler for SIGINT
 * @param sig The signal number received
 */
void handle_sigint(int sig);

/**
 * @brief Controlled exit of the application
 * @param socket_fd The socket file descriptor to close (if opened)
 * @param exit_code The exit code to return
 * @param res The addrinfo structure to free (if allocated)
 */
void controlledExit(int socket_fd, int exit_code, struct addrinfo *res);




#endif // CLIENT_H