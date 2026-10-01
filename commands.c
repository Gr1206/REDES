#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "udp.h"
#include "tcp.h"

#define LOGIN_EXPECTED_REPLY_CODE       "RLI"
#define LOGOUT_EXPECTED_REPLY_CODE      "RLO"
#define UNREGISTER_EXPECTED_REPLY_CODE  "RUR"
#define LIST_EXPECTED_REPLY_CODE        "RLS"
#define VERSIONS_EXPECTED_REPLY_CODE    "RVR"

#define STATUS_OK               "OK"
#define STATUS_REG              "REG"
#define STATUS_NOT_REG          "UNR"
#define STATUS_WRONG_PWD        "WRP"
#define STATUS_ERR              "ERR"
#define STATUS_NOK              "NOK"

#define MSG_WRONG_PWD           "Incorrect password"
#define MSG_UNREGISTERED_USER   "User not registered"
#define MSG_USER_NOT_LOGGED_IN  "User not logged in"
#define MSG_INVALID_REQUEST     "Invalid request"

#define MAX_MSG_LENGTH 256
#define STATUS_CODE_LENGTH 3        // Response status codes are <= 3 chars.

// NOTE: versions()-specific constants.
#define MAX_TCP_REPLY_LENGTH 16384  // ~16 KB: large enough for >100 peer entries.
#define LABEL_MAX_LEN 20
#define AVAILABILITY_LEN 3          // "AVL" or "NAV".
#define PUB_TIME_MAX_LEN 31         // TBD: Provisional (publication_time's format/length)?

// NOTE: list()-specific constants.
#define FILENAME_MAX_LEN 24
#define MAX_LIST_REPLY_LENGTH 2048  // Large enough for replies with 50 filenames.


// Mapping logic: Status code vs corresponding message (that gets displayed to the user).

typedef struct { 
    const char *status;
    const char *message;
} StatusMsg;

static const StatusMsg login_replies[] = {
    {STATUS_OK,  "Successful login"},
    {STATUS_REG, "New user registered"},
    {"NOK", MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const StatusMsg logout_replies[] = {
    {STATUS_OK,  "Successful logout"},
    {"NLG", MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const StatusMsg unregister_replies[] = {
    {STATUS_OK,  "Successful unregister"},
    {"NOK", MSG_USER_NOT_LOGGED_IN},
    {STATUS_NOT_REG, MSG_UNREGISTERED_USER},
    {STATUS_WRONG_PWD, MSG_WRONG_PWD},
    {STATUS_ERR, MSG_INVALID_REQUEST},
};

static const char *lookup_message(const StatusMsg *table, int n, const char *status) {
    for (int i = 0; i < n; i++)
        if (strcmp(table[i].status, status) == 0)
            return table[i].message;
    return "Unrecognized reply from DS";
}

#define LOOKUP(table, status) lookup_message(table, sizeof(table) / sizeof(table[0]), status)


void login(int fd, struct addrinfo *res, User *user, char* uid, char* password, int peerport) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];

    if (user->loggedIn) {
        printf("Already logged in. Logout first\n");
        return;
    }

    snprintf(request, sizeof(request), "LIN %s %s %d\n", uid, password, peerport);

    if (send_and_recv(fd, res, request, reply, sizeof(reply)) == -1)
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
        user->loggedIn = 1;
        strncpy(user->UID, uid, UID_LEN + 1);
        strncpy(user->password, password, PWD_LEN + 1);
    }
}

void logout(int fd, struct addrinfo *res, User *user) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];

    if (!user->loggedIn) {
        printf("No user is currently logged in\n");
        return;
    }

    snprintf(request, sizeof(request), "LOU %s %s\n", user->UID, user->password);

    if (send_and_recv(fd, res, request, reply, sizeof(reply)) == -1)
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
        user->loggedIn = 0;
}

void unregister(int fd, struct addrinfo *res, User *user) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_MSG_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];

    if (!user->loggedIn) {
        printf("No user is currently logged in\n");
        return;
    }

    snprintf(request, sizeof(request), "UNR %s %s\n", user->UID, user->password);

    if (send_and_recv(fd, res, request, reply, sizeof(reply)) == -1)
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
        user->loggedIn = 0; // The DS logs the user out before unregistering it.
}

void versions(struct addrinfo *tcp_addr, const char *filename) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_TCP_REPLY_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];
    int header_len = 0;

    // TODO: Missing filename validation!!

    // NOTE: No need for the user to be logged in, right? TBC!

    snprintf(request, sizeof(request), "VRS %s\n", filename);

    if (tcp_exchange(tcp_addr, request, reply, sizeof(reply)) == -1)
        return;

    // NOTE: Hardcoded error code size.
    if (sscanf(reply, "%3s %3s%n", reply_code, status, &header_len) != 2) {
        printf("Unexpected reply from DS after versions attempt\n");
        return;
    }

    if (strcmp(reply_code, VERSIONS_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after versions attempt: %s\n", reply_code);
        return;
    }

    if (strcmp(status, STATUS_NOK) == 0) {
        printf("No peers currently available for this resource\n");
        return;
    }

    if (strcmp(status, STATUS_OK) != 0) {
        printf("Unexpected status obtained from DS after versions attempt: %s\n", status);
        return;
    }

    // Parse any consecutive "UID Fsize label publication_time availability" 
    // groups, one by one.
    int pos = header_len;
    int len = (int) strlen(reply);
    int peers_found = 0;

    while (pos < len) {
        char uid[UID_LEN + 1];
        char label[LABEL_MAX_LEN + 1];
        char pub_time[PUB_TIME_MAX_LEN + 1];
        char availability[AVAILABILITY_LEN + 1];
        long fsize;
        int consumed = 0;

        // NOTE: Hardcoded field widths.
        int matched = sscanf(reply + pos, "%6s %ld %20s %31s %3s%n",
                             uid, &fsize, label, pub_time, availability, 
                             &consumed);
        if (matched != 5) {
            // TBC: Is there any odd failure/edge case ending up here that
            // should warrant a message to the user?
            break;
        }

        // TODO: Abstract the availability comparison into a helper function?
        // TODO: Extract the string constant to a constant of its own!
        printf("%s | %ld bytes | %s | %s | %s\n", uid, fsize, label, pub_time,
               strcmp(availability, "AVL") == 0 ? "Available" : "Not available");

        peers_found = 1;
        pos += consumed;
    }

    if (!peers_found)
        printf("Unexpected reply from DS: status OK but no peer entries found\n");
}

void list(int fd, struct addrinfo *res) {
    char request[MAX_MSG_LENGTH + 1];
    char reply[MAX_LIST_REPLY_LENGTH + 1];
    char reply_code[STATUS_CODE_LENGTH + 1];
    char status[STATUS_CODE_LENGTH + 1];
    int header_len = 0;
 
    snprintf(request, sizeof(request), "LST\n");
 
    if (send_and_recv(fd, res, request, reply, sizeof(reply)) == -1)
        return;
 
    // NOTE: Hardcoded error code size.
    if (sscanf(reply, "%3s %3s%n", reply_code, status, &header_len) != 2) {
        printf("Unexpected reply from DS after list attempt\n");
        return;
    }
 
    if (strcmp(reply_code, LIST_EXPECTED_REPLY_CODE) != 0) {
        printf("Unexpected reply code obtained from DS after list attempt: %s\n", reply_code);
        return;
    }
 
    if (strcmp(status, STATUS_NOK) == 0) {
        printf("No resources are currently known\n");
        return;
    }
 
    if (strcmp(status, STATUS_OK) != 0) {
        printf("Unexpected status obtained from DS after list attempt: %s\n", status);
        return;
    }
 
    // Parse any existing consecutive filenames, one by one.
    int pos = header_len;
    int len = (int) strlen(reply);
    int files_found = 0;
 
    while (pos < len) {
        char filename[FILENAME_MAX_LEN + 1];
        int consumed = 0;
 
        // NOTE: Hardcoded field width.
        int matched = sscanf(reply + pos, "%24s%n", filename, &consumed);
        if (matched != 1) {
            // TBC: Is there any odd failure/edge case ending up here that
            // should warrant a message to the user?
            break;
        }
 
        printf("%s\n", filename);
        files_found = 1;
        pos += consumed;
    }
 
    if (!files_found)
        printf("Unexpected reply from DS: status OK but no filenames found\n");
}