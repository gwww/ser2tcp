#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <uv.h>

#include "config.h"
#include "serial_bridge.h"
#include "tcp_bridge.h"

int main() {
    struct config *config = parse_config("config.toml");
    printf("serial port: %s, tcp-port: %lld, priority client: %s\n",
            config->serial_port, config->tcp_port,
            inet_ntoa(config->priority_client.sin_addr));

    serial_bridge_init(config);
    tcp_bridge_init(config);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    tcp_bridge_cleanup();
    serial_bridge_cleanup();
}
