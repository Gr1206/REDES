#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "udp.h"

int udp_setup(const char *ds_ip, const char *ds_port, int *fd_out, struct addrinfo **addr_out) {
    struct addrinfo hints, *res;
    struct timeval tv;
    int fd, errcode;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(ds_ip, ds_port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "Error getting UDP address\n");
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
        perror("Error creating UDP socket");
        freeaddrinfo(res);
        return -1;
    }

    // Ensure recvfrom() doesn't hang indefinitely if no reply is received.
    tv.tv_sec = UDP_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("Error setting UDP socket timeout");
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    *fd_out = fd;
    *addr_out = res;
    return 0;
}

ssize_t send_and_recv(int fd, struct addrinfo *res, char *request, char *reply_buffer, size_t buffer_len) {
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in from;

    n = sendto(fd, request, strlen(request), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("Error sending request");
        return -1;
    }

    // Since SO_RCVTIMEO is set on the socket at startup, whenever UDP_TIMEOUT
    // seconds pass without a reply, a timeout gets raised (i.e., recvfrom() 
    // fails with EAGAIN/EWOULDBLOCK).
    addrlen = sizeof(from);
    // buffer_len - 1 reserves space for the null terminator added below,
    // since recvfrom() does not null-terminate the data it writes.
    n = recvfrom(fd, reply_buffer, buffer_len - 1, 0, 
                (struct sockaddr *) &from, &addrlen);
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stderr, "No reply from DS (timeout)\n");
        } else {
            perror("Error receiving response");
        }
        return -1;
    }

    reply_buffer[n] = '\0';
    return n;   
}