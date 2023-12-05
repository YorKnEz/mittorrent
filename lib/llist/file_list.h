#ifndef FILE_LIST_H
#define FILE_LIST_H

#include "file.h"
#include "key.h"
#include "logger.h"

// linked list of local files
typedef struct file_node_s file_node_t;

typedef struct file_node_s {
    file_t file;
    file_node_t *next;
} file_node_t;

typedef file_node_t* file_list_t;

// adds a file to the list
void file_list_add(file_list_t *list, file_t *file);

// removes the file with the specified id from the list
void file_list_remove(file_list_t *list, key2_t *id);

// free the list
void file_list_free(file_list_t *list);

// print the contents of the list
void print_file_list(log_t log_type, file_list_t *list);

#endif