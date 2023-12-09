#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

/*
PROTOCOL REQUESTS
1. Boostrap server:
    DISCONNECT                      - completely disconnect from bootstrap server
    CONNECT_TRACKER                 - user starts tracker
    DISCONNECT_TRACKER              - user stops tracker
    SEARCH                          - search files matching a query on server
    UPLOAD                          - indexes the file for searching by others

2. Chord protocol
    PING                            - asks node if it's alive
    FIND_NEXT                       -
    FIND_PREV                       -
    SET_PREV                        -
    FIND_CLOSEST_PRECEDING_FINGER   -
    UPDATE_FINGER_TABLE             - deprecated
    NOTIFY                          -

3. Download protocol
    SEARCH                          - receives a torrent file from a peer
    DOWNLOAD                        - init download process
    BLOCK                           - request a block from a peer

4. Tracker protocol
    ADD_PEER                        - tell a tracker to add myself as a peer for the torrent file
    MOVE_DATA                       - tell a tracker that it needs to transfer keys
    UPLOAD                          - adds the file to it's local file list
    SHUTDOWN                        - shut down a client thread
*/

typedef enum { 
    DISCONNECT,
    CONNECT_TRACKER,
    DISCONNECT_TRACKER,
    SEARCH,
    UPLOAD,
    SHUTDOWN,
    PING,
    FIND_NEXT,
    FIND_PREV,
    SET_PREV,
    FIND_CLOSEST_PRECEDING_FINGER,
    /* UPDATE_FINGER_TABLE, */ 
    NOTIFY,
    MOVE_DATA,
    DOWNLOAD,
    BLOCKS,
    BLOCK,
    ADD_PEER,
} req_type_t;

typedef struct {
    req_type_t type;
    uint32_t size;
} req_header_t;

typedef enum {
    SUCCESS,    // request handled correctly
    ERROR       // something went wrong
} res_type_t;

typedef struct {
    res_type_t type;
    uint32_t size;
} res_header_t;

// socket creation
// create a socket connected to specified addr
int32_t get_client_socket(struct sockaddr_in *addr);

// create a socket that is bound to the specified addr
int32_t get_server_socket(struct sockaddr_in *addr);


// read and write don't write all bytes in one call in all cases, msg is assumed to be allocated for both cases
int32_t read_full(int32_t socket_fd, void *buf, uint32_t buf_size);
int32_t write_full(int32_t socket_fd, void *buf, uint32_t buf_size);

int32_t send_req(int32_t socket_fd, req_type_t type, void* msg, uint32_t msg_size);
int32_t recv_req(int32_t socket_fd, req_header_t *reqh, char **msg, uint32_t *msg_size);

int32_t send_res(int32_t socket_fd, res_type_t type, void* msg, uint32_t msg_size);
int32_t recv_res(int32_t socket_fd, res_header_t *resh, char **msg, uint32_t *msg_size);

int32_t send_and_recv(int32_t socket_fd, req_type_t type, void* req, uint32_t req_size, char **res, uint32_t *res_size);

// debug

// pretty print addresses
void print_addr(log_t log_type, struct sockaddr_in *addr);

#endif