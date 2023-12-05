#include "local_file.h"

// pretty print file contents
int32_t print_local_file(log_t log_type, local_file_t *file) {
    print(log_type, "id: ");
    print_key(log_type, &file->id);
    print(log_type, "\nname: %s\n", file->path);
}