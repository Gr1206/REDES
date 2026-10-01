#include "client.h" // quero tentar mudar isto
#include "inputHandlers.h"
#include "inputValidation.h"
#include "commands.h"

int parseLogin(CommandParser *parser){
    if(parser->argcount != 3) {    
        printf("Invalid number of arguments for login: login UID password\n");
        return 0;
    }
    if(checkUID(parser->args[1]) && checkPassword(parser->args[2])){
        //tentar diminuir número de args no login!
        login(parser);
    } else {
        printf("UID or password format is invalid\n");
    }
    
    return 1;
}

int parseUnreg(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for unregister: unregister\n");
        return 0;
    }
    //unregister(state.udp_fd, state.ds_addr, &state.user);
    unregister(parser);
    
    return 1;
}

int parseLogout(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for logout: logout\n");
        return 0;
    }
    //logout(state.udp_fd, state.ds_addr, &state.user);
    logout(parser);
    return 1;
}
int parseExit(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for exit: exit\n");
        return 0;
    }
    if(parser->state->user.loggedIn) {
        printf("It is required to logout before exiting\n");
        return 0;
    } else {
        controlledExit(parser->state->udp_fd, 0, parser->state->ds_addr);
        //ainda tenho de pensar neste !
    }
    
    return 1;
}

int parsePublish(CommandParser *parser){
     if(parser->argcount != 3){
        printf("Invalid number of arguments for publish: publish filename label\n");
        return 0;
    }
    //printf("Arg2: %s\n", args[2]);
    if(!checkFilename(parser->args[1])){
        printf("Invalid filename format\n");
        return 0;
    }
    if (!checkLabel(parser->args[2])){
        printf("Invalid label format\n");
    } //DAR refactor completo a este código
    int result = fileExists(parser->args[1]);
    //printf("Result of fileExists: %d\n", result);

    if(result == -1){
        printf("File does not exist\n");
        return 0;
    } else if(result == -2){
        printf("Invalid file size\n");
        return 0;
    }
    //mudar isto do result ser o fileSize, criar helper e chamar no publish func
    publishFile(parser, result);
    
    
    return 1;
}

int parseRemoveF(CommandParser *parser){
    if(parser->argcount != 2){
        printf("Invalid number of arguments for remove: remove filename\n");
        return 0;
    }
    if(!checkFilename(parser->args[1])){
        printf("Invalid filename format\n");
        return 0;
    }
    removeFile(parser);
    return 1;
}

int parseListF(CommandParser *parser){
    if(parser->argcount != 1){
        printf("Invalid number of arguments for list: list\n");
        return 0;
    }
    return 1;
}