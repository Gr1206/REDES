#ifndef UDP_H
#define UDP_H

#include "app_state.h"

#define UDP_TIMEOUT 5

int udp_setup(AppState *state, const char *ds_ip, const char *ds_port);

int send_and_recv(AppState *state, char *request, char *reply_buffer);

#endif