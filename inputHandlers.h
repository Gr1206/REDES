#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H

#include "app_state.h"

#define MAX_ARGS 10

typedef struct {
    char *command; //command executed in the cmd
    char *args[MAX_ARGS]; //arguments from the cmd command
    int argcount;
    AppState *state;
} CommandParser;


/**
 * @brief Parse the login information, and calls login protocol function
 */
void parseLogin(CommandParser *parser);

/**
 * @brief Parse the unregister information, and calls unregister protocol function
 */
void parseUnreg(CommandParser *parser);

/**
 * @brief Parse the logout information, and calls logout protocol function
 */
void parseLogout(CommandParser *parser);

/**
 * @brief Parses the exit command and calls the exit function
 * @param parser The command parser containing the command and its arguments.
 */
void parseExit(CommandParser *parser);

/**
 * @brief Parse the publish information, and calls publish protocol function
 * @param parser The command parser containing the publish command and its arguments.
 */
void parsePublish(CommandParser *parser);

/**
 * @brief Parse the remove file information, and calls remove file protocol function
 * @param parser The command parser containing the remove file command and its arguments.
 */
void parseRemoveF(CommandParser *parser);

/**
 * @brief Parses the list file command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 */
void parseListF(CommandParser *parser);

/**
 * @brief Parses the file versions command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 */
void parseVersionsF(CommandParser *parser);

/**
 * @brief Parses the get file command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 */
void parseGetF(CommandParser *parser);


#endif