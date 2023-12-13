#include "client.h"
#include "error.h"

client_t client;

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
    int32_t status = 0;

    if (argc < 3) {
        ERR_GENERIC("usage: ./client TRACKER_IP TRACKER_PORT");
        exit(-1);
    }

    if (CHECK(client_init(&client))) {
        ERR(status, "cannot initialize client\n");
        exit(-1);
    }

    while (1) {
        char cmd_raw[512];
        print(LOG, "\n> ");
        fgets(cmd_raw, 511, stdin);
        cmd_raw[strlen(cmd_raw) - 1] = 0;
        uint32_t cmd_raw_size = strlen(cmd_raw);

        cmd_t cmd;

        if (CHECK(cmd_parse(&cmd, cmd_raw))) {
            ERR(status, "invalid command format");
            continue;
        }

        if (strcmp(cmd.name, "tracker") == 0) {
            if (cmd.args_size != 1) {
                ERR_GENERIC("invalid number of args");
                continue;
            }

            if (strcmp(cmd.args[0].flag, "-start") == 0) {
                if (client.tracker) {
                    ERR_GENERIC("tracker already started\n");
                    continue;
                }

                if (CHECK(client_start_tracker(&client, argv[1], argv[2]))) {
                    ERR(status, "cannot start tracker");
                    continue;
                }

                continue;
            }

            if (strcmp(cmd.args[0].flag, "-stop") == 0) {
                if (!client.tracker) {
                    ERR_GENERIC("you must enable the tracker");
                    continue;
                }

                if (CHECK(client_stop_tracker(&client))) {
                    ERR(status, "cannot stop tracker");
                    continue;
                }
                
                continue;
            }

            if (strcmp(cmd.args[0].flag, "-state") == 0) {
                if (!client.tracker) {
                    ERR_GENERIC("you must enable the tracker");
                    continue;
                }

                tracker_state(client.tracker);
                
                continue;
            }

            if (strcmp(cmd.args[0].flag, "-stab") == 0) {
                if (!client.tracker) {
                    ERR_GENERIC("you must enable the tracker");
                    continue;
                }
                
                if (CHECK(tracker_stabilize(client.tracker))) {
                    ERR(status, "stabilize error");
                } else if (status == 1) {
                    ERR_GENERIC("stopping tracker, rejoin network");
                    
                    if (CHECK(client_stop_tracker(&client))) {
                        ERR(status, "cannot stop tracker");
                    }
                }

                continue;
            }
        }

        // search and download have implementations for both trackers and non-trackers
        if (strcmp(cmd.name, "search") == 0) {
            if (!(1 <= cmd.args_size && cmd.args_size <= 3)) {
                ERR_GENERIC("invalid number of args");
                continue;
            }
            
            query_t query;
            memset(&query, 0, sizeof(query_t));
            query.ignore_id = query.ignore_name = query.ignore_size = 1;

            int32_t search_status = 0;

            for (uint32_t i = 0; i < cmd.args_size; i++) {
                char *buf = cmd.args[i].value;
                
                // query.id
                if (strcmp(cmd.args[i].flag, "-i") == 0) {
                    query.ignore_id = 0;

                    if (CHECK(key_from_text(&query.id, buf))) {
                        ERR(status, "invalid key format");
                        search_status = -1;
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
                        ERR_GENERIC("invalid size format");
                        search_status = -1;
                        break;
                    }

                    // query.size is initialized
                    
                    continue;
                }
            }

            if (-1 == search_status) {
                continue;
            }

            if (query.ignore_id * query.ignore_name * query.ignore_size > 0) {
                ERR_GENERIC("invalid query");
                continue;
            }

            query_result_t* results;
            uint32_t results_size;

            // non trackers and trackers use the same search function
            if (CHECK(client_search(&client, &query, &results, &results_size))) {
                ERR(status, "search error");
                continue;
            }

            // no results found
            if (results_size == 0) {
                ERR_GENERIC("error: no results found");
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
                ERR_GENERIC("invalid number of args");
                continue;
            }

            // download a key
            if (strcmp(cmd.args[0].flag, "-k") == 0) {
                key2_t id;

                if (CHECK(key_from_text(&id, cmd.args[0].value))) {
                    ERR(status, "invalid key format");
                    continue;
                }
                
                if (!client.tracker) {
                    if (CHECK(client_download(&client, &id))) {
                        ERR(status, "download error");
                        continue;
                    }                    
                } else {
                    if (CHECK(tracker_download(client.tracker, &id))) {
                        ERR(status, "download error");
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
                if (CHECK(downloader_pause(&client.downloader, atoi(cmd.args[0].value)))) {
                    ERR(status, "cannot pause download");
                }

                continue;
            }
        }

        if (strcmp(cmd.name, "upload") == 0) {
            if (!client.tracker) {
                ERR_GENERIC("you must enable the tracker");
                continue;
            }

            if (cmd.args_size != 1) {
                ERR_GENERIC("invalid number of args");
                continue;
            }

            // upload file by path
            if (strcmp(cmd.args[0].flag, "-p") == 0) {
                if (CHECK(tracker_upload(client.tracker, cmd.args[0].value, client.bootstrap_fd))) {
                    ERR(status, "upload error");
                    continue;
                }

                continue;
            }
        }

        // regular client commands
        if (strcmp(cmd.name, "help") == 0) {
            if (cmd.args_size != 0) {
                ERR_GENERIC("invalid number of args");
                continue;
            }

            print_help();
            
            continue;
        }

        if (strcmp(cmd.name, "clear") == 0) {
            if (cmd.args_size != 0) {
                ERR_GENERIC("invalid number of args");
                continue;
            }
            
            system("clear");
            continue;
        }

        if (strcmp(cmd.name, "quit") == 0) {
            if (cmd.args_size != 0) {
                ERR_GENERIC("invalid number of args");
                continue;
            }
            
            break;
        }

        ERR_GENERIC("invalid command");
    }

    if (CHECK(client_cleanup(&client))) {
        ERR(status, "cannot stop client\n");
        exit(-1);
    }

    exit(0);
}

int32_t client_init(client_t *client) {
    int32_t status = 0;

    if (CHECK(client_init_bootstrap_server_connection(client))) {
        print(LOG_ERROR, "[client_init] Error at client_init_bootstrap_server_connection\n");
        return status;
    }

    client->tracker = NULL; // client is not a tracker yet

    // init downloader module
    if (CHECK(downloader_init(&client->downloader))) {
        print(LOG_ERROR, "[client_init] Error at downloader_init\n");
        return status;
    }

    return status;
}

int32_t client_init_bootstrap_server_connection(client_t *client) {
    int32_t status = 0;

    // connect to bootstrap server address
    client->bootstrap_addr.sin_family = AF_INET;
    client->bootstrap_addr.sin_addr.s_addr = BOOTSTRAP_SERVER_IP;
    client->bootstrap_addr.sin_port = htons(BOOTSTRAP_SERVER_PORT);

    if (CHECK(client->bootstrap_fd = get_client_socket(&client->bootstrap_addr))) {
        print(LOG_ERROR, "[client_init_bootstrap_server_connection] Error at get_client_socket\n");
        return status;
    }

    return status;
}

int32_t client_cleanup(client_t *client) {
    int32_t status = 0;

    // shutdown downloader
    downloader_cleanup(&client->downloader);
    
    if (client->tracker) {
        if (CHECK(client_stop_tracker(client))) {
            print(LOG_ERROR, "[client_cleanup] Error at client_stop_tracker\n");
            return status;
        }
    }

    char *msg;
    uint32_t msg_size;

    // notify server that we want to disconnect
    if (CHECK(send_and_recv(client->bootstrap_fd, DISCONNECT, NULL, 0, &msg, &msg_size))) {
        // if we get here, it's bad
        print(LOG_ERROR, "[client_cleanup] Error at send_and_recv, disconnecting anyway\n");
        return status;
    }

    free(msg);

    shutdown(client->bootstrap_fd, SHUT_RDWR);
    close(client->bootstrap_fd);

    return status;
}

int32_t client_start_tracker(client_t *client, const char *tracker_ip, const char *tracker_port) {
    int32_t status = 0;

    // create DEFAULT_SAVE_LOCATION if it doesn't exist
    if (CHECK(access(DEFAULT_SAVE_LOCATION, F_OK))) {
        if (CHECK(mkdir(DEFAULT_SAVE_LOCATION, 0755))) {
            print(LOG_ERROR, "[client_start_tracker] Cannot create `%s` folder\n", DEFAULT_SAVE_LOCATION);
            return status;
        }
    }

    client->tracker = (tracker_t*)malloc(sizeof(tracker_t));

    if (CHECK(tracker_init(client->tracker, tracker_ip, tracker_port, client->bootstrap_fd))) {
        print(LOG_ERROR, "[client_start_tracker] Cannot start tracker\n");
        return status;
    }

    client->tracker->downloader = &client->downloader;

    return status;
}

int32_t client_stop_tracker(client_t *client) {
    int32_t status = 0;

    char *msg;
    uint32_t msg_size;

    // notify server that we want to disconnect tracker
    if (CHECK(send_and_recv(client->bootstrap_fd, DISCONNECT_TRACKER, &client->tracker->node.id, sizeof(key2_t), &msg, &msg_size))) {
        // if we get here, it's bad
        print(LOG_ERROR, "[client_stop_tracker] Error at send_and_recv, disconnecting anyway\n");
        status = 0;
    }

    free(msg);

    if (CHECK(tracker_cleanup(client->tracker))) {
        print(LOG_ERROR, "[client_stop_tracker] Error at tracker_cleanup\n");
        return status;
    }

    free(client->tracker);
    client->tracker = NULL;

    return status;
}

int32_t client_search(client_t *client, query_t *query, query_result_t** results, uint32_t *results_size) {
    int32_t status = 0;

    if (CHECK(send_and_recv(client->bootstrap_fd, SEARCH, query, sizeof(query_t), (char**)results, results_size))) {
        print(LOG_ERROR, "[tracker_search] Error at send_and_recv\n");
        free(results);
        return status;
    }

    return status;
}

int32_t client_download(client_t *client, key2_t *id) {
    int32_t status = 0;

    char *msg;
    uint32_t msg_size;

    // we know for certain the node exists, ask it if the key id has a value
    if (CHECK(send_and_recv(client->bootstrap_fd, SEARCH_PEER, id, sizeof(key2_t), &msg, &msg_size))) {
        print(LOG_ERROR, "[client_download] Error at send_and_recv\n");
        free(msg);
        return status;
    }

    if (msg == NULL) {
        print(LOG_ERROR, "[client_download] File not found\n");
        // no need to free
        status = ERR_FILE_NOT_FOUND;
        return status;
    }

    file_t file;
    deserialize_file(&file, msg, msg_size);

    if (CHECK(downloader_add(&client->downloader, &file))) {
        print(LOG_ERROR, "[client_download] Error at downloader_add\n");
        return status;
    }

    free(msg);

    return status;
}
