#include "downloader.h"

int32_t downloader_init(downloader_t *downloader) {
    download_list_init(&downloader->downloads);
    
    pthread_mutex_init(&downloader->lock, NULL);

    downloader->running = 1;

    for (uint32_t i = 0; i < DOWNLOADER_POOL_SIZE; i++) {
        pthread_create(&downloader->tid[i], NULL, (void *(*)(void*))&downloader_thread, downloader);
    }
}

void downloader_thread(downloader_t *downloader) {
    int32_t running = 1;

    while (running) {
        // check running state of downloader
        pthread_mutex_lock(&downloader->lock);

        int32_t download_index = -1;

        do {
            running = downloader->running;

            if (!running) {
                break;
            }

            for (uint32_t i = 0; i < downloader->downloads.size; i++) {
                pthread_mutex_lock(&downloader->downloads.buffer[i]->lock);
                if (downloader->downloads.buffer[i]->state == IDLE) {
                    download_index = i;
                    downloader->downloads.buffer[i]->state = RUNNING; // declare the download as running so that other threads won't take it
                }
                pthread_mutex_unlock(&downloader->downloads.buffer[i]->lock);
            }
        } while (0);

        pthread_mutex_unlock(&downloader->lock);

        if (!running) {
            break;
        }

        if (download_index != -1) {
            // a download is found
            if (-1 == download_start(downloader->downloads.buffer[download_index])) {
                print(LOG_ERROR, "[downloader_thread] Error at download_start\n");
                continue;
            }

            pthread_mutex_lock(&downloader->downloads.buffer[download_index]->lock);
            download_state_t local_state = downloader->downloads.buffer[download_index]->state;
            pthread_mutex_unlock(&downloader->downloads.buffer[download_index]->lock);

            if (local_state == DONE) {

            } else if (local_state == PAUSED) {

            }
        } else {
            // if a download is not found, we can sleep
            sleep(5);
        }
    }
    
    pthread_exit(NULL);
}

void downloader_cleanup(downloader_t *downloader) {
    pthread_mutex_lock(&downloader->lock);
    downloader->running = 0;
    pthread_mutex_unlock(&downloader->lock);

    for (uint32_t i = 0; i < DOWNLOADER_POOL_SIZE; i++) {
        pthread_join(downloader->tid[i], NULL);
    }

    download_list_free(&downloader->downloads);
    pthread_mutex_destroy(&downloader->lock);
}

int32_t downloader_add(downloader_t *downloader, file_t *file) {
    int32_t status = 0;
    
    pthread_mutex_lock(&downloader->lock);

    // see if the file is already in the downloads list
    // if the download is found and is paused, retry it, else ignore the download request
    int32_t found = 0;

    for (uint32_t i = 0; i < downloader->downloads.size; i++) {
        // no lock needed for the if as local_file is read-only
        if (key_cmp(&downloader->downloads.buffer[i]->local_file.id, &file->id) == 0) {
            pthread_mutex_lock(&downloader->downloads.buffer[i]->lock);

            // if the download is PAUSED, make it IDLE so it can be retried
            if (downloader->downloads.buffer[i]->state == PAUSED) {
                download_init_peers(downloader->downloads.buffer[i], file);

                downloader->downloads.buffer[i]->state = IDLE;
            }

            pthread_mutex_unlock(&downloader->downloads.buffer[i]->lock);

            found = 1;
            break;
        }
    }

    do {
        if (found) {
            break;
        }

        download_t download;

        if (-1 == download_init(&download, file)) {
            print(LOG_ERROR, "[downloader_add] Error at download_init\n");
            status = -1;
            break;
        }

        // add download to buffer
        download_list_add(&downloader->downloads, &download);
    } while (0);

    pthread_mutex_unlock(&downloader->lock);

    return status;
}

void print_downloader(log_t log_type, downloader_t *downloader) {
    pthread_mutex_lock(&downloader->lock);
    print_download_list(log_type, &downloader->downloads);
    pthread_mutex_unlock(&downloader->lock);
}