#include "tracker.h"

int32_t tracker_init(tracker_t *tracker, const char *tracker_ip, const char *tracker_port, int32_t boostrap_server_fd) {
    tracker->files = NULL;
    tracker->local_files = NULL;
    
    if (-1 == tracker_init_local_server(tracker, tracker_ip, tracker_port)) {
        print(LOG_ERROR, "[tracker_init] Error at tracker_init_local_server\n");
        return -1;
    }

    if (-1 == tracker_init_dht_connection(tracker, boostrap_server_fd)) {
        print(LOG_ERROR, "[tracker_init] Error at tracker_init_dht_connection\n");
        return -1;
    }

    // init downlaoder module
    if (-1 == downloader_init(&tracker->downloader)) {
        print(LOG_ERROR, "[tracker_init] Error at downloader_init\n");
        return -1;
    }

    return 0;
}

int32_t tracker_init_local_server(tracker_t *tracker, const char *tracker_ip, const char *tracker_port) {
    // bind to some address
    tracker->addr.sin_family = AF_INET;
    tracker->addr.sin_addr.s_addr = inet_addr(tracker_ip);
    tracker->addr.sin_port = htons(atoi(tracker_port));

    if (-1 == (tracker->fd = get_server_socket(&tracker->addr))) {
        print(LOG_ERROR, "[tracker_init_local_server] Error at get_server_socket\n");
        return -1;
    }

    pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&tracker->mlock, &init, sizeof(pthread_mutex_t));
    memcpy(&tracker->lock, &init, sizeof(pthread_mutex_t));

    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&tracker->tid[i], NULL, (void *(*)(void*))tracker_local_server_thread, tracker);
    }

    return 0;
}

void tracker_local_server_thread(tracker_t *tracker) {
    int32_t tracker_fd;
    struct sockaddr_in tracker_addr;
    socklen_t tracker_size = sizeof(tracker_addr);

    int32_t running = 1;

    while (running) {
        pthread_mutex_lock(&tracker->mlock);

        tracker_fd = accept(tracker->fd, (struct sockaddr*)&tracker_addr, &tracker_size);

        pthread_mutex_unlock(&tracker->mlock);

        if (-1 == tracker_fd) {
            print(LOG_ERROR, "[tracker_local_server_thread] Error at accept\n");
            continue;
        }

        // handle connection
        print(LOG_DEBUG, "[tracker_local_server_thread] Received new connection\n");

        req_header_t reqh;
        char *msg;
        uint32_t msg_size;

        if (-1 == recv_req(tracker_fd, &reqh, &msg, &msg_size)) {
            print(LOG_ERROR, "[tracker_local_server_thread] Error at recv_req\n");
            shutdown(tracker_fd, SHUT_RDWR);
            close(tracker_fd);
            continue;
        }

        switch (reqh.type) {
        case PING: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received PING\n");

            if (-1 == send_res(tracker_fd, SUCCESS, NULL, 0)) {
                print(LOG_ERROR, "[PING] Error at send_res\n");
                break;
            }

            break;
        }
        case FIND_NEXT: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received FIND_NEXT\n");

            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));

            node_remote_t res;

            pthread_mutex_lock(&tracker->lock);

            int32_t status = node_find_next(&tracker->node, &id, &res);

            pthread_mutex_unlock(&tracker->lock);

            if (-1 == status) {
                print(LOG_ERROR, "[FIND_NEXT] Error at node_find_next\n");

                if (-1 == send_res(tracker_fd, ERROR, INTERNAL_ERROR, strlen(INTERNAL_ERROR))) {
                    print(LOG_ERROR, "[FIND_NEXT] Error at send_res\n");
                    break;
                }

                break;
            }

            if (-1 == send_res(tracker_fd, SUCCESS, &res, sizeof(node_remote_t))) {
                print(LOG_ERROR, "[FIND_NEXT] Error at send_res\n");
                break;
            }

            break;
        }
        case FIND_PREV: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received FIND_PREV\n");

            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));

            node_remote_t res;

            pthread_mutex_lock(&tracker->lock);

            int32_t status = node_find_prev(&tracker->node, &id, &res);

            pthread_mutex_unlock(&tracker->lock);

            if (-1 == status) {
                print(LOG_ERROR, "[FIND_PREV] Error at node_find_prev\n");

                if (-1 == send_res(tracker_fd, ERROR, INTERNAL_ERROR, strlen(INTERNAL_ERROR))) {
                    print(LOG_ERROR, "[FIND_PREV] Error at send_res\n");
                    break;
                }

                break;
            }
            
            if (-1 == send_res(tracker_fd, SUCCESS, &res, sizeof(node_remote_t))) {
                print(LOG_ERROR, "[FIND_PREV] Error at send_res\n");
                break;
            }

            break;
        }
        case SET_PREV: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received SET_PREV\n");

            node_remote_t new_prev;
            memcpy(&new_prev, msg, sizeof(node_remote_t));

            pthread_mutex_lock(&tracker->lock);

            int32_t status = node_set_prev(&tracker->node, &new_prev);

            pthread_mutex_unlock(&tracker->lock);

            if (-1 == status) {
                print(LOG_ERROR, "[SET_PREV] Error at node_set_prev\n");

                if (-1 == send_res(tracker_fd, ERROR, INTERNAL_ERROR, strlen(INTERNAL_ERROR))) {
                    print(LOG_ERROR, "[SET_PREV] Error at send_res\n");
                    break;
                }

                break;
            }
            
            if (-1 == send_res(tracker_fd, SUCCESS, NULL, 0)) {
                print(LOG_ERROR, "[SET_PREV] Error at send_res\n");
                break;
            }

            break;
        }
        case FIND_CLOSEST_PRECEDING_FINGER: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received FIND_CLOSEST_PRECEDING_FINGER\n");

            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));

            node_remote_t res;

            pthread_mutex_lock(&tracker->lock);

            int32_t status = node_find_closest_preceding(&tracker->node, &id, &res);

            pthread_mutex_unlock(&tracker->lock);

            if (-1 == status) {
                print(LOG_ERROR, "[FIND_CLOSEST_PRECEDING_FINGER] Error at node_find_closest_preceding\n");

                if (-1 == send_res(tracker_fd, ERROR, INTERNAL_ERROR, strlen(INTERNAL_ERROR))) {
                    print(LOG_ERROR, "[FIND_CLOSEST_PRECEDING_FINGER] Error at send_res\n");
                    break;
                }

                break;
            }
            
            if (-1 == send_res(tracker_fd, SUCCESS, &res, sizeof(node_remote_t))) {
                print(LOG_ERROR, "[FIND_CLOSEST_PRECEDING_FINGER] Error at send_res\n");
                break;
            }

            break;
        }
        case NOTIFY: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received NOTIFY\n");

            node_remote_t peer;
            memcpy(&peer, msg, sizeof(node_remote_t));

            pthread_mutex_lock(&tracker->lock);

            int32_t status = node_notify(&tracker->node, &peer);

            if (-1 == status) {
                print(LOG_ERROR, "[NOTIFY] Error at node_notify\n");

                if (-1 == send_res(tracker_fd, ERROR, INTERNAL_ERROR, strlen(INTERNAL_ERROR))) {
                    print(LOG_ERROR, "[NOTIFY] Error at send_res\n");
                    pthread_mutex_unlock(&tracker->lock);
                    break;
                }

                pthread_mutex_unlock(&tracker->lock);
                break;
            }

            if (-1 == send_res(tracker_fd, SUCCESS, NULL, 0)) {
                print(LOG_ERROR, "[NOTIFY] Error at send_res\n");
                pthread_mutex_unlock(&tracker->lock);
                break;
            }

            pthread_mutex_unlock(&tracker->lock);

            break;
        }
        case SEARCH: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received SEARCH\n");

            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));

            char *res = NULL;
            uint32_t res_size = 0;

            pthread_mutex_lock(&tracker->lock);

            // look for the specified id in the torrent files list
            file_node_t *p = tracker->files;

            while (!res && p) {
                if (key_cmp(&id, &p->file.id) == 0) {
                    serialize_file(&p->file, &res, &res_size);

                    break;
                }

                p = p->next;
            }

            pthread_mutex_unlock(&tracker->lock);

            if (-1 == send_res(tracker_fd, SUCCESS, res, res_size)) {
                print(LOG_ERROR, "[SEARCH] Error at send_res\n");
                shutdown(tracker_fd, SHUT_RDWR);
                close(tracker_fd);
                continue;
            }
            
            free(res);
            
            break;
        }
        case DOWNLOAD: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received DOWNLOAD\n");

            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));

            char *res = NULL;
            uint32_t res_size = 0;

            pthread_mutex_lock(&tracker->lock);

            // look for the specified id in the local files list and the downloads list
            do {
                local_file_node_t *p = tracker->local_files;

                while (!res && p) {
                    if (key_cmp(&id, &p->file.id) == 0) {
                        res_size = p->file.size / FILE_BLOCK_SIZE + (p->file.size % FILE_BLOCK_SIZE > 0);
                        res = (char*)malloc(res_size);
                        memset(res, 1, res_size);
                        break;
                    }
                    
                    p = p->next;
                }

                if (res) {
                    break;
                }
                
                pthread_mutex_lock(&tracker->downloader.lock);

                for (uint32_t i = 0; i < tracker->downloader.downloads_size; i++) {
                    if (key_cmp(&id, &tracker->downloader.downloads[i].local_file.id) == 0) {
                        res_size = tracker->downloader.downloads[i].local_file.size / FILE_BLOCK_SIZE + (tracker->downloader.downloads[i].local_file.size % FILE_BLOCK_SIZE > 0);
                        res = (char*)malloc(res_size);
                        memset(res, 1, res_size);
                        break;
                    }
                }
                
                pthread_mutex_unlock(&tracker->downloader.lock);
            } while (0);
            
            pthread_mutex_unlock(&tracker->lock);

            if (-1 == send_res(tracker_fd, SUCCESS, res, res_size)) {
                print(LOG_ERROR, "[DOWNLOAD] Error at send_res\n");
                free(res);
                break;
            }

            free(res);
            
            break;
        }
        case BLOCK: {
            // TODO: heavily inefficient
            
            print(LOG_DEBUG, "[tracker_local_server_thread] Received BLOCK\n");

            block_t block;
            memcpy(&block, msg, sizeof(block_t));

            local_file_t *res = NULL;

            pthread_mutex_lock(&tracker->lock);

            // look for the specified id in the local files list and the downloads list
            do {
                local_file_node_t *p = tracker->local_files;

                while (!res && p) {
                    if (key_cmp(&block.id, &p->file.id) == 0) {
                        res = &p->file;
                        break;
                    }
                    
                    p = p->next;
                }

                if (res) {
                    break;
                }
                
                pthread_mutex_lock(&tracker->downloader.lock);

                for (uint32_t i = 0; i < tracker->downloader.downloads_size; i++) {
                    if (key_cmp(&block.id, &tracker->downloader.downloads[i].local_file.id) == 0) {
                        res = &tracker->downloader.downloads[i].local_file;
                        break;
                    }
                }
                
                pthread_mutex_unlock(&tracker->downloader.lock);
            } while (0);
            
            pthread_mutex_unlock(&tracker->lock);

            if (res) {
                int32_t fd;
                char *buf[FILE_BLOCK_SIZE];
                uint32_t buf_size = 0;

                if (-1 == (fd = open(res->path, O_RDONLY))) {
                    print(LOG_ERROR, "[BLOCK] Error at open\n");
                    break;
                }

                if (-1 == lseek(fd, block.index * FILE_BLOCK_SIZE, SEEK_SET)) {
                    print(LOG_ERROR, "[BLOCK] Error at lseek");
                    close(fd);
                    break;
                }

                if (-1 == (buf_size = read(fd, buf, FILE_BLOCK_SIZE))) {
                    print(LOG_ERROR, "[BLOCK] Error at read\n");
                    close(fd);
                    break;
                }
                
                close(fd);
                
                if (-1 == send_res(tracker_fd, SUCCESS, buf, buf_size)) {
                    print(LOG_ERROR, "[BLOCK] Error at send_res\n");
                    break;
                }
            } else {
                if (-1 == send_res(tracker_fd, SUCCESS, NULL, 0)) {
                    print(LOG_ERROR, "[BLOCK] Error at send_res\n");
                    break;
                }
            }
            
            break;
        }
        case ADD_PEER: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received ADD_PEER\n");

            node_remote_t peer;
            key2_t id;
            int32_t found = 0;

            memcpy(&peer, msg, sizeof(node_remote_t));
            memcpy(&id, msg + sizeof(node_remote_t), sizeof(key2_t));

            pthread_mutex_lock(&tracker->lock);

            // look for the specified id in the torrent files list
            file_node_t *p = tracker->files;

            while (p) {
                if (key_cmp(&id, &p->file.id) == 0) {
                    add_peer(&p->file, &peer);
                    found = 1;
                    break;
                }

                p = p->next;
            }
            
            pthread_mutex_unlock(&tracker->lock);

            if (found == 1) {
                if (-1 == send_res(tracker_fd, SUCCESS, NULL, 0)) {
                    print(LOG_ERROR, "[ADD_PEER] Error at send_res\n");
                    break;
                }
            } else {
                if (-1 == send_res(tracker_fd, ERROR, INTERNAL_ERROR, strlen(INTERNAL_ERROR))) {
                    print(LOG_ERROR, "[ADD_PEER] Error at send_res\n");
                    break;
                }
            }

            break;
        }
        case MOVE_DATA: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received MOVE_DATA\n");

            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));

            file_t zero;
            memset(&zero, 0, sizeof(file_t));

            pthread_mutex_lock(&tracker->lock);

            // transfer all files in the linked list until encountering id
            file_node_t *p = tracker->files;

            while (p) {
                if (key_in(&p->file.id, &tracker->node.id, 0, &id, 1)) {
                    char *file;
                    uint32_t file_size;

                    serialize_file(&p->file, &file, &file_size);
                    
                    if (-1 == send_res(tracker_fd, SUCCESS, file, file_size)) {
                        print(LOG_ERROR, "[MOVE_DATA] Error at send_res\n");
                        free(file);
                        break;
                    }

                    free(file);

                    key2_t tmp_id;
                    memcpy(&tmp_id, &p->file.id, sizeof(key2_t));

                    p = p->next;

                    file_list_remove(&tracker->files, &tmp_id);
                } else {
                    p = p->next;
                }
            }

            // send a zeroed file_t to the peer so it knows we're done sending files
            if (-1 == send_res(tracker_fd, SUCCESS, &zero, sizeof(file_t))) {
                print(LOG_ERROR, "[MOVE_DATA] Error at send_res\n");
                break;
            }

            pthread_mutex_unlock(&tracker->lock);

            break;
        }
        case UPLOAD: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received UPLOAD\n");

            file_t file;
            deserialize_file(&file, msg, msg_size);

            pthread_mutex_lock(&tracker->lock);
            
            if (-1 == save_file(&file, DEFAULT_SAVE_LOCATION)) {
                print(LOG_ERROR, "[UPLOAD] Error at save_file\n");
                break;
            }

            file_list_add(&tracker->files, &file);
            
            pthread_mutex_unlock(&tracker->lock);

            // send a zeroed file_t to the peer so it knows we're done sending files
            if (-1 == send_res(tracker_fd, SUCCESS, NULL, 0)) {
                print(LOG_ERROR, "[UPLOAD] Error at send_res\n");
                break;
            }

            break;
        }
        case SHUTDOWN: {
            print(LOG_DEBUG, "[tracker_local_server_thread] Received SHUTDOWN\n");

            running = 0;

            break;
        }
        default: {
            print(LOG_ERROR, "[tracker_local_server_thread] Invalid command\n");

            if (-1 == send_res(tracker_fd, ERROR, INVALID_COMMAND, strlen(INVALID_COMMAND))) {
                print(LOG_ERROR, "[tracker_local_server_thread] Error at send_res\n");
                continue;
            }

            break;
        }
        }

        free(msg);
        shutdown(tracker_fd, SHUT_RDWR);
        close(tracker_fd);
    }

    pthread_exit(NULL);
}

int32_t tracker_init_dht_connection(tracker_t *tracker, int32_t bootstrap_fd) {
    // initialize key
    key_from_addr(&tracker->node.id, &tracker->addr);

    // initialize addr
    memcpy(&tracker->node.addr, &tracker->addr, sizeof(tracker->addr));

    char *msg;
    uint32_t msg_size;

    if (-1 == send_and_recv(bootstrap_fd, CONNECT_TRACKER, &tracker->node, sizeof(node_remote_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[tracker_init_dht_connection] Error at send_and_recv\n");
        free(msg);
        return -1;
    }

    node_remote_t *peer = NULL;

    // if msg_size > 0 it means we received a peer, otherwise we are the only tracker in the network
    if (msg_size > 0) {
        peer = (node_remote_t*)malloc(sizeof(node_remote_t));
        memcpy(peer, msg, sizeof(node_remote_t));

        print(LOG_DEBUG, "[tracker_init_dht_connection] Received peer:\n");
        print_remote_node(LOG_DEBUG, peer);
        print(LOG_DEBUG, "\n");
    } else {
        print(LOG_DEBUG, "[tracker_init_dht_connection] No peer received\n");
    }

    free(msg);

    if (-1 == node_join(&tracker->node, peer)) {
        print(LOG_ERROR, "[tracker_init_dht_connection] Error at node_join\n");
        free(peer);
        return -1;
    }

    print(LOG_DEBUG, "[tracker_init_dht_connection] Joined network successfully\n");

    // after a successful join, we need to ask our successor to pass to us the keys in the interval (node.next.prev, node]
    // if the node is the only peer, we can skip this step
    if (peer) {
        // connect to node
        int32_t fd;

        if (-1 == (fd = get_client_socket(&tracker->node.finger[0].node.addr))) {
            print(LOG_ERROR, "[tracker_init_dht_connection] Error at get_client_socket\n");
            free(peer);
            return -1;
        }

        if (-1 == send_req(fd, MOVE_DATA, &tracker->node.id, sizeof(key2_t))) {
            print(LOG_ERROR, "[tracker_init_dht_connection] Error at connect\n");
            close(fd);
            free(peer);
            return -1;
        }

        res_header_t resh;
        file_t file, zero;
        memset(&zero, 0, sizeof(file_t));

        // read file_t's until we encounter zero
        while (1) {
            if (-1 == recv_res(fd, &resh, &msg, &msg_size)) {
                print(LOG_ERROR, "[tracker_init_dht_connection] Error at recv_res\n");
                free(msg);
                continue;
            }

            if (resh.type == ERROR) {
                print(LOG_ERROR, "[tracker_init_dht_connection] %s\n", msg);
                free(msg);
                continue;
            }

            // we read a null file_t, stop
            if (msg_size == sizeof(file_t) && memcmp(msg, &zero, sizeof(file_t)) == 0) {
                break;
            }

            deserialize_file(&file, msg, msg_size);

            free(msg);

            if (-1 == save_file(&file, DEFAULT_SAVE_LOCATION)) {
                print(LOG_ERROR, "[tracker_init_dht_connection] Error at save_file\n");
                continue;
            }

            file_list_add(&tracker->files, &file);
        }

        close(fd);
    }

    free(peer);

    return 0;
}

int32_t tracker_cleanup(tracker_t *tracker) {
    // shutdown downloader
    downloader_cleanup(&tracker->downloader);
    
    // send shutdown request to all threads
    // TODO: maybe some form of validation that the tracker wants to shutdown its threads
    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        int32_t fd;

        if (-1 == (fd = get_client_socket(&tracker->addr))) {
            print(LOG_ERROR, "[tracker_cleanup] Error at get_client_socket\n");
            return -1;
        }

        if (-1 == send_req(fd, SHUTDOWN, NULL, 0)) {
            print(LOG_ERROR, "[tracker_cleanup] Error at send_req\n");
            return -1;
        }
    }
    
    // join all threads after gracefully shutting em down
    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(tracker->tid[i], NULL);
    }

    shutdown(tracker->fd, SHUT_RDWR);
    close(tracker->fd);

    // before leaving, we must send the keys that we handle to our successor
    while (tracker->files) {
        char *file_ser;
        uint32_t file_ser_size;

        serialize_file(&tracker->files->file, &file_ser, &file_ser_size);

        char *msg;
        uint32_t msg_size;
        
        if (-1 == node_req(&tracker->node.finger[0].node, UPLOAD, file_ser, file_ser_size, &msg, &msg_size)) {
            print(LOG_ERROR, "[tracker_upload] Error at node_req\n");
            free(file_ser);
            return -1;
        }

        free(msg);
        free(file_ser);

        file_list_remove(&tracker->files, &tracker->files->file.id);
    }

    local_file_list_free(&tracker->local_files);

    return 0;
}



void tracker_state(tracker_t *tracker) {
    pthread_mutex_lock(&tracker->lock);

    print(LOG, "local server addr: ");
    print_addr(LOG, &tracker->addr);
    print(LOG, "\n");

    print(LOG, "threads tids:\n");
    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        print(LOG, "- %d: %d\n", i, tracker->tid[i]);
    }

    print(LOG, "local node info\n");
    print(LOG, "- remote info: ");
    print_remote_node(LOG, (node_remote_t*)&tracker->node);
    print(LOG, "\n");

    /*
    print(LOG, "- finger table:\n");
    for (int32_t i = 0; i < KEY_BITS; i++) {
        if (tracker->node.finger[i].initialized == 0) {
            continue;
        }

        if (i == 0) {
            print(LOG, "    next.start: ");
            print_key(LOG, tracker->node.finger[i].start);
            print(LOG, "\n");
            print(LOG, "    next.node: ");
            print_remote_node(LOG, &tracker->node.finger[i].node);
            print(LOG, "\n");
        } else {
            print(LOG, "    finger[%d].start: ", i);
            print_key(LOG, tracker->node.finger[i].start);
            print(LOG, "\n");
            print(LOG, "    finger[%d].node: ", i);
            print_remote_node(LOG, &tracker->node.finger[i].node);
            print(LOG, "\n");
        }
    }
    */

    print(LOG, "- next:\n    start: ");
    print_key(LOG, &tracker->node.finger[0].start);
    print(LOG, "\n  - node: ");
    print_remote_node(LOG, &tracker->node.finger[0].node);
    print(LOG, "\n");

    print(LOG, "- prev: ");
    if (tracker->node.prev_initialized == 0) {
        print(LOG, "-");
    } else {
        print_remote_node(LOG, &tracker->node.prev);
    }
    print(LOG, "\n\n");

    print(LOG, "files\n");
    print_file_list(LOG, &tracker->files);
    print(LOG, "\n");

    pthread_mutex_unlock(&tracker->lock);
}

int32_t tracker_stabilize(tracker_t *tracker) {
    pthread_mutex_lock(&tracker->lock);

    // check all nodes to see if they're still alive

    // check all other fingers
    for (int32_t i = 0; i < KEY_BITS; i++) {
        if (tracker->node.finger[i].initialized == 0) {
            continue;
        }

        char *msg;
        uint32_t msg_size;

        if (-1 == node_req(&tracker->node.finger[i].node, PING, NULL, 0, &msg, &msg_size)) {
            tracker->node.finger[i].initialized = 0;
            free(msg);
            continue;
        }

        free(msg);

        // if we can connect, see if next pointer must be updated
        if (tracker->node.finger[0].initialized == 0) {
            memcpy(&tracker->node.finger[0].node, &tracker->node.finger[i].node, sizeof(node_remote_t));
            tracker->node.finger[0].initialized = 1;

            // we found a new next, notify it
            if (key_cmp(&tracker->node.id, &tracker->node.finger[0].node.id)) {
                if (-1 == node_notify_remote(&tracker->node, &tracker->node.finger[0].node, (node_remote_t*)&tracker->node)) {
                    print(LOG_ERROR, "[tracker_stabilize] Error at node_notify_remote\n");
                    return -1;
                }
            }
        }
    }

    // check previous pointer
    if (tracker->node.prev_initialized == 1) {
        char *msg;
        uint32_t msg_size;

        if (-1 == node_req(&tracker->node.prev, PING, NULL, 0, &msg, &msg_size)) {
            tracker->node.prev_initialized = 0;
        } else {
            // if we can connect, see if next pointer must be updated
            if (tracker->node.finger[0].initialized == 0) {
                memcpy(&tracker->node.finger[0].node, &tracker->node.prev, sizeof(node_remote_t));
                tracker->node.finger[0].initialized = 1;

                // we found a new next, notify it
                if (key_cmp(&tracker->node.id, &tracker->node.finger[0].node.id)) {
                    if (-1 == node_notify_remote(&tracker->node, &tracker->node.finger[0].node, (node_remote_t*)&tracker->node)) {
                        print(LOG_ERROR, "[tracker_stabilize] Error at node_notify_remote\n");
                        return -1;
                    }
                }
            }
        }

        free(msg);
    }

    // if there was no next found, tell client that he must rejoin network
    if (tracker->node.finger[0].initialized == 0) {
        return 1;
    }

    int32_t status = node_stabilize(&tracker->node);

    for (int32_t i = 0; i < 5 && status == 0; i++) {
        status = node_fix_fingers(&tracker->node);
    }

    if (-1 == status) {
        print(LOG_ERROR, "[tracker_stabilize] Error at stabilization\n");
        return -1;
    }

    file_node_t *p = tracker->files;

    // get rid of keys that i shouldn't handle and send them to my peers
    while (p) {
        if (!key_in(&p->file.id, &tracker->node.prev.id, 0, &tracker->node.id, 1)) {
            node_remote_t peer;

            if (-1 == node_find_next(&tracker->node, &p->file.id, &peer)) {
                print(LOG_DEBUG, "[tracker_stabilize] Error at node_find_next\n");
            }

            char *file_ser;
            uint32_t file_ser_size;

            serialize_file(&p->file, &file_ser, &file_ser_size);

            char *msg;
            uint32_t msg_size;
            
            if (-1 == node_req(&peer, UPLOAD, file_ser, file_ser_size, &msg, &msg_size)) {
                print(LOG_ERROR, "[tracker_upload] Error at node_req\n");
                free(file_ser);
                return -1;
            }

            free(msg);
            free(file_ser);

            file_list_remove(&tracker->files, &p->file.id);
        }
        
        p = p->next;
    }

    pthread_mutex_unlock(&tracker->lock);

    return 0;
}

// search a file on the network based on a query
int32_t tracker_search(tracker_t *tracker, query_t *query, int32_t server_fd, query_result_t** results, uint32_t *results_size) {
    if (-1 == send_and_recv(server_fd, SEARCH, query, sizeof(query_t), (char**)results, results_size)) {
        print(LOG_ERROR, "[tracker_search] Error at send_and_recv\n");
        free(results);
        return -1;
    }

    return 0;
}

// download the specified torrent file
int32_t tracker_download(tracker_t *tracker, key2_t *id) {
    // find the peer that handles id
    node_remote_t peer;
    if (-1 == node_find_next(&tracker->node, id, &peer)) {
        print(LOG_ERROR, "[tracker_download] Error at node_find_next\n");
        return -1;
    }

    char *msg;
    uint32_t msg_size;

    // we know for certain the node exists, ask it if the key id has a value
    if (-1 == node_req(&peer, SEARCH, id, sizeof(key2_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[tracker_download] Error at node_req\n");
        free(msg);
        return -1;
    }

    if (msg == NULL) {
        print(LOG_ERROR, "[tracker_download] File not found\n");
        // no need to free
        return -1;
    }

    file_t file;
    deserialize_file(&file, msg, msg_size);

    if (-1 == downloader_add(&tracker->downloader, &file)) {
        print(LOG_ERROR, "[tracker_download] Error at downloader_add\n");
        return -1;
    }

    struct {
        node_remote_t peer;
        key2_t id;
    } data;
    memcpy(&data.peer, &tracker->node, sizeof(node_remote_t));
    memcpy(&data.id, id, sizeof(key2_t));

    if (-1 == node_req(&peer, ADD_PEER, &data, sizeof(data), &msg, &msg_size)) {
        print(LOG_ERROR, "[tracker_download] Error at node_req\n");
        free(msg);
        return -1;
    }

    free(msg);

    return 0;
}

// upload the regular file at path to the network
int32_t tracker_upload(tracker_t *tracker, const char *path, int32_t server_fd) {
    if (strlen(path) >= 512) {
        print(LOG_ERROR, "[tracker_upload] File path too large\n");
        return -1;
    }

    // create the torrent file associated with this file
    file_t file;

    if (-1 == create_file(&file, path, (node_remote_t*)&tracker->node)) {
        print(LOG_ERROR, "[tracker_upload] Error at create_file\n");
        return -1;
    }

    // find the peer that should take care of this file
    node_remote_t peer;
    if (-1 == node_find_next(&tracker->node, &file.id, &peer)) {
        print(LOG_ERROR, "[tracker_upload] Error at node_find_next\n");
        return -1;
    }

    // send the file to the peer
    char *file_ser;
    uint32_t file_ser_size;

    serialize_file(&file, &file_ser, &file_ser_size);

    char *msg;
    uint32_t msg_size;
    
    if (-1 == node_req(&peer, UPLOAD, file_ser, file_ser_size, &msg, &msg_size)) {
        print(LOG_ERROR, "[tracker_upload] Error at node_req\n");
        free(file_ser);
        return -1;
    }

    free(msg);
    free(file_ser);

    // save the original file entry
    local_file_t local_file;

    local_file_from_file(&local_file, &file, path);

    local_file_list_add(&tracker->local_files, &local_file);

    // notify the server that we uploaded a file
    // send the file_t without the peers list because it's hard to maintain
    if (-1 == send_and_recv(server_fd, UPLOAD, &file, sizeof(file_t) - sizeof(list_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[tracker_upload] Error at send_and_recv\n");
        free(msg);
        return -1;
    }

    free(msg);

    return 0;
}
