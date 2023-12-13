#include "file.h"

int32_t create_file(file_t *file, const char *path, node_remote_t *initial_peer) {
    int32_t status = 0;

    // open the file
    int32_t fd;

    if (CHECK(fd = open(path, O_RDONLY, 0))) {
        print(LOG_ERROR, "[create_file] Cannot open file\n");
        status = ERR_INVALID_PATH;
        return ERR_INVALID_PATH;
    }
    
    // init magic bytes
    memcpy(file->magic, MAGIC, 8);

    // get file id and size
    if (CHECK(key_from_file(&file->id, fd, &file->size))) {
        print(LOG_ERROR, "[create_file] Error at key_from_file\n");
        return status;
    }

    // extract name from path
    // we assume the file contains at least a backslash
    memset(file->name, 0, 512);
    // TODO: extract the filename properly
    strcpy(file->name, strrchr(path, '/') + 1);

    memset(file->path, 0, 512);

    // add the first peer to the list
    file->peers = NULL;
    list_add(&file->peers, initial_peer);

    close(fd);

    return status;
}

int32_t save_file(file_t *file, const char *path) {
    int32_t status = 0;

    // get fullname of file as it will be saved on disk
    uint32_t torrent_path_size = strlen(path) + strlen(file->name) + 1 + 2 * sizeof(key2_t) + 1 + strlen(EXT_NAME);

    if (torrent_path_size >= 512) {
        print(LOG_ERROR, "[save_file] File path is too large");
        status = ERR_PATH_TOO_LARGE;
        return status;
    }
    
    strcpy(file->path, path);
    strcat(file->path, file->name);
    strcat(file->path, "_");
    for (int32_t i = 0; i < sizeof(key2_t); i++) {
        sprintf(file->path + strlen(path) + strlen(file->name) + 1 + 2 * i, "%02x", file->id.key[i]);
    }
    strcat(file->path, ".");
    strcat(file->path, EXT_NAME);

    int32_t fd;
    if (CHECK(fd = open(file->path, O_CREAT | O_TRUNC | O_RDWR, 0666))) {
        print(LOG_ERROR, "[save_file] Error at creating out file\n");
        return status;
    }

    if (CHECK(ftruncate(fd, sizeof(file_t)))) {
        print(LOG_ERROR, "[save_file] Error at truncating file\n");
        close(fd);
        return status;
    }

    // save magic bytes to disk
    if (CHECK(write(fd, file->magic, 8))) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return status;
    }

    // save file id to disk
    if (CHECK(write(fd, &file->id, sizeof(key2_t)))) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return status;
    }

    // save file name to disk
    if (CHECK(write(fd, file->name, 512))) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return status;
    }

    // save file path to disk
    if (CHECK(write(fd, file->path, 512))) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return status;
    }

    // save file size to disk
    if (CHECK(write(fd, &file->size, sizeof(uint64_t)))) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return status;
    }

    // save peers list to disk
    node_t *p = file->peers;

    while (p) {
        if (CHECK(write(fd, &p->node, sizeof(node_remote_t)))) {
            print(LOG_ERROR, "[save_file] Error at write\n");
            close(fd);
            return status;
        }

        p = p->next;
    }

    close(fd);

    return status;
}

int32_t load_file(file_t *file) {
    int32_t status = 0;

    int32_t fd;
    if (CHECK(fd = open(file->path, O_RDONLY, 0))) {
        print(LOG_ERROR, "[load_file] Error at opening file\n");
        status = ERR_CANNOT_OPEN;
        return status;
    }

    // load magic bytes from disk
    if (CHECK(read(fd, file->magic, 8))) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return status;
    }

    if (strcmp(MAGIC, file->magic)) {
        print(LOG_ERROR, "[load_file] Invalid file\n");
        close(fd);
        status = ERR_INVALID_FILE;
        return status;
    }

    // load file id from disk
    if (CHECK(read(fd, &file->id, sizeof(key2_t)))) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return status;
    }

    // load file name from disk
    if (CHECK(read(fd, file->name, 512))) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return status;
    }

    // load file name from disk
    if (CHECK(read(fd, file->path, 512))) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return status;
    }

    // load file size from disk
    if (CHECK(read(fd, &file->size, sizeof(uint64_t)))) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return status;
    }

    // load peers list from disk
    node_remote_t peer;
    int32_t read_bytes;

    while ((read_bytes = read(fd, &peer, sizeof(node_remote_t)))) {
        if (CHECK(read_bytes)) {
            print(LOG_ERROR, "[load_file] Error at read\n");
            close(fd);
            return status;
        }

        add_peer(file, &peer);
    }

    close(fd);
    
    return status;
}

int32_t delete_file(file_t *file) {
    int32_t status = 0;

    int32_t fd;
    if (CHECK(fd = open(file->path, O_RDONLY, 0))) {
        print(LOG_ERROR, "[load_file] Error at opening file\n");
        status = ERR_CANNOT_OPEN;
        return status;
    }

    if (strcmp(MAGIC, file->magic)) {
        print(LOG_ERROR, "[load_file] Invalid file\n");
        status = ERR_INVALID_FILE;
        return status;
    }

    close(fd);
    
    remove(file->path);

    return status;
}

int32_t print_file(log_t log_type, file_t *file) {
    int32_t status = 0;

    if (strcmp(MAGIC, file->magic)) {
        print(LOG_ERROR, "[load_file] Invalid file\n");
        status = ERR_INVALID_FILE;
        return status;
    }

    print(log_type, "id: ");
    print_key(log_type, &file->id);
    print(log_type, "\nname: %s\n", file->name);
    print(log_type, "path: %s\n", file->path);
    print(log_type, "size: %llu\n", file->size);
    print(log_type, "peers:\n");
    print_list(log_type, &file->peers);

    return status;
}

// serialize file contents in order to be written on the network
void serialize_file(file_t *file, char **msg, uint32_t *msg_size) {
    // get linked list size
    uint32_t peers_size = 0;
    node_t *peer = file->peers;

    while (peer) {
        peers_size += sizeof(node_remote_t);
        peer = peer->next;
    }

    // allocate a buffer for storing the file content
    uint32_t size_without_list = sizeof(file_t) - sizeof(list_t);
    *msg_size = size_without_list + peers_size;
    *msg = (char*)malloc(*msg_size);

    // copy file contents to the pointer
    memcpy(*msg, file, size_without_list);

    peer = file->peers;
    peers_size = 0;

    // copy peers to the buffer
    while (peer) {
        memcpy(*msg + size_without_list + peers_size, &peer->node, sizeof(node_remote_t));

        peers_size += sizeof(node_remote_t);
        peer = peer->next;
    }
}

// deserialize file contents in order to be read from the network
void deserialize_file(file_t *file, char *msg, uint32_t msg_size) {
    uint32_t size_without_list = sizeof(file_t) - sizeof(list_t);
    memcpy(file, msg, size_without_list);
    file->peers = NULL;

    for (uint32_t i = size_without_list; i < msg_size; i += sizeof(node_remote_t)) {
        list_add(&file->peers, (node_remote_t*)(msg + i));
    }
}

void add_peer(file_t *file, node_remote_t *peer) {
    list_add(&file->peers, peer);
}

void remove_peer(file_t *file, node_remote_t *peer) {
    list_remove(&file->peers, &peer->id);
}
