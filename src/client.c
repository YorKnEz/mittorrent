#include "client.h"
#include "cmd_parser.h"
#include "error.h"

client_t client;


int32_t main(int32_t argc, char **argv) {
    int32_t status = 0;
    int32_t running = 1;

    if (argc < 3) {
        ERR_GENERIC("usage: ./client TRACKER_IP TRACKER_PORT");
        exit(-1);
    }

    if (CHECK(client_init(&client))) {
        ERR(status, "cannot initialize client");
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
        cmds_get_cmd_type(&client.cmds, &cmd, &cmd_type);

        if (cmd_type == CMD_ERROR) {
            continue;
        }

        // 3. execute the command
        switch (cmd_type) {
        case CMD_TRACKER_START: {
            if (client.tracker) {
                ERR_GENERIC("tracker already started");
                continue;
            }

            if (CHECK(client_start_tracker(&client, argv[1], argv[2]))) {
                ERR(status, "cannot start tracker");
                continue;
            }
            break;
        }
        case CMD_TRACKER_STOP: {
            if (!client.tracker) {
                ERR_GENERIC("you must enable the tracker");
                continue;
            }

            if (CHECK(client_stop_tracker(&client))) {
                ERR(status, "cannot stop tracker");
                continue;
            }
            break;
        }
        case CMD_TRACKER_STABILIZE: {
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

            break;
        }
        case CMD_TRACKER_STATE: {
            if (!client.tracker) {
                ERR_GENERIC("you must enable the tracker");
                continue;
            }

            tracker_state(client.tracker);

            break;
        }
        case CMD_TRACKER_HELP: {
            print_cmd_help(LOG, &client.cmds.list[0]);
            break;
        }
        case CMD_SEARCH: {
            // TODO: check flags with short or long names
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

            query_result_t *results;
            uint32_t results_size;

            // non trackers and trackers use the same search function
            if (CHECK(
                    client_search(&client, &query, &results, &results_size))) {
                ERR(status, "search error");
                continue;
            }

            // no results found
            if (results_size == 0) {
                ERR_GENERIC("no results found");
                continue;
            }

            results_size = results_size / sizeof(query_result_t);

            // see the keys
            for (uint32_t i = 0; i < results_size; i++) {
                print(LOG, "%d. ", i + 1);
                print_result(LOG, &results[i]);
            }

            free(results);

            break;
        }
        case CMD_SEARCH_HELP: {
            print_cmd_help(LOG, &client.cmds.list[1]);
            break;
        }
        case CMD_DOWNLOAD: {
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

            break;
        }
        case CMD_DOWNLOAD_LIST: {
            print_downloader(LOG, &client.downloader);
            break;
        }
        case CMD_DOWNLOAD_PAUSE: {
            if (CHECK(downloader_pause(&client.downloader,
                                       atoi(cmd.args[0].value)))) {
                ERR(status, "cannot pause download");
            }

            break;
        }
        case CMD_DOWNLOAD_HELP: {
            print_cmd_help(LOG, &client.cmds.list[2]);
            break;
        }
        case CMD_UPLOAD: {
            if (CHECK(tracker_upload(client.tracker, cmd.args[0].value,
                                     &client.bootstrap_addr))) {
                ERR(status, "upload error");
                continue;
            }

            break;
        }
        case CMD_UPLOAD_HELP: {
            print_cmd_help(LOG, &client.cmds.list[3]);
            break;
        }
        case CMD_HELP: {
            print_cmds_help(LOG, &client.cmds);
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
        default: {
            ERR_GENERIC("unknown command, see help");
            break;
        }
        }
    }

    if (CHECK(client_cleanup(&client))) {
        ERR(status, "cannot stop client");
        exit(-1);
    }

    exit(0);
}

int32_t client_init(client_t *client) {
    int32_t status = 0;

    if (CHECK(client_init_bootstrap_server_connection(client))) {
        print(
            LOG_ERROR,
            "[client_init] Error at client_init_bootstrap_server_connection\n");
        return status;
    }

    client->tracker = NULL; // client is not a tracker yet

    // init downloader module
    if (CHECK(downloader_init(&client->downloader))) {
        print(LOG_ERROR, "[client_init] Error at downloader_init\n");
        return status;
    }

    cmds_t cmds = {
        "This is the torrent client of the project",
        "The list of commands is described below",
        7,
        {
            {
                CMD_UNKNOWN,
                "tracker",
                "Manage tracker operations.",
                5,
                {
                    {CMD_TRACKER_HELP, 1, "-h", "--help", NULL,
                     "Print this message."},
                    {CMD_TRACKER_START, 1, "-s", "--start", NULL,
                     "Start the tracker in order to be able to upload and seed "
                     "for "
                     "others"},
                    {CMD_TRACKER_STOP, 1, "-t", "--stop", NULL,
                     "Stop the tracker"},
                    {CMD_TRACKER_STATE, 1, "-l", "--state", NULL,
                     "List the tracker state"},
                    {CMD_TRACKER_STABILIZE, 1, "-r", "--stabilize", NULL,
                     "Stabilize the state of the tracker"},
                },
            },
            {
                CMD_SEARCH,
                "search",
                "Search for a file on the network.",
                4,
                {
                    {CMD_SEARCH_HELP, 1, "-h", "--help", NULL,
                     "Print this message."},
                    {CMD_SEARCH, 0, "-i", "--id", "FILE_ID",
                     "Search by id of the file"},
                    {CMD_SEARCH, 0, "-n", "--name", "FILE_NAME",
                     "Search all files that include " ANSI_COLOR_GREEN
                     "FILE_NAME" ANSI_COLOR_RESET " in their name"},
                    {CMD_SEARCH, 0, "-s", "--size", "FILE_SIZE",
                     "Search by size of the file"},
                },
            },
            {
                CMD_UNKNOWN,
                "download",
                "Manage downloader operations.",
                4,
                {
                    {CMD_DOWNLOAD_HELP, 1, "-h", "--help", NULL,
                     "Print this message."},
                    {CMD_DOWNLOAD, 1, "-i", "--id", "FILE_ID",
                     "Add the file with given id to the downloads list. If the "
                     "id "
                     "is "
                     "already in the download list, the download is "
                     "restarted."},
                    {CMD_DOWNLOAD_LIST, 1, "-l", "--list", NULL,
                     "List all of the downloads."},
                    {CMD_DOWNLOAD_PAUSE, 1, "-p", "--pause", "INDEX",
                     "Pause the download with the given index. The index is "
                     "taken "
                     "out "
                     "of the downloads list."},
                },
            },
            {
                CMD_UNKNOWN,
                "upload",
                "Upload a file to the network.",
                2,
                {
                    {CMD_UPLOAD_HELP, 1, "-h", "--help", NULL,
                     "Print this message."},
                    {CMD_UPLOAD, 1, "-p", "--path", "PATH",
                     "The path of the file to upload on the network."},
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

    memcpy(&client->cmds, &cmds, sizeof(cmds_t));

    return status;
}

int32_t client_init_bootstrap_server_connection(client_t *client) {
    int32_t status = 0;

    // connect to bootstrap server address
    client->bootstrap_addr.sin_family = AF_INET;
    client->bootstrap_addr.sin_addr.s_addr = BOOTSTRAP_SERVER_IP;
    client->bootstrap_addr.sin_port = htons(BOOTSTRAP_SERVER_PORT);

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

    return status;
}

int32_t client_start_tracker(client_t *client, const char *tracker_ip,
                             const char *tracker_port) {
    int32_t status = 0;

    // create DEFAULT_SAVE_LOCATION if it doesn't exist
    if (CHECK(access(DEFAULT_SAVE_LOCATION, F_OK))) {
        if (CHECK(mkdir(DEFAULT_SAVE_LOCATION, 0755))) {
            print(LOG_ERROR,
                  "[client_start_tracker] Cannot create `%s` folder\n",
                  DEFAULT_SAVE_LOCATION);
            return status;
        }
    }

    client->tracker = (tracker_t *)malloc(sizeof(tracker_t));

    if (CHECK(tracker_init(client->tracker, tracker_ip, tracker_port,
                           &client->bootstrap_addr))) {
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
    if (CHECK(request(&client->bootstrap_addr, DISCONNECT_TRACKER,
                      &client->tracker->node.id, sizeof(key2_t), &msg,
                      &msg_size))) {
        // if we get here, it's bad
        print(LOG_ERROR,
              "[client_stop_tracker] Error at request, disconnecting anyway\n");
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

int32_t client_search(client_t *client, query_t *query,
                      query_result_t **results, uint32_t *results_size) {
    int32_t status = 0;

    if (CHECK(request(&client->bootstrap_addr, SEARCH, query, sizeof(query_t),
                      (char **)results, results_size))) {
        print(LOG_ERROR, "[client_search] Error at request\n");
        free(*results);
        return status;
    }

    return status;
}

int32_t client_download(client_t *client, key2_t *id) {
    int32_t status = 0;

    char *msg;
    uint32_t msg_size;

    // we know for certain the node exists, ask it if the key id has a value
    if (CHECK(request(&client->bootstrap_addr, SEARCH_PEER, id, sizeof(key2_t),
                      &msg, &msg_size))) {
        print(LOG_ERROR, "[client_download] Error at request\n");
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
