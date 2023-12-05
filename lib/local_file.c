#include "local_file.h"

// pretty print file contents
int32_t print_local_file(local_file_t *file) {
    print(LOG_DEBUG, "id: ");
    print_key(&file->id);
    print(LOG_DEBUG, "\nname: %s\n", file->path);
}

// adds file to the list
void local_file_list_add(local_file_list_t *list, local_file_t *file) {
    local_file_node_t *new_node = (local_file_node_t*)malloc(sizeof(local_file_node_t));
    memcpy(&new_node->file, file, sizeof(local_file_t));
    
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
    local_file_node_t *p = *list;

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
void local_file_list_remove(local_file_list_t *list, key2_t *id) {
    local_file_node_t *p = *list;

    // remove the head
    if (key_cmp(&(*list)->file.id, id) == 0) {
        *list = (*list)->next;
        free(p);
        return;
    }
    
    while (p->next) {
        if (key_cmp(&p->next->file.id, id) == 0) {
            local_file_node_t *tmp = p->next;
            p->next = p->next->next;
            free(tmp);
            return;
        }
        
        p = p->next;
    }

    // if we get here, there is nothing to do
}

void local_file_list_free(local_file_list_t *list) {
    // list is empty
    if (*list == NULL) return;

    local_file_node_t *p = *list, *tmp;

    while (p) {
        tmp = p;
        p = p->next;
        free(tmp);
    }
}

// find file path by id
int32_t local_file_list_find(local_file_list_t *list, key2_t *id, char path[512]) {
    local_file_node_t *p = *list;

    while (p) {
        if (key_cmp(id, &p->file.id) == 0) {
            memcpy(path, p->file.path, 512);
            return 0;
        }
        
        p = p->next;
    }
    
    return -1;
}

// print the contents of the list
void print_local_file_list(local_file_list_t *list) {
    local_file_node_t *p = *list;

    while (p) {
        print_local_file(&p->file);
        print(LOG_DEBUG, "\n");

        p = p->next;
    }
}