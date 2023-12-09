#include "local_file.h"

void local_file_from_file(local_file_t *local_file, file_t *file, const char *path) {
    memcpy(&local_file->id, &file->id, sizeof(key2_t));
    strcpy(local_file->path, path);
    local_file->size = file->size;
}

// pretty print file contents
int32_t print_local_file(log_t log_type, local_file_t *file) {
    print(log_type, "id: ");
    print_key(log_type, &file->id);
    print(log_type, "\npath: %s\n", file->path);
}