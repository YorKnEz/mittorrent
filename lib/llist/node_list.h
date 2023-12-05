#ifndef NODE_LIST_H
#define NODE_LIST_H

#include "dht.h"
#include "key.h"
#include "logger.h"

// linked list for servers to keep track of clients
typedef struct node_s node_t;

typedef struct node_s {
    node_remote_t node;
    node_t *next;
} node_t;

typedef node_t* list_t;

// adds node to the list
void list_add(list_t *list, node_remote_t *node);

// removes the node with the specified id from the list
void list_remove(list_t *list, key2_t *id);

// free nodes in the list
void list_free(list_t *list);

// print the contents of the list
void print_list(log_t log_type, list_t *list);

#endif