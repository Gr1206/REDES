#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H

#include "app_state.h"

#define MAX_ARGS 10
#define MAX_FILESIZE (10LL * 1000 * 1000) //MANTEM EM 64 BITS

typedef struct {
    char *command;
    char *args[MAX_ARGS];
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
 * @return a definir
 */
int parsePublish(CommandParser *parser);

/**
 * @brief Parse the remove file information, and calls remove file protocol function
 * @return a definir
 */
int parseRemoveF(CommandParser *parser);

/**
 * @brief Parse the list file information, and calls list file protocol function
 * @return a definir
 */
int parseListF(CommandParser *parser);


#endif