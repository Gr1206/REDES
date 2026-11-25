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
#include <errno.h>

#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "user.h"
#include "commands.h"
#include "app_state.h"
#include "udp.h"
#include "tcp.h"
#include "peer_server.h"
#include "inputHandlers.h"
#include "inputValidation.h"

#define USAGE           "./user -m peerport [-n DSIP] [-p DSport]"
#define DEFAULT_DS_IP   "193.136.138.142"
#define DEFAULT_DS_PORT "59000"

#define MAX_INPUT_LEN   256   // User input buffer size.
#define INPUT_OK        0
#define INPUT_TOO_LONG  1
#define INPUT_EOF      -1
#define SELECT_TIMEOUT  5
volatile sig_atomic_t stop_req = 0;

/**
 * @brief Signal handler for SIGINT
 * @param sig The signal number received
 */
static void handle_sigint(int sig) {
    (void)sig;
    stop_req = 1; //SIGINT received
}

/**
 * @brief Controlled exit of the application
 * @param state The shared app state object
 * @param exit_code The exit code to return
 */
static void controlledExit(AppState *state, int exit_code) {
    if(state->ds_addr != NULL) freeaddrinfo(state->ds_addr);
    if(state->ds_tcp_addr != NULL) freeaddrinfo(state->ds_tcp_addr);
    if(state->udp_fd >= 0) close(state->udp_fd); //close the socket if still open
    if(state->peer_fd >= 0) close(state->peer_fd);
    exit(exit_code);
}

/**
 * @brief Reads one line of user input.
 *
 * This function prevents issues whenever user input is longer than the 
 * provided buffer - any input exceeding the buffer size would remain in 
 * stdin and immediately be read as if it were new user input. This function 
 * prevents that issue by ensuring any input exceeding the buffer size gets 
 * immediately discarded.
 *
 * @param buf Where the line is stored.
 * @param size Size of the buffer.
 * @return INPUT_OK, INPUT_TOO_LONG or INPUT_EOF.
 */
static int read_input_line(char *buf, size_t size) {
    if (fgets(buf, size, stdin) == NULL)
        return INPUT_EOF;

    if (strchr(buf, '\n') != NULL)
        return INPUT_OK;

    // No '\n'? Either the input did not fit the buffer (or somehow it ended 
    // without a final '\n'). Discard everything else to ensure nothing is 
    // left in the buffer.
    int c, leftover_chars = 0;
    while ((c = getchar()) != '\n' && c != EOF)
        leftover_chars = 1;

    return leftover_chars ? INPUT_TOO_LONG : INPUT_OK;
}

/**
 * @brief Parses the startup user program command line arguments.
 *
 * @param argc Arg count (as received by main).
 * @param argv Arg vector (as received by main).
 * @param peer_port The validated peer TCP port.
 * @param ds_ip The DS IP (or hostname).
 * @param ds_port The validated DS port (as a string).
 * @return 0 on success, -1 on invalid input.
 */
static int parse_startup_args(int argc, char *argv[], int *peer_port,
                              const char **ds_ip, const char **ds_port) {
    *peer_port = 0;
    *ds_ip = DEFAULT_DS_IP;
    *ds_port = DEFAULT_DS_PORT;

    // Check if user provided a correct number of arguments
    if (argc % 2 == 0) {
        printf("Incorrect format: " USAGE "\n");
        return -1;
    }

    // ./user -m peerport [-n DSIP] [-p DSport]
    for (int i = 1; i < argc; i += 2) {
        if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
            printf("Incorrect format: " USAGE "\n");
            return -1;
        }

        switch (argv[i][1]) {
            case 'm':
                if (!checkPortStr(argv[i+1], peer_port)) {
                    printf("Invalid peer port\n");
                    return -1;
                }
                break;
            case 'n':
                *ds_ip = argv[i+1];
                break;
            case 'p': {
                int port;
                if (!checkPortStr(argv[i+1], &port)) {
                    printf("Invalid DS port\n");
                    return -1;
                }
                *ds_port = argv[i+1];
                break;
            }
            default:
                printf("Flag not recognized\n" USAGE "\n");
                return -1;
        }
    }

    if (*peer_port == 0) {
        printf("Peer port is required\n");
        return -1;
    }
    return 0;
}

/** @brief Prints the command prompt. */
static void print_prompt(void) {
    printf("> ");
    fflush(stdout);     //to ensure the buffer gets flushed
}

int main(int argc, char *argv[]){
    // Parses CLI args, sets up the UDP socket to the DS, and runs the
    // interactive command loop.
    AppState state = {
        // Everything else is left as default in the instance (either 0 or NULL)
        .udp_fd = -1, 
        .peer_fd = -1
    };
    const char *DSIP;
    const char *DSPORT;

    signal(SIGPIPE, SIG_IGN);            //ignore SIGPIPE
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));          //initialize to zero
    sa.sa_handler = handle_sigint; 
    sigaction(SIGINT, &sa, NULL);        //handle SIGINT

    if (parse_startup_args(argc, argv, &state.peer_tcp_port, &DSIP, &DSPORT) != 0)
        exit(1);
    
    // Initial UDP setup
    if (udp_setup(DSIP, DSPORT, &state.udp_fd, &state.ds_addr) != 0) {
        controlledExit(&state, 1);
    }

    // Initial TCP setup
    if (tcp_setup(DSIP, DSPORT, &state.ds_tcp_addr) != 0) {
        controlledExit(&state, 1);
    }

    // Initial peer-to-peer server setup
    state.peer_fd = peer_setup(state.peer_tcp_port);
    if (state.peer_fd == -1) {
        controlledExit(&state, 1);
    }

    CommandParser parser;
    parser.state = &state;

    // With stdin's default buffering, extra lines would sit in the buffer, 
    // unseen by select. With an unbuffered stdin, fgets reads byte by byte 
    // up to '\n', leaving any further input in the kernel, where select() 
    // can still see it.
    setvbuf(stdin, NULL, _IONBF, 0);

    print_prompt();


    while (!stop_req) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(state.peer_fd, &readfds);
        struct timeval selectTv;
        selectTv.tv_sec = SELECT_TIMEOUT;
        selectTv.tv_usec = 0;

        int maxfd = (state.peer_fd > STDIN_FILENO) ? state.peer_fd : STDIN_FILENO;

        // TODO: We need select to have a timeout as well, as stated in class by the professor!
        if (select(maxfd + 1, &readfds, NULL, NULL, &selectTv) == -1) {
            if (errno == EINTR)
                continue;   // SIGINT
            perror("Error waiting for input");
            break;
        }

        if (FD_ISSET(state.peer_fd, &readfds)) {
            int connfd = accept(state.peer_fd, NULL, NULL);
            if (connfd == -1) {
                fprintf(stderr, "\nError accepting peer connection: %s\n", strerror(errno));
            } else {
                serve_peer(connfd, &state);
            }
            print_prompt();
        }

        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            char line[MAX_INPUT_LEN];
            int input_status = read_input_line(line, sizeof(line));

            if (input_status == INPUT_EOF) { //unable to read input
                /*if(stop_req) {                              //handle SIGINT
                    if(state.user.loggedIn) { //o prof não quer isto supostamente
                        logout(state.udp_fd, state.ds_addr, &state.user);
                    } */
                printf("\nExiting\n");
                break;
            }
            if (input_status == INPUT_TOO_LONG) {
                printf("Input too long (max %d characters)\n", MAX_INPUT_LEN - 1);
                print_prompt();
                continue;
            }

            // Parse the input into args
            parser.argcount = 0;
            char *token = strtok(line, " \n");
            while (token != NULL && parser.argcount < MAX_ARGS) {
                parser.args[parser.argcount++] = token;
                token = strtok(NULL, " \n");
            }

            if (parser.argcount > 0) {
                parser.command = parser.args[0];
                executeCommand(&parser);
            }

            if (state.exit_requested)
                break;

            print_prompt();
        }
    }
    
    controlledExit(&state, 0);
}