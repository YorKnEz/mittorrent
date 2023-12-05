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

// pretty print file contents
int32_t print_local_file(log_t log_type, local_file_t *file);


// linked list functions

#endif