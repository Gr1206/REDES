#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "udp.h"
// #include "app_state.h"

// int udp_setup(AppState *state, char *ds_ip, char *ds_port) {

//     int errcode;
//     struct addrinfo hints;
//     struct timeval tv;

//     state->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (state->udp_fd == -1) {
//         perror("Erro ao criar o socket");
//         return -1;
//     }

//     tv.tv_sec = UDP_TIMEOUT;
//     tv.tv_usec = 0;
//     if (setsockopt(state->udp_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
//         perror("Erro ao definir o timeout do socket UDP");
//         close(state->udp_fd);
//         return -1;
//     }

//     memset(&hints, 0, sizeof(hints));
//     hints.ai_family=AF_INET;
//     hints.ai_socktype=SOCK_DGRAM;

//     errcode = getaddrinfo(ds_ip, ds_port, &hints, &state->ds_addr);
//     if (errcode != 0) {
//         perror("Erro ao comunicar com o Directory Server");
//         close(state->udp_fd);
//         return -1;
//     }

//     return 0;
// }

ssize_t send_and_recv(int fd, struct addrinfo *res, char *request, char *reply_buffer, size_t buffer_len) {

    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in from;

    // send_to
    n = sendto(fd, request, strlen(request), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("Erro ao enviar pedido");
        return -1;
    }

    // recvfrom
    // NOTE: SO_RCVTIMEO is set on the socket at startup, so recvfrom() 
    // will fail with EAGAIN/EWOULDBLOCK after UDP_TIMEOUT seconds, rather 
    // than blocking forever.
    addrlen = sizeof(from);
    n = recvfrom(fd, reply_buffer, buffer_len - 1, 0, 
                (struct sockaddr *) &from, &addrlen);
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stderr, "No reply from DS (timeout).\n");
        } else {
            perror("Erro ao receber pedido");
        }
        return -1;
    }

    // return
    reply_buffer[n] = '\0';
    return n;   
}