#include <stdio.h>
#include "inputHandlers.h"
#include "inputValidation.h"
#include "commands.h"

void parseLogin(CommandParser *parser){
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

void parseUnreg(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for unregister: unregister\n");
        return;
    }
    //unregister(state.udp_fd, state.ds_addr, &state.user);
    unregister(parser);
}

void parseLogout(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for logout: logout\n");
        return;
    }
    logout(parser);
}

void parseExit(CommandParser *parser){
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

void parsePublish(CommandParser *parser){
     if(parser->argcount != 3){
        printf("Invalid number of arguments for publish: publish filename label\n");
        return;
    }
    //printf("Arg2: %s\n", args[2]);
    if(!checkFilename(parser->args[1])){
        printf("Invalid filename format\n");
        return;
    }
    if (!checkLabel(parser->args[2])){
        printf("Invalid label format\n");
        return;
    } //DAR refactor completo a este código
    int result = fileExists(parser->args[1]);
    //printf("Result of fileExists: %d\n", result);

    if(result == -1){
        printf("File does not exist\n");
        return;
    } else if(result == -2){
        printf("Invalid file size\n");
        return;
    }
    //mudar isto do result ser o fileSize, criar helper e chamar no publish func
    publishFile(parser, result);
}

void parseRemoveF(CommandParser *parser){
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

void parseListF(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for list: list\n");
        return;
    }
    listFiles(parser);
}

void parseVersionsF(CommandParser *parser){
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

void parseGetF(CommandParser *parser){
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