#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "udp.h"
#include "inputHandlers.h"

#define LOGIN_EXPECTED_REPLY_CODE       "RLI"
#define LOGOUT_EXPECTED_REPLY_CODE      "RLO"
#define UNREGISTER_EXPECTED_REPLY_CODE  "RUR"
#define PUBLISH_EXPECTED_REPLY_CODE     "RPB"
#define REMOVE_EXPECTED_REPLY_CODE      "RRM"

#define STATUS_OK               "OK"
#define STATUS_REG              "REG"
#define STATUS_NOT_REG          "UNR"
#define STATUS_WRONG_PWD        "WRP"
#define STATUS_ERR              "ERR"
#define STATUS_NOK              "NOK"
#define STATUS_NOT_LOGGED_IN    "NLG"

#define MSG_WRONG_PWD           "Incorrect password"
#define MSG_UNREGISTERED_USER   "User not registered"
#define MSG_USER_NOT_LOGGED_IN  "User not logged in"
#define MSG_INVALID_REQUEST     "Invalid request"

#define MAX_MSG_LENGTH 256
#define STATUS_CODE_LENGTH 3  // Response status codes are <= 3 chars.


// Mapping logic: Status code vs corresponding message (that gets displayed to the user).

typedef struct { 
    const char *status;
    const char *message;
} StatusMsg;

static const StatusMsg login_replies[] = {
    {STATUS_OK,  "Successful login"},
    {STATUS_REG, "New user registered"},
    {STATUS_NOK, MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const StatusMsg logout_replies[] = {
    {STATUS_OK,  "Successful logout"},
    {STATUS_NOT_LOGGED_IN, MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const StatusMsg unregister_replies[] = {
    {STATUS_OK,  "Successful unregister"},
    {STATUS_NOK, MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const StatusMsg publish_replies[] = {
    {STATUS_OK,  "Successful publication"},
    {STATUS_NOT_LOGGED_IN, MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_NOK, "Unsuccessful publication"},
    {STATUS_ERR, MSG_INVALID_REQUEST}
};

static const StatusMsg remove_replies[] = {
    {STATUS_OK, "Sucessful removal"},
    {STATUS_NOT_LOGGED_IN, MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_NOK, "This file was not published by you"}, //nao existe tb ?
    {STATUS_ERR, MSG_INVALID_REQUEST}
};

static const char *lookup_message(const StatusMsg *table, int n, const char *status) {
    for (int i = 0; i < n; i++)
        if (strcmp(table[i].status, status) == 0)
            return table[i].message;
    return "Unrecognized reply from DS";
}

#define LOOKUP(table, status) lookup_message(table, sizeof(table) / sizeof(table[0]), status)



void login(CommandParser *parser) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];

    if (parser->state->user.loggedIn) {
        printf("Already logged in. Logout first\n");
        return;
    }

    snprintf(request, sizeof(request), "LIN %s %s %d\n", parser->args[1], parser->args[2], parser->state->peer_tcp_port);
    //printf("Sending login request: %s", request);
    if (send_and_recv(parser->state->udp_fd, parser->state->ds_addr, request, reply, sizeof(reply)) == -1)
        return;

    // NOTE: Hardcoded error code size.
    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {
        printf("Unexpected reply from DS after login attempt\n");
        return;
    }
    
    if (strcmp(reply_code, LOGIN_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after login attempt: %s\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(login_replies, status));

    if (strcmp(status, STATUS_OK) == 0 || strcmp(status, STATUS_REG) == 0) {
        parser->state->user.loggedIn = 1;
        strncpy(parser->state->user.UID, parser->args[1], UID_LEN + 1);
        strncpy(parser->state->user.password, parser->args[2], PWD_LEN + 1);
    }
}

void logout(CommandParser *parser) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];

    if (!parser->state->user.loggedIn) {
        printf("No user is currently logged in\n");
        return;
    }

    snprintf(request, sizeof(request), "LOU %s %s\n", parser->state->user.UID, parser->state->user.password);
    //printf("Sending logout request: %s", request);
    if (send_and_recv(parser->state->udp_fd, parser->state->ds_addr, request, reply, sizeof(reply)) == -1)
        return;

    // NOTE: Hardcoded error code size.
    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {
        printf("Unexpected reply from DS after logout attempt\n");
        return;
    }

    if (strcmp(reply_code, LOGOUT_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after logout attempt: %s\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(logout_replies, status));

    if (strcmp(status, STATUS_OK) == 0)
        parser->state->user.loggedIn = 0;
}

void unregister(CommandParser *parser) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];

    if (!parser->state->user.loggedIn) {
        printf("No user is currently logged in\n");
        return;
    }

    snprintf(request, sizeof(request), "UNR %s %s\n", parser->state->user.UID, parser->state->user.password);

    if (send_and_recv(parser->state->udp_fd, parser->state->ds_addr, request, reply, sizeof(reply)) == -1)
        return;

    // NOTE: Hardcoded error code size.
    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {
        printf("Unexpected reply from DS after unregister attempt\n");
        return;
    }

    if (strcmp(reply_code, UNREGISTER_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after unregister attempt: %s\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(unregister_replies, status));

    if (strcmp(status, STATUS_OK) == 0)
        parser->state->user.loggedIn = 0; // The DS logs the user out before unregistering it.
}

//dar refactor ao argumento fileSIze
void publishFile(CommandParser *parser, int fileSize) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];
    
    if(!parser->state->user.loggedIn){
        printf("No user is currently logged in\n");
        return;
    }

    
    snprintf(request, sizeof(request), "PUB %s %s %s %d %s\n", 
    parser->state->user.UID, parser->state->user.password, parser->args[1], fileSize, parser->args[2]);
    //printf("Sending publish request: %s", request);
    if(send_and_recv(parser->state->udp_fd, parser->state->ds_addr, request, reply, sizeof(reply)) == -1){
        printf("Failed to send publish request or receive reply from DS\n");
        return;
    }
    
    //printf("Reply from DS: %s", reply);
    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {
        printf("Unexpected reply from DS after publish attempt\n");
        return;
    }

    if (strcmp(reply_code, PUBLISH_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after publish attempt: %s\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(publish_replies, status));
    
    //verificar replies anomalas por parte do DS
}

void removeFile(CommandParser *parser){
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1]; 
    
    if(!parser->state->user.loggedIn){
        printf("No user is currently logged in\n");
        return;
    }

    snprintf(request, sizeof(request), "REM %s %s %s\n", parser->state->user.UID, parser->state->user.password, parser->args[1]);
    printf("Sending remove file request: %s", request);
    //fazer o send
    if(send_and_recv(parser->state->udp_fd, parser->state->ds_addr, request, reply, sizeof(reply)) == -1){
        printf("Failed to send remove file request or receive reply from DS\n");
        return;
    }

    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {
        printf("Unexpected reply from DS after remove attempt\n");
        return;
    }

    if (strcmp(reply_code, REMOVE_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after remove attempt: %s\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(remove_replies, status));
    
}

void listFiles(){
    //NÃO É PRECISO LOGIN!!!
}