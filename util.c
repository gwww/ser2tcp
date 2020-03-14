#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

uv_buf_t* create_uv_buf_with_data(char *buffer, int length)
{
    uv_buf_t* iov;

    iov = malloc(sizeof(uv_buf_t));
    iov->base = malloc(length);
    iov->len = length;
    memcpy(iov->base, buffer, length);

    return iov;
}

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
