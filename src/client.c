#include "client.h"

client_t client;

void handle_error(const char* msg) {
    print(LOG_ERROR, msg);
    client_cleanup(&client);
    exit(1);
}

int32_t main(int32_t argc, char **argv) {
    if (argc < 3) {
        handle_error("Usage: ./client TRACKER_IP TRACKER_PORT \n");
    }

    client_init(&client);

    while (1) {
        char cmd_raw[512];
        print(LOG, "\n> ");
        fgets(cmd_raw, 511, stdin);
        cmd_raw[strlen(cmd_raw) - 1] = 0;
        uint32_t cmd_raw_size = strlen(cmd_raw);

        // parse command
        char *cmd = strtok(cmd_raw, " ");
        char *arg1 = strtok(NULL, " ");
        char *arg2 = strtok(NULL, " ");

        if (cmd == NULL) {
            continue;
        }

        // tracker commands
        if (strcmp(cmd, "start_tracker") == 0) {
            if (-1 == client_start_tracker(&client, argv[1], argv[2])) {
                print(LOG, "error: cannot start tracker\n");
                continue;
            }
            
            continue;
        }

        if (strcmp(cmd, "stop_tracker") == 0) {
            if (!client.tracker) {
                print(LOG, "error: you must enable the tracker\n");
                continue;
            }

            if (-1 == client_stop_tracker(&client)) {
                print(LOG, "error: cannot stop tracker\n");
                continue;
            }
            
            continue;
        }

        if (strcmp(cmd, "state") == 0) {
            if (!client.tracker) {
                print(LOG, "error: you must enable the tracker\n");
                continue;
            }

            tracker_state(client.tracker);
            
            continue;
        }
        
        if (strcmp(cmd, "stabilize") == 0) {
            if (!client.tracker) {
                print(LOG, "error: you must enable the tracker\n");
                continue;
            }
            
            if (-1 == tracker_stabilize(client.tracker)) {
                print(LOG, "error: stabilize error\n");
                continue;
            }

            continue;
        }

        // search and download have implementations for both trackers and non-trackers
        if (strcmp(cmd, "search") == 0) {
            // get the query
            query_t query;
            memset(&query, 0, sizeof(query_t));
            char buf[512];
            
            print(LOG, "id: ");
            memset(buf, 0, 512);
            fgets(buf, 511, stdin);
            buf[strlen(buf) - 1] = 0;
            
            if (strlen(buf) == 1 && buf[0] == '-') {
                query.ignore_id = 1;
            } else {
                query.ignore_id = 0;

                // check if the key is valid
                int32_t valid_key = 1;
                
                if (strlen(buf) != 2 * KEY_SIZE) {
                    valid_key = 0;
                }

                for (uint32_t i = 0; valid_key && i < strlen(buf); i++) {
                    if (!(('0' <= buf[i] && buf[i] <= '9') || ('a' <= buf[i] && buf[i] <= 'f') || ('A' <= buf[i] && buf[i] <= 'F'))) {
                        valid_key = 0;
                        break;
                    }
                }

                if (!valid_key) {
                    print(LOG, "error: invalid key format\n");
                    continue;
                }

                memcpy(&query.id, buf, sizeof(key2_t));
            }
            
            print(LOG, "name: ");
            memset(buf, 0, 512);
            fgets(buf, 511, stdin);
            buf[strlen(buf) - 1] = 0;
            
            if (strlen(buf) == 1 && buf[0] == '-') {
                query.ignore_name = 1;
            } else {
                query.ignore_name = 0;

                memcpy(query.name, buf, strlen(buf));
            }

            print(LOG, "size: ");
            memset(buf, 0, 512);
            fgets(buf, 511, stdin);
            buf[strlen(buf) - 1] = 0;
            
            if (strlen(buf) == 1 && buf[0] == '-') {
                query.ignore_size = 1;
            } else {
                query.ignore_size = 0;

                // check if it's a number
                int32_t valid_size = 1;

                for (uint32_t i = 0; i < strlen(buf); i++) {
                    if (!('0' <= buf[i] && buf[i] <= '9')) {
                        valid_size = 0;
                        break;
                    }
                }

                query.size = 0;

                for (uint32_t i = 0; valid_size && i < strlen(buf); i++) {
                    uint32_t new_size = query.size * 10 + buf[i] - '0';
                    
                    // check for overflow
                    if (new_size < query.size) {
                        valid_size = 0;
                        break;
                    }
                    
                    query.size = new_size;
                }

                if (!valid_size) {
                    print(LOG, "error: invalid size format\n");
                    continue;
                }

                // query.size is initialized
            }

            // print_query(LOG_DEBUG, &query);

            if (!client.tracker) {
                // TODO: implement search for non trackers
            } else {
                if (-1 == tracker_search(client.tracker, &query, client.bootstrap_fd)) {
                    print(LOG, "error: search error\n");
                    continue;
                }
            }

            continue;
        }

        if (strcmp(cmd, "download") == 0) {
            if (!client.tracker) {
                // TODO: implement download for non trackers
            } else {
                
            }
            
            continue;
        }

        if (strcmp(cmd, "upload") == 0) {
            if (!client.tracker) {
                print(LOG, "error: you must enable the tracker\n");
                continue;
            }

            if (-1 == tracker_upload(client.tracker, arg1, client.bootstrap_fd)) {
                print(LOG, "error: upload error\n");
                continue;
            }

            continue;
        }

        if (strcmp(cmd, "file_test") == 0) {
            file_t file;

            if (-1 == create_file(&file, "/home/yorknez/uaic/2sem1/rc/mitrorrent/src/client.c", (node_remote_t*)&client.tracker->node)) {
                print(LOG, "error: create_file\n");
                continue;
            }
            
            print_file(LOG_DEBUG, &file);

            if (-1 == save_file(&file, DEFAULT_SAVE_LOCATION)) {
                print(LOG, "error: save_file\n");
                continue;
            }

            if (-1 == load_file(&file)) {
                print(LOG, "error: load_file\n");
                continue;
            }

            print_file(LOG_DEBUG, &file);

            if (-1 == delete_file(&file)) {
                print(LOG, "error: delete_file\n");
                continue;
            }

            continue;
        }

        // regular client commands
        if (strcmp(cmd, "clear") == 0) {
            system("clear");
            continue;
        }

        if (strcmp(cmd, "quit") == 0) {
            break;
        }
    }

    client_cleanup(&client);
    exit(0);
}

void client_init(client_t *client) {
    client_init_bootstrap_server_connection(client);
    client->tracker = NULL; // client is not a tracker yet
}

void client_init_bootstrap_server_connection(client_t *client) {
    // init the socket
    if (-1 == (client->bootstrap_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        handle_error("[client_init_dht_connection] Error at boostrap socket\n");
    }

    // connect to bootstrap server address
    client->bootstrap_addr.sin_family = AF_INET;
    client->bootstrap_addr.sin_addr.s_addr = BOOTSTRAP_SERVER_IP;
    client->bootstrap_addr.sin_port = htons(BOOTSTRAP_SERVER_PORT);

    if (-1 == connect(client->bootstrap_fd, (struct sockaddr*)&client->bootstrap_addr, sizeof(client->bootstrap_addr))) {
        handle_error("[client_init_dht_connection] Error at connect\n");
    }
}

void client_cleanup(client_t *client) {
    if (client->tracker) {
        client_stop_tracker(client);
    }

    char *msg;
    uint32_t msg_size;

    // notify server that we want to disconnect
    if (-1 == send_and_recv(client->bootstrap_fd, DISCONNECT, NULL, 0, &msg, &msg_size)) {
        // if we get here, it's bad
        print(LOG_ERROR, "[client_cleanup] Error at send_and_recv, disconnecting anyway\n");
    }

    free(msg);

    shutdown(client->bootstrap_fd, SHUT_RDWR);
    close(client->bootstrap_fd);
}

int32_t client_start_tracker(client_t *client, const char *tracker_ip, const char *tracker_port) {
    // create DEFAULT_SAVE_LOCATION if it doesn't exist
    if (-1 == access(DEFAULT_SAVE_LOCATION, F_OK)) {
        if (-1 == mkdir(DEFAULT_SAVE_LOCATION, 0755)) {
            print(LOG_ERROR, "[client_start_tracker] Cannot create `%s` folder\n", DEFAULT_SAVE_LOCATION);
            return -1;
        }
    }

    client->tracker = (tracker_t*)malloc(sizeof(tracker_t));

    if (-1 == tracker_init(client->tracker, tracker_ip, tracker_port, client->bootstrap_fd)) {
        print(LOG_ERROR, "[client_start_tracker] Cannot start tracker\n");
        return -1;
    }

    return 0;
}

int32_t client_stop_tracker(client_t *client) {
    char *msg;
    uint32_t msg_size;

    // notify server that we want to disconnect tracker
    if (-1 == send_and_recv(client->bootstrap_fd, DISCONNECT_TRACKER, &client->tracker->node.id, sizeof(key2_t), &msg, &msg_size)) {
        // if we get here, it's bad
        print(LOG_ERROR, "[client_stop_tracker] Error at send_and_recv, disconnecting anyway\n");
    }

    free(msg);

    if (-1 == tracker_cleanup(client->tracker)) {
        print(LOG_ERROR, "[client_stop_tracker] Error at tracker_cleanup\n");
        return -1;
    }

    free(client->tracker);
    client->tracker = NULL;

    return 0;
}