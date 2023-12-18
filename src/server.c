#include "server.h"

server_t server;

int32_t main() {
    int32_t status = 0;
    int32_t running = 1;

    if (CHECK(server_init(&server))) {
        ERR(status, "cannot initialize server");
        exit(-1);
    }

    while (running) {
        char cmd_raw[512];
        print(LOG, "\n> ");
        fgets(cmd_raw, 511, stdin);
        cmd_raw[strlen(cmd_raw) - 1] = 0;

        // 1. parse the command
        parsed_cmd_t cmd;
        if (CHECK(cmd_parse(&cmd, cmd_raw))) {
            ERR(status, "invalid command format");
            continue;
        }

        // 2. interpret the command
        cmd_type_t cmd_type = CMD_UNKNOWN;
        cmds_get_cmd_type(&server.cmds, &cmd, &cmd_type);

        // 3. execute the command
        switch (cmd_type) {
        case CMD_SERVER_START: {
            // no lock needed because the current thread is the only one that can modify it
            if (server.running == 1) {
                ERR_GENERIC("server is already started");
                break;
            }

            if (CHECK(server_start(&server))) {
                ERR(status, "cannot start server");
                break;
            }

            break;
        }
        case CMD_SERVER_STOP: {
            // no lock needed because the current thread is the only one that can modify it
            if (server.running == 0) {
                ERR_GENERIC("server is already stopped");
                break;
            }

            if (CHECK(server_stop(&server))) {
                ERR(status, "cannot stop server");
                break;
            }

            break;
        }
        case CMD_SERVER_HELP: {
            print_cmd_help(LOG, &server.cmds.list[0]);
            break;
        }
        case CMD_DB_LIST_PEERS: {
            pthread_mutex_lock(&server.lock);
            print_list(LOG, &server.clients);
            pthread_mutex_unlock(&server.lock);

            break;
        }
        case CMD_DB_LIST_UPLOADS: {
            pthread_mutex_lock(&server.lock);
            print_file_list(LOG, &server.uploads);
            pthread_mutex_unlock(&server.lock);

            break;
        }
        case CMD_DB_HELP: {
            print_cmd_help(LOG, &server.cmds.list[1]);
            break;
        }
        case CMD_HELP: {
            print_cmds_help(LOG, &server.cmds);
            break;
        }
        case CMD_CLEAR: {
            system("clear");
            break;
        }
        case CMD_QUIT: {
            running = 0;
            break;
        }
        case CMD_ERROR: {
            // handled already
            break;
        }
        default: {
            ERR_GENERIC("unknown command, see help");
            break;
        }
        }
    }

    if (CHECK(server_cleanup(&server))) {
        ERR(status, "cannot cleanup server");
        exit(-1);
    }

    exit(0);
}

void server_thread(server_t *server) {
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    int32_t local_running = 1;

    while (local_running) {
        pthread_mutex_lock(&server->mlock);

        int32_t client_fd = accept(server->fd, (struct sockaddr*)&client_addr, &client_size);
        
        pthread_mutex_unlock(&server->mlock);

        if (-1 == client_fd) {
            print(LOG_ERROR, "[server_thread] Error at accept\n");
            continue;
        }

        // handle request
        print(LOG_DEBUG, "[server_thread] Received new request\n");

        req_header_t reqh;
        char *msg;
        uint32_t msg_size;

        if (-1 == recv_req(client_fd, &reqh, &msg, &msg_size)) {
            print(LOG_ERROR, "[server_thread] Error at recv_req\n");
            continue;
        }

        switch (reqh.type) {
        case CONNECT_TRACKER: {
            print(LOG_DEBUG, "[server_thread] CONNECT_TRACKER received\n");

            node_remote_t new_client;
            memcpy(&new_client, msg, sizeof(node_remote_t));
            
            // add client to list
            pthread_mutex_lock(&server->lock);

            list_add(&server->clients, &new_client);

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
                print(LOG_ERROR, "[CONNECT_TRACKER] Error at send_res\n");
                break;
            }

            break;
        }
        case DISCONNECT_TRACKER: {
            print(LOG_DEBUG, "[server_thread] DISCONNECT_TRACKER received\n");

            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));
            
            pthread_mutex_lock(&server->lock);

            // if the node we're about to delete is client_read, first update client_read
            if (key_cmp(&id, &server->client_read->node.id) == 0) {
                server->client_read = server->client_read->next;
                if (server->client_read == NULL) server->client_read = server->clients;
            }

            // remove client from clients list
            list_remove(&server->clients, &id);

            // TODO: uploads may need to be recalibrated

            pthread_mutex_unlock(&server->lock);

            if (-1 == send_res(client_fd, SUCCESS, NULL, 0)) {
                print(LOG_ERROR, "[DISCONNECT_TRACKER] Error at send_res\n");
                break;
            }

            break;
        }
        case SEARCH: {
            print(LOG_DEBUG, "[server_thread] SEARCH received\n");

            query_t query;
            memcpy(&query, msg, sizeof(query_t));
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
                print(LOG_ERROR, "[SEARCH] Error at send_res\n");
                break;
            }

            break;
        }
        case UPLOAD: {
            print(LOG_DEBUG, "[server_thread] UPLOAD received\n");

            // it is safe to do memcpy instead of deserializing, because by convention
            // the file is sent without peers because it would be hard to maintain
            // server just keeps the relevant queryable parameters of the file
            file_t file;
            memcpy(&file, msg, sizeof(file_t) - sizeof(list_t));
            file.peers = NULL;

            pthread_mutex_lock(&server->lock);

            file_list_add(&server->uploads, &file);

            pthread_mutex_unlock(&server->lock);

            if (-1 == send_res(client_fd, SUCCESS, NULL, 0)) {
                print(LOG_ERROR, "[UPLOAD] Error at send_res\n");
                break;
            }
            
            break;
        }
        case SEARCH_PEER: {
            print(LOG_DEBUG, "SEARCH_PEER received\n");
            
            key2_t id;
            memcpy(&id, msg, sizeof(key2_t));

            int32_t status = 0;
                
            pthread_mutex_lock(&server->lock);

            char *res;
            uint32_t res_size;

            do {
                // find the peer that handles id
                if (-1 == node_req(&server->client_read->node, FIND_NEXT, &id, sizeof(key2_t), &res, &res_size)) {
                    print(LOG_ERROR, "[SEARCH_PEER] Error at node_find_next\n");
                    status = -1;
                    break;
                }
                    
                if (res == NULL) {
                    break;
                }

                node_remote_t peer;
                memcpy(&peer, res, sizeof(node_remote_t));

                free(res);

                // we know for certain the node exists, ask it if the key id has a value
                if (-1 == node_req(&peer, SEARCH, &id, sizeof(key2_t), &res, &res_size)) {
                    print(LOG_ERROR, "[SEARCH_PEER] Error at node_req\n");
                    free(msg);
                    status = -1;
                    break;
                }

                if (msg == NULL) {
                    print(LOG_ERROR, "[SEARCH_PEER] File not found\n");
                    // no need to free
                    status = -1;
                    break;
                }
             } while (0);

            pthread_mutex_unlock(&server->lock);

            if (-1 == status) {
                if (-1 == send_res(client_fd, ERROR, INTERNAL_ERROR, strlen(INTERNAL_ERROR))) {
                    print(LOG_ERROR, "Error at send_res\n");
                }
            } else {
                if (-1 == send_res(client_fd, SUCCESS, res, res_size)) {
                    print(LOG_ERROR, "Error at send_res\n");
                }
            }

            free(res);
                
            break;
        }
        case SHUTDOWN: {
            // this request is used to break the blocking accept in case the thread doesn't realize that it should stop
            print(LOG_DEBUG, "[server_thread] Received SHUTDOWN\n");
            break;
        }
        default: {
            print(LOG_DEBUG, "[server_thread] Invalid command\n");

            if (-1 == send_res(client_fd, ERROR, INVALID_COMMAND, strlen(INVALID_COMMAND))) {
                print(LOG_ERROR, "[server_thread] Error at send_res\n");
                break;
            }

            break;
        }
        }

        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        free(msg);

        pthread_mutex_lock(&server->running_lock);
        local_running = server->running;
        pthread_mutex_unlock(&server->running_lock);
    }

    pthread_exit(NULL);
}

int32_t server_init(server_t *server) {
    int32_t status = 0;

    // init locks
    pthread_mutex_init(&server->running_lock, NULL);
    pthread_mutex_init(&server->lock, NULL);
    pthread_mutex_init(&server->mlock, NULL);

    // init socket and threads
    if (CHECK(server_start(server))) {
        print(LOG_ERROR, "[server_init] Error at server_start\n");
        return status;
    }

    cmds_t cmds = {
        "This is the torrent server of the project.",
        "The list of commands is described below.",
        5,
        {
            {
                CMD_UNKNOWN,
                "server",
                "Manage server state.",
                3,
                {
                    {CMD_SERVER_HELP, 1, "-h", "--help", NULL,
                     "Print this message."},
                    {CMD_SERVER_START, 1, "-s", "--start", NULL,
                     "Starts the server."},
                    {CMD_SERVER_STOP, 1, "-t", "--stop", NULL,
                     "Stops the server."},
                },
            },
            {
                CMD_UNKNOWN,
                "db",
                "Manage database state.",
                3,
                {
                    {CMD_DB_HELP, 1, "-h", "--help", NULL,
                     "Print this message."},
                    {CMD_DB_LIST_PEERS, 1, "-p", "--peers", NULL,
                     "List the currently connected peers."},
                    {CMD_DB_LIST_UPLOADS, 1, "-u", "--uploads", NULL,
                     "List the file that the server indexed."},
                },
            },
            {
                CMD_HELP,
                "help",
                "List all commands of the client.",
                0,
                {},
            },
            {
                CMD_CLEAR,
                "clear",
                "Clear the terminal screen.",
                0,
                {},
            },
            {
                CMD_QUIT,
                "quit",
                "Exit the client.",
                0,
                {},
            },
        }};

    memcpy(&server->cmds, &cmds, sizeof(cmds_t));

    return status;
}

int32_t server_cleanup(server_t *server) {
    int32_t status = 0;

    if (CHECK(server_stop(server))) {
        print(LOG_ERROR, "[server_cleanup] Error at server_stop\n");
        return status;
    }

    pthread_mutex_destroy(&server->running_lock);
    pthread_mutex_destroy(&server->lock);
    pthread_mutex_destroy(&server->mlock);

    return status;
}

int32_t server_start(server_t *server) {
    int32_t status = 0;

    // server default address
    server->addr.sin_family = AF_INET;
    server->addr.sin_addr.s_addr = BOOTSTRAP_SERVER_IP;
    server->addr.sin_port = htons(BOOTSTRAP_SERVER_PORT);

    if (CHECK(server->fd = get_server_socket(&server->addr))) {
        print(LOG_ERROR, "[server_init] Error at get_server_socket\n");
        return status;
    }

    server->running = 1;

    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&server->tid[i], NULL, (void *(*)(void*))&server_thread, server);
    }

    return status;
}

int32_t server_stop(server_t *server) {
    int32_t status = 0;

    pthread_mutex_lock(&server->lock);
    
    node_t *p = server->clients;

    while (p) {
        char *res;
        uint32_t res_size;

        if (CHECK(node_req(&p->node, SERVER_STOPPED, NULL, 0, &res, &res_size))) {
            print(LOG_ERROR, "[server_stop] Error at node_req\n");
            continue;
        }

        free(res);

        p = p->next;
    }

    list_free(&server->clients);
    server->clients = NULL;
    server->client_read = NULL;
    file_list_free(&server->uploads);
    server->uploads = NULL;

    pthread_mutex_lock(&server->running_lock);
    server->running = 0;
    pthread_mutex_unlock(&server->running_lock);

    pthread_mutex_unlock(&server->lock);

    // send shutdown request to all threads
    // this is to make sure that all threads break out of their accepts
    // TODO: maybe some form of validation that the server wants to shutdown its threads
    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        int32_t fd;

        if (CHECK(fd = get_client_socket(&server->addr))) {
            print(LOG_ERROR, "[server_stop] Error at get_client_socket\n");
            return status;
        }

        if (CHECK(send_req(fd, SHUTDOWN, NULL, 0))) {
            print(LOG_ERROR, "[server_stop] Error at send_req\n");
            return status;
        }
    }
    
    // join all threads after gracefully shutting em down
    for (int32_t i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(server->tid[i], NULL);
    }

    shutdown(server->fd, SHUT_RDWR);
    close(server->fd);

    return status;
}
