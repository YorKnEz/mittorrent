#ifndef LOCAL_FILE_H
#define LOCAL_FILE_H

#include <stdlib.h>

#include "key.h"
#include "logger.h"

// we need a structure that links the original file and the torrent file
typedef struct {
    key2_t id;                      // key of the torrent file
    char path[512];                 // path of the original file
} local_file_t;

// linked list of local files
typedef struct local_file_node_s local_file_node_t;

typedef struct local_file_node_s {
    local_file_t file;
    local_file_node_t *next;
} local_file_node_t;

typedef local_file_node_t* local_file_list_t;

// pretty print file contents
int32_t print_local_file(local_file_t *file);


// linked list functions

// adds a file to the list
void local_file_list_add(local_file_list_t *list, local_file_t *file);

// removes the file with the specified id from the list
void local_file_list_remove(local_file_list_t *list, key2_t *id);

// free the list
void local_file_list_free(local_file_list_t *list);

// find file path by id
int32_t local_file_list_find(local_file_list_t *list, key2_t *id, char path[512]);

// print the contents of the list
void print_local_file_list(local_file_list_t *list);

#endif