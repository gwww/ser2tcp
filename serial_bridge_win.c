#include <stdio.h>
#include <uv.h>
#include <fcntl.h>

#include "tcp_bridge.h"
#include "config.h"
#include "util.h"

static uv_device_t SerialDevice;
static uint64_t OpenRetryTime = 1;
static uv_timer_t OpenTimerHandle;
static char *SerialPortName;
static int SerialPortBaudRate;

static void serial_open(uv_timer_t *);

static void serial_close(uv_handle_t *handle) {
    dprintf(3, "");
    if (SerialDevice.handle == NULL)
        return;
    uv_close((uv_os_fd_t)handle, NULL);
    SerialDevice.handle = NULL;
    uv_timer_start(&OpenTimerHandle, serial_open, 0, 0);
}

static void serial_write_complete(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Serial write error: %s\n", uv_strerror(status));
        serial_close((uv_handle_t *)req);
    }

    free(((uv_buf_t*)req->data)->base);
    free(req->data);
    free(req);
}

void serial_write(char *buffer, size_t length) {
    if (SerialDevice.handle == NULL)
        return;
    hex_dump("SerTx:   ", buffer, length);
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    if (req) {
        req->data = (void*)create_uv_buf_with_data(buffer, length);
        if (NULL == req->data) {
            free(req);
            fprintf(stderr, "Out of memory, in serial_write\n");
            return;
        }
        uv_write(req, (uv_stream_t *)&SerialDevice, req->data, 1, serial_write_complete);
    }
}

static void serial_read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Serial read error %s\n", uv_err_name((int)nread));
        serial_close((uv_handle_t*)client);
    } else if (nread > 0) {
        hex_dump("SerRx: ", buf->base, nread);
        tcp_write(buf->base, nread);
    }
    if (buf->base) free(buf->base);
}

static void serial_open(uv_timer_t *handle) {
    memset(&SerialDevice, 0, sizeof(SerialDevice));
    int err = uv_device_init(uv_default_loop(), &SerialDevice, SerialPortName, O_RDWR);
    if (err) {
        fprintf(stderr, "Error opening serial port: %s\n", uv_strerror(err));
        fprintf(stderr, "Retrying serial port open in %lld seconds\n", OpenRetryTime);
        uv_timer_start(&OpenTimerHandle, serial_open, OpenRetryTime * 1000, 0);
        OpenRetryTime = (OpenRetryTime < 32) ? OpenRetryTime * 2 : 60;
    }
    else {
        dprintf(3, "");
        OpenRetryTime = 1;
        set_serial_attribs(SerialDevice.handle, SerialPortBaudRate);
        err = uv_read_start((uv_stream_t*)&SerialDevice, alloc_buffer, serial_read_cb);
    }
}

void serial_bridge_init(struct config *config) {
    SerialPortName = strdup(config->serial_port);
    SerialPortBaudRate = config->baud_rate;
    uv_timer_init(uv_default_loop(), &OpenTimerHandle);
    uv_timer_start(&OpenTimerHandle, serial_open, 0, 0);
}

void serial_bridge_cleanup() {
}