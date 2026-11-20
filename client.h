#ifndef CLIENT_H
#define CLIENT_H

/**
 * @file client.h
 * @brief Client module: implements signal handling and controlled exit.
 */

#include <netdb.h>

#include "app_state.h"

// TODO: Both of these functions are now used exclusively in client_udp.c! 
//       Shall we delete this .h file and simply make these functions static?

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