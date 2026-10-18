#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "peer.h"
#include "commands.h"
#include "inputValidation.h"

#define PEER_REQUEST_MAX 64    // Longest valid request is 29 bytes.

// TODO: Optimal size? TBC!
#define PEER_CHUNK_SIZE 4096   // Size of the chunks used to stream a file.
#define PEER_HEADER_MAX 32     // RGT OK <size> <data>

// Get request outcome
#define PEER_SERVED     0      // File sent successfully.
#define PEER_DROPPED    1      // Peer closed or timed out before sending a full request.
#define PEER_MALFORMED  2      // Syntax error (RGT ERR).
#define PEER_REFUSED    3      // Valid request, but unserved (RGT NOK).
#define PEER_FAILED     4      // Failure while replying (socket error, file changed...).

// write() may send fewer bytes than requested, so a loop is required.
// TODO: Shall we abstract this within tcp.h as well? Maybe adding a verbose optional arg?
static int write_all(int fd, const char *buf, size_t len) {
    size_t written = 0;
    while (written < len) {
        ssize_t n = write(fd, buf + written, len - written);
        if (n <= 0)
            return -1;
        written += (size_t) n;
    }
    return 0;
}

static void send_status(int fd, const char *status) {
    char reply[16];
    snprintf(reply, sizeof(reply), "RGT %s\n", status);
    write_all(fd, reply, strlen(reply));
}

// Reads until the first '\n' and replaces it with '\0'.
// Returns 1 for a complete request, 0 for a malformed one, or -1 if the peer 
// closed, timed out or failed.
static int read_request(int fd, char *buf, size_t size) {
    size_t total = 0;

    while (total < size - 1) {
        ssize_t n = read(fd, buf + total, size - 1 - total);
        if (n <= 0)
            return -1;
        total += (size_t) n;

        char *newline = memchr(buf, '\n', total);
        if (newline != NULL) {
            if (newline != buf + total - 1 || memchr(buf, '\0', total) != NULL)
                return 0;
            *newline = '\0';
            return 1;
        }
    }
    return 0;   // Buffer full without a '\n'
}

// Only regular files within the protocol's size limit can be served.
static int servable(const struct stat *st) {
    // TODO: To be refactored once the inputValidation function gets fixed!
    return S_ISREG(st->st_mode) && checkFileSize(st->st_size);
}

// Sends "RGT OK <size> ", the file contents, and a trailing '\n'.
// ASSUMPTION: layout not fully specified (see notes).
static int stream_file(int connfd, int filefd) {
    char header[PEER_HEADER_MAX];
    char chunk[PEER_CHUNK_SIZE];
    struct stat st;
    off_t remaining;

    // Checked the opened fd, so the checks apply to the file actually served.
    if (fstat(filefd, &st) == -1 || !servable(&st)) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    snprintf(header, sizeof(header), "RGT OK %lld ", (long long) st.st_size);
    if (write_all(connfd, header, strlen(header)) == -1)
        return PEER_FAILED;

    // Exactly st_size bytes are sent. If the file shrinks in the meantime, 
    // abort so the peer sees a premature close.
    remaining = st.st_size;
    while (remaining > 0) {
        size_t want = remaining < (off_t) sizeof(chunk) ? (size_t) remaining : sizeof(chunk);
        ssize_t n = read(filefd, chunk, want);
        if (n <= 0 || write_all(connfd, chunk, (size_t) n) == -1)
            return PEER_FAILED;
        remaining -= n;
    }

    if (write_all(connfd, "\n", 1) == -1)
        return PEER_FAILED;
    return PEER_SERVED;
}

static int send_file(int connfd, const char *filename) {
    int filefd = open(filename, O_RDONLY | O_NONBLOCK); // O_NONBLOCK: a FIFO swapped in under a valid name can't hang open()
    if (filefd == -1) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    int result = stream_file(connfd, filefd);
    close(filefd);
    return result;
}

static int handle_request(int connfd, AppState *state, char *request) {
    struct stat st;
    int result = read_request(connfd, request, PEER_REQUEST_MAX);

    if (result == -1)
        return PEER_DROPPED;

    // Must match "GET <valid filename>" exactly.
    if (result == 0 || strncmp(request, "GET ", 4) != 0 || !checkFilename(request + 4)) {
        send_status(connfd, "ERR");
        return PEER_MALFORMED;
    }

    // TODO: Shouldn't this check appear before the first one?
    if (!state->user.loggedIn) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    // Local check for the file.
    if (stat(request + 4, &st) == -1 || !servable(&st)) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    // Serve only files that we have published.
    if (where_peer(state->udp_fd, state->ds_addr, request + 4, state->user.UID,
                   NULL, 0, NULL) != WHERE_FOUND) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    return send_file(connfd, request + 4);
}

static const char *outcome_message(int outcome) {
    switch (outcome) {
        case PEER_SERVED:    return "File sent successfully";
        case PEER_DROPPED:   return "Connection closed before a full request";
        case PEER_MALFORMED: return "Malformed request";
        case PEER_REFUSED:   return "Request refused";
        default:             return "File transfer failed";
    }
}

void serve_peer(int connfd, AppState *state) {
    char request[PEER_REQUEST_MAX];
    struct timeval tv;
    int outcome;

    tv.tv_sec = PEER_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
        setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1)
        outcome = PEER_FAILED;
    else
        outcome = handle_request(connfd, state, request);

    close(connfd);

    if (outcome == PEER_SERVED || outcome == PEER_REFUSED || outcome == PEER_FAILED)
        printf("\nPeer request for %s: %s\n> ", request + 4, outcome_message(outcome));
    else
        printf("\nPeer request: %s\n> ", outcome_message(outcome));
    fflush(stdout);
}

int peer_setup(int port) {
    struct addrinfo hints, *res;
    char port_str[6];   // TODO: Port str constant!
    int fd, errcode;

    // Flag to set the option that prevents bind errors upon app restart.
    int opt = 1;

    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // Required to bind to all interfaces (requires NULL host below).

    errcode = getaddrinfo(NULL, port_str, &hints, &res);
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

    // Workaround for the TIME_WAIT state blocking the upcoming bind action after quick restarts.
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Error setting peer socket options");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        // EADDRINUSE: port already taken.
        // TODO: Any other error possible here?
        perror("Error binding peer socket");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }
    freeaddrinfo(res);

    if (listen(fd, PEER_BACKLOG) == -1) {
        perror("Error listening on peer socket");
        close(fd);
        return -1;
    }

    return fd;
}

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

    // 7-char prefix "RGT OK " + 1-8 digits (MAX_FILESIZE has 8) + a trailing whitespace char.
    int digits = (int) len - 8;
    if (strncmp(hdr, "RGT OK ", 7) != 0 || hdr[len - 1] != ' ' || digits < 1 || digits > 8) {
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
    char port_str[6];   // TODO: Port str constant!
    int fd, errcode;

    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;   // The address is already numeric: no DNS lookups.

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

    tv.tv_sec = PEER_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        perror("Error setting peer socket timeout");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    // TODO: Connect might also time out - same gap as in tcp_exchange!
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