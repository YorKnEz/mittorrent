#include "network.h"

// create a socket connected to specified addr
int32_t get_client_socket(struct sockaddr_in *addr) {
    int32_t fd;

    if (-1 == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
        print(LOG_ERROR, "[get_client_socket] Error at socket\n");
        return -1;
    }

    if (-1 == connect(fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in))) {
        print(LOG_ERROR, "[get_client_socket] Error at connect\n");
        close(fd);
        return -1;
    }

    return fd;
}

// create a socket that is bound to the specified addr
int32_t get_server_socket(struct sockaddr_in *addr) {
    int32_t fd;
    // init the socket
    if (-1 == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
        print(LOG_ERROR, "[get_server_socket] Error at socket\n");
        return -1;
    }

    // enable SO_REUSEADDR
    int32_t on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (-1 == bind(fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in))) {
        print(LOG_ERROR, "[get_server_socket] Error at bind\n");
        close(fd);
        return -1;
    }

    if (-1 == listen(fd, 0)) {
        print(LOG_ERROR, "[get_server_socket] Error at listen\n");
        close(fd);
        return -1;
    }

    print(LOG_DEBUG, "[get_server_socket] Listening for connections on: ");
    print_addr(LOG_DEBUG, addr);
    print(LOG_DEBUG, "\n");

    return fd;
}

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
void print_addr(log_t log_type, struct sockaddr_in *addr) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr.s_addr, buf, sizeof(buf));
    print(log_type, "%s:%u", buf, addr->sin_port);
}