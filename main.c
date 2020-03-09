#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <uv.h>

#include "serial_bridge.h"
#include "tcp_bridge.h"

uv_loop_t *loop;

int main() {
    loop = uv_default_loop();

    serial_bridge_init();
    tcp_bridge_init();

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    tcp_bridge_cleanup();
    serial_bridge_cleanup();
}
