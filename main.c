#include <stdio.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <uv.h>

#include "config.h"
#include "util.h"
#include "serial_bridge.h"
#include "tcp_bridge.h"

int main() {
    struct config *config = parse_config("config.toml");

    DebugLevel = config->debug_level;

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(config->priority_client.sin_addr), addr_str, INET_ADDRSTRLEN);
    dprintf(1, "serial port: %s, tcp-port: %d, priority client: %s\n",
            config->serial_port, config->tcp_port,
            addr_str);

    serial_bridge_init(config);
    tcp_bridge_init(config);

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    tcp_bridge_cleanup();
    serial_bridge_cleanup();

    return 0;
}