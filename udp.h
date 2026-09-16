#ifndef UDP_H
#define UDP_H

#include <netdb.h>
#include <sys/types.h>

#define UDP_TIMEOUT 5

// int udp_setup(AppState *state, char *ds_ip, char *ds_port);
// ssize_t send_and_recv(AppState *state, char *request, char *reply_buffer);
ssize_t send_and_recv(int fd, struct addrinfo *res, char *request, char *reply_buffer, size_t buffer_len);

#endif