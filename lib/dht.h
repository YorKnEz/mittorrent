#ifndef DHT_H
#define DHT_H

#include <stdlib.h>

#include "key.h"
#include "logger.h"
#include "network.h"
#include "error.h"

// data of a remote node
typedef struct {
    key2_t id;                      // key of peer
    struct sockaddr_in addr;        // connect info of node
} node_remote_t;

// finger table structure as documented by Chord
typedef struct {
    int32_t initialized;            // specifies if the entry has been inited or not
    key2_t start;                   // (node.id + 2^(i-1)) mod 2^m
    node_remote_t node;             // can be either local or remote
} finger_t;

// data of the local node
// first two fields match the structure of node_remote_t
// this is because a node might have to return itself on the network
typedef struct {
    key2_t id;                      // key of peer
    struct sockaddr_in addr;
    finger_t finger[KEY_BITS];
    int32_t prev_initialized;       // can be set to nil
    node_remote_t prev;             // can be only remote
} node_local_t;

// the following functions have two variants: local and remote

// used for remote procedure calls
int32_t node_req(node_remote_t *node, req_type_t type, void *req, uint32_t req_size, char **res, uint32_t *res_size);

// ask node to find the successor of id
int32_t node_find_next(node_local_t *node, key2_t *id, node_remote_t *res);
int32_t node_find_next_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res);

// ask node to find the predecessor of id
int32_t node_find_prev(node_local_t *node, key2_t *id, node_remote_t *res);
int32_t node_find_prev_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res);

// set the prev field of node
int32_t node_set_prev(node_local_t *node, node_remote_t *new_prev);
int32_t node_set_prev_remote(node_local_t *node, node_remote_t *fwd_node, node_remote_t *new_prev);

// find closest finger of node preceding id
int32_t node_find_closest_preceding(node_local_t *node, key2_t *id, node_remote_t *res);
int32_t node_find_closest_preceding_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res);

// node joined the network
// peer is an arbitrary node in the network
int32_t node_join(node_local_t *node, node_remote_t *peer);

// periodically verify n's immediate successor, and tell the successor about n
int32_t node_stabilize(node_local_t *node);

// peer thinks it might be our predecessor
int32_t node_notify(node_local_t *node, node_remote_t *peer);
int32_t node_notify_remote(node_local_t *node, node_remote_t *fwd_node, node_remote_t *peer);

// periodically refresh finger table entries
int32_t node_fix_fingers(node_local_t *node);

// pretty print node_remote_t
void print_remote_node(log_t log_type, node_remote_t *node);

#endif
