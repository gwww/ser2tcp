#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <uv.h>

void on_read(uv_fs_t *req);

uv_fs_t open_req;
uv_fs_t read_req;
uv_fs_t write_req;

static char buffer[1024];
static uv_buf_t iov;

#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

int set_serial_attribs(int fd, int speed)
{
    struct termios tty;
    int baud;

    if (tcgetattr (fd, &tty) != 0) {
        fprintf(stderr, "error %d from tcgetattr", errno);
        return -1;
    }

    switch (speed) {
        case 4800:   baud = B4800;   break;
        case 9600:   baud = B9600;   break;
        case 19200:  baud = B19200;  break;
        case 38400:  baud = B38400;  break;
        case 115200: baud = B115200; break;
        default:
            fprintf(stderr, "warning: baud rate %u not supported, using 9600.\n",
              speed);
            baud = B9600;
            break;
    }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars

    if (tcsetattr (fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}

void on_write(uv_fs_t *req) {
    /* printf("on_write\n"); */
    if (req->result < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror((int)req->result));
        return;
    }
    uv_fs_read(uv_default_loop(), &read_req, open_req.result, &iov, 1, -1, on_read);
}

void on_read(uv_fs_t *req) {
    /* printf("on_read\n"); */
    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
        return;
    } 

    if (req->result == 0) {
        uv_fs_t close_req;
        // synchronous
        uv_fs_close(uv_default_loop(), &close_req, open_req.result, NULL);
        return;
    }

    int i;
    for (i=0; i < req->result; i++) {
      if (iscntrl(buffer[i])) {
          buffer[i] = '\n';
      }
    }

    /* printf("read len=%zd, data=%s", req->result, buff); */

    iov.len = req->result;
    uv_fs_write(uv_default_loop(), &write_req, 1, &iov, 1, -1, on_write);
}

void on_open(uv_fs_t *req) {
    printf("on_open\n");
    // The request passed to the callback is the same as the one the call setup
    // function was passed.
    assert(req == &open_req);
    if (req->result < 0) {
        fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
        return;
    }

    set_serial_attribs(req->result, 4800);
    iov = uv_buf_init(buffer, sizeof(buffer));
    uv_fs_read(uv_default_loop(), &read_req, req->result, &iov, 1, -1, on_read);
}

void serial_bridge_init() {
    uv_fs_open(uv_default_loop(),
            &open_req, "/dev/cu.KeySerial1", UV_FS_O_RDWR, 0, on_open);
}

void serial_bridge_cleanup() {
    uv_fs_req_cleanup(&open_req);
    uv_fs_req_cleanup(&read_req);
    uv_fs_req_cleanup(&write_req);
}
