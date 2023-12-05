#include "file_list.h"

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