#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "serial_bridge.h"
#include "config.h"
#include "util.h"

struct sockaddr_in addr;
struct sockaddr_in PriorityClientAddress;
uv_tcp_t ServerHandle;
uv_tcp_t *RegularClientHandle = NULL;
uv_tcp_t *PriorityClientHandle = NULL;

static void tcp_write_to_client(uv_tcp_t *client, char *buffer, int length);

static void alloc_buffer(uv_handle_t *handle, size_t len, uv_buf_t *buf) {
    buf->base = (char*)malloc(len);
    buf->len = len;
}

static void tcp_close_cb(uv_handle_t* client) {
    printf("tcp_close callback\n");
    free(client);
}

static void tcp_close(uv_handle_t* client) {
    printf("tcp_close\n");
    uv_close(client, tcp_close_cb);
    if (client == (uv_handle_t *)PriorityClientHandle) {
        printf("PRIORITY client\n");
        PriorityClientHandle = NULL;
        tcp_write_to_client(RegularClientHandle, "~~RESUME\n", 9);
    } else if (client == (uv_handle_t *)RegularClientHandle) {
        printf("REGULAR client\n");
        RegularClientHandle = NULL;
    } else {
        printf("UNKNOWN client\n");
    }
}

static void tcp_write_cb(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "TCP write error: %s\n", uv_strerror(status));
        tcp_close((uv_handle_t*)req->handle);
    }

    uv_buf_t *buf = (uv_buf_t *)req->data;
    free(buf->base);
    free(buf);
    free(req);
}

static void tcp_write_to_client(uv_tcp_t *client, char *buffer, int length) {
    if (client == NULL)
        return;

    uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
    req->data = (void *)create_uv_buf_with_data(buffer, length);
    uv_write(req, (uv_stream_t *)client, req->data, 1, tcp_write_cb);
}

void tcp_write(char *buffer, int length) {
    uv_stream_t* sendto;

    printf("tcp_write\n");
    sendto = (uv_stream_t*)
        (PriorityClientHandle ? PriorityClientHandle : RegularClientHandle);

    tcp_write_to_client((uv_tcp_t *)sendto, buffer, length);
}

static void tcp_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    printf("tcp_read callback\n");
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "TCP read error %s\n", uv_err_name(nread));
        tcp_close((uv_handle_t *)client);
    } else if (nread > 0) {
        /* if (*(buf->base) == '~') exit(0); */
        if (PriorityClientHandle) {
            if (client == (uv_stream_t *)PriorityClientHandle)
                serial_write(buf->base, nread);
        } else {
            serial_write(buf->base, nread);
        }
    }
    if (buf->base) free(buf->base);
}

static void send_pause_control_command(uv_tcp_t *client) {
    if (NULL == PriorityClientHandle)
        return;
    tcp_write_to_client(RegularClientHandle, "~~PAUSE\n", 8);
}

static void save_client_handle(uv_tcp_t *client) {
    struct sockaddr_in tmp_addr;
    int addrlen;

    addrlen = sizeof(tmp_addr);
    uv_tcp_getpeername(client, (struct sockaddr*)&tmp_addr, &addrlen);

    char *ipv4_string = inet_ntoa(tmp_addr.sin_addr);
    printf("New connection from: %s\n", ipv4_string);

    if (tmp_addr.sin_addr.s_addr == PriorityClientAddress.sin_addr.s_addr) {
        printf("PRIORITY client\n");
        if (PriorityClientHandle != NULL) {
            tcp_close((uv_handle_t *)client);
            return;
        }
        PriorityClientHandle = client;

    } else {
        printf("REGULAR client\n");
        if (RegularClientHandle != NULL) {
            tcp_close((uv_handle_t *)client);
            return;
        }
        RegularClientHandle = client;
    }
    send_pause_control_command(client);
}

static void on_new_connection(uv_stream_t *ServerHandle, int status) {
    uv_tcp_t *client;

    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), client);

    if (uv_accept(ServerHandle, (uv_stream_t*)client) != 0) {
        tcp_close((uv_handle_t*) client);
        return;
    }

    save_client_handle(client);
    uv_read_start((uv_stream_t*)client, alloc_buffer, tcp_read_cb);
}

int tcp_bridge_init(struct config *config) {
    uv_tcp_init(uv_default_loop(), &ServerHandle);

    uv_ip4_addr("0.0.0.0", config->tcp_port, &addr);
    PriorityClientAddress = config->priority_client;

    uv_tcp_bind(&ServerHandle, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&ServerHandle, 128, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return -1;
    }
    return 0;
}

void tcp_bridge_cleanup() {
}
