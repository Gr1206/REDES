#ifndef CLIENT_H
#define CLIENT_H

/**
 * @file client.h
 * @brief Client module: implements signal handling, controlled exit, and
 * input validation.
 */

#include <netdb.h>

#include "app_state.h"

/**
 * @brief Signal handler for SIGINT
 * @param sig The signal number received
 */
void handle_sigint(int sig);

/**
 * @brief Controlled exit of the application
 * @param state The shared app state object
 * @param exit_code The exit code to return
 */
void controlledExit(AppState *state, int exit_code);

/**
 * @brief Check if the UID has 6 digits
 * @param uid The UID to check
 * @return 1 if the UID is valid, 0 invalid 
 */
int checkUID(char* uid);

/**
 * @brief Check if the password has 8 alphanumeric chars
 * @param password The password to check
 * @return 1 if the password is valid, 0 invalid
 */
int checkPassword(char* password);

/**
 * @brief Check if the port is in the range 1-65535
 * @param port The port to check
 * @return 1 if the port is valid, 0 invalid
 */
int checkPort(int port);

#endif // CLIENT_H