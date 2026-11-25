#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "peer_client.h"
#include "inputValidation.h"
#include "tcp.h"
#include "protocol.h"

#define PEER_CLIENT_TIMEOUT 5  // Seconds to wait for a serving peer's send/receive.

#define PEER_REQUEST_MAX    64 // Longest valid request is 29 bytes.

// TODO: Optimal size? TBC!
#define PEER_CHUNK_SIZE 4096   // Size of the chunks used to stream a file.
#define PEER_HEADER_MAX 32     // RGT OK <size> <data>


// Delete partial file
static void discard(int out, const char *tmp_path) {
    close(out);
    unlink(tmp_path);
}

// Reads "RGT OK <size> " or "RGT NOK\n".
static int read_rgt_header(int fd, long *size_out) {
    char hdr[PEER_HEADER_MAX + 1];
    size_t len = 0;
    int spaces = 0;

    while (len < PEER_HEADER_MAX) {
        ssize_t n = read(fd, &hdr[len], 1);
        if (n == -1) {
            perror("Error receiving reply from peer");
            return PEER_DL_FAILED;
        }
        if (n == 0) {
            fprintf(stderr, "Peer closed the connection early\n");
            return PEER_DL_FAILED;
        }
        char c = hdr[len++];
        if (c == '\n')
            break;  // RGT NOK\n
        if (c == ' ' && ++spaces == 3)
            break;  // RGT OK <size> <data>
    }
    hdr[len] = '\0';

    if (strcmp(hdr, "RGT NOK\n") == 0)
        return PEER_DL_REFUSED;

    // 7-char prefix "RGT OK " + 1 to 8 digits (FSIZE_MAX_DIGITS is 8) + a trailing whitespace char.
    int digits = (int) len - 8;
    if (strncmp(hdr, "RGT OK ", 7) != 0 || hdr[len - 1] != ' ' || digits < 1 || digits > FSIZE_MAX_DIGITS) {
        fprintf(stderr, "Malformed reply from peer\n");
        return PEER_DL_FAILED;
    }
    for (int i = 0; i < digits; i++) {
        if (!isdigit((unsigned char) hdr[7 + i])) {
            fprintf(stderr, "Malformed reply from peer\n");
            return PEER_DL_FAILED;
        }
    }

    long size = strtol(hdr + 7, NULL, 10);
    if (!checkFileSize(size)) {
        fprintf(stderr, "Peer announced an invalid file size\n");
        return PEER_DL_FAILED;
    }
    *size_out = size;
    return PEER_DL_OK;
}

// Streams exactly 'size' bytes into <filename>.part, then renames it over <filename>.
static int save_body(int fd, const char *filename, long size) {
    char tmp_path[FILENAME_MAX_LEN + 6];    // ".part" + '\0'
    char chunk[PEER_CHUNK_SIZE];
    long remaining = size;
    int out;

    snprintf(tmp_path, sizeof(tmp_path), "%s.part", filename);
    out = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out == -1) {
        perror("Error creating local file");
        return -1;
    }

    while (remaining > 0) {
        size_t want = remaining < (long) sizeof(chunk) ? (size_t) remaining : sizeof(chunk);
        ssize_t n = read(fd, chunk, want);
        if (n == -1) {
            perror("Error receiving file");
            discard(out, tmp_path);
            return -1;
        }
        if (n == 0) {
            fprintf(stderr, "Peer closed the connection before the file was complete\n");
            discard(out, tmp_path);
            return -1;
        }
        if (write_all(out, chunk, (size_t) n) == -1) {
            perror("Error writing local file");
            discard(out, tmp_path);
            return -1;
        }
        remaining -= n;
    }

    if (close(out) == -1) {
        perror("Error closing local file");
        unlink(tmp_path);
        return -1;
    }
    if (rename(tmp_path, filename) == -1) {
        perror("Error saving local file");
        unlink(tmp_path);
        return -1;
    }
    return 0;
}

// Returns a socket connected to ip:port, or returns -1 on failure.
static int peer_connect(const char *ip, int port) {
    struct addrinfo hints, *res;
    struct timeval tv;
    char port_str[PORT_STR_MAX_LEN + 1];
    int fd, errcode;

    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;   // The address is already numeric (no DNS lookups).

    errcode = getaddrinfo(ip, port_str, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "Error getting peer address\n");
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
        perror("Error creating peer socket");
        freeaddrinfo(res);
        return -1;
    }

    tv.tv_sec = PEER_CLIENT_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        perror("Error setting peer socket timeout");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    // TODO: Connect might also time out - same as in tcp_exchange!
    // NOTE : ACHO QUE É UM POUCO COMPLICADO DE IMPLEMENTAR para a utiildade que pode ter
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Error connecting to peer");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);
    return fd;
}

int peer_download(const char *ip, int port, const char *filename, long *size_out) {
    char request[PEER_REQUEST_MAX];
    long size;
    int fd, result;

    // Create the socket and establish the TCP connection.
    fd = peer_connect(ip, port);
    if (fd == -1)
        return PEER_DL_FAILED;

    // Send the request.
    snprintf(request, sizeof(request), "GET %s\n", filename);
    if (write_all(fd, request, strlen(request)) == -1) {
        perror("Error sending request to peer");
        close(fd);
        return PEER_DL_FAILED;
    }

    // Retrieve the response.
    result = read_rgt_header(fd, &size);
    if (result == PEER_DL_OK) {
        if (save_body(fd, filename, size) == -1)
            result = PEER_DL_FAILED;
        else
            *size_out = size;
    }

    // The requester is expected to close.
    close(fd);
    return result;
}