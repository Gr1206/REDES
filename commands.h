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

void listFiles();



#endif // COMMANDS_H