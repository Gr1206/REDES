#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H

#include "app_state.h"

#define MAX_ARGS 10

typedef struct {
    char *command;          // Command input by the user (same as args[0]).
    char *args[MAX_ARGS];   // Tokens for the full input (including the command).
    int argcount;
    AppState *state;
} CommandParser;

/**
 * @brief Runs a specific command.
 *
 * Calls the command's handler function, which performs command-specific 
 * argument validation and runs the corresponding protocol logic, if 
 * successful.
 *
 * @param parser The command parser containing the command, its arguments and the app state.
 */
void executeCommand(CommandParser *parser);


#endif