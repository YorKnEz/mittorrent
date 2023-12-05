#include "network.h"

int32_t read_full(int32_t socket_fd, void *buf, uint32_t buf_size) {
    uint32_t read_bytes = 0, to_read = buf_size;

    while (to_read > 0) {
        read_bytes = read(socket_fd, buf + (buf_size - to_read), to_read);

        if (-1 == read_bytes) return -1;

        to_read -= read_bytes;
    }

    return 0;
}

int32_t write_full(int32_t socket_fd, void *buf, uint32_t buf_size) {
    uint32_t wrote_bytes = 0, to_write = buf_size;

    while (to_write > 0) {
        wrote_bytes = write(socket_fd, buf + (buf_size - to_write), to_write);

        if (-1 == wrote_bytes) return -1;

        to_write -= wrote_bytes;
    }

    return 0;
}

int32_t send_req(int32_t socket_fd, req_type_t type, void* msg, uint32_t msg_size) {
    req_header_t reqh;
    reqh.type = type;
    reqh.size = msg_size;

    uint32_t buf_size = sizeof(reqh) + msg_size;
    char *buf = (char*)malloc(buf_size);
    memcpy(buf, &reqh, sizeof(reqh));
    memcpy(buf + sizeof(reqh), msg, msg_size);

    if (-1 == write_full(socket_fd, buf, buf_size)) return -1;

    free(buf);

    return 0;
}

int32_t recv_req(int32_t socket_fd, req_header_t *reqh, char **msg, uint32_t *msg_size) {
    if (-1 == read_full(socket_fd, reqh, sizeof(*reqh))) return -1;

    *msg = (char*) malloc (reqh->size);
    memset(*msg, 0, reqh->size);
    *msg_size = reqh->size;

    if (-1 == read_full(socket_fd, *msg, reqh->size)) return -1;
    
    return 0;
}

int32_t send_res(int32_t socket_fd, res_type_t type, void* msg, uint32_t msg_size) {
    res_header_t resh;
    resh.type = type;
    resh.size = msg_size;

    uint32_t buf_size = sizeof(resh) + msg_size;
    char *buf = (char*)malloc(buf_size);
    memcpy(buf, &resh, sizeof(resh));
    memcpy(buf + sizeof(resh), msg, msg_size);

    if (-1 == write_full(socket_fd, buf, buf_size)) return -1;

    free(buf);

    return 0;
}

int32_t recv_res(int32_t socket_fd, res_header_t *resh, char **msg, uint32_t *msg_size) {
    if (-1 == read_full(socket_fd, resh, sizeof(*resh))) return -1;

    *msg = (char*) malloc (resh->size);
    memset(*msg, 0, resh->size);
    *msg_size = resh->size;

    if (-1 == read_full(socket_fd, *msg, resh->size)) {
        free(*msg);
        return -1;
    }
    
    return 0;
}

int32_t send_and_recv(int32_t socket_fd, req_type_t type, void* req, uint32_t req_size, char **res, uint32_t *res_size) {
    if (-1 == send_req(socket_fd, type, req, req_size)) {
        print(LOG_ERROR, "[send_and_recv] Error at send_req\n");
        return -1;
    }

    res_header_t resh;

    if (-1 == recv_res(socket_fd, &resh, res, res_size)) {
        print(LOG_ERROR, "[send_and_recv] Error at recv_res\n");
        return -1;
    }

    if (resh.type == ERROR) {
        print(LOG_ERROR, "[send_and_recv] %s\n", *res);
        return -1;
    }

    return 0;
}

// pretty print sockaddr_in
void print_addr(struct sockaddr_in *addr) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr.s_addr, buf, sizeof(buf));
    print(LOG_DEBUG, "%s:%u", buf, addr->sin_port);
}