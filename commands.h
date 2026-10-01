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
 * @param fd The UDP socket file descriptor.
 * @param res The resolved DS address.
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
 * @param tcp_addr The resolved DS TCP address.
 * @param filename The resource name to request for.
 */
void versionsFile(CommandParser *parser, const char *filename);
 
#endif // COMMANDS_H