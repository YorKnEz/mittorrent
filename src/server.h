#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <sys/select.h>

#include "common.h"
#include "dht.h"

#define THREAD_POOL_SIZE 2
#define INVALID_COMMAND "Invalid command"

typedef struct {
    pthread_mutex_t lock;   // struct lock

    int32_t fd;
    struct sockaddr_in addr;

    pthread_mutex_t mlock;
    pthread_t tid[THREAD_POOL_SIZE];

    list_t clients;
    node_t *client_read; // read cursor to "uniformly" distribute peers to new clients

    file_list_t uploads;
} server_t;

void server_init(server_t *server);

// the server thread uses select to handle requests
// this is done in order to keep alive the connections with the clients
void server_thread(server_t *server);

void server_cleanup(server_t *server);

#endif