#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H

#include "app_state.h"

#define MAX_ARGS 10
#define MAX_FILESIZE (10LL * 1000 * 1000) //MANTEM EM 64 BITS

typedef struct {
    char *command; //command executed in the cmd
    char *args[MAX_ARGS]; //arguments frmo the cmd command
    int argcount;
    AppState *state;
} CommandParser;


/**
 * @brief Parse the login information, and calls login protocol function
 * @return a definir
 */
int parseLogin(CommandParser *parser);

/**
 * @brief Parse the unregister information, and calls unregister protocol function
 * @return a definir
 */
int parseUnreg(CommandParser *parser);

/**
 * @brief Parse the logout information, and calls logout protocol function
 * @return a definir
 */
int parseLogout(CommandParser *parser);

/**
 * @brief Parse the exit information, and calls exit function
 * //ou retornar 1 para chamar controlled exit, tenho de pensar ainda
 * //ou meter controlledexit aqui talvez
 * @return a definir
 */
int parseExit(CommandParser *parser);

/**
 * @brief Parse the publish information, and calls publish protocol function
 * @param parser The command parser containing the publish command and its arguments.
 * @return a definir
 */
int parsePublish(CommandParser *parser);

/**
 * @brief Parse the remove file information, and calls remove file protocol function
 * @param parser The command parser containing the remove file command and its arguments.
 * @return a definir
 */
int parseRemoveF(CommandParser *parser);

/**
 * @brief Parses the list file command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 * @return a definir
 */
int parseListF(CommandParser *parser);

/**
 * @brief Parses the file versions command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 * @return a definir
 */
int parseVersionsF(CommandParser *parser);


#endif