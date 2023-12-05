#include "node_list.h"

// adds node to the list
void list_add(list_t *list, node_remote_t *node) {
    node_t *new_node = (node_t*)malloc(sizeof(node_t));
    memcpy(&new_node->node, node, sizeof(node_remote_t));
    
    // list is empty
    if (*list == NULL) {
        new_node->next = NULL;
        *list = new_node;
        return;
    }

    int32_t res = key_cmp(&new_node->node.id, &(*list)->node.id);

    // add the node so that ids are in ascending order
    if (res < 0) { /* find first node that succeeds new_node */
        new_node->next = (*list);
        *list = new_node;
        return;
    } else if (res == 0) { /* stop if node is already added */
        return;
    }
    
    // previous and current pointers
    node_t *p = *list;

    while (p->next) {
        res = key_cmp(&new_node->node.id, &p->next->node.id);
        
        if (res < 0) { /* find first node that succeeds new_node */
            new_node->next = p->next;
            p->next = new_node;
            return;
        } else if (res == 0) { /* stop if node is already added */
            return;
        }

        p = p->next;
    }

    // if we get here, it means the new_node should be added at the end of the list
    new_node->next = NULL;
    p->next = new_node;
}

// removes the node with the specified id from the list
void list_remove(list_t *list, key2_t *id) {
    node_t *p = *list;

    // remove the head
    if (key_cmp(&(*list)->node.id, id) == 0) {
        *list = (*list)->next;
        free(p);
        return;
    }
    
    while (p->next) {
        if (key_cmp(&p->next->node.id, id) == 0) {
            node_t *tmp = p->next;
            p->next = p->next->next;
            free(tmp);
            return;
        }
        
        p = p->next;
    }

    // if we get here, there is nothing to do
}

// free nodes in the list
void list_free(list_t *list) {
    // list is empty
    if (*list == NULL) return;

    node_t *p = *list, *tmp;

    while (p) {
        tmp = p;
        p = p->next;
        free(tmp);
    }
}

// print the contents of the list
void print_list(log_t log_type, list_t *list) {
    node_t *p = *list;

    while (p) {
        print_remote_node(log_type, &p->node);
        print(log_type, "\n");

        p = p->next;
    }
}