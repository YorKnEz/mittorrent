#ifndef DOWNLOAD_LIST_H
#define DOWNLOAD_LIST_H

#include "download.h"
#include "logger.h"

// dynamic vector for downloads
typedef struct {
    uint32_t size;
    uint32_t capacity;
    download_t **buffer;
} download_list_t;

void download_list_init(download_list_t *list);

// adds download to the list
void download_list_add(download_list_t *list, download_t *download);

// removes the download with the specified index from the list
void download_list_remove(download_list_t *list, uint32_t index);

// free download in the list
void download_list_free(download_list_t *list);

// print the contents of the list
void print_download_list(log_t log_type, download_list_t *list);

#endif