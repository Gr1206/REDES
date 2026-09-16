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

#include "user.h"
#include "commands.h"

#define MAX_ARGS 10

// EDIT: Passei esta struct para um módulo user.h (para usar em commands.h, p.ex.!)
// typedef struct User { //so vou dar assign depois do login ser estabelecido
//     char UID[7];
//     char password[9];
//     int loggedIn;
// } User;

void controlledExit(int socket_fd, int exit_code, struct addrinfo *res) {
    //limpar memoria
    //fechar sockets
    //sair do programa
    if(res != NULL) freeaddrinfo(res);
    close(socket_fd);
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
    struct User user = {"", "", 0}; //provavelemnte tem de ser char
    int peerport = 0;
    char* DSIP = "193.136.138.142"; //default 
    char* DSPORT = "59000"; //default
    int fd; //socket fd
    struct addrinfo hints, *res = NULL; //para n dar erro no exit
    struct sockaddr_in server_addr;

    char *args[MAX_ARGS];
    int argcount = 0;

/* TENHO DE VER SE TENGO DE DAR LOGOUT AQUI
    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa;
    sa.sa_handler = ;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGINT, &sa, NULL); 
     */

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
                peerport = atoi(argv[i+1]);
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

    if (peerport == 0) {
        printf("Porta do peer é obrigatória\n");
        exit(1);
    }

    if (!checkPort(peerport)) {
        printf("Porta do peer inválida\n");
        exit(1);
    }

      
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("Erro ao criar socket");
        controlledExit(fd, 1, res);
        // TODO: Hmmm, exit() repetido? Também está em controlledExit! Confirmar!
        exit(1); //saída controlada talvez
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_DGRAM; //UDP

    int errcode = getaddrinfo(DSIP, DSPORT, &hints, &res);
    if (errcode != 0) { //meter mensagem de erro
        fprintf(stderr, "Erro ao obter endereço\n");
        controlledExit(fd, 1, res);
    }

    while(1){
        char line[256];
        // EDIT: Adicionei a validação para o caso do stdin ficar indisponível 
        // entretanto e line não ser inicializada!
        if (fgets(line, sizeof(line), stdin) == NULL)
            break;

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
            if(argcount != 4) {     // EDIT: Alterei aqui para 4 - por causa da peerport!
                printf("Número de argumentos inválido: login UID password\n");
                continue;
            }
            //chamar função de login
            // EDIT: Acrescentei aqui uma lógica meio martelada para a peerport (só para funcionar!)
            if(checkUID(args[1]) && checkPassword(args[2]) && checkPort(atoi(args[3]))){
                login(fd, res, &user, args[1], args[2], args[3]);
            } else {
                printf("Formato de UID ou Password inválido\n");
            }

        } else if(strcmp(command, "unregister") == 0) {
            if(argcount != 1){
                printf("Número de argumentos inválido: unregister\n");
                continue;
            }
            unregister(fd, res, &user);

        } else if(strcmp(command, "logout") == 0) {
            // logout
            if(argcount != 1){
                printf("Número de argumentos inválido: logout\n");
                continue;
            }
            logout(fd, res, &user);

        } else if(strcmp(command, "exit") == 0) {
            if(argcount != 1){
                printf("Número de argumentos inválido: exit\n");
                continue;
            }
            if(user.loggedIn) {
                printf("É necessário dar logout antes de sair\n");
                continue;
            } else {
                controlledExit(fd, 0, res);
                //break; 
            }
        } else {
            printf("Comando não reconhecido\nLista de comandos válidos:\n-login\n-logout\n-unregister\n-exit\n");
        }

    }   
    
    
    return 0;
}