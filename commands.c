
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "udp.h"

#define LOGIN_EXPECTED_REPLY_CODE       "RLI"
#define LOGOUT_EXPECTED_REPLY_CODE      "RLO"
#define UNREGISTER_EXPECTED_REPLY_CODE  "RUR"

#define STATUS_OK               "OK"
#define STATUS_REG              "REG"
#define STATUS_NOT_REG          "UNR"
#define STATUS_WRONG_PWD        "WRP"
#define STATUS_ERR              "ERR"

#define MSG_WRONG_PWD           "incorrect password"
#define MSG_UNREGISTERED_USER   "user not registered"
#define MSG_USER_NOT_LOGGED_IN  "user not logged in"
#define MSG_INVALID_REQUEST     "invalid request"


// Mapeamento entre códigos de status e mensagens para o utilizador.

typedef struct { 
    const char *status; const char *message; 
} StatusMsg;

static const StatusMsg login_replies[] = {
    {STATUS_OK,  "successful login"},
    {STATUS_REG, "new user registered"},
    {"NOK", MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const StatusMsg logout_replies[] = {
    {STATUS_OK,  "successful logout"},
    {"NLG", MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const StatusMsg unregister_replies[] = {
    {STATUS_OK,  "successful unregister"},
    {"NOK", MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const char *lookup_message(const StatusMsg *table, int n, const char *status) {
    for (int i = 0; i < n; i++)
        if (strcmp(table[i].status, status) == 0)
            return table[i].message;
    return "unrecognized reply from DS";
}

#define LOOKUP(table, status) lookup_message(table, sizeof(table) / sizeof(table[0]), status)



void login(int fd, struct addrinfo *res, User *user, char* uid, char* password, int peerport) {
    char request[128];  // TODO: Shall we set a default constant for max request length?
    char reply[128];  // TODO: Shall we set a default constant for max reply length?
    char reply_code[4];  // TODO: What about malformed values with length > 3?
    char status[4];   // TODO: What about malformed values with length > 3?

    if (user->loggedIn) {
        printf("Already logged in. Logout first.\n");
        return;
    }

    // Validações do input (UID, password, etc) feitas no client_udp. Passar para aqui dentro eventualmente?
    // (...)

    snprintf(request, sizeof(request), "LIN %s %s %d\n", uid, password, peerport);

    if (send_and_recv(fd, res, request, reply, sizeof(reply)) == -1)
        return;

    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {    // TODO: String width limit TBC!
        printf("Unexpected reply from DS after login attempt.\n");
        return;
    }
    
    if (strcmp(reply_code, LOGIN_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after login attempt: %s.\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(login_replies, status));

    if (strcmp(status, STATUS_OK) == 0 || strcmp(status, STATUS_REG) == 0) {
        user->loggedIn = 1;
        strncpy(user->UID, uid, UID_LEN + 1);
        strncpy(user->password, password, PWD_LEN + 1);
    }
}

void logout(int fd, struct addrinfo *res, User *user) {
    char request[128];  // TODO: Shall we set a default constant for max request length?
    char reply[128];  // TODO: Shall we set a default constant for max reply length?
    char reply_code[4];  // TODO: What about malformed values with length > 3?
    char status[4];   // TODO: What about malformed values with length > 3?

    if (!user->loggedIn) {
        printf("No user is currently logged in.\n");
        return;
    }

    snprintf(request, sizeof(request), "LOU %s %s\n", user->UID, user->password);

    if (send_and_recv(fd, res, request, reply, sizeof(reply)) == -1)
        return;

    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {    // TODO: String width limit TBC!
        printf("Unexpected reply from DS after logout attempt.\n");
        return;
    }

    if (strcmp(reply_code, LOGOUT_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after logout attempt: %s.\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(logout_replies, status));

    if (strcmp(status, STATUS_OK) == 0)
        user->loggedIn = 0;
}

void unregister(int fd, struct addrinfo *res, User *user) {
    char request[128];  // TODO: Shall we set a default constant for max request length?
    char reply[128];  // TODO: Shall we set a default constant for max reply length?
    char reply_code[4];  // TODO: What about malformed values with length > 3?
    char status[4];   // TODO: What about malformed values with length > 3?

    if (!user->loggedIn) {
        printf("No user is currently logged in.\n");
        return;
    }

    snprintf(request, sizeof(request), "UNR %s %s\n", user->UID, user->password);

    if (send_and_recv(fd, res, request, reply, sizeof(reply)) == -1)
        return;

    if (sscanf(reply, "%3s %3s", reply_code, status) != 2) {    // TODO: String width limit TBC!
        printf("Unexpected reply from DS after unregister attempt.\n");
        return;
    }

    if (strcmp(reply_code, UNREGISTER_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after logout attempt: %s.\n", reply_code);
        return;
    }

    printf("%s\n", LOOKUP(unregister_replies, status));

    if (strcmp(status, STATUS_OK) == 0)
        user->loggedIn = 0; // The DS logs the user out before unregistering it.
}
