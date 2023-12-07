#ifndef FILE_H
#define FILE_H

#include <unistd.h>
#include <fcntl.h>

#include "logger.h"
#include "key.h"
#include "dht.h"
#include "node_list.h"

#define MAGIC "HART\0\0\0\0"
#define EXT_NAME "torrent"
// by default, all files should be saved here, but client may load .torrents from other sources as well
#define DEFAULT_SAVE_LOCATION "./torrent/"

// the data stored in a key, this is the torrent file
// the naming convention for torrent files is name_id.torrent
typedef struct {
    char magic[8];                  // will be set to HART\0\0\0\0
    key2_t id;                      // the key
    char name[512];                 // name of the original file
    char path[512];                 // path of the torrent file
    uint64_t size;                  // size of the original file in bytes
    list_t peers;                   // list of peers that own this file
} file_t;

// takes a regular file path as input and makes a torrent file out of it
int32_t create_file(file_t *file, const char *path, node_remote_t *initial_peer);

// saves a torrent file to disk
// path is the location where to save the file
// if path is `/home/test/`, file should be found as `/home/test/name_id.ext`
// torrent_path will store the path to the file, this buffer is assumed to be allocated (TODO: maybe don't assume?) with 512 bytes
int32_t save_file(file_t *file, const char *path);

// loads a torrent file from disk
// path is the location where the file is located `home/test/name_id.ext`
int32_t load_file(file_t *file);

// delete a torrent file from disk
// path is the location where the file is located `home/test/name_id.ext`
// this is required in case the file must be moved to another peer
int32_t delete_file(file_t *file);

// pretty print file contents
int32_t print_file(log_t log_type, file_t *file);

// serialize file contents in order to be written on the network
void serialize_file(file_t *file, char **msg, uint32_t *msg_size);

// deserialize file contents in order to be read from the network
void deserialize_file(file_t *file, char *msg, uint32_t msg_size);

// add a peer to the file's peer list
void add_peer(file_t *file, node_remote_t *peer);

// remove a peer from the file's peer list
void remove_peer(file_t *file, node_remote_t *peer);

#endif