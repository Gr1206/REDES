#ifndef COMMANDS_H
#define COMMANDS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "user.h"
// #include "app_state.h"


// typedef void (*function_handler)(char *args, AppState *state, int *running);

// typedef struct {
//     const char function_name[128];    // TODO: const - with or without?
//     function_handler function;
// } Command;

// const Command *find_command(const char *name);

// void handle_login(char *args, AppState *state, int *running);
// void handle_logout(char *args, AppState *state, int *running);
// void handle_unregister(char *args, AppState *state, int *running);
// void handle_exit(char *args, AppState *state, int *running);

void login(int fd, struct addrinfo *res, User *user, char* uid, char* password, int peerport);
void logout(int fd, struct addrinfo *res, User *user);
void unregister(int fd, struct addrinfo *res, User *user);

#endif