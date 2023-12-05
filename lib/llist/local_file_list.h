#ifndef LOCAL_FILE_LIST_H
#define LOCAL_FILE_LIST_H

#include "local_file.h"
#include "key.h"
#include "logger.h"

// linked list of local files
typedef struct local_file_node_s local_file_node_t;

typedef struct local_file_node_s {
    local_file_t file;
    local_file_node_t *next;
} local_file_node_t;

typedef local_file_node_t* local_file_list_t;

// adds a file to the list
void local_file_list_add(local_file_list_t *list, local_file_t *file);

// removes the file with the specified id from the list
void local_file_list_remove(local_file_list_t *list, key2_t *id);

// free the list
void local_file_list_free(local_file_list_t *list);

// find file path by id
int32_t local_file_list_find(local_file_list_t *list, key2_t *id, char path[512]);

// print the contents of the list
void print_local_file_list(log_t log_type, local_file_list_t *list);

#endif