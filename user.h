#ifndef USER_H
#define USER_H

#define UID_LEN 6
#define PWD_LEN 8

typedef struct User {
    char UID[UID_LEN + 1]; // TODO: What about larger/malformed user input sizes? Is this logic robust enough?
    char password[PWD_LEN + 1]; // TODO: What about larger/malformed user input sizes? Is this logic robust enough?
    int loggedIn;
} User;

#endif