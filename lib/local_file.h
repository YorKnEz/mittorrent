#ifndef LOCAL_FILE_H
#define LOCAL_FILE_H

#include <stdlib.h>

#include "key.h"
#include "logger.h"
#include "file.h"

// as described in the download protocol, a file is composed of blocks
// each local file has a list of blocks that it has downloaded already or not
// this allows downloading from multiple peers even if the peers don't have
// the full file yet
#define FILE_BLOCK_SIZE 4096

// metadata about a block of data
// the max size of a block is FILE_BLOCK_SIZE
// the id identifies the file to which the block belongs
// but it can be lower
// the index represents at which point in the file this goes
typedef struct {
    key2_t id;
    uint32_t index;
} block_t;

// we need a structure that links the original file and the torrent file
typedef struct {
    key2_t id;                      // key of the torrent file
    char path[512];                 // path of the original file
    uint32_t size;                  // size of the original file in bytes
} local_file_t;

// create a local file from a torrent file
// the path given is assumed to be valid
void local_file_from_file(local_file_t *local_file, file_t *file, const char *path);

// pretty print file contents
int32_t print_local_file(log_t log_type, local_file_t *file);

#endif