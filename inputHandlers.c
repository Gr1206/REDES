#include <stdio.h>
#include <string.h>
#include "inputHandlers.h"
#include "inputValidation.h"
#include "commands.h"

/**
 * @brief Parse the login information, and calls login protocol function
 */
static void parseLogin(CommandParser *parser){
    if(parser->argcount != 3) {    
        printf("Invalid number of arguments for login: login UID password\n");
        return;
    }
    if(checkUID(parser->args[1]) && checkPassword(parser->args[2])){
        //tentar diminuir número de args no login!
        login(parser);
    } else {
        printf("UID or password format is invalid\n");
    }
}

/**
 * @brief Parse the unregister information, and calls unregister protocol function
 */
static void parseUnreg(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for unregister: unregister\n");
        return;
    }
    unregister(parser);
}

/**
 * @brief Parse the logout information, and calls logout protocol function
 */
static void parseLogout(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for logout: logout\n");
        return;
    }
    logout(parser);
}

/**
 * @brief Parses the exit command and calls the exit function
 * @param parser The command parser containing the command and its arguments.
 */
static void parseExit(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for exit: exit\n");
        return;
    }
    if(parser->state->user.loggedIn) {
        printf("It is required to logout before exiting\n");
        return;
    }
    parser->state->exit_requested = 1;
}

/**
 * @brief Parse the publish information, and calls publish protocol function
 * @param parser The command parser containing the publish command and its arguments.
 */
static void parsePublish(CommandParser *parser){
     if(parser->argcount != 3){
        printf("Invalid number of arguments for publish: publish filename label\n");
        return;
    }
    if(!checkFilename(parser->args[1])){
        printf("Invalid filename format\n");
        return;
    }
    if (!checkLabel(parser->args[2])){
        printf("Invalid label format\n");
        return;
    } //DAR refactor completo a este código

    long fileSize;
    switch (getLocalFileSize(parser->args[1], &fileSize)) {
        case LOCAL_FILE_NOT_FOUND:
            printf("File does not exist\n");
            return;
        case LOCAL_FILE_NOT_REGULAR:
            printf("Not a regular file\n");
            return;
        case LOCAL_FILE_TOO_LARGE:
            printf("Invalid file size (too large - max %d bytes)\n", MAX_FILESIZE);
            return;
    }
    //mudar isto do result ser o fileSize, criar helper e chamar no publish func
    publishFile(parser, fileSize);
}

/**
 * @brief Parse the remove file information, and calls remove file protocol function
 * @param parser The command parser containing the remove file command and its arguments.
 */
static void parseRemoveF(CommandParser *parser){
    if(parser->argcount != 2){
        printf("Invalid number of arguments for remove: remove filename\n");
        return;
    }
    if(!checkFilename(parser->args[1])){
        printf("Invalid filename format\n");
        return;
    }
    removeFile(parser);
}

/**
 * @brief Parses the list file command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 */
static void parseListF(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for list: list\n");
        return;
    }
    listFiles(parser);
}

/**
 * @brief Parses the file versions command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 */
static void parseVersionsF(CommandParser *parser){
    if(parser->argcount != 2){
        printf("Invalid number of arguments for versions: versions filename\n");
        return;
    }
    if(!checkFilename(parser->args[1])){
        printf("Invalid filename format\n");
        return;
    }
    versionsFile(parser, parser->args[1]);
}

/**
 * @brief Parses the get file command args, and calls its protocol function
 * @param parser The command parser containing the command and its arguments.
 */
static void parseGetF(CommandParser *parser){
    if(parser->argcount != 2){
        printf("Invalid number of arguments for get: get filename\n");
        return;
    }
    if(!checkFilename(parser->args[1])){
        printf("Invalid filename format\n");
        return;
    }
    getFile(parser, parser->args[1]);
}

// Command mapping logic: command vs corresponding function (and usage format).
typedef struct {
    const char *name;
    void (*handler)(CommandParser *);
    const char *usage;
} Command;

static const Command commandTable[] = {
    {"login",       parseLogin,     "login <UID> <password>"},
    {"logout",      parseLogout,    "logout"},
    {"unregister",  parseUnreg,     "unregister"},
    {"publish",     parsePublish,   "publish <filename> <label>"},
    {"remove",      parseRemoveF,   "remove <filename>"},
    {"list",        parseListF,     "list"},
    {"versions",    parseVersionsF, "versions <filename>"},
    {"get",         parseGetF,      "get <filename>"},
    {"exit",        parseExit,      "exit"},
};

#define NUM_COMMANDS (sizeof(commandTable) / sizeof(commandTable[0]))

static void printCommandList(void) {
    printf("Command not recognized\nList of valid commands:\n");
    for (size_t i = 0; i < NUM_COMMANDS; i++)
        printf("- %s\n", commandTable[i].usage);
}

void executeCommand(CommandParser *parser) {
    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(parser->command, commandTable[i].name) == 0) {
            commandTable[i].handler(parser);
            return;
        }
    }
    printCommandList();
}