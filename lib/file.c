#include "file.h"

int32_t create_file(file_t *file, const char *path, node_remote_t *initial_peer) {
    // open the file
    int32_t fd;

    if (-1 == (fd = open(path, O_RDONLY, 0))) {
        print(LOG_ERROR, "[create_file] Cannot open file\n");
        return -1;
    }
    
    // init magic bytes
    memcpy(file->magic, MAGIC, 8);

    // get file id and size
    if (-1 == key_from_file(&file->id, fd, &file->size)) {
        print(LOG_ERROR, "[create_file] Error at key_from_file\n");
        return -1;
    }

    // extract name from path
    // we assume the file contains at least a backslash
    memset(file->name, 0, 512);
    // TODO: extract the filename properly
    strcpy(file->name, strrchr(path, '/') + 1);

    memset(file->path, 0, 512);

    // init list
    file->peers = NULL;
    // TODO: could optimize this but whatever
    // add the first peer to the list
    list_add(&file->peers, initial_peer);

    close(fd);

    return 0;
}

int32_t save_file(file_t *file, const char *path) {
    // get fullname of file as it will be saved on disk
    uint32_t torrent_path_size = strlen(path) + strlen(file->name) + 1 + 2 * KEY_SIZE + 1 + strlen(EXT_NAME);

    if (torrent_path_size >= 512) {
        print(LOG_ERROR, "[save_file] Filename is too large");
        return -1;
    }
    
    strcpy(file->path, path);
    strcat(file->path, file->name);
    strcat(file->path, "_");
    for (int32_t i = 0; i < SHA256_BLOCK_SIZE; i++) {
        sprintf(file->path + strlen(path) + strlen(file->name) + 1 + 2 * i, "%02x", file->id.key[i]);
    }
    strcat(file->path, ".");
    strcat(file->path, EXT_NAME);

    print(LOG_DEBUG, "[save_file] %s\n", file->path);

    int32_t fd;
    if (-1 == (fd = open(file->path, O_CREAT | O_TRUNC | O_RDWR, 0666))) {
        print(LOG_ERROR, "[save_file] Error at creating out file\n");
        return -1;
    }

    if (-1 == ftruncate(fd, sizeof(file_t))) {
        print(LOG_ERROR, "[save_file] Error at truncating file\n");
        close(fd);
        return -1;
    }

    // save magic bytes to disk
    if (-1 == write(fd, file->magic, 8)) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return -1;
    }

    // save file id to disk
    if (-1 == write(fd, &file->id, sizeof(key2_t))) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return -1;
    }

    // save file name to disk
    if (-1 == write(fd, file->name, 512)) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return -1;
    }

    // save file path to disk
    if (-1 == write(fd, file->path, 512)) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return -1;
    }

    // save file size to disk
    if (-1 == write(fd, &file->size, sizeof(uint64_t))) {
        print(LOG_ERROR, "[save_file] Error at write\n");
        close(fd);
        return -1;
    }

    // save peers list to disk
    node_t *p = file->peers;

    while (p) {
        if (-1 == write(fd, &p->node, sizeof(node_remote_t))) {
            print(LOG_ERROR, "[save_file] Error at write\n");
            close(fd);
            return -1;
        }

        p = p->next;
    }

    close(fd);

    return 0;
}

int32_t load_file(file_t *file) {
    int32_t fd;
    if (-1 == (fd = open(file->path, O_RDONLY, 0))) {
        print(LOG_ERROR, "[load_file] Error at opening file\n");
        return -1;
    }

    // load magic bytes from disk
    if (-1 == read(fd, file->magic, 8)) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return -1;
    }

    if (strcmp(MAGIC, file->magic)) {
        print(LOG_ERROR, "[load_file] Invalid file\n");
        close(fd);
        return -1;
    }

    // load file id from disk
    if (-1 == read(fd, &file->id, sizeof(key2_t))) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return -1;
    }

    // load file name from disk
    if (-1 == read(fd, file->name, 512)) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return -1;
    }

    // load file name from disk
    if (-1 == read(fd, file->path, 512)) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return -1;
    }

    // load file size from disk
    if (-1 == read(fd, &file->size, sizeof(uint64_t))) {
        print(LOG_ERROR, "[load_file] Error at read\n");
        close(fd);
        return -1;
    }

    // load peers list from disk
    node_remote_t peer;
    int32_t read_bytes;

    while ((read_bytes = read(fd, &peer, sizeof(node_remote_t)))) {
        if (-1 == read_bytes) {
            print(LOG_ERROR, "[load_file] Error at read\n");
            close(fd);
            return -1;
        }

        add_peer(file, &peer);
    }

    close(fd);
    
    return 0;
}

int32_t delete_file(file_t *file) {
    int32_t fd;
    if (-1 == (fd = open(file->path, O_RDONLY, 0))) {
        print(LOG_ERROR, "[load_file] Error at opening file\n");
        return -1;
    }

    if (strcmp(MAGIC, file->magic)) {
        print(LOG_ERROR, "[load_file] Invalid file\n");
        return -1;
    }

    close(fd);
    
    remove(file->path);

    return 0;
}

int32_t print_file(log_t log_type, file_t *file) {
    if (strcmp(MAGIC, file->magic)) {
        print(LOG_ERROR, "[load_file] Invalid file\n");
        return -1;
    }

    print(log_type, "id: ");
    print_key(log_type, &file->id);
    print(log_type, "\nname: %s\n", file->name);
    print(log_type, "path: %s\n", file->path);
    print(log_type, "size: %llu\n", file->size);
    print(log_type, "peers:\n");
    print_list(log_type, &file->peers);

    return 0;
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

// adds file to the list
void file_list_add(file_list_t *list, file_t *file) {
    file_node_t *new_node = (file_node_t*)malloc(sizeof(file_node_t));

    // memcpy is not safe because of the linked list
    // memcpy(&new_node->file, file, sizeof(file_t));
    // TODO: maybe it's not the most efficient way
    char *msg;
    uint32_t msg_size;
    serialize_file(file, &msg, &msg_size);
    deserialize_file(&new_node->file, msg, msg_size);
    
    // list is empty
    if (*list == NULL) {
        new_node->next = NULL;
        *list = new_node;
        return;
    }

    int32_t res = key_cmp(&new_node->file.id, &(*list)->file.id);

    // add the file so that ids are in ascending order
    if (res < 0) { /* find first file that succeeds new_node */
        new_node->next = (*list);
        *list = new_node;
        return;
    } else if (res == 0) { /* stop if file is already added */
        return;
    }
    
    // previous and current pointers
    file_node_t *p = *list;

    while (p->next) {
        res = key_cmp(&new_node->file.id, &p->next->file.id);
        
        if (res < 0) { /* find first file that succeeds new_node */
            new_node->next = p->next;
            p->next = new_node;
            return;
        } else if (res == 0) { /* stop if file is already added */
            return;
        }

        p = p->next;
    }

    // if we get here, it means the new_node should be added at the end of the list
    new_node->next = NULL;
    p->next = new_node;
}

// removes the node with the specified id from the list
void file_list_remove(file_list_t *list, key2_t *id) {
    file_node_t *p = *list;

    // remove the head
    if (key_cmp(&(*list)->file.id, id) == 0) {
        *list = (*list)->next;
        // delete file from disk if it exists
        remove(p->file.path);
        free(p);
        return;
    }
    
    while (p->next) {
        if (key_cmp(&p->next->file.id, id) == 0) {
            file_node_t *tmp = p->next;
            p->next = p->next->next;
            // delete file from disk if it exists
            remove(tmp->file.path);
            free(tmp);
            return;
        }
        
        p = p->next;
    }

    // if we get here, there is nothing to do
}

void file_list_free(file_list_t *list) {
    // list is empty
    // if (*list == NULL) return;

    // file_node_t *p = *list, *tmp;

    // while (p) {
    //     tmp = p;
    //     p = p->next;
    //     free(tmp);
    // }
    // keep popping the head until the list becomes empty
    while (*list) {
        file_list_remove(list, &(*list)->file.id);
    }
}

// print the contents of the list
void print_file_list(log_t log_type, file_list_t *list) {
    file_node_t *p = *list;

    while (p) {
        print_file(log_type, &p->file);
        print(log_type, "\n");

        p = p->next;
    }
}