#ifndef TRACKER_H
#define TRACKER_H

#include <pthread.h>
#include <sys/select.h>

#include "dht.h"
#include "file.h"
#include "local_file.h"
#include "logger.h"
#include "network.h"
#include "query.h"
#include "file_list.h"
#include "local_file_list.h"
#include "downloader.h"
#include "error.h"

#define THREAD_POOL_SIZE 2

#define INVALID_COMMAND "Invalid command"
#define INTERNAL_ERROR "Internal error"

// tracker interface
typedef struct {
    // used by threads to know when to stop
    int32_t running;
    pthread_mutex_t running_lock;

    // used by threads to notify client that tracker should self-stop
    int32_t server_running;
    pthread_mutex_t server_running_lock;
    
    pthread_mutex_t lock;               // tracker structure lock

    // local server data
    int32_t fd;
    struct sockaddr_in addr;
    pthread_mutex_t mlock;              // accept lock
    pthread_t tid[THREAD_POOL_SIZE];

    node_local_t node;                  // the dht network local data

    file_list_t files;                  // list of torrent files stored by the tracker
    local_file_list_t local_files;      // list of files that the tracker can transfer upon a download request

    downloader_t *downloader;           // a pointer to the downloader module of the client
} tracker_t;

void tracker_local_server_thread(tracker_t *tracker);

int32_t tracker_init(tracker_t *tracker, const char *tracker_ip, const char *tracker_port, struct sockaddr_in *server_addr);
int32_t tracker_init_local_server(tracker_t *tracker, const char *tracker_ip, const char *tracker_port);
int32_t tracker_init_dht_connection(tracker_t *tracker, struct sockaddr_in *server_addr);
int32_t tracker_cleanup(tracker_t *tracker);

// tracker commands

// print tracker state
void tracker_state(tracker_t *tracker);

// stabilize tracker using calls to node_stabilize and node_fix_fingers
int32_t tracker_stabilize(tracker_t *tracker);

// download the specified torrent file
int32_t tracker_download(tracker_t *tracker, key2_t *id);

// upload the regular file at path to the network
int32_t tracker_upload(tracker_t *tracker, const char *path, struct sockaddr_in *server_addr);

#endif
