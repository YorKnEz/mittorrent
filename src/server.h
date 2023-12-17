#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <sys/select.h>

#include "common.h"
#include "dht.h"
#include "tracker.h"

#include "file_list.h"
#include "node_list.h"

#define INVALID_COMMAND "Invalid command"
#define THREAD_POOL_SIZE 2

typedef struct {
    // used by threads to know when to stop
    int32_t running;
    pthread_mutex_t running_lock;

    pthread_mutex_t lock;   // struct lock

    int32_t fd;
    struct sockaddr_in addr;

    pthread_mutex_t mlock;
    pthread_t tid[THREAD_POOL_SIZE];

    list_t clients;
    node_t *client_read;    // read cursor to "uniformly" distribute peers to new clients

    file_list_t uploads;

    cmds_t cmds;            // command parser module
} server_t;
// the server thread uses select to handle requests
// this is done in order to keep alive the connections with the clients
void server_thread(server_t *server);

int32_t server_init(server_t *server);
int32_t server_cleanup(server_t *server);

int32_t server_start(server_t *server);
int32_t server_stop(server_t *server);

int32_t server_list_clients(server_t *server);
int32_t server_list_uploads(server_t *server);

#endif
