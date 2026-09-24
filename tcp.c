#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "tcp.h"

int tcp_setup(const char *ds_ip, const char *ds_tcp_port, struct addrinfo **tcp_addr) {
    struct addrinfo hints;
    int errcode;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(ds_ip, ds_tcp_port, &hints, tcp_addr);
    if (errcode != 0) {
        fprintf(stderr, "Error getting TCP address\n");
        return -1;
    }

    return 0;
}

ssize_t tcp_exchange(struct addrinfo *tcp_addr, const char *request, char *reply_buffer, size_t buffer_len) {
    int fd;
    ssize_t n;
    size_t written, req_len, total;
    struct timeval tv;

    fd = socket(tcp_addr->ai_family, tcp_addr->ai_socktype, tcp_addr->ai_protocol);
    if (fd == -1) {
        perror("Error creating TCP socket");
        return -1;
    }

    // Timeout logic for both read and write.
    tv.tv_sec = TCP_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        perror("Error setting TCP socket timeout");
        close(fd);
        return -1;
    }

    // TODO: Connect might also time out - an issue that still needs to be looked into!
    if (connect(fd, tcp_addr->ai_addr, tcp_addr->ai_addrlen) == -1) {
        perror("Error connecting to DS");
        close(fd);
        return -1;
    }

    // Since write() may send fewer bytes than requested, a loop is required.
    req_len = strlen(request);
    written = 0;
    while (written < req_len) {
        n = write(fd, request + written, req_len - written);
        if (n == -1) {
            perror("Error sending request");
            close(fd);
            return -1;
        }
        written += (size_t) n;
    }

    // Reads until:
    // a) a '\n' is seen;
    // b) the buffer is exhausted;
    // c) the connection closes early (incomplete/failed reply).
    total = 0;
    while (total < buffer_len - 1) {
        n = read(fd, reply_buffer + total, buffer_len - 1 - total);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fprintf(stderr, "No reply from DS (timeout)\n");
            } else {
                perror("Error receiving response");
            }
            close(fd);
            return -1;
        }
        if (n == 0) {
            fprintf(stderr, "Connection closed before a complete reply was received\n");
            close(fd);
            return -1;
        }
        total += (size_t) n;
        reply_buffer[total] = '\0';
        if (memchr(reply_buffer, '\n', total) != NULL)
            break;
    }

    if (memchr(reply_buffer, '\n', total) == NULL) {
        fprintf(stderr, "Reply too large or missing terminator\n");
        close(fd);
        return -1;
    }

    close(fd);
    return (ssize_t) total;
}