#ifndef CLIENT_H
#define CLIENT_H

#include <sys/stat.h>

#include "common.h"
#include "tracker.h"

typedef struct {
    struct sockaddr_in bootstrap_addr;  // address of the server

    cmds_t cmds;                        // command parser module
    downloader_t downloader;            // downloader module
    tracker_t *tracker;                 // tracker module
} client_t;

int32_t client_init(client_t *client);
int32_t client_init_bootstrap_server_connection(client_t *client);
int32_t client_cleanup(client_t *client);

int32_t client_start_tracker(client_t *client, const char *tracker_ip, const char *tracker_port);
int32_t client_stop_tracker(client_t *client);

// search a file on the network based on a query
int32_t client_search(client_t *client, query_t *query, query_result_t** results, uint32_t *results_size);

// download the file given by id
int32_t client_download(client_t *client, key2_t *id);

#endif
