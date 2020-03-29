#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "serial_bridge.h"
#include "config.h"
#include "util.h"

static struct sockaddr_in addr;
static struct sockaddr_in PriorityClientAddress;
static uv_tcp_t ServerHandle;
static uv_tcp_t *RegularClientHandle = NULL;
static uv_tcp_t *PriorityClientHandle = NULL;
static char* ControlCmdFmt;

static void tcp_write_to_client(uv_tcp_t *client, char *buffer, int length);

static void tcp_close_cb(uv_handle_t* client) {
    dprintf(3, "");
    free(client);
}

static void send_control_command(char* command)
{
    char buffer[256];

    if (ControlCmdFmt == NULL)
        return;
    snprintf(buffer, sizeof buffer, ControlCmdFmt, command);
    tcp_write_to_client(RegularClientHandle, buffer, strlen(buffer));
}

static void tcp_close(uv_handle_t* client) {
    dprintf(3, "");
    uv_close(client, tcp_close_cb);
    if (client == (uv_handle_t *)PriorityClientHandle) {
        dprintf(3, "PRIORITY client");
        PriorityClientHandle = NULL;
        send_control_command("RESUME");
    } else if (client == (uv_handle_t *)RegularClientHandle) {
        dprintf(3, "REGULAR client");
        RegularClientHandle = NULL;
    } else {
        dprintf(3, "UNKNOWN client");
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

    if (buffer == NULL)
        return;

    uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
    if (req) {
        req->data = (void *)create_uv_buf_with_data(buffer, length);
        uv_write(req, (uv_stream_t *)client, req->data, 1, tcp_write_cb);
    }
}

void tcp_write(char *buffer, int length) {
    uv_stream_t* sendto;

    dprintf(3, "");
    sendto = (uv_stream_t*)
        (PriorityClientHandle ? PriorityClientHandle : RegularClientHandle);

    tcp_write_to_client((uv_tcp_t *)sendto, buffer, length);
}

static void tcp_read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    dprintf(3, "");
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
    send_control_command("PAUSE");
}

static void save_client_handle(uv_tcp_t *client) {
    struct sockaddr_in tmp_addr;
    int addrlen;

    addrlen = sizeof(tmp_addr);
    uv_tcp_getpeername(client, (struct sockaddr*)&tmp_addr, &addrlen);
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(tmp_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
    dprintf(3, "New connection from: %s", addr_str);

    if (tmp_addr.sin_addr.s_addr == PriorityClientAddress.sin_addr.s_addr) {
        dprintf(3, "PRIORITY client");
        if (PriorityClientHandle != NULL) {
            tcp_close((uv_handle_t *)client);
            return;
        }
        PriorityClientHandle = client;

    } else {
        dprintf(3, "REGULAR client");
        if (RegularClientHandle != NULL) {
            tcp_close((uv_handle_t *)client);
            return;
        }
        RegularClientHandle = client;
    }
    send_pause_control_command(client);
}

static void new_connection_cb(uv_stream_t *ServerHandle, int status) {
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
    ControlCmdFmt = strdup(config->control_cmd_fmt);

    uv_tcp_init(uv_default_loop(), &ServerHandle);

    uv_ip4_addr("0.0.0.0", config->tcp_port, &addr);
    PriorityClientAddress = config->priority_client;

    uv_tcp_bind(&ServerHandle, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&ServerHandle, 128, new_connection_cb);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return -1;
    }
    return 0;
}

void tcp_bridge_cleanup() {
} 