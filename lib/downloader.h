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

/*
The download protocol shall be described like this:
A user wants to download a file, it notifies the downloader module, which creates a new socket which it binds to some port, it sends this information to the peer to download the file from. The peer will connect to this server and start sending blocks of a file to the downloader
The downloader's job is to collect these blocks and recompose the file

Each peer holds a list of blocks that it owns as peers might participate in file transfer even if they haven't got the full file yet
*/

#define DOWNLOADER_POOL_SIZE 2 // split the download process across threads

typedef struct {
    node_remote_t peer;
    char *blocks;
} download_peer_t;

// download job info
typedef struct {
    local_file_t local_file;
    uint32_t peers_size;
    download_peer_t *peers;
} download_t;

typedef struct {
    pthread_mutex_t lock;   // struct lock

    int32_t fd;
    struct sockaddr_in addr;

    pthread_mutex_t mlock;
    pthread_t tid[DOWNLOADER_POOL_SIZE];

    // dynamic buffer of download jobs
    uint32_t downloads_size;
    download_t *downloads;
} downloader_t;

int32_t download_init(downloader_t *downloader, file_t *file);

int32_t download_cleanup(downloader_t *downloader, download_t *download);

int32_t downloader_init(downloader_t *downloader, const char *ip, const char *port);

void downloader_thread(downloader_t *downloader);

void downloader_cleanup(downloader_t *downloader);

#endif