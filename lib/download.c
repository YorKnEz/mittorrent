#include "download.h"

int32_t download_init(download_t *download, file_t *file) {
    pthread_mutex_init(&download->lock, NULL);
    
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

    download_init_peers(download, file);

    return 0;
}

void download_init_peers(download_t *download, file_t *file) {
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

    // no lock needed here as the id is never modified
    block_t block;
    memcpy(&block.id, &download->local_file.id, sizeof(key2_t));
    int32_t blocks_received = 1;
    int32_t running = 1;

    for (uint32_t i = 0; running && i < download->blocks_size; i++) {
        pthread_mutex_lock(&download->lock);
        
        do {
            // stop the download if it's state has been changed from running
            if (download->state != RUNNING) {
                running = 0;
                break;
            }
            
            // skip already owned blocks
            if (download->blocks[i] == 1) {
                break;
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

                    break; // block has been received successfully, no need to continue
                }

            }
        } while (0);

        pthread_mutex_unlock(&download->lock);

        // if we didn't manage to get a certain block, it means download must be retried later, but still try to get the rest of the blocks
        // no lock needed here, as blocks array can only be modified by the above for
        if (download->blocks[i] == 0) {
            blocks_received = 0;
        }

        sleep(10); // wait seconds between packets so i can test this shit
    }

    pthread_mutex_lock(&download->lock);

    // if state is running, it means no external interrupts happened
    // only case when this is not true is if a user pauses a download, in which case the state will be PAUSED, which is correct
    if (download->state == RUNNING) {
        if (blocks_received == 1) {
            download->state = DONE;     // download is done, local_file can be transfered
        } else {
            download->state = PAUSED;   // download is not done, so pause it, giving the user the ability to retry at any point
        }
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
    pthread_mutex_destroy(&download->lock);
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

void print_download_short(log_t log_type, download_t *download) {
    // state column - 7 chars
    switch (download->state) {
    case    IDLE: { print(log_type, "IDLE   "); break; }
    case RUNNING: { print(log_type, "RUNNING"); break; }
    case  PAUSED: { print(log_type, "PAUSED "); break; }
    case    DONE: { print(log_type, "DONE   "); break; }
    }
    print(log_type, " | ");
    
    // name column - 16 chars
    char *name = strrchr(download->local_file.path, '/') + 1;
    uint32_t len = strlen(name);
    uint32_t max_len = 32;
    if (len > max_len) {
        char old = name[max_len - 3];
        name[max_len - 3] = 0;
        print(log_type, "%s...", name);
        name[max_len - 3] = old;
    } else {
        print(log_type, "%s", name);
        for (uint32_t i = 0; i < 32 - len; i++) {
            print(log_type, " ");
        }
    }
    print(log_type, " | ");

    // status column - 39 chars
    uint32_t state = 0;
    for (uint32_t i = 0; i < download->blocks_size; i++) {
        if (download->blocks[i] == 1) {
            state += 1;
        }
    }
    state = (state * 100) / download->blocks_size;
    print(log_type, "[");
    uint32_t state_len = 32;
    // map block state on state_len characters
    for (uint32_t i = 0; i < state_len * state / 100; i++) {
        print(log_type, "#");
    }
    for (uint32_t i = state_len * state / 100; i < state_len; i++) {
        print(log_type, " ");
    }
    print(log_type, "] %3d%%", state);
    print(log_type, " | ");
    
    // peers column - 5 chars
    print(log_type, "%5d\n", download->peers_size);
}
