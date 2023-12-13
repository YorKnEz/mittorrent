#ifndef CLIENT_H
#define CLIENT_H

#include <sys/stat.h>

#include "common.h"
#include "tracker.h"

typedef struct {
    // the connection to the bootstrap server
    int32_t bootstrap_fd;
    struct sockaddr_in bootstrap_addr;  // address of the server

    downloader_t downloader;            // downloader module
    tracker_t *tracker;                 // tracker module
} client_t;

void client_init(client_t *client);
void client_init_bootstrap_server_connection(client_t *client);
void client_cleanup(client_t *client);

int32_t client_start_tracker(client_t *client, const char *tracker_ip, const char *tracker_port);
int32_t client_stop_tracker(client_t *client);


// download the file given by id
int32_t client_download(client_t *client, key2_t *id);

#endif
