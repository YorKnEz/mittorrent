#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <pthread.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

#include "dht.h"
#include "download.h"
#include "download_list.h"
#include "file.h"
#include "local_file.h"
#include "network.h"


// TODO: move downlaoder to client

#define DOWNLOADER_POOL_SIZE 2 // split the download process across threads

typedef struct {
    pthread_mutex_t lock;   // struct lock
    int32_t running;        // flag for stopping downloader

    pthread_t tid[DOWNLOADER_POOL_SIZE];

    download_list_t downloads;
} downloader_t;

int32_t downloader_init(downloader_t *downloader);

void downloader_thread(downloader_t *downloader);

void downloader_cleanup(downloader_t *downloader);

// creates a new download and adds it to the downloaders job list
int32_t downloader_add(downloader_t *downloader, file_t *file);

// prints current state of downloads
void print_downloader(log_t log_type, downloader_t *downloader);

#endif