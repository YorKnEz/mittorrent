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

typedef enum { 
    // bootstrap server request types
    DISCONNECT,                     // completely disconnect from bootstrap server
                                    // connect not needed
    CONNECT_TRACKER,                // user starts tracker
    DISCONNECT_TRACKER,             // user stops tracker
    SEARCH,                         // search files matching a query on server
    DOWNLOAD,                       //
    UPLOAD,                         // client: adds the file to it's local file list
                                    // server: indexes the file for searching by others
    SHUTDOWN,                       // client: shut down a client thread
    // Chord protocol
    PING,                           // asks node if it's alive
    FIND_NEXT,                      //
    FIND_PREV,                      //
    SET_PREV,                       //
    FIND_CLOSEST_PRECEDING_FINGER,  //
    /* UPDATE_FINGER_TABLE, */ 
    NOTIFY,                         //
    MOVE_DATA,                      // tell a tracker that it needs to transfer keys
} req_type_t;

typedef struct {
    req_type_t type;
    uint32_t size;
} req_header_t;

typedef enum {
    SUCCESS,                        // request handled correctly
    ERROR                           // something went wrong
} res_type_t;

typedef struct {
    res_type_t type;
    uint32_t size;
} res_header_t;

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