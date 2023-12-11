#include "downloader.h"

int32_t download_init(download_t *download, file_t *file) {
    pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&download->lock, &init, sizeof(pthread_mutex_t));
    
    download->state = IDLE;
    
    // try creating the file on disk
    char path[512];

    print(LOG, "where do you want to save the file?\n> ");
    
    memset(path, 0, 512);
    fgets(path, 511, stdin);
    path[strlen(path) - 1] = 0;

    int32_t fd;
    struct stat info;

    if (-1 == stat(path, &info)) {
        print(LOG, "error: invalid path\n");
        return -1;
    }

    if (!S_ISDIR(info.st_mode)) {
        print(LOG, "error: not a directory path\n");
        return -1;
    }

    strcat(path, "/");
    strcat(path, file->name);

    if (0 == access(path, F_OK)) {
        print(LOG, "error: file already exists\n");
        return -1;
    }

    if (-1 == (fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0666))) {
        print(LOG_ERROR, "[download_init] Error at open\n");
        return -1;
    }

    if (-1 == ftruncate(fd, file->size)) {
        print(LOG_ERROR, "[download_init] Error at ftruncate\n");
        remove(path);
        close(fd);
        return -1;
    }

    close(fd);

    // create local_file entry
    local_file_from_file(&download->local_file, file, path);

    download->blocks_size = download->local_file.size / FILE_BLOCK_SIZE + (download->local_file.size % FILE_BLOCK_SIZE > 0);
    download->blocks = (char*)malloc(download->blocks_size);
    memset(download->blocks, 0, download->blocks_size);

    // init peers buffer
    download->peers_size = 0;
    download->peers = NULL;

    node_t *p = file->peers;
    char *msg;
    uint32_t msg_size;

    while (p) {
        // node is probably dead
        if (-1 == node_req(&p->node, DOWNLOAD, &file->id, sizeof(key2_t), &msg, &msg_size)) {
            free(msg);
            p = p->next;
            continue;
        }

        if (msg == NULL) {
            // key was not found for some reason
            continue;
        }
        
        // node answered and sent us the blocks it has of this file
        download_peer_t peer;
        memcpy(&peer.peer, &p->node, sizeof(node_remote_t));
        peer.blocks = (char*)malloc(msg_size);
        memcpy(peer.blocks, msg, msg_size);

        // add peer to peers buffer
        download_peer_t *tmp = (download_peer_t*)malloc((1 + download->peers_size) * sizeof(download_peer_t));
        memcpy(tmp, download->peers, download->peers_size * sizeof(download_peer_t));
        memcpy(&tmp[download->peers_size], &peer, sizeof(download_peer_t));
        download->peers_size += 1;
        free(download->peers);
        download->peers = tmp;
        tmp = NULL;
        
        p = p->next;
    }
    
    return 0;
}

int32_t download_start(download_t *download) {
    // open the file
    int32_t fd;
    if (-1 == (fd = open(download->local_file.path, O_RDWR))) {
        print(LOG_ERROR, "[download_start] Error at open\n");
        return -1;
    }
    
    // download can be started at this point
    // TODO: improve block download algorithm
    // TODO: option to retry a download if a file is not fully downloaded
    // for now the algorithm takes each block in order and tries getting it from the peers that have it, if it fails, the file cannot be downloaded yet

    char *msg;
    uint32_t msg_size;

    block_t block;
    memcpy(&block.id, &download->local_file.id, sizeof(key2_t));
    int32_t blocks_received = 1;

    for (uint32_t i = 0; i < download->blocks_size; i++) {
        // stop the download if it's state has been changed from running
        pthread_mutex_lock(&download->lock);
        download_state_t local_state = download->state;
        pthread_mutex_unlock(&download->lock);

        if (local_state != RUNNING) {
            break;
        }
        
        // skip already owned blocks
        if (download->blocks[i] == 1) {
            continue;
        }

        block.index = i;    // index of the block

        // go through each peer and see if owns the block
        for (uint32_t j = 0; j < download->peers_size; j++) {
            // if it does, request it and try writing to file, if any error occurs, try next peer
            if (download->peers[j].blocks[i] == 1) {
                if (-1 == node_req(&download->peers[j].peer, BLOCK, &block, sizeof(block_t), &msg, &msg_size)) {
                    free(msg);
                    continue;
                }

                if (msg == NULL) {
                    print(LOG_ERROR, "[BLOCK] Block not found\n");
                    // no need to free
                    continue;
                }

                if (-1 == (lseek(fd, i * FILE_BLOCK_SIZE, SEEK_SET))) {
                    print(LOG_ERROR, "[BLOCK] Error at lseek\n");
                    continue;
                }

                if (-1 == write(fd, msg, msg_size)) {
                    print(LOG_ERROR, "[BLOCK] Error at write");
                    continue;
                }
                
                download->blocks[block.index] = 1;

                free(msg);
            }

            sleep(5); // wait 5 seconds between packets so i can test this shit
        }

        // if we didn't manage to get a certain block, it means download must be retried later, but still try to get the rest of the blocks
        if (download->blocks[i] == 0) {
            blocks_received = 0;
        }
    }

    pthread_mutex_lock(&download->lock);

    if (blocks_received == 1) {
        download->state = DONE;     // download is done, local_file can be transfered
    } else {
        download->state = PAUSED;   // download is not done, so pause it, giving the user the ability to retry at any point
    }

    pthread_mutex_unlock(&download->lock);
    
    close(fd);

    return 0;
}

int32_t download_cleanup(download_t *download) {
    for (uint32_t i = 0; i < download->peers_size; i++) {
        free(download->peers[i].blocks);
    }
    free(download->peers);
}

void print_download(log_t log_type, download_t *download) {
    print(log_type, "state: ");
    switch (download->state) {
    case    IDLE: { print(log_type, "IDLE   "); break; }
    case RUNNING: { print(log_type, "RUNNING"); break; }
    case  PAUSED: { print(log_type, "PAUSED "); break; }
    case    DONE: { print(log_type, "DONE   "); break; }
    }
    print(log_type, "\n");
    
    print_local_file(log_type, &download->local_file);

    // pretty print the block state
    print(log_type, "status: ");
    uint32_t state = 0;
    for (uint32_t i = 0; i < download->blocks_size; i++) {
        if (download->blocks[i] == 1) {
            state += 1;
        }
    }
    state = (state * 100) / download->blocks_size;
    print(log_type, "[");
    // map block state on 32 characters
    for (uint32_t i = 0; i < 32 * state / 100; i++) {
        print(log_type, "#");
    }
    for (uint32_t i = 32 * state / 100; i < 32; i++) {
        print(log_type, " ");
    }
    print(log_type, "] %d%%\n", state);
    
    print(log_type, "peers: %d\n", download->peers_size);
}

int32_t downloader_init(downloader_t *downloader) {
    downloader->downloads_size = 0;
    downloader->downloads = NULL;
    
    pthread_mutex_init(&downloader->lock, NULL);

    downloader->running = 1;

    for (uint32_t i = 0; i < DOWNLOADER_POOL_SIZE; i++) {
        pthread_create(&downloader->tid[i], NULL, (void *(*)(void*))&downloader_thread, downloader);
    }
}

void downloader_thread(downloader_t *downloader) {
    int32_t running = 1;

    while (running) {
        // check running state of downloader
        pthread_mutex_lock(&downloader->lock);

        int32_t download_index = -1;

        do {
            running = downloader->running;

            if (!running) {
                break;
            }

            for (uint32_t i = 0; i < downloader->downloads_size; i++) {
                pthread_mutex_lock(&downloader->downloads[i].lock);
                if (downloader->downloads[i].state == IDLE) {
                    download_index = i;
                }
                downloader->downloads[i].state = RUNNING; // declare the download as running so that other threads won't take it
                pthread_mutex_unlock(&downloader->downloads[i].lock);
            }
        } while (0);

        pthread_mutex_unlock(&downloader->lock);

        if (!running) {
            break;
        }

        // a download is found
        if (download_index != -1) {
            download_start(&downloader->downloads[download_index]);

            download_state_t local_state;

            pthread_mutex_lock(&downloader->downloads[download_index].lock);
            local_state = downloader->downloads[download_index].state;
            pthread_mutex_unlock(&downloader->downloads[download_index].lock);

            if (local_state == DONE) {

            } else if (local_state == PAUSED) {

            }
        }
    }
    
    pthread_exit(NULL);
}

void downloader_cleanup(downloader_t *downloader) {
    pthread_mutex_lock(&downloader->lock);
    downloader->running = 0;
    pthread_mutex_unlock(&downloader->lock);

    for (uint32_t i = 0; i < DOWNLOADER_POOL_SIZE; i++) {
        pthread_join(downloader->tid[i], NULL);
    }

    pthread_mutex_destroy(&downloader->lock);
}

int32_t downloader_add(downloader_t *downloader, file_t *file) {
    download_t download;

    if (-1 == download_init(&download, file)) {
        print(LOG_ERROR, "[downloader_add] Error at download_init\n");
        return -1;
    }

    // add download to buffer
    pthread_mutex_lock(&downloader->lock);

    download_t *tmp = (download_t*)malloc((1 + downloader->downloads_size) * sizeof(download_t));
    memcpy(tmp, downloader->downloads, downloader->downloads_size * sizeof(download_t));
    memcpy(&tmp[downloader->downloads_size], &download, sizeof(download_t));
    downloader->downloads_size += 1;
    free(downloader->downloads);
    downloader->downloads = tmp;
    tmp = NULL;
    
    pthread_mutex_unlock(&downloader->lock);

    return 0;
}

void print_downloader(log_t log_type, downloader_t *downloader) {
    pthread_mutex_lock(&downloader->lock);

    for (uint32_t i = 0; i < downloader->downloads_size; i++) {
        pthread_mutex_lock(&downloader->downloads[i].lock);
        print_download(log_type, &downloader->downloads[i]);
        pthread_mutex_unlock(&downloader->downloads[i].lock);
    }

    pthread_mutex_unlock(&downloader->lock);
}