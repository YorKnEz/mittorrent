#include "server.h"

server_t server;

void handle_error(const char* msg) {
    print(LOG_ERROR, msg);
    server_cleanup(&server);
    exit(1);
}

int32_t main() {
    server_init(&server);

    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(server.tid[i], NULL);
    }

    server_cleanup(&server);
    exit(0);
}

void server_init(server_t *server) {
    // init locks
    pthread_mutex_t init = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&server->lock, &init, sizeof(pthread_mutex_t));
    memcpy(&server->mlock, &init, sizeof(pthread_mutex_t));

    // init the socket
    if (-1 == (server->fd = socket(AF_INET, SOCK_STREAM, 0))) {
        handle_error("Error at socket\n");
    }

    // enable SO_REUSEADDR
    int32_t on = 1;
    setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // bind it to the default address
    server->addr.sin_family = AF_INET;
    server->addr.sin_addr.s_addr = BOOTSTRAP_SERVER_IP;
    server->addr.sin_port = htons(BOOTSTRAP_SERVER_PORT);

    if (-1 == bind(server->fd, (struct sockaddr*)&server->addr, sizeof(server->addr))) {
        handle_error("Error at bind\n");
    }

    // start listening for connections
    if (-1 == listen(server->fd, 0)) {
        handle_error("Error at listen\n");
    }

    print(LOG_DEBUG, "Bootsrap server is listening for connections\n");

    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&server->tid[i], NULL, (void *(*)(void*))&server_thread, server);
    }
}

void server_thread(server_t *server) {
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    // fd set of clients
    fd_set allfd;
    FD_ZERO(&allfd);
    FD_SET(server->fd, &allfd);
    int32_t max_fd = server->fd;
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
            print(LOG_ERROR, "Error at select\n");
            break;
        }

        // workaround for new connections
        // if we rely on the first select for handling new connections, the threads
        // have no way of knowing if another thread already handled the connection
        // so deadlock will occur
        // thus, we make a separate set only for the server->fd and select on it
        // with a timeout, so that way each thread can check if the server->fd
        // connection has been handled
        pthread_mutex_lock(&server->mlock);

        FD_SET(server->fd, &server_fd); // re-add server->fd to set

        if (-1 == select(server->fd + 1, &server_fd, NULL, NULL, &tv)) {
            print(LOG_ERROR, "Error at select2\n");
            break;
        }

        if (FD_ISSET(server->fd, &server_fd)) {
            int32_t fd = accept(server->fd, (struct sockaddr*)&client_addr, &client_size);

            if (-1 != fd) {
                FD_SET(fd, &allfd);

                if (fd > max_fd) max_fd = fd;
            } else {
                print(LOG_ERROR, "Error at accept\n");
            }
        }
        
        pthread_mutex_unlock(&server->mlock);

        for (int32_t client_fd = 0; client_fd <= max_fd; client_fd++) {
            if (client_fd != server->fd && FD_ISSET(client_fd, &rdfd)) {
                // handle request
                print(LOG_DEBUG, "Received new request\n");

                req_header_t reqh;
                char *msg;
                uint32_t msg_size;

                if (-1 == recv_req(client_fd, &reqh, &msg, &msg_size)) {
                    print(LOG_ERROR, "Error at recv_req\n");
                    shutdown(client_fd, SHUT_RDWR);
                    close(client_fd);
                    continue;
                }

                switch (reqh.type) {
                case DISCONNECT: {
                    print(LOG_DEBUG, "DISCONNECT received\n");

                    // remove client fd from list
                    FD_CLR(client_fd, &allfd);

                    break;
                }
                case CONNECT_TRACKER: {
                    node_remote_t new_client;
                    memcpy(&new_client, msg, sizeof(node_remote_t));
                    
                    print(LOG_DEBUG, "CONNECT_TRACKER received\n");

                    // add client to list
                    pthread_mutex_lock(&server->lock);

                    list_add(&server->clients, &new_client);
                    print_list(LOG_DEBUG, &server->clients);

                    // give the new client a peer to connect to
                    // so the it can connect to the dht network
                    void *data;
                    uint32_t data_size;

                    // if the list has only one element,
                    // it means there is only one client in the network
                    // so there is no peer to send
                    if (server->clients->next == NULL) {
                        data = NULL;
                        data_size = 0;
                        // this is a good time to initialize client_read ptr
                        server->client_read = server->clients;
                    } else {
                        node_remote_t peer;
                        memcpy(&peer, &server->client_read->node, sizeof(node_remote_t));
                        
                        data = &peer;
                        data_size = sizeof(node_remote_t);

                        // treat linked list as circular
                        server->client_read = server->client_read->next;
                        if (server->client_read == NULL) server->client_read = server->clients;
                    }

                    pthread_mutex_unlock(&server->lock);

                    if (-1 == send_res(client_fd, SUCCESS, data, data_size)) {
                        print(LOG_ERROR, "Error at send_res\n");
                        shutdown(client_fd, SHUT_RDWR);
                        close(client_fd);
                        break;
                    }

                    break;
                }
                case DISCONNECT_TRACKER: {
                    key2_t id;
                    memcpy(&id, msg, sizeof(key2_t));

                    print(LOG_DEBUG, "DISCONNECT_TRACKER received\n");
                    
                    pthread_mutex_lock(&server->lock);

                    // if the node we're about to delete is client_read, first update client_read
                    if (key_cmp(&id, &server->client_read->node.id) == 0) {
                        server->client_read = server->client_read->next;
                        if (server->client_read == NULL) server->client_read = server->clients;
                    }

                    // remove client from clients list
                    list_remove(&server->clients, &id);
                    print_list(LOG_DEBUG, &server->clients);

                    // TODO: uploads may need to be recalibrated

                    pthread_mutex_unlock(&server->lock);

                    if (-1 == send_res(client_fd, SUCCESS, NULL, 0)) {
                        print(LOG_ERROR, "Error at send_res\n");
                        shutdown(client_fd, SHUT_RDWR);
                        close(client_fd);
                        break;
                    }

                    break;
                }
                case SEARCH: {
                    query_t query;
                    memcpy(&query, msg, sizeof(query_t));

                    print(LOG_DEBUG, "SEARCH received\n");
                    // print_query(LOG_DEBUG, &query);

                    pthread_mutex_lock(&server->lock);

                    file_node_t *p = server->uploads;

                    // dynamic buffer for results, i don't want to implement another linked list..
                    char *results = NULL;
                    uint32_t results_size = 0;
                    query_result_t result;

                    while (p) {
                        int32_t q_id = key_cmp(&p->file.id, &query.id) == 0;
                        int32_t q_name = strstr(p->file.name, query.name) != NULL;
                        int32_t q_size = p->file.size == query.size;
                        
                        if ((query.ignore_id || q_id) && (query.ignore_name || q_name) && (query.ignore_size || q_size)) {
                            // dynamically add result to buffer
                            memset(&result, 0, sizeof(query_result_t));
                            memcpy(&result.id, &p->file.id, sizeof(key2_t));
                            memcpy(result.name, p->file.name, 512);
                            result.size = p->file.size;

                            char *tmp = (char*)malloc(results_size + sizeof(query_result_t));
                            memcpy(tmp, results, results_size);
                            memcpy(tmp + results_size, &result, sizeof(query_result_t));
                            results_size += sizeof(query_result_t);
                            
                            free(results);
                            results = tmp;
                            tmp = NULL;
                        }
                        
                        p = p->next;
                    }
                    
                    pthread_mutex_unlock(&server->lock);

                    if (-1 == send_res(client_fd, SUCCESS, results, results_size)) {
                        print(LOG_ERROR, "Error at send_res\n");
                        shutdown(client_fd, SHUT_RDWR);
                        close(client_fd);
                        break;
                    }

                    break;
                }
                case UPLOAD: {
                    // it is safe to do memcpy instead of deserializing, because by convention
                    // the file is sent without peers because it would be hard to maintain
                    // server just keeps the relevant queriable parameters of the file
                    file_t file;
                    memcpy(&file, msg, sizeof(file_t) - sizeof(list_t));
                    file.peers = NULL;
                    
                    print(LOG_DEBUG, "UPLOAD received\n");

                    pthread_mutex_lock(&server->lock);

                    file_list_add(&server->uploads, &file);

                    pthread_mutex_unlock(&server->lock);

                    if (-1 == send_res(client_fd, SUCCESS, NULL, 0)) {
                        print(LOG_ERROR, "Error at send_res\n");
                        shutdown(client_fd, SHUT_RDWR);
                        close(client_fd);
                        break;
                    }
                    
                    break;
                }
                default: {
                    print(LOG_DEBUG, "Invalid command\n");

                    if (-1 == send_res(client_fd, ERROR, INVALID_COMMAND, strlen(INVALID_COMMAND))) {
                        print(LOG_ERROR, "Error at send_res\n");
                        shutdown(client_fd, SHUT_RDWR);
                        close(client_fd);
                        break;
                    }

                    break;
                }
                }

                free(msg);
            }
        }
    }

    pthread_exit(NULL);
}

void server_cleanup(server_t *server) {
    shutdown(server->fd, SHUT_RDWR);
    close(server->fd);

    list_free(&server->clients);
}