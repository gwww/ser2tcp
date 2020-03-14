#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <uv.h>

#include "tcp_bridge.h"
#include "util.h"

static uv_fs_t OpenRequest;
static uv_fs_t ReadRequest;
static char ReadBuffer[1024];
static uv_buf_t IOV;
static int OpenRetryTime = 1;
static uv_timer_t OpenTimerHandle;

static void serial_open_timeout(uv_timer_t *);

static void serial_close() {
    uv_fs_t close_req;

    printf("serial_close\n");
    uv_fs_close(uv_default_loop(), &close_req, OpenRequest.result, NULL);
    OpenRequest.result = -1;
    uv_timer_start(&OpenTimerHandle, serial_open_timeout, 0, 0);
}

static void serial_write_complete(uv_fs_t *req) {
    if (req->result < 0) {
        fprintf(stderr, "Serial write error: %s\n", uv_strerror((int)req->result));
        serial_close();
        return;
    }
    free(((uv_buf_t*)req->data)->base);
    free(req->data);
    free(req);
}

void serial_write(char *ReadBuffer, int length) {
    if (OpenRequest.result < 0)
        return;
    printf("serial_write\n");
    uv_fs_t *req = (uv_fs_t *) malloc(sizeof(uv_fs_t));
    req->data = (void *)create_uv_buf_with_data(ReadBuffer, length);
    uv_fs_write(uv_default_loop(),
        req, OpenRequest.result, req->data, 1, -1, serial_write_complete);
}

static void serial_read_cb(uv_fs_t *req) {
    printf("serial_read_cb\n");
    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
        return;
    }

    if (req->result == 0) {
        serial_close();
        return;
    }

    tcp_write(ReadBuffer, req->result);

    IOV.len = sizeof(ReadBuffer);
    uv_fs_read(uv_default_loop(),
        &ReadRequest, OpenRequest.result, &IOV, 1, -1, serial_read_cb);
}

static void serial_open_cb(uv_fs_t *req) {
    printf("serial_open_cb\n");
    assert(req == &OpenRequest);

    if (req->result < 0) {
        fprintf(stderr, "Error opening serial port: %s\n",
                uv_strerror((int)req->result));
        fprintf(stderr, "Retrying serial port open in %d seconds\n", OpenRetryTime);
        uv_timer_start(&OpenTimerHandle, serial_open_timeout, OpenRetryTime*1000, 0);
        if (OpenRetryTime < 60) OpenRetryTime *= 2;
        return;
    }
    OpenRetryTime = 1;

    set_serial_attribs(req->result, 4800);

    IOV = uv_buf_init(ReadBuffer, sizeof(ReadBuffer));
    uv_fs_read(uv_default_loop(),
        &ReadRequest, req->result, &IOV, 1, -1, serial_read_cb);
}

static void serial_open_timeout(uv_timer_t *handle) {
    /* uv_fs_open(uv_default_loop(), */
    /*         &OpenRequest, "/dev/cu.KeySerial1", UV_FS_O_RDWR, 0, serial_open_cb); */
    uv_fs_open(uv_default_loop(),
            &OpenRequest, "/dev/ttys005", UV_FS_O_RDWR, 0, serial_open_cb);
}

void serial_bridge_init() {
    OpenRequest.result = -1;
    uv_timer_init(uv_default_loop(), &OpenTimerHandle);
    uv_timer_start(&OpenTimerHandle, serial_open_timeout, 0, 0);
}

void serial_bridge_cleanup() {
    uv_fs_req_cleanup(&OpenRequest);
    uv_fs_req_cleanup(&ReadRequest);
}
