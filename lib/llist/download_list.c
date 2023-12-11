#include "download_list.h"

void download_list_init(download_list_t *list) {
    list->size = 0;
    list->capacity = 1;
    list->buffer = (download_t**)malloc(list->capacity * sizeof(download_t*));
}

// adds download to the list
void download_list_add(download_list_t *list, download_t *download) {
    if (list->size >= list->capacity) {
        // grow buffer
        download_t **tmp = (download_t**)malloc(2 * list->capacity * sizeof(download_t*));
        memcpy(tmp, list->buffer, list->capacity * sizeof(download_t*));
        memset(&tmp[list->capacity], 0, list->capacity * sizeof(download_t*));
        free(list->buffer);
        list->buffer = tmp;
        tmp = NULL;
        list->capacity *= 2;
    }

    list->buffer[list->size] = (download_t*)malloc(sizeof(download_t));
    memcpy(list->buffer[list->size], download, sizeof(download_t));
    list->size += 1;
}

// removes the download with the specified index from the list
void download_list_remove(download_list_t *list, uint32_t index) {
    if (!(0 <= index && index < list->size)) {
        return;
    }

    download_cleanup(list->buffer[index]);
    free(list->buffer[index]);
    list->size -= 1;

    for (uint32_t i = index; i < list->size; i++) {
        list->buffer[i] = list->buffer[i + 1];
    }
}

// free download in the list
void download_list_free(download_list_t *list) {
    for (uint32_t i = 0; i < list->size; i++) {
        download_cleanup(list->buffer[i]);
        free(list->buffer[i]);
    }
    free(list->buffer);
}

// print the contents of the list
void print_download_list(log_t log_type, download_list_t *list) {
    print(log_type, "#    | state   | name                             | status                                  | peers\n");
    
    for (uint32_t i = 0; i < list->size; i++) {
        print(log_type, "%4d | ", i + 1);
        pthread_mutex_lock(&list->buffer[i]->lock);
        print_download_short(log_type, list->buffer[i]);
        pthread_mutex_unlock(&list->buffer[i]->lock);
    }
}