#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

#include "peer_server.h"
#include "commands.h"
#include "inputValidation.h"
#include "tcp.h"
#include "protocol.h"

#define PEER_SERVER_TIMEOUT 5  // Seconds to wait for a requesting peer's send/receive.
#define PEER_BACKLOG        16 // Peer connections that can wait to be accepted.

#define PEER_REQUEST_MAX    64 // Longest valid request is 29 bytes.

// TODO: Optimal size? TBC!
#define PEER_CHUNK_SIZE 4096   // Size of the chunks used to stream a file.
#define PEER_HEADER_MAX 32     // RGT OK <size> <data>

// Get request outcome
#define PEER_SERVED     0      // File sent successfully.
#define PEER_DROPPED    1      // Peer closed or timed out before sending a full request.
#define PEER_MALFORMED  2      // Syntax error (RGT ERR).
#define PEER_REFUSED    3      // Valid request, but unserved (RGT NOK).
#define PEER_FAILED     4      // Failure while replying (socket error, file changed...).


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
    return S_ISREG(st->st_mode) && checkFileSize(st->st_size);
}

// Sends "RGT OK <size> ", the file contents, and a trailing '\n'.
static int stream_file(int connfd, int filefd) {
    char header[PEER_HEADER_MAX];
    char chunk[PEER_CHUNK_SIZE];
    struct stat st;
    off_t remaining;

    // Check the opened file again (fstat on the fd, not stat on the path), 
    // to prevent cases where the path may now refer to a different file 
    // than the one checked before.
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
    int filefd = open(filename, O_RDONLY);
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

    tv.tv_sec = PEER_SERVER_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
        setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        fprintf(stderr, "\nError setting peer socket timeout: %s\n", strerror(errno));
        close(connfd);
        return;
    }

    outcome = handle_request(connfd, state, request);
    close(connfd);

    if (outcome == PEER_SERVED || outcome == PEER_REFUSED || outcome == PEER_FAILED)
        printf("\nPeer request for %s: %s\n", request + 4, outcome_message(outcome));
    else
        printf("\nPeer request: %s\n", outcome_message(outcome));
}

int peer_setup(int port) {
    struct addrinfo hints, *res;
    char port_str[PORT_STR_MAX_LEN + 1];
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