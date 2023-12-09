#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <pthread.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "dht.h"
#include "file.h"
#include "local_file.h"
#include "network.h"

#define DOWNLOADER_POOL_SIZE 2 // split the download process across threads

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

typedef struct {
    pthread_mutex_t lock;   // struct lock
    int32_t running;        // flag for stopping downloader

    pthread_t tid[DOWNLOADER_POOL_SIZE];

    // dynamic buffer of download jobs
    uint32_t downloads_size;
    download_t *downloads;
} downloader_t;

// given a file, initializes a download object
int32_t download_init(download_t *download, file_t *file);

// effectively downloads a file
int32_t download_start(download_t *download);

// frees the memory allocated to a download
int32_t download_cleanup(download_t *download);

// print state of a download
void print_download(log_t log_type, download_t *download);

int32_t downloader_init(downloader_t *downloader);

void downloader_thread(downloader_t *downloader);

void downloader_cleanup(downloader_t *downloader);

// creates a new download and adds it to the downloaders job list
int32_t downloader_add(downloader_t *downloader, file_t *file);

// prints current state of downloads
void print_downloader(log_t log_type, downloader_t *downloader);

#endif