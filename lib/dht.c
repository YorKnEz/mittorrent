#include "dht.h"

int32_t node_req(node_remote_t *node, req_type_t type, void *req, uint32_t req_size, char **res, uint32_t *res_size) {
    // connect to node
    int32_t fd;

    if (-1 == (fd = get_client_socket(&node->addr))) {
        print(LOG_ERROR, "[node_req] Error at get_client_socket\n");
        *res = NULL;
        return -1;
    }

    if (-1 == send_and_recv(fd, type, req, req_size, res, res_size)) {
        print(LOG_ERROR, "[node_req] Error at send_and_recv\n");
        shutdown(fd, SHUT_RDWR);
        close(fd);
        return -1;
    }

    shutdown(fd, SHUT_RDWR);
    close(fd);

    return 0;
}

// ask node to find the successor of id
int32_t node_find_next(node_local_t *node, key2_t *id, node_remote_t *res) {
    // asked of successor of myself
    if (key_cmp(&node->id, id) == 0) {
        memcpy(res, &node->finger[0].node, sizeof(node_remote_t));
        return 0;
    }

    // forward the request until successor is found
    node_remote_t prev;
    if (-1 == node_find_prev(node, id, &prev)) {
        print(LOG_ERROR, "[node_find_next] Error at node_find_prev\n");
        return -1;
    }
    // ask for successor of prev
    if (-1 == node_find_next_remote(node, &prev, &prev.id, res)) {
        print(LOG_ERROR, "[node_find_next] Error at node_find_next_remote\n");
        return -1;
    }

    return 0;
}
int32_t node_find_next_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res) {
    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (-1 == node_find_next(node, id, res)) {
            print(LOG_ERROR, "[node_find_next_remote] Error at node_find_next\n");
            return -1;
        }

        return 0;
    }
    
    char *msg;
    uint32_t msg_size;
    
    if (-1 == node_req(fwd_node, FIND_NEXT, id, sizeof(key2_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[node_find_next_remote] Error at node_req\n");
        return -1;
    }

    memcpy(res, msg, sizeof(node_remote_t));

    free(msg);

    return 0;
}

// ask node to find the predecessor of id
//
// in the implementation provided by Chord, they use a while
// in the case of this implementation each node forwards the request and responds after it has received the answer from the chain
int32_t node_find_prev(node_local_t *node, key2_t *id, node_remote_t *res) {
    // asked of predecessor of myself
    if (key_cmp(&node->id, id) == 0) {
        if (!node->prev_initialized) {
            print(LOG_ERROR, "[node_find_prev] Prev is uninitalized\n");
            return -1;
        }

        memcpy(res, &node->prev, sizeof(node_remote_t));
        return 0;
    }

    node_remote_t current;
    memcpy(&current, node, sizeof(node_remote_t));
    node_remote_t next;
    memcpy(&next, &node->finger[0].node, sizeof(node_remote_t));

    while (!key_in(id, &current.id, 0, &next.id, 1)) {
        if (-1 == node_find_closest_preceding_remote(node, &current, id, &current)) {
            print(LOG_ERROR, "[node_find_prev] Error at node_find_closest_preceding_remote\n");
            return -1;
        }

        if (-1 == node_find_next_remote(node, &current, &current.id, &next)) {
            print(LOG_ERROR, "[node_find_prev] Error at node_find_next_remote\n");
            return -1;
        }
    }

    memcpy(res, &current, sizeof(node_remote_t));

    return 0;
}
int32_t node_find_prev_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res) {
    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (-1 == node_find_prev(node, id, res)) {
            print(LOG_ERROR, "[node_find_prev_remote] Error at node_find_prev\n");
            return -1;
        }

        return 0;
    }

    char *msg;
    uint32_t msg_size;
    
    if (-1 == node_req(fwd_node, FIND_PREV, id, sizeof(key2_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[node_find_prev_remote] Error at node_req\n");
        return -1;
    }

    memcpy(res, msg, sizeof(node_remote_t));

    free(msg);

    return 0;
}

// set the prev field of node
int32_t node_set_prev(node_local_t *node, node_remote_t *new_prev) {
    memcpy(&node->prev, new_prev, sizeof(node_remote_t));
    node->prev_initialized = 1;

    return 0;
}
int32_t node_set_prev_remote(node_local_t *node, node_remote_t *fwd_node, node_remote_t *new_prev) {
    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        node_set_prev(node, new_prev);

        return 0;
    }
    
    char *msg;
    uint32_t msg_size;
    
    // response doesn't matter
    if (-1 == node_req(fwd_node, SET_PREV, new_prev, sizeof(node_remote_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[node_set_prev] Error at node_req\n");
        return -1;
    }

    free(msg);

    return 0;
}

// find closest finger of node preceding id
int32_t node_find_closest_preceding(node_local_t *node, key2_t *id, node_remote_t *res) {
    for (int32_t i = KEY_BITS - 1; i >= 0; i--) {
        if (!node->finger[i].initialized) {
            continue;
        }

        if (key_in(&node->finger[i].node.id, &node->id, 0, id, 0)) {
            memcpy(res, &node->finger[i].node, sizeof(node_remote_t));
            return 0;
        }
    }

    print(LOG_ERROR, "[node_find_closest_preceding] Not found\n");
    return -1;
}
int32_t node_find_closest_preceding_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res) {
    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (-1 == node_find_closest_preceding(node, id, res)) {
            print(LOG_ERROR, "[node_find_closest_preceding_remote] Error at node_find_closest_preceding\n");
            return -1;
        }

        return 0;
    }

    char *msg;
    uint32_t msg_size;
    
    if (-1 == node_req(fwd_node, FIND_CLOSEST_PRECEDING_FINGER, id, sizeof(key2_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[node_find_closest_preceding_remote] Error at node_req\n");
        return -1;
    }

    memcpy(res, msg, sizeof(node_remote_t));

    free(msg);

    return 0;
}


/*
int32_t node_join(node_local_t *node, node_remote_t *peer) {
    if (peer) {
        if (-1 == node_init_finger_table(node, peer)) {
            print(LOG_ERROR, "[node_join] Error at node_init_finger_table\n");
            return -1;
        }

        if (-1 == node_update_others(node)) {
            print(LOG_ERROR, "[node_join] Error at node_update_others\n");
            return -1;
        }

        // move keys in (prev, n] from next
    } else { // this is the only node in the network
        for (int32_t i = 0; i < KEY_BITS; i++) {
            memcpy(&node->finger[i].node, node, sizeof(node_remote_t));
        }
        memcpy(&node->prev, node, sizeof(node_remote_t));
    }

    return 0;
}

int32_t node_init_finger_table(node_local_t *node, node_remote_t *peer) {
    // pow2 = 2^0
    key2_t *pow2;
    memset(pow2, 0, sizeof(pow2));
    key_add_int(pow2, 1);
    
    // node->finger[0].start = node->id + 2^0
    memcpy(node->finger[0].start, node->id, SHA256_BLOCK_SIZE);
    key_add(node->finger[0].start, pow2);
    // node->finger[0].node = peer->find_next(finger[0].start)
    if (-1 == node_find_next_remote(peer, node->finger[0].start, &node->finger[0].node)) {
        print(LOG_ERROR, "[node_init_finger_table] Error at node_find_next_remote\n");
        return -1;
    }
    
    // node->prev = node->finger[0].node.prev
    if (-1 == node_find_prev_remote(&node->finger[0].node, node->finger[0].node.id, &node->prev)) {
        print(LOG_ERROR, "[node_init_finger_table] Error at node_find_prev_remote\n");
        return -1;
    }
    // node->finger[0].node.prev = node
    if (-1 == node_set_prev_remote(&node->finger[0].node, (node_remote_t*)node)) {
        print(LOG_ERROR, "[node_init_finger_table] Error at node_set_prev_remote\n");
        return -1;
    }

    for (int32_t i = 0; i < KEY_BITS - 1; i++) {
        // pow2 *= 2
        key_double(pow2);
        
        // node->finger[i + 1].start = node->id + 2^(i + 1)
        memcpy(node->finger[i + 1].start, node->id, SHA256_BLOCK_SIZE);
        key_add(node->finger[i + 1].start, pow2);
        
        if (key_in(node->finger[i + 1].start, node->id, 1, node->finger[i].node.id, 0)) {
            // node->finger[i + 1].node = node->finger[i].node
            memcpy(&node->finger[i + 1].node, &node->finger[i].node, sizeof(node_remote_t));
        } else {
            // node->finger[i + 1].node = peer->find_next(finger[i + 1].start)
            if (-1 == node_find_next_remote(peer, node->finger[i + 1].start, &node->finger[i + 1].node)) {
                print(LOG_ERROR, "[node_init_finger_table] Error at node_find_next_remote\n");
                return -1;
            }
        }
    }

    return 0;
}

int32_t node_update_others(node_local_t *node) {
    node_remote_t prev;

    // pow2 = 2^0
    key2_t *pow2;
    memset(pow2, 0, SHA256_BLOCK_SIZE);
    key_add_int(pow2, 1);

    // node_id = node->id - 2^0
    key2_t *node_id;
    memcpy(node_id, node->id, SHA256_BLOCK_SIZE);
    key_sub(node_id, pow2);

    for (int32_t i = 0; i < KEY_BITS; i++) {
        // at this point node_id = node->id - 2^i
        
        // find last node whose ith finger might be n
        if (-1 == node_find_prev(node, node_id, &prev)) {
            print(LOG_ERROR, "[node_update_others] Error at node_find_prev\n");
            return -1;
        }
        
        if (-1 == node_update_finger_table_remote(&prev, (node_remote_t*)node, i)) {
            print(LOG_ERROR, "[node_update_others] Error at node_update_finger_table_remote\n");
            return -1;
        }

        // node_id = node->id - 2^(i+1)
        key_sub(node_id, pow2);
        // pow2 *= 2
        key_double(pow2);
    }
}

int32_t node_update_finger_table(node_local_t *node, node_remote_t *peer, int32_t i) {
    if (key_in(peer->id, node->id, 1, node->finger[i].node.id, 0)) {
        // node->finger[i].node = peer
        memcpy(&node->finger[i].node, peer, sizeof(node_remote_t));

        if (-1 == node_update_finger_table_remote(&node->prev, peer, i)) {
            print(LOG_ERROR, "[node_update_finger_table] Error at node_update_finger_table_remote\n");
            return -1;
        }
    }
}
int32_t node_update_finger_table_remote(node_remote_t *node, node_remote_t *peer, int32_t i) {
    char *msg;
    uint32_t msg_size;
    
    struct {
        node_remote_t node;
        int32_t i;
    } data;

    memcpy(&data.node, peer, sizeof(node_remote_t));
    data.i = i;

    // response is useless
    if (-1 == node_req(node, FIND_CLOSEST_PRECEDING_FINGER, &data, sizeof(data), &msg, &msg_size)) {
        print(LOG_ERROR, "[node_update_finger_table_remote] Error at node_req\n");
        return -1;
    }

    free(msg);

    return 0;
}
*/

// node joined the network
// peer is an arbitrary node in the network
int32_t node_join(node_local_t *node, node_remote_t *peer) {
    // initialize fingers state
    // pow2 = 2^0
    key2_t pow2;
    memset(&pow2, 0, sizeof(key2_t));
    key_add_int(&pow2, 1);

    for (int32_t i = 0; i < KEY_BITS; i++) {
        node->finger[i].initialized = 0;
        
        // node->finger[i].start = node->id + 2^i
        memcpy(&node->finger[i].start, &node->id, sizeof(key2_t));
        key_add(&node->finger[i].start, &pow2);

        // pow2 *= 2
        key_double(&pow2);
    }
    node->prev_initialized = 0;

    if (peer) {
        if (-1 == node_find_next_remote(node, peer, &node->id, &node->finger[0].node)) {
            print(LOG_ERROR, "[node_join] Error at node_find_next_remote\n");
            return -1;
        }

        node->finger[0].initialized = 1; // node->finger[0] has been initalized

        if (-1 == node_notify_remote(node, &node->finger[0].node, (node_remote_t*)node)) {
            print(LOG_ERROR, "[node_join] Error at node_notify_remote\n");
            return -1;
        }

        // ask my successor to send me the files he owns
    } else { // this is the only node in the network
        memcpy(&node->finger[0].node, node, sizeof(node_remote_t));
        node->finger[0].initialized = 1;

        node_set_prev(node, (node_remote_t*)node);
    }

    return 0;
}

// periodically verify n's immediate successor, and tell the successor about n
int32_t node_stabilize(node_local_t *node) {
    node_remote_t prev;

    if (-1 == node_find_prev_remote(node, &node->finger[0].node, &node->finger[0].node.id, &prev)) {
        print(LOG_ERROR, "[node_stabilize] Error at node_find_prev_remote\n");
        return -1;
    }

    if (key_in(&prev.id, &node->id, 0, &node->finger[0].node.id, 0)) {
        memcpy(&node->finger[0].node, &prev, sizeof(node_remote_t));
    }

    if (-1 == node_notify_remote(node, &node->finger[0].node, (node_remote_t*)node)) {
        print(LOG_ERROR, "[node_stabilize] Error at node_notify_remote\n");
        return -1;
    }

    return 0;
}

// peer thinks it might be our predecessor
int32_t node_notify(node_local_t *node, node_remote_t *peer) {
    // check if my prev is alive/valid first
    if (node->prev_initialized == 1) {
        char *msg;
        uint32_t msg_size;

        if (-1 == node_req(&node->prev, PING, NULL, 0, &msg, &msg_size)) {
            node->prev_initialized = 0;
            free(msg);
        }

        free(msg);
    }

    if (!node->prev_initialized || key_in(&peer->id, &node->prev.id, 0, &node->id, 0)) {
        node_set_prev(node, peer);

        return 1; // tells tracker to send the keys
    }

    return 0; // tells tracker to not move anything
}
int32_t node_notify_remote(node_local_t *node, node_remote_t *fwd_node, node_remote_t *peer) {
    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (-1 == node_notify(node, peer)) {
            print(LOG_ERROR, "[node_notify_remote] Error at node_notify\n");
            return -1;
        }

        return 0;
    }

    char *msg;
    uint32_t msg_size;
    
    // response is useless
    if (-1 == node_req(fwd_node, NOTIFY, peer, sizeof(node_remote_t), &msg, &msg_size)) {
        print(LOG_ERROR, "[node_notify_remote] Error at node_req\n");
        return -1;
    }

    free(msg);

    return 0;
}

// periodically refresh finger table entries
int32_t node_fix_fingers(node_local_t *node) {
    // some random seed
    srand(node->id.key[2]);

    int32_t i;
    while ((i = rand() % KEY_BITS) == 0);

    if (-1 == node_find_next(node, &node->finger[i].start, &node->finger[i].node)) {
        print(LOG_ERROR, "[node_fix_fingers] Error at node_find_next\n");
        return -1;
    }

    node->finger[i].initialized = 1;

    return 0;
}

// pretty print node_remote_t
void print_remote_node(log_t log_type, node_remote_t *node) {
    print(log_type, "(id: ");
    print_key(log_type, &node->id);
    print(log_type, ", addr: ");
    print_addr(log_type, &node->addr);
    print(log_type, ")");
}