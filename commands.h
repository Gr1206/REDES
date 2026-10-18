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
#include "inputHandlers.h"

/** @brief where_peer result: the user is publishing the file and has an active session. */
#define WHERE_FOUND      1

/** @brief where_peer result: the DS answered NOK (not published, or no active session). */
#define WHERE_NOT_FOUND  0

/** @brief where_peer result: transport error, ERR status, or malformed/unexpected reply. */
#define WHERE_ERROR     -1

/**
 * @brief Performs a user login, registering the user upon the first login.
 *
 * Sends a LIN request and, if successful (OK or REG), updates the user's 
 * logged in state, UID and password.
 *
 * @param parser The command parser struct containing the command args & current app state
 */
void login(CommandParser *parser);

/**
 * @brief Logs out the currently logged in user.
 *
 * Sends a LOU request and, if successful, clears the user's logged in state.
 *
 * @param parser The command parser struct containing the command args & current app state
 */
void logout(CommandParser *parser);

/**
 * @brief Unregisters the currently logged in user.
 *
 * Sends a UNR request and, if successful, also clears the user's login state 
 * since the DS logs the user out as part of the unregistering process.
 *
 * @param parser The command parser struct containing the command args & current app state
 */
void unregister(CommandParser *parser);

void publishFile(CommandParser *parser, int fileSize);

void removeFile(CommandParser *parser);

/**
 * @brief Requests the list of resources currently known in the network.
 *
 * Sends a LST request over UDP and, if successful, prints a list of shared 
 * filenames - up to a maximum of 50. Unlike most of the other commands, this 
 * command does not require an active login session.
 *
 * @param parser The command parser struct containing the command args & current app state
 */
void listFiles(CommandParser *parser);

/**
 * @brief Requests the list of peers sharing a given resource.
 *
 * Sends a VRS request over TCP and, if successful, prints each peer's
 * UID, file size, label, publication time, and current availability. Unlike 
 * most of the other commands, this command does not require an active login 
 * session.
 *
 * @param parser The command parser struct containing the command args & current app state
 * @param filename The resource name to request for.
 */
void versionsFile(CommandParser *parser, const char *filename);

/**
 * @brief Downloads a given resource from one of the available peers sharing it.
 * 
 * Sends a GET request over TCP to a peer previously confirmed to be available.
 * Does not require an active login session. Requires the versions command to have 
 * been run previously for the same filename and asks for confirmation if the file 
 * already exists locally.
 * 
 * TODO: Does it really make sense being able to get a resource without being logged in?
 *
 * @param parser The command parser struct containing the command args & current app state
 * @param filename The resource name to download (must already be validated).
 */
void getFile(CommandParser *parser, const char *filename);

/**
 * @brief Asks the DS for the address of a user publishing a specific file.
 *
 * Sends a WHR request over UDP, and, if successful, validates the peer's IP 
 * and TCP port. Does not require an active login session.
 *
 * @param fd The UDP socket file descriptor.
 * @param res The resolved DS address.
 * @param filename The resource name (must already be validated).
 * @param uid The publisher's UID (must already be validated).
 * @param ip_out Where to store the peer's IP, or NULL if not needed.
 * @param ip_len Size of ip_out (at least INET_ADDRSTRLEN).
 * @param port_out Where to store the peer's TCP port, or NULL if not needed.
 * @return WHERE_FOUND, WHERE_NOT_FOUND or WHERE_ERROR.
 */
int where_peer(int fd, struct addrinfo *res, const char *filename, const char *uid,
               char *ip_out, size_t ip_len, int *port_out);
 
#endif // COMMANDS_H