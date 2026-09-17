#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include "user.h"
#include "commands.h"
#include "app_state.h"

#define MAX_ARGS 10

volatile sig_atomic_t stop_req = 0;

void handle_sigint(int sig) {
    (void)sig;
    stop_req = 1;
}
void controlledExit(int socket_fd, int exit_code, struct addrinfo *res) {
    if(res != NULL) freeaddrinfo(res);
    if(socket_fd >= 0) close(socket_fd); //verificar caso criação do socket falhe
    exit(exit_code);
}

int checkUID(char* uid){
    //verificar se o UID é válido
    int strsize = strlen(uid);
    if(strsize != 6) return 0; 

    for (int i = 0; i < strsize; i++){
        if(!isdigit(uid[i]))
            return 0;
    }
    return 1; 
}

int checkPassword(char* password){
    //verificar se a password é válida
    int strsize = strlen(password);
    if(strsize != 8) return 0;

    for (int i = 0; i < strsize; i++){
        if(!isalnum(password[i]))
            return 0;
    }
    return 1; 
}

int checkPort(int port){
    //verificar se a porta é válida
    if(port < 1 || port > 65535) return 0;
    return 1;
}

int main(int argc, char *argv[]){
    struct User user = {"", "", 0}; 
    AppState state = {user, 0, NULL, -1}; //default
    char* DSIP = "193.136.138.142"; //default 
    char* DSPORT = "59000"; //default
    struct addrinfo hints;

    char *args[MAX_ARGS];
    int argcount = 0;

    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); //incializa a zero
    sa.sa_handler = handle_sigint; 
    sigaction(SIGINT, &sa, NULL); 
    
    //caso passe flag sem nada
    if (argc % 2 == 0) {
        printf("Formato correto: ./user -m peerport [-n DSIP] [-p DSport]\n");
        exit(1);
    }
    // ./user -m peerport [-n DSIP] [-p DSport]
    for(int i = 1; i < argc; i += 2){
        if (argv[i][0] != '-') {
            printf("Formato correto : ./user -m peerport [-n DSIP] [-p DSport]\n");
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
                printf("Comando inválido\n./user -m peerport [-n DSIP] [-p DSport]\n");
                exit(1);
        }
    }

    if (state.peer_tcp_port == 0) {
        printf("Porta do peer é obrigatória\n");
        exit(1);
    }

    if (!checkPort(state.peer_tcp_port)) {
        printf("Porta do peer inválida\n");
        exit(1);
    }

    state.udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (state.udp_fd == -1) {
        perror("Erro ao criar socket");
        controlledExit(state.udp_fd, 1, state.ds_addr);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_DGRAM; //UDP


    int errcode = getaddrinfo(DSIP, DSPORT, &hints, &state.ds_addr);
    if (errcode != 0) { //meter mensagem de erro
        fprintf(stderr, "Erro ao obter endereço\n");
        controlledExit(state.udp_fd, 1, state.ds_addr);
    }

    while(1){
        char line[256];
        // EDIT: Adicionei a validação para o caso do stdin ficar indisponível 
        // entretanto e line não ser inicializada!
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if(stop_req) {
                if(state.user.loggedIn) {
                    logout(state.udp_fd, state.ds_addr, &state.user);
                }
                printf("\nExiting\n");
                break;
            }
            break;
        }

        //separar por espaços 
        argcount = 0;
        char *token = strtok(line, " \n");
        while (token != NULL && argcount < MAX_ARGS) {
            args[argcount++] = token;
            token = strtok(NULL, " \n");
        }

        //tratr os args recebidos

        if(argcount == 0) { //nada escrito
            continue; 
        }

        char *command = args[0];
        //para cada comando verificar se recebeu o número certo de argumentos
        if(strcmp(command, "login") == 0) {
            if(argcount != 3) {     // EDIT: tem de ser apenas 3 login UID password, isto tem a ver com o parsing do comando, a peerport já está guardada
                printf("Número de argumentos inválido: login UID password\n");
                continue;
            }
            //chamar função de login
            // EDIT: Acrescentei aqui uma lógica meio martelada para a peerport (só para funcionar!)

            //EDIT : tirei a lógica, a peerport já está guardada, não está no arg do login
            if(checkUID(args[1]) && checkPassword(args[2])){
                login(state.udp_fd, state.ds_addr, &state.user, args[1], args[2], state.peer_tcp_port);
            } else {
                printf("Formato de UID ou Password inválido\n");
            }

        } else if(strcmp(command, "unregister") == 0) {
            if(argcount != 1){
                printf("Número de argumentos inválido: unregister\n");
                continue;
            }
            unregister(state.udp_fd, state.ds_addr, &state.user);

        } else if(strcmp(command, "logout") == 0) {
            // logout
            if(argcount != 1){
                printf("Número de argumentos inválido: logout\n");
                continue;
            }
            logout(state.udp_fd, state.ds_addr, &state.user);

        } else if(strcmp(command, "exit") == 0) {
            if(argcount != 1){
                printf("Número de argumentos inválido: exit\n");
                continue;
            }
            if(state.user.loggedIn) {
                printf("É necessário dar logout antes de sair\n");
                continue;
            } else {
                controlledExit(state.udp_fd, 0, state.ds_addr);
                //break; 
            }
        } else {
            printf("Comando não reconhecido\nLista de comandos válidos:\n-login\n-logout\n-unregister\n-exit\n");
        }

    }   
    
    
    controlledExit(state.udp_fd, 0, state.ds_addr);
}