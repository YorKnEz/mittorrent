#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <pthread.h>
#include <sys/stat.h>

#include "dht.h"
#include "local_file.h"
#include "error.h"

typedef struct {
    node_remote_t peer;
    char *blocks;
} download_peer_t;

typedef enum {
    IDLE,       // file is not being downloaded yet
    RUNNING,    // file is being downloaded at this moment
    PAUSED,     // download has been stopped
    DONE,       // download is done (all blocks were received)
} download_state_t;

// download job info
typedef struct {
    pthread_mutex_t lock;       // struct lock
    download_state_t state;
    local_file_t local_file;
    uint32_t blocks_size;       // size of the array, ceil of size / FILE_BLOCK_SIZE
    char *blocks;               // array of true/false values that shows which blocks are downloaded
    uint32_t peers_size;
    download_peer_t *peers;
} download_t;

// given a file, initializes a download object
int32_t download_init(download_t *download, file_t *file);

// init peers buffer
void download_init_peers(download_t *download, file_t *file);

// effectively downloads a file
int32_t download_start(download_t *download);

// frees the memory allocated to a download
int32_t download_cleanup(download_t *download);

// print state of a download
void print_download(log_t log_type, download_t *download);

// print state of a download for print_downloader
void print_download_short(log_t log_type, download_t *download);

#endif
