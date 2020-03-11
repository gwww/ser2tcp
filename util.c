#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <uv.h>

uv_buf_t* create_uv_buf_with_data(char *buffer, int length) {
    uv_buf_t* iov;

    iov = malloc(sizeof(uv_buf_t));
    iov->base = malloc(length);
    iov->len = length;
    memcpy(iov->base, buffer, length);

    return iov;
}
