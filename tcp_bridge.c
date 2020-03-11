#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <uv.h>

#include "serial_bridge.h"
#include "util.h"

struct sockaddr_in addr;
uv_tcp_t server;
uv_tcp_t *client = NULL;
uv_tcp_t *priority_client = NULL;

static void alloc_buffer(uv_handle_t *handle, size_t len, uv_buf_t *buf) {
    buf->base = (char*)malloc(len);
    buf->len = len;
}

static void tcp_write_complete(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
        uv_close((uv_handle_t*)req->handle, NULL);
        client = NULL;
    }

    free(((uv_buf_t*)req->data)->base);
    free(req->data);
    free(req);
}

void tcp_write(char *buffer, int length) {
    printf("tcp_write\n");
    uv_stream_t* sendto;

    sendto = (uv_stream_t*)(priority_client ? priority_client : client);

    if (sendto == NULL)
        return;

    uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
    req->data = (void *)create_uv_buf_with_data(buffer, length);
    uv_write(req, sendto, req->data, 1, tcp_write_complete);
}

static void tcp_read_callback(
        uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
            uv_close((uv_handle_t*) client, NULL);
        }
    } else if (nread > 0) {
        /* uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t)); */
        /* uv_buf_t wrbuf = uv_buf_init(buf->base, nread); */
        /* uv_write(req, client, &wrbuf, 1, tcp_write_complete); */
        serial_write(buf->base, nread);
    }

    if (buf->base) {
        free(buf->base);
    }
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*)client, alloc_buffer, tcp_read_callback);
    } else {
        uv_close((uv_handle_t*) client, NULL);
    }
}

int tcp_bridge_init() {
    uv_tcp_init(uv_default_loop(), &server);

    uv_ip4_addr("0.0.0.0", 7000, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&server, 128, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    return 0;
}

void tcp_bridge_cleanup() {
}
