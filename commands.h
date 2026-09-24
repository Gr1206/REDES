#ifndef COMMANDS_H
#define COMMANDS_H

/**
 * @file commands.h
 * @brief Commands module: implements all user command logic.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "user.h"

/**
 * @brief Performs a user login, registering the user upon the first login.
 *
 * Sends a LIN request and, if successful (OK or REG), updates the user's 
 * logged in state, UID and password.
 *
 * @param fd The UDP socket file descriptor.
 * @param res The resolved DS address.
 * @param user The local user state to update on success.
 * @param uid The UID to log in with.
 * @param password The password to log in with.
 * @param peerport The TCP port this client accepts file transfers on.
 */
void login(int fd, struct addrinfo *res, User *user, char* uid, char* password, int peerport);

/**
 * @brief Logs out the currently logged in user.
 *
 * Sends a LOU request and, if successful, clears the user's logged in state.
 *
 * @param fd The UDP socket file descriptor.
 * @param res The resolved DS address.
 * @param user The local user state to update on success.
 */
void logout(int fd, struct addrinfo *res, User *user);

/**
 * @brief Unregisters the currently logged in user.
 *
 * Sends a UNR request and, if successful, also clears the user's login state 
 * since the DS logs the user out as part of the unregistering process.
 *
 * @param fd The UDP socket file descriptor.
 * @param res The resolved DS address.
 * @param user The local user state to update on success.
 */
void unregister(int fd, struct addrinfo *res, User *user);

void publishFile();

void removeFile();

void listFiles();



#endif // COMMANDS_H