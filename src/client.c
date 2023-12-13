#include "client.h"

client_t client;

void handle_error(const char* msg) {
    print(LOG_ERROR, msg);
    client_cleanup(&client);
    exit(1);
}

// TODO: maybe move the function and add help menus for all commands
void print_help() {
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

    system("clear");

    print(LOG,
        ANSI_COLOR_RED "PROLOG\n" ANSI_COLOR_RESET
        "    This is the torrent client of the project\n\n"
        ANSI_COLOR_RED "DESCRIPTION\n" ANSI_COLOR_RESET
        "    The list of commands is the following\n\n"
        ANSI_COLOR_RED "COMMANDS\n" ANSI_COLOR_RESET
        "    " ANSI_COLOR_GREEN "help    " ANSI_COLOR_RESET " - show this message;\n\n"
        "    " ANSI_COLOR_GREEN "tracker " ANSI_COLOR_RESET " - see " ANSI_COLOR_RED "tracker -h" ANSI_COLOR_RESET ";\n\n"
        "    " ANSI_COLOR_GREEN "search  " ANSI_COLOR_RESET " - see " ANSI_COLOR_RED "search -h" ANSI_COLOR_RESET ";\n\n"
        "    " ANSI_COLOR_GREEN "download" ANSI_COLOR_RESET " - see " ANSI_COLOR_RED "download -h" ANSI_COLOR_RESET ";\n\n"
        "    " ANSI_COLOR_GREEN "upload  " ANSI_COLOR_RESET " - see " ANSI_COLOR_RED "upload -h" ANSI_COLOR_RESET ";\n\n"
        "    " ANSI_COLOR_GREEN "clear   " ANSI_COLOR_RESET " - clears the screen;\n\n"
        "    " ANSI_COLOR_GREEN "quit    " ANSI_COLOR_RESET " - exit this terminal;\n\n");
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

        cmd_t cmd;

        if (-1 == cmd_parse(&cmd, cmd_raw)) {
            print(LOG, "error: invalid command format\n");
            continue;
        }

        if (strcmp(cmd.name, "tracker") == 0) {
            if (cmd.args_size != 1) {
                print(LOG, "error: invalid number of args\n");
                continue;
            }

            if (strcmp(cmd.args[0].flag, "-start") == 0) {
                if (-1 == client_start_tracker(&client, argv[1], argv[2])) {
                    print(LOG, "error: cannot start tracker\n");
                    continue;
                }

                continue;
            }

            if (strcmp(cmd.args[0].flag, "-stop") == 0) {
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

            if (strcmp(cmd.args[0].flag, "-state") == 0) {
                if (!client.tracker) {
                    print(LOG, "error: you must enable the tracker\n");
                    continue;
                }

                tracker_state(client.tracker);
                
                continue;
            }

            if (strcmp(cmd.args[0].flag, "-stab") == 0) {
                if (!client.tracker) {
                    print(LOG, "error: you must enable the tracker\n");
                    continue;
                }
                
                int32_t status = tracker_stabilize(client.tracker);

                if (-1 == status) {
                    print(LOG, "error: stabilize error\n");
                    continue;
                } else if (1 == status) {
                    print(LOG, "error: stopping tracker, rejoin network\n");
                    
                    if (-1 == client_stop_tracker(&client)) {
                    print(LOG, "error: cannot stop tracker\n");
                        continue;
                    }

                    continue;
                }

                continue;
            }
        }

        // search and download have implementations for both trackers and non-trackers
        if (strcmp(cmd.name, "search") == 0) {
            if (!(1 <= cmd.args_size && cmd.args_size <= 3)) {
                print(LOG, "error: invalid number of args\n");
                continue;
            }
            
            query_t query;
            memset(&query, 0, sizeof(query_t));
            query.ignore_id = query.ignore_name = query.ignore_size = 1;

            int32_t status = 0;

            for (uint32_t i = 0; i < cmd.args_size; i++) {
                char *buf = cmd.args[i].value;
                
                // query.id
                if (strcmp(cmd.args[i].flag, "-i") == 0) {
                    query.ignore_id = 0;

                    if (-1 == key_from_text(&query.id, buf)) {
                        print(LOG, "error: invalid key format\n");
                        status = -1;
                        break;
                    }
                    
                    continue;
                }
                
                // query.name
                if (strcmp(cmd.args[i].flag, "-n") == 0) {
                    query.ignore_name = 0;

                    strcpy(query.name, buf);
                    
                    continue;
                }

                // query.size
                if (strcmp(cmd.args[i].flag, "-s") == 0) {
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
                        status = -1;
                        break;
                    }

                    // query.size is initialized
                    
                    continue;
                }
            }

            if (-1 == status) {
                continue;
            }

            if (query.ignore_id * query.ignore_name * query.ignore_size > 0) {
                print(LOG, "error: invalid query\n");
                continue;
            }

            query_result_t* results;
            uint32_t results_size;

            // non trackers and trackers use the same search function
            if (-1 == client_search(&client, &query, &results, &results_size)) {
                print(LOG, "error: search error\n");
                continue;
            }

            // no results found
            if (results_size == 0) {
                print(LOG, "error: no results found\n");
                continue;
            } 

            results_size = results_size / sizeof(query_result_t);

            // see the keys
            for (uint32_t i = 0; i < results_size; i++) {
                print(LOG, "%d. ", i + 1);
                print_result(LOG, &results[i]);
            }

            free(results);

            continue;
        }

        if (strcmp(cmd.name, "download") == 0) {
            if (cmd.args_size != 1) {
                print(LOG, "error: invalid number of args\n");
                continue;
            }

            // download a key
            if (strcmp(cmd.args[0].flag, "-k") == 0) {
                key2_t id;

                if (-1 == key_from_text(&id, cmd.args[0].value)) {
                    print(LOG, "error: invalid key format\n");
                    continue;
                }
                
                if (!client.tracker) {
                    if (-1 == client_download(&client, &id)) {
                        print(LOG, "error: download error\n");
                        continue;
                    }                    
                } else {
                    if (-1 == tracker_download(client.tracker, &id)) {
                        print(LOG, "error: download error\n");
                        continue;
                    }
                }
                
                continue;
            }

            // list downloads
            if (strcmp(cmd.args[0].flag, "-l") == 0) {
                print_downloader(LOG, &client.downloader);

                continue;
            }

            // pause a download
            if (strcmp(cmd.args[0].flag, "-p") == 0) {
                downloader_pause(&client.downloader, atoi(cmd.args[0].value));

                continue;
            }
        }

        if (strcmp(cmd.name, "upload") == 0) {
            if (!client.tracker) {
                print(LOG, "error: you must enable the tracker\n");
                continue;
            }

            if (cmd.args_size != 1) {
                print(LOG, "error: invalid number of args\n");
                continue;
            }

            // upload file by path
            if (strcmp(cmd.args[0].flag, "-p") == 0) {
                if (-1 == tracker_upload(client.tracker, cmd.args[0].value, client.bootstrap_fd)) {
                    print(LOG, "error: upload error\n");
                    continue;
                }

                continue;
            }
        }

        // regular client commands
        if (strcmp(cmd.name, "help") == 0) {
            print_help();
            
            continue;
        }

        if (strcmp(cmd.name, "clear") == 0) {
            if (cmd.args_size != 0) {
                print(LOG, "error: invalid number of args\n");
                continue;
            }
            
            system("clear");
            continue;
        }

        if (strcmp(cmd.name, "quit") == 0) {
            if (cmd.args_size != 0) {
                print(LOG, "error: invalid number of args\n");
                continue;
            }
            
            break;
        }

        print(LOG, "error: invalid command\n");
    }

    client_cleanup(&client);
    exit(0);
}

void client_init(client_t *client) {
    client_init_bootstrap_server_connection(client);
    client->tracker = NULL; // client is not a tracker yet

    // init downloader module
    if (-1 == downloader_init(&client->downloader)) {
        handle_error("[tracker_init] Error at downloader_init\n");
    }
}

void client_init_bootstrap_server_connection(client_t *client) {
    // connect to bootstrap server address
    client->bootstrap_addr.sin_family = AF_INET;
    client->bootstrap_addr.sin_addr.s_addr = BOOTSTRAP_SERVER_IP;
    client->bootstrap_addr.sin_port = htons(BOOTSTRAP_SERVER_PORT);

    if (-1 == (client->bootstrap_fd = get_client_socket(&client->bootstrap_addr))) {
        handle_error("[client_init_bootstrap_server_connection] Error at get_client_socket\n");
    }
}

void client_cleanup(client_t *client) {
    // shutdown downloader
    downloader_cleanup(&client->downloader);
    
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

    client->tracker->downloader = &client->downloader;

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

int32_t client_search(client_t *client, query_t *query, query_result_t** results, uint32_t *results_size) {
    if (-1 == send_and_recv(client->bootstrap_fd, SEARCH, query, sizeof(query_t), (char**)results, results_size)) {
        print(LOG_ERROR, "[tracker_search] Error at send_and_recv\n");
        free(results);
        return -1;
    }

    return 0;
}

int32_t client_download(client_t *client, key2_t *id) {
    char *msg;
    uint32_t msg_size;

    // we know for certain the node exists, ask it if the key id has a value
    if (-1 == send_and_recv(client->bootstrap_fd, SEARCH_PEER, id, sizeof(key2_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[client_download] Error at send_and_recv\n");
        free(msg);
        return -1;
    }

    if (msg == NULL) {
        print(LOG_ERROR, "[client_download] File not found\n");
        // no need to free
        return -1;
    }

    file_t file;
    deserialize_file(&file, msg, msg_size);

    if (-1 == downloader_add(&client->downloader, &file)) {
        print(LOG_ERROR, "[client_download] Error at downloader_add\n");
        return -1;
    }

    free(msg);

    return 0;
}
