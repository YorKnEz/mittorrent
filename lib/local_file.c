#include "local_file.h"

void local_file_from_file(local_file_t *local_file, file_t *file, const char *path) {
    memcpy(&local_file->id, &file->id, sizeof(key2_t));
    strcpy(local_file->path, path);
    local_file->size = file->size;
    local_file->blocks_size = local_file->size / FILE_BLOCK_SIZE + local_file->size % FILE_BLOCK_SIZE;
    local_file->blocks = (char*)malloc(local_file->blocks_size);
    memset(local_file->blocks, 0, local_file->blocks_size);
}

// free the memory of a local file
void free_local_file(local_file_t *file) {
    free(file->blocks);
}

// pretty print file contents
int32_t print_local_file(log_t log_type, local_file_t *file) {
    print(log_type, "id: ");
    print_key(log_type, &file->id);
    print(log_type, "\nname: %s\n", file->path);

    // pretty print the block state
    uint32_t state = 0;
    for (uint32_t i = 0; i < file->blocks_size; i++) {
        if (file->blocks[i] == 1) {
            state += 1;
        }
    }
    state = (state * 100) / file->blocks_size;
    print(log_type, "[");
    // map block state on 32 characters
    for (uint32_t i = 0; i < 32 * state / 100; i++) {
        print(log_type, "#");
    }
    for (uint32_t i = 32 * state / 100; i < 32; i++) {
        print(log_type, " ");
    }
    print(log_type, "] %d%%\n", state);
}