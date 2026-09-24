#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "user.h"
#include "commands.h"
#include "app_state.h"
#include "client.h"
#include "udp.h"
#include "inputHandlers.h"
#include "inputValidation.h" //vai sair daqui quando o parse do argv tiver noutro sitio

volatile sig_atomic_t stop_req = 0;

void handle_sigint(int sig) {
    (void)sig;
    stop_req = 1; //SIGINT received
}

void controlledExit(int socket_fd, int exit_code, struct addrinfo *res) {
    if(res != NULL) freeaddrinfo(res);
    if(socket_fd >= 0) close(socket_fd); //close the socket if it was opened
    exit(exit_code);
}


int main(int argc, char *argv[]){
    // Parses CLI args, sets up the UDP socket to the DS, and runs the
    // interactive command loop.
    struct User user = {"", "", 0}; 
    AppState state = {user, 0, NULL, -1}; //default
    char* DSIP = "193.136.138.142";       //default 
    char* DSPORT = "59000";               //default
    struct addrinfo hints;


    signal(SIGPIPE, SIG_IGN);            //ignore SIGPIPE
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));          //initialize to zero
    sa.sa_handler = handle_sigint; 
    sigaction(SIGINT, &sa, NULL);        //handle SIGINT
    
    // Check if user provided a correct number of arguments
    //maybe meter numa função esta lógica tambem
    if (argc % 2 == 0) {
        printf("Incorrect format: ./user -m peerport [-n DSIP] [-p DSport]\n");
        exit(1);
    }
    // ./user -m peerport [-n DSIP] [-p DSport]
    for(int i = 1; i < argc; i += 2){
        if (argv[i][0] != '-') { //look for flag
            printf("Incorrect format: ./user -m peerport [-n DSIP] [-p DSport]\n");
            exit(1);
        }   
        switch (argv[i][1]){
            case 'm':
                state.peer_tcp_port = atoi(argv[i+1]);
                break;
            case 'n':
                DSIP = argv[i+1];
                break;
            case 'p':
                DSPORT = argv[i+1];
                break;
            default:
                printf("Flag not recognized\n./user -m peerport [-n DSIP] [-p DSport]\n");
                exit(1);
        }
    }

    if (state.peer_tcp_port == 0) {
        printf("Peer port is required\n");
        exit(1);
    }

    if (!checkPort(state.peer_tcp_port)) {
        printf("Invalid peer port\n");
        exit(1);
    }

    state.udp_fd = socket(AF_INET, SOCK_DGRAM, 0);  //udp socket
    if (state.udp_fd == -1) {
        perror("Error creating UDP socket");
        controlledExit(state.udp_fd, 1, state.ds_addr);
    }

    // Ensure recvfrom doesn't hang indefinitely if no response is received
    struct timeval tv;
    tv.tv_sec = UDP_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(state.udp_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("Error setting UDP socket timeout");
        controlledExit(state.udp_fd, 1, state.ds_addr);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_DGRAM; //UDP

    int errcode = getaddrinfo(DSIP, DSPORT, &hints, &state.ds_addr);
    if (errcode != 0) { 
        fprintf(stderr, "Error getting address\n"); 
        controlledExit(state.udp_fd, 1, state.ds_addr);
    }

    CommandParser parser;
    parser.state = &state;
    while(1){
        char line[256];

        printf("> ");
        fflush(stdout);                                 //to ensure the buffer gets flushed

        if (fgets(line, sizeof(line), stdin) == NULL) { //unable to read input
            if(stop_req) {                              //handle SIGINT
                if(state.user.loggedIn) {
                    logout(state.udp_fd, state.ds_addr, &state.user);
                }
                printf("\nExiting\n");
                break;
            }
            break;
        }

        // Parse the input into args
        parser.argcount = 0;
        char *token = strtok(line, " \n");
        while (token != NULL && parser.argcount < MAX_ARGS) {
            parser.args[parser.argcount++] = token;
            token = strtok(NULL, " \n");
        }

        if(parser.argcount == 0) { //nothing written
            continue; 
        }

        char *command = parser.args[0];
        parser.command = command;
        if(strcmp(command, "login") == 0) {
            parseLogin(&parser);
            

        } else if(strcmp(command, "unregister") == 0) {
           parseUnreg(&parser);
        } else if(strcmp(command, "logout") == 0) {
            parseLogout(&parser);
        } else if(strcmp(command, "exit") == 0) {
            parseExit(&parser);
        } else if(strcmp(command, "publish") == 0) {
            parsePublish(&parser);
        } else if(strcmp(command, "remove") == 0){
            parseRemoveF(&parser);
        } else if(strcmp(command, "list") == 0){
            parseListF(&parser);
        }else {
            printf("Command not recognized\nList of valid commands:\n-login\n-logout\n-unregister\n-exit\n");
        }

    }   
    
    controlledExit(state.udp_fd, 0, state.ds_addr);
}