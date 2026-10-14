#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "peer.h"
#include "commands.h"
#include "inputValidation.h"

#define PEER_REQUEST_MAX 64    // Longest valid request is "GET " + 24 chars + '\n' = 29 bytes.
#define PEER_CHUNK_SIZE 4096   // Size of the chunks used to stream a file.
#define PEER_HEADER_MAX 32     // Room for "RGT OK <size> ".

// Outcome of serving one request (reported to the user).
#define PEER_SERVED     0      // File sent successfully.
#define PEER_DROPPED    1      // Peer closed or timed out before sending a full request.
#define PEER_MALFORMED  2      // Syntax error (answered with RGT ERR).
#define PEER_REFUSED    3      // Valid request, not served (answered with RGT NOK).
#define PEER_FAILED     4      // Failure while replying (socket error, file changed...).

// write() may send fewer bytes than requested, so a loop is required.
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
    write_all(fd, reply, strlen(reply));   // Best effort: nothing else to do on failure.
}

// Reads until the first '\n' and replaces it with '\0'.
// Returns 1 for a complete request, 0 for a malformed one (too long, embedded
// '\0', or bytes after the '\n'), -1 if the peer closed, timed out or failed.
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
    return 0;   // Buffer full without a '\n'.
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

    // Checked on the opened fd, so the checks apply to the file actually served.
    if (fstat(filefd, &st) == -1 || !servable(&st)) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    snprintf(header, sizeof(header), "RGT OK %lld ", (long long) st.st_size);
    if (write_all(connfd, header, strlen(header)) == -1)
        return PEER_FAILED;

    // Exactly st_size bytes are sent. If the file shrinks meanwhile, abort
    // instead of padding, so the receiver sees a premature close.
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
    // O_NONBLOCK: a FIFO swapped in under a valid name can't hang open().
    int filefd = open(filename, O_RDONLY | O_NONBLOCK);
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

    // Strict parse: exactly "GET <valid filename>". checkFilename also rejects
    // '/' and ".", so nothing unvalidated ever reaches the filesystem.
    if (result == 0 || strncmp(request, "GET ", 4) != 0 || !checkFilename(request + 4)) {
        send_status(connfd, "ERR");
        return PEER_MALFORMED;
    }

    // TODO: Shouldn't this check appear before the first one?
    // Every refusal below is the same NOK, so peers can't probe why.
    if (!state->user.loggedIn) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    // Cheap local check first, so requests for missing files don't cost a DS round trip.
    if (stat(request + 4, &st) == -1 || !servable(&st)) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    // We only serve files that we publish ourselves, and the DS is the source of truth.
    // Fails closed: NOK or error from the DS both end up as a refusal.
    if (where_peer(state->udp_fd, state->ds_addr, request + 4, state->user.UID,
                   NULL, 0, NULL) != WHERE_FOUND) {
        send_status(connfd, "NOK");
        return PEER_REFUSED;
    }

    return send_file(connfd, request + 4);
}

static const char *outcome_message(int outcome) {
    switch (outcome) {
        case PEER_SERVED:    return "File sent";
        case PEER_DROPPED:   return "Connection closed before a full request";
        case PEER_MALFORMED: return "Malformed request";
        case PEER_REFUSED:   return "Request refused";
        default:             return "Transfer failed";
    }
}

void serve_peer(int connfd, AppState *state) {
    char request[PEER_REQUEST_MAX];
    struct timeval tv;
    int outcome;

    // Accepted sockets don't reliably inherit the listening socket's options.
    tv.tv_sec = PEER_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
        setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1)
        outcome = PEER_FAILED;
    else
        outcome = handle_request(connfd, state, request);

    close(connfd);

    // The request only holds a validated filename once parsing has passed, so
    // raw peer bytes are never echoed to the terminal. The leading '\n' and
    // trailing "> " are because this lands in the middle of the prompt line.
    if (outcome == PEER_SERVED || outcome == PEER_REFUSED || outcome == PEER_FAILED)
        printf("\nPeer request for %s: %s\n> ", request + 4, outcome_message(outcome));
    else
        printf("\nPeer request: %s\n> ", outcome_message(outcome));
    fflush(stdout);
}