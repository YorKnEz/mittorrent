#ifndef CLIENT_H
#define CLIENT_H

#include <sys/stat.h>

#include "common.h"
#include "tracker.h"
#include "cmd_parser.h"

typedef struct {
    // the connection to the bootstrap server
    int32_t bootstrap_fd;
    struct sockaddr_in bootstrap_addr;  // address of the server

    tracker_t *tracker;                 // the tracker component
} client_t;

void client_init(client_t *client);
void client_init_bootstrap_server_connection(client_t *client);
void client_cleanup(client_t *client);

int32_t client_start_tracker(client_t *client, const char *tracker_ip, const char *tracker_port);
int32_t client_stop_tracker(client_t *client);

#endif