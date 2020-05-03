#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "util.h"

int DebugLevel = 0;

uv_buf_t* create_uv_buf_with_data(char *buffer, size_t length)
{
    uv_buf_t* iov;

    iov = malloc(sizeof(uv_buf_t));
    if (NULL == iov)
        return NULL;
    iov->base = malloc(length);
    if (NULL == iov->base)
        return NULL;
    iov->len = (ULONG)length;
    memcpy(iov->base, buffer, length);

    return iov;
}

void alloc_buffer(uv_handle_t* handle, size_t len, uv_buf_t* buf) {
    buf->base = (char*)malloc(len);
    buf->len = (ULONG)len;
}

int set_serial_attribs(serial_handle fd, int speed)
{
#ifdef WIN32
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    BOOL status = GetCommState((HANDLE)fd, &dcbSerialParams);
    dcbSerialParams.BaudRate = speed;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState((HANDLE)fd, &dcbSerialParams);

    COMMTIMEOUTS timeouts;
    GetCommTimeouts((HANDLE)fd, &timeouts);
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts((HANDLE)fd, &timeouts);
#else
    struct termios tty;
    int baud;

    if (tcgetattr(fd, &tty) != 0) {
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

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return -1;
    }
#endif
    return 0;
}

void hex_dump(const char* prefix, const void* data, size_t size) {
#ifdef _DEBUG


    char ascii[17];
    size_t i, j;

    if (DebugLevel < 7)
        return;

    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        if (i % 16 == 0)
            printf("%s", prefix);
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        }
        else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size) {
            printf(" ");
            if ((i + 1) % 16 == 0) {
                printf("|  %s \n", ascii);
            }
            else if (i + 1 == size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
    fflush(stdout);
#endif // _DEBUG
}

char* setup_control_string(char* str) {
    size_t i = 0;
    char* s = str;
    int found = FALSE;
    
    if (NULL == s || '\0' == *s)
        return NULL;

    while (*s) {
        if (*s == '%' && *(s + 1) == 's') {
            found = TRUE;
            break;
        }
        s++;
        i++;
    }

    if (!found)
        return NULL;

    char* ret = malloc(strlen(str) + 4);
    strncpy(ret, str, i);
    ret[i] = '\0';
    strcat(ret, "%s%s");
    strcat(ret, str + i);

    return ret;
}