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




#endif // CLIENT_H