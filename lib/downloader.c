#include "downloader.h"

int32_t download_init(downloader_t *downloader, file_t *file) {
    download_t download;
    // init local_file
    char path[512];

    print(LOG, "where do you want to save the file?\n> ");
    
    memset(path, 0, 512);
    fgets(path, 511, stdin);
    path[strlen(path) - 1] = 0;

    struct stat info;

    if (-1 == stat(path, &info)) {
        print(LOG, "error: invalid path\n");
        return -1;
    }

    if (!S_iSDIR(path)) {
        print(LOG, "error: not a directory path\n");
        return -1;
    }

    strcat(path, "/");
    strcat(path, file->name);

    if (0 == access(path, F_OK)) {
        print(LOG, "error: file already exists\n");
        return -1;
    }

    // create the file
    int32_t fd;
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
    
    local_file_from_file(&download.local_file, file, path);

    // init peers buffer
    download.peers_size = 0;
    download.peers = NULL;

    node_t *p = file->peers;
    char *msg;
    uint32_t msg_size;

    struct {
        key2_t file_id;
        struct sockaddr_in dwldr_addr;
    } data;

    memcpy(&data.file_id, &file->id, sizeof(key2_t));
    memcpy(&data.dwldr_addr, &downloader->addr, sizeof(struct sockaddr_in));

    while (p) {
        // node is probably dead
        if (-1 == node_req(&p->node, DOWNLOAD, &data, sizeof(data), &msg, &msg_size)) {
            free(msg);
            p = p->next;
            continue;
        }
        
        // node answered and sent us the blocks it has of this file
        download_peer_t peer;
        memcpy(&peer, &p->node, sizeof(node_remote_t));
        memcpy(peer.blocks, msg, download.local_file.blocks_size);

        free(msg);

        // add peer to peers buffer
        download_peer_t *tmp = (download_peer_t*)malloc((1 + download.peers_size) * sizeof(download_peer_t));
        memcpy(tmp, download.peers, download.peers_size * sizeof(download_peer_t));
        memcpy(&tmp[download.peers_size], &peer, sizeof(download_peer_t));
        download.peers_size += 1;
        free(download.peers);
        download.peers = tmp;
        tmp = NULL;
        
        p = p->next;
    }
    
    // download can be started so add the created download to the downloads buffer
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

int32_t download_cleanup(downloader_t *downloader, download_t *download) {
}

int32_t downloader_init(downloader_t *downloader, const char *ip, const char *port) {
    downloader->addr.sin_family = AF_INET;
    downloader->addr.sin_addr.s_addr = htonl(inet_addr(ip));
    downloader->addr.sin_port = htons(atoi(port));
    
    if (-1 == (downloader->fd = get_server_socket(&downloader->addr))) {
        print(LOG_ERROR, "[download_init] Error at get_server_socket");
        return -1;
    }
    
    downloader->downloads_size = 0;
    downloader->downloads = NULL;
    
    pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&downloader->lock, &init, sizeof(pthread_mutex_t));
    memcpy(&downloader->mlock, &init, sizeof(pthread_mutex_t));

    for (uint32_t i = 0; i < DOWNLOADER_POOL_SIZE; i++) {
        pthread_create(&downloader->tid[i], NULL, (void *(*)(void*))&downloader_thread, downloader);
    }
}

void downloader_thread(downloader_t *downloader) {
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    // fd set of clients
    fd_set allfd;
    FD_ZERO(&allfd);
    FD_SET(downloader->fd, &allfd);
    int32_t max_fd = downloader->fd;
    // int32_t max_fd = 0;

    // fd set of listener socket
    fd_set server_fd;
    FD_ZERO(&server_fd);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (1) {
        fd_set rdfd;
        memcpy(&rdfd, &allfd, sizeof(fd_set));

        if (-1 == select(max_fd + 1, &rdfd, NULL, NULL, NULL)) {
            print(LOG_ERROR, "[downloader_thread] Error at select\n");
            break;
        }

        // workaround for new connections
        // if we rely on the first select for handling new connections, the threads
        // have no way of knowing if another thread already handled the connection
        // so deadlock will occur
        // thus, we make a separate set only for the server->fd and select on it
        // with a timeout, so that way each thread can check if the server->fd
        // connection has been handled
        pthread_mutex_lock(&downloader->mlock);

        FD_SET(downloader->fd, &server_fd); // re-add server->fd to set

        if (-1 == select(downloader->fd + 1, &server_fd, NULL, NULL, &tv)) {
            print(LOG_ERROR, "[downloader_thread] Error at select2\n");
            break;
        }

        if (FD_ISSET(downloader->fd, &server_fd)) {
            int32_t fd = accept(downloader->fd, (struct sockaddr*)&client_addr, &client_size);

            if (-1 != fd) {
                FD_SET(fd, &allfd);

                if (fd > max_fd) max_fd = fd;
            } else {
                print(LOG_ERROR, "[downloader_thread] Error at accept\n");
            }
        }
        
        pthread_mutex_unlock(&downloader->mlock);

        for (int32_t client_fd = 0; client_fd <= max_fd; client_fd++) {
            if (client_fd != downloader->fd && FD_ISSET(client_fd, &rdfd)) {
                // handle request
                print(LOG_DEBUG, "[downloader_thread] Received new request\n");

                req_header_t reqh;
                char *msg;
                uint32_t msg_size;

                if (-1 == recv_req(client_fd, &reqh, &msg, &msg_size)) {
                    print(LOG_ERROR, "[downloader_thread] Error at recv_req\n");
                    shutdown(client_fd, SHUT_RDWR);
                    close(client_fd);
                    continue;
                }

                switch (reqh.type) {
                case SHUTDOWN: {
                    // TODO: shutdown downloader thread
                    break;
                }
                case BLOCK: {
                    block_t block;
                    memcpy(&block, msg, sizeof(block_t));

                    pthread_mutex_lock(&downloader->lock);

                    uint32_t i;
                    // find the file to which the block belongs
                    for (i = 0; i < downloader->downloads_size; i++) {
                        if (key_cmp(&downloader->downloads[i].local_file.id, &block.id) == 0) {
                            break;
                        }
                    }

                    // runs only once
                    int32_t status = 0;
                    do {
                        int32_t fd;

                        if (-1 == (fd = open(&downloader->downloads[i].local_file.path, O_RDWR))) {
                            print(LOG_ERROR, "[BLOCK] Cannot open file\n");
                            status = -1;
                            break;
                        }

                        if (-1 == (lseek(fd, block.index * FILE_BLOCK_SIZE, SEEK_SET))) {
                            print(LOG_ERROR, "[BLOCK] Error at lseek\n");
                            status = -1;
                            break;
                        }

                        if (-1 == write(fd, msg + sizeof(block_t), block.size)) {
                            print(LOG_ERROR, "[BLOCK] Error at write");
                            status = -1;
                            break;
                        }
                        
                        downloader->downloads[i].local_file.blocks[block.index] = 1;

                        close(fd);
                    } while (0);
                    
                    pthread_mutex_unlock(&downloader->lock);

                    if (-1 == status) {
                        
                    } else {

                    }

                    break;
                }
                }
            }
        }
    }
    
    pthread_exit(NULL);
}

void downloader_cleanup(downloader_t *downloader) {

}