#include "dht.h"

int32_t node_req(node_remote_t *node, req_type_t type, void *req, uint32_t req_size, char **res, uint32_t *res_size) {
    int32_t status = 0;

    // connect to node
    int32_t fd;

    if (CHECK((fd = get_client_socket(&node->addr)))) {
        print(LOG_ERROR, "[node_req] Error at get_client_socket\n");
        *res = NULL;
        return status;
    }

    if (CHECK(send_and_recv(fd, type, req, req_size, res, res_size))) {
        print(LOG_ERROR, "[node_req] Error at send_and_recv\n");
        shutdown(fd, SHUT_RDWR);
        close(fd);
        return status;
    }

    shutdown(fd, SHUT_RDWR);
    close(fd);

    return status;
}

// ask node to find the successor of id
int32_t node_find_next(node_local_t *node, key2_t *id, node_remote_t *res) {
    int32_t status = 0;

    // asked of successor of myself
    if (key_cmp(&node->id, id) == 0) {
        memcpy(res, &node->finger[0].node, sizeof(node_remote_t));
        return status;
    }

    // forward the request until successor is found
    node_remote_t prev;
    if (CHECK(node_find_prev(node, id, &prev))) {
        print(LOG_ERROR, "[node_find_next] Error at node_find_prev\n");
        return status;
    }
    // ask for successor of prev
    if (CHECK(node_find_next_remote(node, &prev, &prev.id, res))) {
        print(LOG_ERROR, "[node_find_next] Error at node_find_next_remote\n");
        return status;
    }

    return status;
}
int32_t node_find_next_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res) {
    int32_t status = 0;

    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (CHECK(node_find_next(node, id, res))) {
            print(LOG_ERROR, "[node_find_next_remote] Error at node_find_next\n");
            return status;
        }

        return status;
    }
    
    char *msg;
    uint32_t msg_size;
    
    if (CHECK(node_req(fwd_node, FIND_NEXT, id, sizeof(key2_t), &msg, &msg_size))) {
        print(LOG_ERROR, "[node_find_next_remote] Error at node_req\n");
        return status;
    }

    memcpy(res, msg, sizeof(node_remote_t));

    free(msg);

    return status;
}

// ask node to find the predecessor of id
//
// in the implementation provided by Chord, they use a while
// in the case of this implementation each node forwards the request and responds after it has received the answer from the chain
int32_t node_find_prev(node_local_t *node, key2_t *id, node_remote_t *res) {
    int32_t status = 0;

    // asked of predecessor of myself
    if (key_cmp(&node->id, id) == 0) {
        if (!node->prev_initialized) {
            print(LOG_ERROR, "[node_find_prev] Prev is uninitalized\n");
            status = -1;
            return status;
        }

        memcpy(res, &node->prev, sizeof(node_remote_t));
        return 0;
    }

    node_remote_t current;
    memcpy(&current, node, sizeof(node_remote_t));
    node_remote_t next;
    memcpy(&next, &node->finger[0].node, sizeof(node_remote_t));

    while (!key_in(id, &current.id, 0, &next.id, 1)) {
        if ((status = node_find_closest_preceding_remote(node, &current, id, &current)) != 0) {
            print(LOG_ERROR, "[node_find_prev] Error at node_find_closest_preceding_remote\n");
            return status;
        }

        if (CHECK(node_find_next_remote(node, &current, &current.id, &next))) {
            print(LOG_ERROR, "[node_find_prev] Error at node_find_next_remote\n");
            return status;
        }
    }

    memcpy(res, &current, sizeof(node_remote_t));

    return status;
}
int32_t node_find_prev_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res) {
    int32_t status = 0;

    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (CHECK(node_find_prev(node, id, res))) {
            print(LOG_ERROR, "[node_find_prev_remote] Error at node_find_prev\n");
            return status;
        }

        return status;
    }

    char *msg;
    uint32_t msg_size;
    
    if (CHECK(node_req(fwd_node, FIND_PREV, id, sizeof(key2_t), &msg, &msg_size))) {
        print(LOG_ERROR, "[node_find_prev_remote] Error at node_req\n");
        return status;
    }

    memcpy(res, msg, sizeof(node_remote_t));

    free(msg);

    return status;
}

// set the prev field of node
int32_t node_set_prev(node_local_t *node, node_remote_t *new_prev) {
    int32_t status = 0;

    memcpy(&node->prev, new_prev, sizeof(node_remote_t));
    node->prev_initialized = 1;

    return status;
}
int32_t node_set_prev_remote(node_local_t *node, node_remote_t *fwd_node, node_remote_t *new_prev) {
    int32_t status = 0;

    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        node_set_prev(node, new_prev);

        return status;
    }
    
    char *msg;
    uint32_t msg_size;
    
    // response doesn't matter
    if (CHECK(node_req(fwd_node, SET_PREV, new_prev, sizeof(node_remote_t), &msg, &msg_size))) {
        print(LOG_ERROR, "[node_set_prev] Error at node_req\n");
        return status;
    }

    free(msg);

    return status;
}

// find closest finger of node preceding id
int32_t node_find_closest_preceding(node_local_t *node, key2_t *id, node_remote_t *res) {
    int32_t status = 0;

    for (int32_t i = KEY_BITS - 1; i >= 0; i--) {
        if (!node->finger[i].initialized) {
            continue;
        }

        if (key_in(&node->finger[i].node.id, &node->id, 0, id, 0)) {
            memcpy(res, &node->finger[i].node, sizeof(node_remote_t));
            return status;
        }
    }

    print(LOG_ERROR, "[node_find_closest_preceding] Not found\n");
    status = -1;
    return status;
}
int32_t node_find_closest_preceding_remote(node_local_t *node, node_remote_t *fwd_node, key2_t *id, node_remote_t *res) {
    int32_t status = 0;

    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (CHECK(node_find_closest_preceding(node, id, res))) {
            print(LOG_ERROR, "[node_find_closest_preceding_remote] Error at node_find_closest_preceding\n");
            return status;
        }

        return status;
    }

    char *msg;
    uint32_t msg_size;
    
    if (CHECK(node_req(fwd_node, FIND_CLOSEST_PRECEDING_FINGER, id, sizeof(key2_t), &msg, &msg_size))) {
        print(LOG_ERROR, "[node_find_closest_preceding_remote] Error at node_req\n");
        return status;
    }

    memcpy(res, msg, sizeof(node_remote_t));

    free(msg);

    return status;
}

// node joined the network
// peer is an arbitrary node in the network
int32_t node_join(node_local_t *node, node_remote_t *peer) {
    int32_t status = 0;

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
        if (CHECK(node_find_next_remote(node, peer, &node->id, &node->finger[0].node))) {
            print(LOG_ERROR, "[node_join] Error at node_find_next_remote\n");
            return status;
        }

        node->finger[0].initialized = 1; // node->finger[0] has been initalized

        if (CHECK(node_notify_remote(node, &node->finger[0].node, (node_remote_t*)node))) {
            print(LOG_ERROR, "[node_join] Error at node_notify_remote\n");
            return status;
        }

        // ask my successor to send me the files he owns
    } else { // this is the only node in the network
        memcpy(&node->finger[0].node, node, sizeof(node_remote_t));
        node->finger[0].initialized = 1;

        node_set_prev(node, (node_remote_t*)node);
    }

    return status;
}

// periodically verify n's immediate successor, and tell the successor about n
int32_t node_stabilize(node_local_t *node) {
    int32_t status = 0;

    node_remote_t prev;

    if (CHECK(node_find_prev_remote(node, &node->finger[0].node, &node->finger[0].node.id, &prev))) {
        print(LOG_ERROR, "[node_stabilize] Error at node_find_prev_remote\n");
        return status;
    }

    if (key_in(&prev.id, &node->id, 0, &node->finger[0].node.id, 0)) {
        memcpy(&node->finger[0].node, &prev, sizeof(node_remote_t));
    }

    if (CHECK(node_notify_remote(node, &node->finger[0].node, (node_remote_t*)node))) {
        print(LOG_ERROR, "[node_stabilize] Error at node_notify_remote\n");
        return status;
    }

    return status;
}

// peer thinks it might be our predecessor
int32_t node_notify(node_local_t *node, node_remote_t *peer) {
    int32_t status = 0;

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
    }

    return status;
}
int32_t node_notify_remote(node_local_t *node, node_remote_t *fwd_node, node_remote_t *peer) {
    int32_t status = 0;

    // check if the request isnt't actually a local one
    if (key_cmp(&node->id, &fwd_node->id) == 0) {
        if (CHECK(node_notify(node, peer))) {
            print(LOG_ERROR, "[node_notify_remote] Error at node_notify\n");
            return status;
        }

        return status;
    }

    char *msg;
    uint32_t msg_size;
    
    // response is useless
    if (CHECK(node_req(fwd_node, NOTIFY, peer, sizeof(node_remote_t), &msg, &msg_size))) {
        print(LOG_ERROR, "[node_notify_remote] Error at node_req\n");
        return status;
    }

    free(msg);

    return status;
}

// periodically refresh finger table entries
int32_t node_fix_fingers(node_local_t *node) {
    int32_t status = 0;

    // some random seed
    srand(node->id.key[2]);

    int32_t i;
    while ((i = rand() % KEY_BITS) == 0);

    if (CHECK(node_find_next(node, &node->finger[i].start, &node->finger[i].node))) {
        print(LOG_ERROR, "[node_fix_fingers] Error at node_find_next\n");
        return status;
    }

    node->finger[i].initialized = 1;

    return status;
}

// pretty print node_remote_t
void print_remote_node(log_t log_type, node_remote_t *node) {
    print(log_type, "(id: ");
    print_key(log_type, &node->id);
    print(log_type, ", addr: ");
    print_addr(log_type, &node->addr);
    print(log_type, ")");
}
