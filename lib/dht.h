#ifndef DHT_H
#define DHT_H

#include <stdlib.h>

#include "key.h"
#include "logger.h"
#include "network.h"

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

// linked list for servers to keep track of clients
typedef struct node_s node_t;

typedef struct node_s {
    node_remote_t node;
    node_t *next;
} node_t;

typedef node_t* list_t;


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


// join that doesn't support concurrency

/*
// node joined the network
// peer is an arbitrary node in the network
int32_t node_join(node_local_t *node, node_remote_t *peer);

// initialize the finger table of local node
// peer is an arbitrary node in the network
int32_t node_init_finger_table(node_local_t *node, node_remote_t *peer);

// update all nodes whose finger tables should refer to node
int32_t node_update_others(node_local_t *node);

// if s is ith finger of n, update n's finger table with s
int32_t node_update_finger_table(node_local_t *node, node_remote_t *peer, int32_t i);
int32_t node_update_finger_table_remote(node_remote_t *node, node_remote_t *peer, int32_t i);
*/

// join that supports concurrency

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


// linked list functions

// adds node to the list
void list_add(list_t *list, node_remote_t *node);

// removes the node with the specified id from the list
void list_remove(list_t *list, key2_t *id);

// free nodes in the list
void list_free(list_t *list);


// useful debug stuff

// pretty print node_remote_t
void print_remote_node(node_remote_t *node);

// print the contents of the list
void print_list(list_t *list);

#endif